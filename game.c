#include "binheap.h"
#include "game.h"
#include <stdlib.h> // for rand()
#include <time.h>

static search_info doAstarSearch(int startRow, int startCol, int targetRow, int targetCol, struct game_state *Game, struct game_info *Info);
static int getGoodGuyCount (int row, int col, int distance, struct game_state *Game, struct game_info *Info);
static int getBadGuyCount (int row, int col, int distance, struct game_state *Game, struct game_info *Info);
static inline void getNearestNonWaterCoords(int* row, int* col, struct game_info *Info);
static void recruitTheTroops(int row, int col, ACTION_e action, int recruitDistance, struct game_state *Game, struct game_info *Info, int destrow, int destcol);
int hasLegalMoves(int ant, struct game_state *Game, struct game_info *Info);

void printMap(struct game_info *Info);



int* inWhichQueue;
int* gCost;
int* parent;

mission* antMissions;

void do_turn(struct game_state *Game, struct game_info *Info) 
{	
    int i, j, k;
	int offset, currentOffset;
	search_info local;
	int closestFoodIndex, closestFoodDistance, foodDistance;
	int closestEnemyIndex, closestEnemyDistance, enemyDistance;
	int closestEnemyHiveIndex, closestEnemyHiveDistance, enemyHiveDistance;
	int closestDefenderIndex, closestDefenderDistance, defenderDistance;
	int localGoodGuys, localBadGuys;
	int nearMyHiveDistance;
	int enemiesNearMyHive = 0;
	int myHiveUnderAttackIndex = 65535;
#if DEBUG_ENABLE
	clock_t time0, time1;
	time0 = clock()/(CLOCKS_PER_SEC/1000);
	srand(time(NULL));
#endif
	
	//printMap(Info);
    
	
	// Zero out ant missions from previous turn 
	// \\TODO: Find a way to preserve these across turns?
	for (i = 0; i < Info->rows*Info->cols; i++)
	{
		antMissions[i].action = NO_ACTION;
		antMissions[i].col = 0;
		antMissions[i].row = 0;
	}
	
	// find out how many bad guys are near my hives
	// for now only save the location of one hive that is under attack
	// in the future we can keep a list of which ones we need to defend
	for (k = 0; k < Game->my_hive_count; k++)
	{				
		for (j = 0; j < Game->enemy_count; j++)
		{
			nearMyHiveDistance = distance(Game->my_hives[k].row, Game->my_hives[k].col, Game->enemy_ants[j].row, Game->enemy_ants[j].col, Info);
			if (nearMyHiveDistance < YOURE_GETTING_TOO_CLOSE_DISTANCE)
				enemiesNearMyHive++;
		}
		if (enemiesNearMyHive > TOO_MANY_ENEMIES_NEAR_MY_HIVE_DISTANCE)
			myHiveUnderAttackIndex = k;
		enemiesNearMyHive = 0;
	}
	
	// if i have more than a certain number of ants, 
	// the ant closest to my hive should always stay to defend it
	if (Game->my_count > HIVE_DEFENDER_AMOUNT)
	{
		// for each of my hives iterate through my ants and find the closest one
		for (k = 0; k < Game->my_hive_count; k++)
		{
			for (i = 0; i < Game->my_count; i++)
			{
				defenderDistance = distance (ROW, COL, Game->my_hives[k].row, Game->my_hives[k].col, Info);
				if (defenderDistance < closestDefenderDistance && defenderDistance != 0)
				{
					closestDefenderIndex = i;
					closestDefenderDistance = defenderDistance;
				}
			}
			// Set up the defender for this hive
			antMissions[closestDefenderIndex].action = WATCH_THE_HIVE;
			
			// don't bother populating a column or row because his job is not to move
		}
		
	}
	
	for (i = 0; i < Game->my_count; i++) 
	{		
		DEBUG("evaluate for ant %d (r:%d c:%d)\n", i, ROW, COL);
		DEBUG("food count = %d, enemy count = %d\n", Game->food_count, Game->enemy_count);
		closestFoodIndex = 65535;
		closestFoodDistance = 65535;
		closestEnemyIndex = 65535;
		closestEnemyDistance = 65535;
		closestEnemyHiveIndex = 65535;
		closestEnemyHiveDistance = 65535;		
		
        // the location within the map array where our ant is currently
		offset = Game->my_ants[i].row*Info->cols + Game->my_ants[i].col;
		
		if (hasLegalMoves(i, Game, Info) && antMissions[i].action != WATCH_THE_HIVE)
		{

			// check if we need to defend the hive
			if (myHiveUnderAttackIndex != 65535)
			{
				recruitTheTroops(Game->my_hives[myHiveUnderAttackIndex].row, Game->my_hives[myHiveUnderAttackIndex].col, DEFEND_THE_HIVE, RECRUIT_DISTANCE_HELP_THE_HIVE, Game, Info, Game->my_hives[myHiveUnderAttackIndex].row, Game->my_hives[myHiveUnderAttackIndex].col);
			}
			
			// check if we need to run away
			if (antMissions[i].action > DEFEND_THE_HIVE)
			{
				//	// find the closest enemy ant
				for (j = 0; j < Game->enemy_count; j++)
				{
					enemyDistance = distance (ROW, COL, EROW, ECOL, Info);
					if (enemyDistance < ENEMY_SEARCH_DISTANCE)
					{
						DEBUG("found enemy %d (r:%d c:%d) at distance %d...", j, EROW, ECOL, enemyDistance);
						search_info etemp = doAstarSearch(ROW, COL, EROW, ECOL, Game, Info); 
						enemyDistance = etemp.cost;
						DEBUG("cost is %d...\n", enemyDistance);
						if (enemyDistance < closestEnemyDistance)
						{
							closestEnemyIndex = j;
							closestEnemyDistance = enemyDistance;
						}		
					}
				}
				if (closestEnemyDistance < ENEMY_AWARENESS_DISTANCE)
				{
					localGoodGuys = getGoodGuyCount(ROW, COL, FRIEND_DISTANCE, Game, Info);
					localBadGuys = getBadGuyCount(ROW, COL, ENEMY_DISTANCE, Game, Info);
					if (localGoodGuys > localBadGuys && localBadGuys > 0)
					{
						// attack the closest dude
						
						// find the closest enemy ant
						for (j = 0; j < Game->enemy_count; j++)
						{
							enemyDistance = distance (ROW, COL, EROW, ECOL, Info);
							if (enemyDistance < ENEMY_SEARCH_DISTANCE)
							{
								DEBUG("found enemy %d (r:%d c:%d) at distance %d...", j, EROW, ECOL, enemyDistance);
								search_info etemp = doAstarSearch(ROW, COL, EROW, ECOL, Game, Info); 
								enemyDistance = etemp.cost;
								DEBUG("cost is %d...\n", enemyDistance);
								if (enemyDistance < closestEnemyDistance)
								{
									closestEnemyIndex = j;
									closestEnemyDistance = enemyDistance;
								}		
							}
						}
						if (closestEnemyDistance < ENEMY_AWARENESS_DISTANCE)
						{
							// form a fighting group and attack the closest ant
							recruitTheTroops(Game->enemy_ants[closestEnemyIndex].row, Game->enemy_ants[closestEnemyIndex].col, FORM_FIGHTING_GROUP, ENEMY_DISTANCE, Game, Info, Game->enemy_ants[closestEnemyIndex].row, Game->enemy_ants[closestEnemyIndex].col);							
						}
					}
					else if (localBadGuys > 0) 
					{
						// run!
						
						int r, c;
						if (Game->enemy_ants[closestEnemyIndex].row > ROW)
						{
							r = (ROW - RUN_DISTANCE < 0 ? Info->rows - 1 - (EXPLORE_DISTANCE - ROW) : ROW - EXPLORE_DISTANCE);
						}
						else 
						{
							r = (ROW + RUN_DISTANCE > Info->rows - 1 ? RUN_DISTANCE - ROW : ROW + RUN_DISTANCE);
						}
						
						if (Game->enemy_ants[closestEnemyIndex].col > COL)
						{										
							c = (COL - RUN_DISTANCE < 0 ? Info->cols - 1 - (RUN_DISTANCE- COL) : COL - RUN_DISTANCE);
						}
						else 
						{
							c = (COL + RUN_DISTANCE > Info->cols - 1 ? COL + RUN_DISTANCE - (Info->cols - 1) : COL + RUN_DISTANCE);						
						}

						getNearestNonWaterCoords(&r, &c, Info);
						recruitTheTroops(ROW, COL, FRIEND_DISTANCE, RUN, Game, Info, r, c);
					}
				}
			}
			
			if (antMissions[i].action > RUN)
			{
				// collect nearby food
				
				// find the closest food
				for (j = 0; j < Game->food_count; j++)
				{
					foodDistance = distance(ROW, COL, FROW, FCOL, Info);
					DEBUG("f%d (r:%d c:%d) dist:%d\n", j, FROW, FCOL, foodDistance);
					if (foodDistance < FOOD_SEARCH_DISTANCE)
					{
						DEBUG("found food %d (r:%d c:%d) at distance %d...", j, FROW, FCOL, foodDistance);
						search_info temp = doAstarSearch(ROW, COL, FROW, FCOL, Game, Info);
						foodDistance = temp.cost;
						DEBUG("cost is %d...\n", foodDistance);
						if (foodDistance < closestFoodDistance)
						{
							closestFoodIndex = j;
							closestFoodDistance = foodDistance;
						}
					}
				}
				
				if(closestFoodDistance < FOOD_PERSUAL_DISTANCE)
				{
					DEBUG("ant %u (r:%u c:%u): found food %u (r:%u c:%u) nom nom\n", i, ROW, COL, closestFoodIndex, Game->food[closestFoodIndex].row, Game->food[closestFoodIndex].col);
					antMissions[i].action = GET_FOOD;
					antMissions[i].row = Game->food[closestFoodIndex].row;
					antMissions[i].col = Game->food[closestFoodIndex].col;			
				}
			}
			
			if (antMissions[i].action > GET_FOOD)
			{
				// check if we have an enemy hive to attack
				
				// find the closest enemy hive
				for (j = 0; j < Game->enemy_hive_count; j++)
				{
					enemyHiveDistance = distance (ROW, COL, Game->enemy_hives[j].row, Game->enemy_hives[j].col, Info);
					if (enemyHiveDistance < ENEMY_HIVE_SEARCH_DISTANCE)
					{
						DEBUG("found enemy hive %d (r:%d c:%d) at distance %d...", j, Game->enemy_hives[j].row, Game->enemy_hives[j].col, enemyHiveDistance);
						search_info etemp = doAstarSearch(ROW, COL, Game->enemy_hives[j].row, Game->enemy_hives[j].col, Game, Info); 
						enemyHiveDistance = etemp.cost;
						DEBUG("cost is %d...\n", enemyHiveDistance);
						if (enemyHiveDistance < closestEnemyHiveDistance)
						{
							closestEnemyHiveIndex = j;
							closestEnemyHiveDistance = enemyHiveDistance;
						}
					}
					if (closestEnemyHiveDistance < ENEMY_HIVE_AWARENESS_DISTANCE)
					{
						// Do we have more of our peeps around this guy than they have around their hive?
						localGoodGuys = getGoodGuyCount(ROW, COL, RECRUIT_DISTANCE_ATTACK_A_HIVE, Game, Info);
						localBadGuys = getBadGuyCount(Game->enemy_hives[closestEnemyHiveIndex].row, Game->enemy_hives[closestEnemyHiveIndex].col ,RECRUIT_DISTANCE_ATTACK_A_HIVE, Game, Info);
						if (localGoodGuys > localBadGuys)
						{
							// recruit to attack the hive
							recruitTheTroops(Game->enemy_hives[closestEnemyHiveIndex].row, Game->enemy_hives[closestEnemyHiveIndex].col, RECRUIT_DISTANCE_ATTACK_A_HIVE, RECRUIT_DISTANCE_ATTACK_A_HIVE, Game, Info, Game->enemy_hives[closestEnemyHiveIndex].row, Game->enemy_hives[closestEnemyHiveIndex].col);
						}
						else 
						{
							// recruit around me to build up the army
							recruitTheTroops(ROW, COL, RECRUIT_DISTANCE_ATTACK_A_HIVE, RECRUIT_DISTANCE_ATTACK_A_HIVE, Game, Info, ROW, COL);							
						}						
					}
				}
				
			}

			// commented out because we already formed fighting groups above when checking if we need to run
//			if (antMissions[i].action > ATTACK_ENEMY_HIVE)
//			{
//
//			}
			
			if (antMissions[i].action > FORM_FIGHTING_GROUP)
			{
				// guess we're going exploring... someone call dora
			
				int exploreDir;
				
				exploreDir = i % 4;
				
				DEBUG("exploreDir = %d\n", exploreDir);
				int r, c;
				if (exploreDir == 0) // north east
				{	r = (ROW - EXPLORE_DISTANCE < 0 ? Info->rows - 1 - (EXPLORE_DISTANCE - ROW) : ROW - EXPLORE_DISTANCE);
					c = (COL + EXPLORE_DISTANCE > Info->cols - 1 ? COL + EXPLORE_DISTANCE - (Info->cols - 1) : COL + EXPLORE_DISTANCE);	
					DEBUG("ant %d: exploring the world and going north\n", i);
				}
				else if (exploreDir == 1)// south east 
				{
					r = (ROW + EXPLORE_DISTANCE > Info->rows - 1 ? EXPLORE_DISTANCE + ROW - (Info->rows - 1) : ROW + EXPLORE_DISTANCE);
					c = (COL + EXPLORE_DISTANCE > Info->cols - 1 ? COL + EXPLORE_DISTANCE - (Info->cols - 1) : COL + EXPLORE_DISTANCE);	
					DEBUG("ant %d: exploring the world and going east\n", i);
				}
				else if (exploreDir == 2) // south west
				{
					r = (ROW + EXPLORE_DISTANCE > Info->rows - 1 ? EXPLORE_DISTANCE + ROW - (Info->rows - 1) : ROW + EXPLORE_DISTANCE);
					c = (COL - EXPLORE_DISTANCE < 0 ? Info->cols - 1 - (EXPLORE_DISTANCE- COL) : COL - EXPLORE_DISTANCE);
					DEBUG("ant %d: exploring the world and going south\n", i);				
				}
				else // north west
				{
					r = (ROW - EXPLORE_DISTANCE < 0 ? Info->rows - 1 - (EXPLORE_DISTANCE - ROW) : ROW - EXPLORE_DISTANCE);
					c = (COL - EXPLORE_DISTANCE < 0 ? Info->cols - 1 - (EXPLORE_DISTANCE - COL) : COL - EXPLORE_DISTANCE);	
					DEBUG("ant %d: exploring the world and going west\n", i);				
				}	
				
				DEBUG("finding non water coords at %d %d...", r, c);
				getNearestNonWaterCoords(&r, &c, Info);
				DEBUG("found %d %d\n", r, c);

				antMissions[i].action = EXPLORE;
				antMissions[i].row = r;
				antMissions[i].col = c;
				
			}
		}
			
//		//	// find the closest enemy ant
//			for (j = 0; j < Game->enemy_count; j++)
//			{
//				enemyDistance = distance (ROW, COL, EROW, ECOL, Info);
//				if (enemyDistance < ENEMY_SEARCH_DISTANCE)
//				{
//					DEBUG("found enemy %d (r:%d c:%d) at distance %d...", j, EROW, ECOL, enemyDistance);
//					search_info etemp = doAstarSearch(ROW, COL, EROW, ECOL, Game, Info); 
//					enemyDistance = etemp.cost;
//					DEBUG("cost is %d...\n", enemyDistance);
//					if (enemyDistance < closestEnemyDistance)
//					{
//						closestEnemyIndex = j;
//						closestEnemyDistance = enemyDistance;
//					}		
//				}
//			}
//			
//			// find the closest food
//			for (j = 0; j < Game->food_count; j++)
//			{
//				foodDistance = distance(ROW, COL, FROW, FCOL, Info);
//				DEBUG("f%d (r:%d c:%d) dist:%d\n", j, FROW, FCOL, foodDistance);
//				if (foodDistance < FOOD_SEARCH_DISTANCE)
//				{
//					DEBUG("found food %d (r:%d c:%d) at distance %d...", j, FROW, FCOL, foodDistance);
//					search_info temp = doAstarSearch(ROW, COL, FROW, FCOL, Game, Info);
//					foodDistance = temp.cost;
//					DEBUG("cost is %d...\n", foodDistance);
//					if (foodDistance < closestFoodDistance)
//					{
//						closestFoodIndex = j;
//						closestFoodDistance = foodDistance;
//					}
//				}
//			}
//
//			// now decide what to do
//			// priorities:
//			// 1. defend my hive
//			// 2. attack enemy hive
//			// 3. attack or run from enemy ants
//			// 4. gather food that is reasonably close
//			// 5. explore
//			if (myHiveUnderAttackIndex != 65535)
//			{
//				// gotta defend
//				DEBUG("ant %u (r:%u c:%u): defend my hive %u (r:%u c:%u)\t\tgood guys=%u, bad guys=%u\n", i, ROW, COL, myHiveUnderAttackIndex, Game->my_hives[myHiveUnderAttackIndex].row, Game->my_hives[myHiveUnderAttackIndex].col, getGoodGuyCount(i, Game, Info), getGoodGuyCount(i, Game, Info));
//				local = doAstarSearch(ROW, COL, Game->my_hives[myHiveUnderAttackIndex].row, Game->my_hives[myHiveUnderAttackIndex].col, Game, Info);			
//				currentOffset = local.offset; 
//			}
//			else if (closestEnemyHiveDistance < ENEMY_HIVE_AWARENESS_DISTANCE && getGoodGuyCount(i, Game, Info) > getBadGuyCount(i, Game, Info))
//			{
//				// an enemy hive
//				// kick some ass boys
//				// i wanna see you get in there and give 110 %
//				
//				DEBUG("ant %u (r:%u c:%u): attack enemy hive %u (r:%u c:%u)\t\tgood guys=%u, bad guys=%u\n", i, ROW, COL, closestEnemyHiveIndex, Game->enemy_hives[closestEnemyHiveIndex].row, Game->enemy_hives[closestEnemyHiveIndex].col, getGoodGuyCount(i, Game, Info), getGoodGuyCount(i, Game, Info));
//				// fight!
//				local = doAstarSearch(ROW, COL, Game->enemy_hives[closestEnemyHiveIndex].row, Game->enemy_hives[closestEnemyHiveIndex].col, Game, Info);			
//				currentOffset = local.offset; 
//				
//			}
//			else if (closestEnemyDistance < ENEMY_AWARENESS_DISTANCE)
//			{
//				// an enemy!
//				// fight or flight?
//							
//				if (getGoodGuyCount(i, Game, Info) > getBadGuyCount(i, Game, Info))
//				{
//					DEBUG("ant %u (r:%u c:%u): fight with enemy ant %u (r:%u c:%u)\t\tgood guys=%u, bad guys=%u\n", i, ROW, COL, closestEnemyIndex, Game->enemy_ants[closestEnemyIndex].row, Game->enemy_ants[closestEnemyIndex].col, getGoodGuyCount(i, Game, Info), getGoodGuyCount(i, Game, Info));
//					// fight!
//					local = doAstarSearch(ROW, COL, Game->enemy_ants[closestEnemyIndex].row, Game->enemy_ants[closestEnemyIndex].col, Game, Info);			
//					currentOffset = local.offset; 
//				}
//				else 
//				{
//					DEBUG("ant %u (r:%u c:%u): run from enemy ant %u (r:%u c:%u)\t\tgood guys=%u, bad guys=%u\n", i, ROW, COL, closestEnemyIndex, Game->enemy_ants[closestEnemyIndex].row, Game->enemy_ants[closestEnemyIndex].col, getGoodGuyCount(i, Game, Info), getGoodGuyCount(i, Game, Info));
//					// run for the hills
//					int column, row;
//					if (Game->enemy_ants[closestEnemyIndex].col > COL)
//					{
//						column = (COL - RUN_DISTANCE < 0 ? Info->cols - 1 - (RUN_DISTANCE - COL) : COL - RUN_DISTANCE);
//					}
//					else 
//					{
//						column = (COL + RUN_DISTANCE > Info->cols - 1 ? COL + RUN_DISTANCE - (Info->cols - 1) : COL + RUN_DISTANCE);
//					}
//
//					if (Game->enemy_ants[closestEnemyIndex].row > ROW)
//					{
//						row = (ROW - RUN_DISTANCE < 0 ? Info->rows - 1 - (RUN_DISTANCE - ROW) : ROW - RUN_DISTANCE);
//					}
//					else 
//					{
//						row = (ROW + RUN_DISTANCE > Info->rows - 1 ? ROW + RUN_DISTANCE - (Info->rows - 1) : ROW + RUN_DISTANCE);
//					}
//
//					DEBUG("finding non water coords at %d %d...", row, column);
//					getNearestNonWaterCoords(&row, &column, Info);
//					DEBUG("found %d %d\n", row, column);
//					local = doAstarSearch(ROW, COL, row, column, Game, Info);
//					currentOffset = local.offset;
//				}
//			}
//
//			// Now do an A* search to find the shortest path to that food
//			// for this ant and start him off in that direction.		
//			else if(closestFoodDistance < FOOD_PERSUAL_DISTANCE)
//			{
//				DEBUG("ant %u (r:%u c:%u): found food %u (r:%u c:%u) nom nom\n", i, ROW, COL, closestFoodIndex, Game->food[closestFoodIndex].row, Game->food[closestFoodIndex].col);
//				local = doAstarSearch(ROW, COL, Game->food[closestFoodIndex].row, Game->food[closestFoodIndex].col, Game, Info);
//				currentOffset = local.offset;
//			}
//			else 
//			{			
//				// explore, my pretties!
//			
//				int exploreDir;
//				
//				exploreDir = i % 4;
//
//				DEBUG("exploreDir = %d\n", exploreDir);
//				int r, c;
//				if (exploreDir == 0) // north east
//				{	r = (ROW - EXPLORE_DISTANCE < 0 ? Info->rows - 1 - (EXPLORE_DISTANCE - ROW) : ROW - EXPLORE_DISTANCE);
//					c = (COL + EXPLORE_DISTANCE > Info->cols - 1 ? COL + EXPLORE_DISTANCE - (Info->cols - 1) : COL + EXPLORE_DISTANCE);	
//					DEBUG("ant %d: exploring the world and going north\n", i);
//				}
//				else if (exploreDir == 1)// south east 
//				{
//					r = (ROW + EXPLORE_DISTANCE > Info->rows - 1 ? EXPLORE_DISTANCE - ROW : ROW + EXPLORE_DISTANCE);
//					c = (COL + EXPLORE_DISTANCE > Info->cols - 1 ? COL + EXPLORE_DISTANCE - (Info->cols - 1) : COL + EXPLORE_DISTANCE);	
//					DEBUG("ant %d: exploring the world and going east\n", i);
//				}
//				else if (exploreDir == 2) // south west
//				{
//					r = (ROW + EXPLORE_DISTANCE > Info->rows - 1 ? EXPLORE_DISTANCE - ROW : ROW + EXPLORE_DISTANCE);
//					c = (COL - EXPLORE_DISTANCE < 0 ? Info->cols - 1 - (EXPLORE_DISTANCE- COL) : COL - EXPLORE_DISTANCE);
//					DEBUG("ant %d: exploring the world and going south\n", i);				
//				}
//				else // north west
//				{
//					r = (ROW - EXPLORE_DISTANCE < 0 ? Info->rows - 1 - (EXPLORE_DISTANCE - ROW) : ROW - EXPLORE_DISTANCE);
//					c = (COL - EXPLORE_DISTANCE < 0 ? Info->cols - 1 - (EXPLORE_DISTANCE - COL) : COL - EXPLORE_DISTANCE);	
//					DEBUG("ant %d: exploring the world and going west\n", i);				
//				}	
//
//				DEBUG("finding non water coords at %d %d...", r, c);
//				getNearestNonWaterCoords(&r, &c, Info);
//				DEBUG("found %d %d\n", r, c);
//				local = doAstarSearch(ROW, COL, r, c, Game, Info);
//				currentOffset = local.offset;
//			}
//			
//			
//			
//			// Now we have the location that we'd like to move to.
//			// Determine which direction it is and make the move.
//			char dir = -1;
//			
//			if (currentOffset == offset + UP || currentOffset == offset + (Info->rows - 1)*Info->cols)
//				dir = 'N';
//			else if (currentOffset == offset + RIGHT || currentOffset == offset - Info->cols - 1)
//				dir = 'E';
//			else if (currentOffset == offset + DOWN || currentOffset == offset - (Info->rows - 1)*Info->cols)
//				dir = 'S';
//			else if (currentOffset == offset + LEFT || currentOffset == offset + Info->cols - 1)
//				dir = 'W';
//			
//			if (dir != -1)
//			{
//				int occupied = false;
//				int temprow, tempcol;
//				
//				// Before we make our move, make sure we aren't crashing into another of our ants
//				for (k = 0; k < Game->my_count; k++)
//				{
//					// doing this switch statement may seem slow, but it's actually way faster than doing an integer
//					// divide to find the column and row from the offset
//					if ('N' == dir)
//					{
//						tempcol = COL;
//						temprow = (ROW == 0 ? Info->rows - 1 : ROW - 1);
//					}
//					else if ('E' == dir)
//					{
//						tempcol = (COL + 1 == Info->cols - 1 ? 0 : COL + 1);
//						temprow = ROW;
//					}
//					else if ('S' == dir)
//					{
//						tempcol = COL;
//						temprow = (ROW + 1 == Info->rows - 1 ? 0 : ROW + 1);
//
//					}
//					else if ('W' == dir)
//					{
//						tempcol = (COL == 0 ? Info->cols - 1 : COL - 1);
//						temprow = ROW;
//					}
//					
//					if (Game->my_ants[k].row == temprow && Game->my_ants[k].col == tempcol)
//					{
//						occupied = true;
//						break;
//					}
//				}
//				if (!occupied)
//					move(i, dir, Game, Info);
//				else 
//				{
//					DEBUG("ant %d (r:%d c:%d) blocked from moving %c\n", i, ROW, COL, dir);
//				}
//			}
//		} // hasLegalMoves
    }
//#if DEBUG_ENABLE	
//	printMap(Info);
//#endif
	DEBUG("rows = %d, cols = %d\n", Info->rows, Info->cols);
		// now do the searching and make the move for each of my ants
		for (i = 0; i < Game->my_count; i++)
		{
			if (antMissions[i].action != NO_ACTION && antMissions[i].action != WATCH_THE_HIVE)
			{
				DEBUG("ant %d (r:%d c:%d) mission = %d\t r:%d c:%d\n", i, ROW, COL, antMissions[i].action, antMissions[i].row, antMissions[i].col);
				
				// Do the Astar search
				local = doAstarSearch(ROW, COL, antMissions[i].row, antMissions[i].col, Game, Info);
				currentOffset = local.offset;

				offset = Game->my_ants[i].row*Info->cols + Game->my_ants[i].col;
			
				// Now we have the location that we'd like to move to.
				// Determine which direction it is and make the move.
				char dir = -1;
				
				if (currentOffset == offset + UP || currentOffset == offset + (Info->rows - 1)*Info->cols)
					dir = 'N';
				else if (currentOffset == offset + RIGHT || currentOffset == offset - Info->cols - 1)
					dir = 'E';
				else if (currentOffset == offset + DOWN || currentOffset == offset - (Info->rows - 1)*Info->cols)
					dir = 'S';
				else if (currentOffset == offset + LEFT || currentOffset == offset + Info->cols - 1)
					dir = 'W';
				
				if (dir != -1)
				{
					int occupied = false;
					int temprow, tempcol;
					
					// Before we make our move, make sure we aren't crashing into another of our ants
					for (k = 0; k < Game->my_count; k++)
					{
						// doing this switch statement may seem slow, but it's actually way faster than doing an integer
						// divide to find the column and row from the offset
						if ('N' == dir)
						{
							tempcol = COL;
							temprow = (ROW == 0 ? Info->rows - 1 : ROW - 1);
						}
						else if ('E' == dir)
						{
							tempcol = (COL + 1 == Info->cols - 1 ? 0 : COL + 1);
							temprow = ROW;
						}
						else if ('S' == dir)
						{
							tempcol = COL;
							temprow = (ROW + 1 == Info->rows - 1 ? 0 : ROW + 1);
							
						}
						else if ('W' == dir)
						{
							tempcol = (COL == 0 ? Info->cols - 1 : COL - 1);
							temprow = ROW;
						}
						
						if (Game->my_ants[k].row == temprow && Game->my_ants[k].col == tempcol)
						{
							occupied = true;
							break;
						}
					}
					if (!occupied)
						move(i, dir, Game, Info);
					else 
					{
						DEBUG("ant %d (r:%d c:%d) blocked from moving %c\n", i, ROW, COL, dir);
					}
				}
				else 
				{
					DEBUG("ant %d doesn't have a dir\n", i);
				}

			}
		}
		
#if DEBUG_ENABLE
	time1 = clock() / (CLOCKS_PER_SEC/1000);
	DEBUG("time taken for turn (ms): %.2f\n", (double)time1-time0);
#endif // DEBUG_ENABLE
}

