#ifndef GAME_H
#define GAME_H

#include "ants.h"

// defining things just so we can do less writing
// UP and DOWN move up and down rows while LEFT and RIGHT
// move side to side. The map is just one big array.
#define UP -Info->cols
#define DOWN Info->cols
#define LEFT -1
#define RIGHT 1

#define true (1)
#define false (0)

// defining things to do less writing again
#define ROW Game->my_ants[i].row
#define COL Game->my_ants[i].col
#define ID Game->my_ants[i].id
#define FROW Game->food[j].row
#define FCOL Game->food[j].col
#define EROW Game->enemy_ants[j].row
#define ECOL Game->enemy_ants[j].col

#define CLOSED (1)
#define OPEN (2)
#define NO_QUEUE (0)
#define OPEN_LIST_SIZE (1000)

#define FOOD_SEARCH_DISTANCE (10)
#define FOOD_PERSUAL_DISTANCE (15)

#define ENEMY_SEARCH_DISTANCE (7)
#define ENEMY_AWARENESS_DISTANCE (6)

#define ENEMY_HIVE_SEARCH_DISTANCE (10)
#define ENEMY_HIVE_AWARENESS_DISTANCE (15)

#define YOURE_GETTING_TOO_CLOSE_DISTANCE (10)
#define TOO_MANY_ENEMIES_NEAR_MY_HIVE_DISTANCE (3)

#define FRIEND_DISTANCE (2)
#define ENEMY_DISTANCE (4)
#define RUN_DISTANCE (5)
#define EXPLORE_DISTANCE (8)

#define RECRUIT_DISTANCE_1_ENEMY (3)
#define RECRUIT_DISTANCE_2_ENEMIES (5)
#define RECRUIT_DISTANCE_3_ENEMIES (6)
#define RECRUIT_DISTANCE_4_PLUS_ENEMIES (8)
#define RECRUIT_DISTANCE_HELP_THE_HIVE (15)
#define RECRUIT_DISTANCE_ATTACK_A_HIVE (12)

// number of ants i have to have before i keep 1 at base the entire time to defend
#define HIVE_DEFENDER_AMOUNT (10) 

#define DEBUG_ENABLE 1

//#define DEBUG(fmt, ...)\
//do { if (DEBUG_ENABLE) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
//						__LINE__, __func__, __VA_ARGS__); } while (0)

#define DEBUG(fmt, ...) \
do { if (DEBUG_ENABLE) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

typedef struct
{
	int offset;
	int cost;
} search_info;


typedef enum
{
	WATCH_THE_HIVE = 0,  // sorted by priority, lowest # = highest priority
	DEFEND_THE_HIVE,
	RUN,
	GET_FOOD,
	ATTACK_ENEMY_HIVE,
	FORM_FIGHTING_GROUP,
	EXPLORE,
	NO_ACTION,
} ACTION_e;

typedef struct
{
	ACTION_e action;
	int row;
	int col;
} mission;


void do_turn(struct game_state *Game, struct game_info *Info);
int distance(int row1, int col1, int row2, int col2, struct game_info *Info);
void move(int index, char dir, struct game_state* Game, struct game_info* Info);
void _init_arrays(struct game_info *Info);
#endif // GAME_H