// Perform an A * search for a target
// return an offset 1step away from the starting position or the starting position if no target was found
static search_info doAstarSearch(int startRow, int startCol, int targetRow, int targetCol, struct game_state *Game, struct game_info *Info)
{
	int k;
	int cost, offset, currentOffset, neighbourOffset;
	char obj_north, obj_east, obj_south, obj_west;
	int tempOffset;
	int goalFound = false;
	struct node current, temp;	
	search_info returnValue;
	returnValue.cost = -1;
	returnValue.offset = -1;
	
	PriorityQueue Open = Initialize(OPEN_LIST_SIZE);
	
	// TODO: maybe make these globals so i don't have to declare them for each search?
//	int inWhichQueue[Info->rows*Info->cols];
//	int gCost[Info->rows*Info->cols];
//	int parent[Info->rows*Info->cols];
	k = Info->rows*Info->cols;
	memset(inWhichQueue, 0, sizeof(int)*k);
	memset(gCost, 0, sizeof(int)*k);
	memset(parent, 0, sizeof(int)*k);
	
	// clear this stuff for each ant
//	for (k = 0; k < Info->rows*Info->cols; k++)
//	{
//		inWhichQueue[k] = NO_QUEUE;
//		gCost[k] = 65535;
//		parent[k] = 0;
//	}
	
	offset = startRow*Info->cols + startCol;
	
	current.costToGetHere = 0;
	current.row = startRow;
	current.col = startCol;
	
	inWhichQueue[offset] = OPEN;
	gCost[offset] = 0;
	
	MakeEmpty(Open);
	Insert(current, Open);
	
	while(1)
	{
		do
		{
			current = FindMin(Open);
			currentOffset = current.row*Info->cols + current.col;
			if (inWhichQueue[currentOffset] != OPEN)
				DeleteMin(Open);
		} while (!IsEmpty(Open) && inWhichQueue[currentOffset] != OPEN);
		
		if (IsEmpty(Open))
		{
			goalFound = false;
			DEBUG("Error: empty queue escape during search%d\n", goalFound);
			break;
		}
		
		if ((current.row == targetRow) && 
			(current.col == targetCol))
		{
			goalFound = true;
			break; // we are at the goal node
		}
		
		// add current to the closed list and remove it from the priority queue
		inWhichQueue[currentOffset] = CLOSED;
		//DeleteMin(Open);
		
		// figure out what's happening around us
		// NORTH
		if (current.row != 0)
			obj_north = Info->map[currentOffset + UP];
		else
			obj_north = Info->map[currentOffset + (Info->rows - 1)*Info->cols];			
		
		// EAST
		if (current.col != Info->cols - 1)
			obj_east = Info->map[currentOffset + RIGHT];
		else
			obj_east = Info->map[currentOffset - Info->cols - 1];			
		
		// SOUTH
		if (current.row != Info->rows - 1)
			obj_south = Info->map[currentOffset + DOWN];
		else
			obj_south = Info->map[currentOffset - (Info->rows - 1)*Info->cols];			
		
		// WEST
		if (current.col != 0)
			obj_west = Info->map[currentOffset + LEFT];
		else
			obj_west = Info->map[currentOffset + Info->cols - 1];
		
		
		// for each neighbour of current:
		cost = current.costToGetHere + 1; // cost is always 1 to get to any adjacent square
		
		// NORTH
		// if this is a valid square to occupy
		if (obj_north != '%' && obj_north != 'a')
		{
			if (current.row != 0)
				neighbourOffset = currentOffset + UP;
			else
				neighbourOffset = currentOffset + (Info->rows - 1)*Info->cols;	
			
			if ((inWhichQueue[neighbourOffset] == OPEN) &&
				(cost < gCost[neighbourOffset]))
			{
				// New path is better, remove the north neighbour from OPEN
				inWhichQueue[neighbourOffset] = NO_QUEUE;
			}
			
			if ((inWhichQueue[neighbourOffset] == CLOSED) &&
				(cost < gCost[neighbourOffset]))
			{
				// Remove neighbour from closed
				inWhichQueue[neighbourOffset] = NO_QUEUE;					
			}	
			
			if (inWhichQueue[neighbourOffset] == NO_QUEUE)
			{
				temp.costToGetHere = cost;
				temp.col = current.col;
				if (current.row != 0)
					temp.row = current.row - 1;	
				else 
					temp.row = Info->rows - 1;
				temp.costPlusEstimate = cost + distance(temp.row, temp.col, targetRow, targetCol, Info);
				parent[neighbourOffset] = currentOffset;
				gCost[neighbourOffset] = cost;
				
				// Add to the open list.
				Insert(temp, Open);
				inWhichQueue[neighbourOffset] = OPEN;
			}
		}
		
		// EAST
		if (obj_east != '%' && obj_east != 'a')
		{
			if (current.col != Info->cols - 1)
				neighbourOffset = currentOffset + RIGHT;
			else
				neighbourOffset = currentOffset - Info->cols - 1;			
			
			
			if ((inWhichQueue[neighbourOffset] == OPEN) &&
				(cost < gCost[neighbourOffset]))
			{
				// New path is better, remove the north neighbour from OPEN
				inWhichQueue[neighbourOffset] = NO_QUEUE;
			}
			
			if ((inWhichQueue[neighbourOffset] == CLOSED) &&
				(cost < gCost[neighbourOffset]))
			{
				// Remove neighbour from closed
				inWhichQueue[neighbourOffset] = NO_QUEUE;					
			}	
			
			if (inWhichQueue[neighbourOffset] == NO_QUEUE)
			{
				temp.costToGetHere = cost;
				temp.row = current.row;
				if (current.col != Info->cols - 1)
					temp.col = current.col + 1;	
				else 
					temp.col = 0;
				temp.costPlusEstimate = cost + distance(temp.row, temp.col, targetRow, targetCol, Info);
				parent[neighbourOffset] = currentOffset;
				gCost[neighbourOffset] = cost;
				
				// Add to the open list.
				Insert(temp, Open);
				inWhichQueue[neighbourOffset] = OPEN;
			}
		}
		
		// SOUTH
		if (obj_south != '%' && obj_south != 'a')
		{
			if (current.row != Info->rows - 1)
				neighbourOffset = currentOffset + DOWN;
			else
				neighbourOffset = currentOffset - (Info->rows - 1)*Info->cols;
			
			if ((inWhichQueue[neighbourOffset] == OPEN) &&
				(cost < gCost[neighbourOffset]))
			{
				// New path is better, remove the north neighbour from OPEN
				inWhichQueue[neighbourOffset] = NO_QUEUE;
			}
			
			if ((inWhichQueue[neighbourOffset] == CLOSED) &&
				(cost < gCost[neighbourOffset]))
			{
				// Remove neighbour from closed
				inWhichQueue[neighbourOffset] = NO_QUEUE;					
			}	
			
			if (inWhichQueue[neighbourOffset] == NO_QUEUE)
			{
				temp.costToGetHere = cost;
				temp.col = current.col;
				if (current.row != Info->rows - 1)
					temp.row = current.row + 1;	
				else 
					temp.row = 0;
				temp.costPlusEstimate = cost + distance(temp.row, temp.col, targetRow, targetCol, Info);
				parent[neighbourOffset] = currentOffset;
				gCost[neighbourOffset] = cost;
				
				// Add to the open list.
				Insert(temp, Open);
				inWhichQueue[neighbourOffset] = OPEN;
				tempOffset = temp.row*Info->cols + temp.col;
			}
		}		
		
		// WEST
		if (obj_west != '%' && obj_west != 'a')
		{
			
			if (current.col != 0)
				neighbourOffset = currentOffset + LEFT;
			else
				neighbourOffset = currentOffset + Info->cols - 1;
			
			if ((inWhichQueue[neighbourOffset] == OPEN) &&
				(cost < gCost[neighbourOffset]))
			{
				// New path is better, remove the north neighbour from OPEN
				inWhichQueue[neighbourOffset] = NO_QUEUE;
			}
			
			if ((inWhichQueue[neighbourOffset] == CLOSED) &&
				(cost < gCost[neighbourOffset]))
			{
				// Remove neighbour from closed
				inWhichQueue[neighbourOffset] = NO_QUEUE;					
			}	
			
			if (inWhichQueue[neighbourOffset] == NO_QUEUE)
			{
				temp.costToGetHere = cost;
				temp.row = current.row;
				if (current.col != 0)
					temp.col = current.col - 1;	
				else 
					temp.col = Info->cols - 1;
				temp.costPlusEstimate = cost + distance(temp.row, temp.col, targetRow, targetCol, Info);
				parent[neighbourOffset] = currentOffset;
				gCost[neighbourOffset] = cost;
				
				// Add to the open list.
				Insert(temp, Open);
				inWhichQueue[neighbourOffset] = OPEN;
				tempOffset = temp.row*Info->cols + temp.col;
			}
		}				
	} // while 1	
	
	
	// Now we have a list of the best path from our current ant to the closest
	// food item.
	// Let's iterate backwards through it and play the first move.
	
	// currentOffset has our current location and offset has the ant's location
	
	if (goalFound)
	{
		
		//			int tempr, tempc;
		//			tempc = currentOffset % Info->cols;
		//			tempr = currentOffset / Info->cols;
		//			printf("Goal found at row = %u and col = %u\n", tempr, tempc);
		//			printf("food lives at row = %u and col = %u\n", Game->food[closestFoodIndex].row, Game->food[closestFoodIndex].col);
		//			printf("food offset = %u and my offset = %u\n", Game->food[closestFoodIndex].row*Info->cols + Game->food[closestFoodIndex].col, currentOffset);
		
		currentOffset = current.row*Info->cols + current.col;
		returnValue.cost = current.costToGetHere;
		//
		//			printf("traversing back towards the starting spot..\n");
		//			printf("row = %u\tcol = %u\n", current.row, current.col);
		//			
		while (parent[currentOffset] != offset)
		{
			currentOffset = parent[currentOffset];
			if (parent[currentOffset] == 0)
				break;
			
			//				tempc = currentOffset % Info->cols;
			//				tempr = currentOffset / Info->cols;
			//				printf("row = %u\tcol = %u\n", tempr, tempc);
		}
		returnValue.offset = currentOffset;
		return returnValue;
	}
	else 
	{
		DEBUG("goal not found, last offset = %d\n", returnValue.offset);
		return returnValue;
	}

	
}

// returns the number of good guys within distance
static int getGoodGuyCount (int row, int col, int distance, struct game_state *Game, struct game_info *Info)
{
	// for now, if we have more people in the vicinity, fight
	int localGoodGuyCount = 0;
	int k;
	
	int north_row = (row - distance < 0 ? Info->rows - 1 - (distance - row) : row - distance);
	int south_row = (row + distance > Info->rows - 1 ? row + distance - (Info->rows - 1) : row + distance);
	int east_col = (col + distance > Info->cols - 1 ? col + distance - (Info->cols - 1) : col + distance);
	int west_col = (col - distance < 0 ? Info->cols - 1 - (distance - col) : col - distance);
	
	// figure out how many good guys are in the vicinity
	for (k = 0; k < Game->my_count; k++)
	{
		// check north-south
		if (south_row > north_row)
		{
			// no wrapping north-south, normal check
			if (Game->my_ants[k].row <= south_row && Game->my_ants[k].row >= north_row)
			{
				// we're in range north-south, check east-west
				if (east_col > west_col)
				{
					// no wrapping, normal check
					if (Game->my_ants[k].col >= west_col && Game->my_ants[k].col <= east_col)
					{
						// we found a friend
						localGoodGuyCount++;
					}
					else 
					{
						// make sure we account for wrapping
						// if we are between 0 and the east col or between the west col and cols 
						if (Game->my_ants[k].col <= east_col || Game->my_ants[k].col >= west_col)
						{
							// we found a friend
							localGoodGuyCount++;
						}
					}		
					
				}
			}
		}
		else 
		{
			// make sure we account for wrapping
			// if we are between 0 and the north row or between the south row and rows
			if (Game->my_ants[k].row <= north_row || Game->my_ants[k].row >= south_row)
			{						
				// we're in bounds north-south, check east-west
				if (east_col > west_col)
				{
					// no wrapping, normal check
					if (Game->my_ants[k].col >= west_col && Game->my_ants[k].col <= east_col)
					{
						// we found a friend
						localGoodGuyCount++;
					}
					
				}
				else 
				{
					// make sure we account for wrapping
					// if we are between 0 and the east col or between the west col and cols 
					if (Game->my_ants[k].col <= east_col || Game->my_ants[k].col >= west_col)
					{
						// we found a friend
						localGoodGuyCount++;
					}
				}						
			}					
		}
	}
	DEBUG("%d good guys  row %d, col %d at dist %d\n", localGoodGuyCount, row, col, distance);
	return localGoodGuyCount;
}

// returns the number of bad guys within distance
static int getBadGuyCount (int row, int col, int distance, struct game_state *Game, struct game_info *Info)
{
	int localBadGuyCount = 0;
	int k;
	
	int north_row = (row - distance < 0 ? Info->rows - 1 - (distance - row) : row - distance);
	int south_row = (row + distance > Info->rows - 1 ? row + distance - (Info->rows - 1) : row + distance);
	int east_col = (col + distance > Info->cols - 1 ? col + distance - (Info->cols - 1) : col + distance);
	int west_col = (col - distance < 0 ? Info->cols - 1 - (distance - col) : col - distance);
	
	// figure out how many bad guys are in the vicinity
	for (k = 0; k < Game->enemy_count; k++)
	{
		// check north-south
		if (south_row > north_row)
		{
			// no wrapping north-south, normal check
			if (Game->enemy_ants[k].row <= south_row && Game->enemy_ants[k].row >= north_row)
			{
				// we're in range north-south, check east-west
				if (east_col > west_col)
				{
					// no wrapping, normal check
					if (Game->enemy_ants[k].col >= west_col && Game->enemy_ants[k].col <= east_col)
					{
						// grrrr
						localBadGuyCount++;
					}
					else 
					{
						// make sure we account for wrapping
						// if we are between 0 and the east col or between the west col and cols 
						if (Game->enemy_ants[k].col <= east_col || Game->enemy_ants[k].col >= west_col)
						{
							// grrrr
							localBadGuyCount++;
						}
					}		
					
				}
			}
		}
		else 
		{
			// make sure we account for wrapping
			// if we are between 0 and the north row or between the south row and rows
			if (Game->enemy_ants[k].row <= north_row || Game->enemy_ants[k].row >= south_row)
			{						
				// we're in bounds north-south, check east-west
				if (east_col > west_col)
				{
					// no wrapping, normal check
					if (Game->enemy_ants[k].col >= west_col && Game->enemy_ants[k].col <= east_col)
					{
						// grrrr
						localBadGuyCount++;
					}
					
				}
				else 
				{
					// make sure we account for wrapping
					// if we are between 0 and the east col or between the west col and cols 
					if (Game->enemy_ants[k].col < east_col || Game->enemy_ants[k].col > west_col)
					{
						// grrrr
						localBadGuyCount++;
					}
				}						
			}					
		}	
	}
	DEBUG("%d bad guys around row %d, col %d at dist %d\n", localBadGuyCount, row, col, distance);
	return localBadGuyCount;
}


static void recruitTheTroops(int row, int col, ACTION_e action, int recruitDistance,  struct game_state *Game, struct game_info *Info, int destrow, int destcol)
{
	int k;
	int north_row = (row - recruitDistance < 0 ? Info->rows - 1 - (recruitDistance - row) : row - recruitDistance);
	int south_row = (row + recruitDistance > Info->rows - 1 ? row + recruitDistance - (Info->rows - 1) : row + recruitDistance);
	int east_col = (col + recruitDistance > Info->cols - 1 ? col + recruitDistance - (Info->cols - 1) : col + recruitDistance);
	int west_col = (col - recruitDistance < 0 ? Info->cols - 1 - (recruitDistance - col) : col - recruitDistance);
	
	DEBUG("recruiting troops to r:%d c:%d for %d to r:%d c:%d...", row, col, action, destrow, destcol);
	
	// recruit all of the friends are in the vicinity
	for (k = 0; k < Game->my_count; k++)
	{
		// check north-south
		if (south_row > north_row)
		{
			// no wrapping north-south, normal check
			if (Game->my_ants[k].row <= south_row && Game->my_ants[k].row >= north_row)
			{
				// we're in range north-south, check east-west
				if (east_col > west_col)
				{
					// no wrapping, normal check
					if (Game->my_ants[k].col >= west_col && Game->my_ants[k].col <= east_col)
					{
						if (antMissions[k].action > action)
						{
							antMissions[k].action = action;
							antMissions[k].row = destrow;
							antMissions[k].col = destcol;
							DEBUG(" %u ", k);
						}
					}
					else 
					{
						// make sure we account for wrapping
						// if we are between 0 and the east col or between the west col and cols 
						if (Game->my_ants[k].col <= east_col || Game->my_ants[k].col >= west_col)
						{
							if (antMissions[k].action > action)
							{
								antMissions[k].action = action;
								antMissions[k].row = destrow;
								antMissions[k].col = destcol;
								DEBUG(" %u ", k);
							}
						}
					}		
					
				}
			}
		}
		else 
		{
			// make sure we account for wrapping
			// if we are between 0 and the north row or between the south row and rows
			if (Game->my_ants[k].row <= north_row || Game->my_ants[k].row >= south_row)
			{						
				// we're in bounds north-south, check east-west
				if (east_col > west_col)
				{
					// no wrapping, normal check
					if (Game->my_ants[k].col >= west_col && Game->my_ants[k].col <= east_col)
					{
						if (antMissions[k].action > action)
						{
							antMissions[k].action = action;
							antMissions[k].row = destrow;
							antMissions[k].col = destcol;
							DEBUG(" %u ", k);
						}
					}
					
				}
				else 
				{
					// make sure we account for wrapping
					// if we are between 0 and the east col or between the west col and cols 
					if (Game->my_ants[k].col < east_col || Game->my_ants[k].col > west_col)
					{
						if (antMissions[k].action > action)
						{
							antMissions[k].action = action;
							antMissions[k].row = destrow;
							antMissions[k].col = destcol;							
							DEBUG(" %u ", k);							
						}
					}
				}						
			}					
		}	
	}
	DEBUG("...done%d\n", k);
}


static inline void getNearestNonWaterCoords(int* row, int* col, struct game_info *Info)
{
	int currentOffset = (*row)*Info->cols + (*col);
	if (Info->map[currentOffset] != '%')
		return;

	int tempCol, tempRow, i;
	for (i = 0; i < 10; i++)
	{
		tempRow = (*row - i < 0 ? Info->rows - 1 - (i - *row) : *row - i);
		tempCol = *col;
		currentOffset = tempRow*Info->cols + tempCol;
		
		// NORTH
		if (tempRow != 0)
		{
			if(Info->map[currentOffset + UP] != '%')
			{
				*row = tempRow - 1;
				return;
			}
		}
		else
		{
			if (Info->map[currentOffset + (Info->rows - 1)*Info->cols] != '%')
			{
				*row = Info->rows - 1;
				return;
			}
		}
			
		
		tempRow = *row;
		tempCol = (*col + i > Info->cols - 1 ? *col + i - (Info->cols - 1) : *col + i);
		currentOffset = tempRow*Info->cols + tempCol;
		
		// EAST
		if (tempCol != Info->cols - 1)
		{
			if (Info->map[currentOffset + RIGHT] != '%')
			{
				*col = tempCol + 1;
				return;
			}
		}
		else
		{
			if (Info->map[currentOffset - Info->cols - 1] != '%')
			{
				*col = 0;
				return;
			}
		}
			
		tempRow = (*row + i > Info->rows - 1 ? *row + i - (Info->rows - 1) : *row + i);
		tempCol = *col;
		currentOffset = tempRow*Info->cols + tempCol;
		
		// SOUTH
		if (tempRow != Info->rows - 1)
		{
			if (Info->map[currentOffset + DOWN] != '%')
			{
				*row = tempRow + 1;
				return;
			}
		}
		else 
		{
			if (Info->map[currentOffset - (Info->rows - 1)*Info->cols] != '%')
			{
				*row = 0;
				return;
			}
		}

		tempRow = *row;
		tempCol = (*col - i < 0 ? Info->cols - 1 - (i - *col) : *col - i);
		currentOffset = tempRow*Info->cols + tempCol;
		
		// WEST
		if (tempCol != 0)
		{
			if (Info->map[currentOffset + LEFT] != '%')
			{
				*col = tempCol - 1;
				return;
			}
		}
		else 
		{
			if (Info->map[currentOffset + Info->cols - 1] != '%')
			{
				*col = Info->cols - 1;
				return;
			}
		}		
	}
}

void _init_arrays(struct game_info *Info)
{
	int rowsCols = Info->rows*Info->cols;
	inWhichQueue = (int*)malloc(rowsCols*sizeof(int));
	gCost = (int*)malloc(rowsCols*sizeof(int));
	parent = (int*)malloc(rowsCols*sizeof(int));
	antMissions = (mission*)malloc(rowsCols*sizeof(mission));
}


int hasLegalMoves(int ant, struct game_state *Game, struct game_info *Info)
{
	int currentOffset= Game->my_ants[ant].row*Info->cols + Game->my_ants[ant].col;
	// figure out what's happening around us
	// NORTH
	if (Game->my_ants[ant].row != 0)
	{
		if(Info->map[currentOffset + UP] != '%' && 
		   Info->map[currentOffset + UP] != 'a')
			return true;
	}
	else
	{
		if (Info->map[currentOffset + (Info->rows - 1)*Info->cols] != '%' &&
			Info->map[currentOffset + (Info->rows - 1)*Info->cols] != 'a')
			return true;
	}
	
	// EAST
	if (Game->my_ants[ant].col != Info->cols - 1)
	{
		if (Info->map[currentOffset + RIGHT] != '%' &&
			Info->map[currentOffset + RIGHT] != 'a')
			return true;
	}
	else
	{
		if (Info->map[currentOffset - Info->cols - 1] != '%' &&
			Info->map[currentOffset - Info->cols - 1] != 'a')
			return true;
	}
	
	// SOUTH
	if (Game->my_ants[ant].row != Info->rows - 1)
	{
		if (Info->map[currentOffset + DOWN] != '%' &&
			Info->map[currentOffset + DOWN] != 'a')
			return true;
	}
	else
	{
		if (Info->map[currentOffset - (Info->rows - 1)*Info->cols] != '%' &&
			Info->map[currentOffset - (Info->rows - 1)*Info->cols] != 'a')
			return true;
	}
		
	
	// WEST
	if (Game->my_ants[ant].col != 0)
	{
		if (Info->map[currentOffset + LEFT] != '%' &&
			Info->map[currentOffset + LEFT] != 'a')
			return true;
	}
	else
	{
		if (Info->map[currentOffset + Info->cols - 1] != '%' &&
			Info->map[currentOffset + Info->cols - 1] != 'a')
			return true;
	}
	return false;
}

void printMap(struct game_info *Info)
{

	int i, j;
    for (i = 0; i < Info->rows; i++) 
	{
		printf("\n");
		for (j = 0; j < Info->cols; j++)
		{
			printf("%c", Info->map[i*Info->cols+j]);
		}
	}
}


