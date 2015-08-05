#include "game.h"
#include "ants.h" 

// returns the absolute value of a number; used in distance function
#define abs(x) (x >= 0 ? (x) : -(x))

int ipow(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }
	
    return result;
}

/*
 // Integer Square Root function
 // Contributors include Arne Steinarson for the basic approximation idea, 
 // Dann Corbit and Mathew Hendry for the first cut at the algorithm, 
 // Lawrence Kirby for the rearrangement, improvments and range optimization
 // and Paul Hsieh for the round-then-adjust idea.
 */
static unsigned fred_sqrt(unsigned long x) {
	static const unsigned char sqq_table[] = {
		0,  16,  22,  27,  32,  35,  39,  42,  45,  48,  50,  53,  55,  57,
		59,  61,  64,  65,  67,  69,  71,  73,  75,  76,  78,  80,  81,  83,
		84,  86,  87,  89,  90,  91,  93,  94,  96,  97,  98,  99, 101, 102,
		103, 104, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116, 117, 118,
		119, 120, 121, 122, 123, 124, 125, 126, 128, 128, 129, 130, 131, 132,
		133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 144, 145,
		146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155, 156, 157,
		158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168,
		169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 178,
		179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188,
		189, 189, 190, 191, 192, 192, 193, 193, 194, 195, 195, 196, 197, 197,
		198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206,
		207, 208, 208, 209, 209, 210, 211, 211, 212, 212, 213, 214, 214, 215,
		215, 216, 217, 217, 218, 218, 219, 219, 220, 221, 221, 222, 222, 223,
		224, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230, 230, 231,
		231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
		239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246,
		246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253,
		253, 254, 254, 255
	};
	
	unsigned long xn;
	
	if (x >= 0x10000)
		if (x >= 0x1000000)
			if (x >= 0x10000000)
				if (x >= 0x40000000) {
					if (x >= 65535UL*65535UL)
						return 65535;
					xn = sqq_table[x>>24] << 8;
				} else
					xn = sqq_table[x>>22] << 7;
	            else
	                if (x >= 0x4000000)
	                    xn = sqq_table[x>>20] << 6;
	                else
	                    xn = sqq_table[x>>18] << 5;
					else {
						if (x >= 0x100000)
							if (x >= 0x400000)
								xn = sqq_table[x>>16] << 4;
							else
								xn = sqq_table[x>>14] << 3;
							else
								if (x >= 0x40000)
									xn = sqq_table[x>>12] << 2;
								else
									xn = sqq_table[x>>10] << 1;
						
						goto nr1;
					}
					else
						if (x >= 0x100) {
							if (x >= 0x1000)
								if (x >= 0x4000)
									xn = (sqq_table[x>>8] >> 0) + 1;
								else
									xn = (sqq_table[x>>6] >> 1) + 1;
								else
									if (x >= 0x400)
										xn = (sqq_table[x>>4] >> 2) + 1;
									else
										xn = (sqq_table[x>>2] >> 3) + 1;
							
							goto adj;
						} else
							return sqq_table[x] >> 4;
	
	/* Run two iterations of the standard convergence formula */
	
	xn = (xn + 1 + x / xn) / 2;
nr1:
	xn = (xn + 1 + x / xn) / 2;
adj:
	
	if (xn * xn > x) /* Correct rounding if necessary */
		xn--;
	
	return xn;
}



// returns the distance between two items on the grid accounting for map wrapping
int distance(int row1, int col1, int row2, int col2, struct game_info *Info) 
{
    int dr, dc;
    int abs1, abs2;

    abs1 = abs(row1 - row2);
    abs2 = Info->rows - abs(row1 - row2);

    if (abs1 > abs2)
        dr = abs2;
    else
        dr = abs1;

    abs1 = abs(col1 - col2);
    abs2 = Info->cols - abs(col1 - col2);

    if (abs1 > abs2)
        dc = abs2;
    else
        dc = abs1;

    return fred_sqrt(ipow(dr, 2) + ipow(dc, 2));
}


// sends a move to the tournament engine and keeps track of ants new location
void move(int index, char dir, struct game_state* Game, struct game_info* Info) 
{
    fprintf(stdout, "O %i %i %c\n", Game->my_ants[index].row, Game->my_ants[index].col, dir);

    switch (dir) {
        case 'N':
            if (Game->my_ants[index].row != 0)
                Game->my_ants[index].row -= 1;
            else
                Game->my_ants[index].row = Info->rows - 1;
            break;
        case 'E':
            if (Game->my_ants[index].col != Info->cols - 1)
                Game->my_ants[index].col += 1;
            else
                Game->my_ants[index].col = 0;
            break;
        case 'S':
            if (Game->my_ants[index].row != Info->rows - 1)
                Game->my_ants[index].row += 1;
            else
                Game->my_ants[index].row = 0;
            break;
        case 'W':
            if (Game->my_ants[index].col != 0)
                Game->my_ants[index].col -= 1;
            else
                Game->my_ants[index].col = Info->cols - 1;
            break;
    }
}


// just a function that returns the string on a given line for i/o
// you don't need to worry about this
char *get_line(char *text) 
{
    char *tmp_ptr = text;
    int len = 0;

    while (*tmp_ptr != '\n') 
	{
        tmp_ptr++;
        len++;
    }

    char *return_str = malloc(len + 1);
    memset(return_str, 0, len + 1);

    int i;
    for (i = 0; i < len; i++) 
	{
        return_str[i] = text[i];
    }

    return return_str;
}


// main, communicates with tournament engine
int main(int argc, char *argv[]) 
{	
    int action = -1;

    struct game_info Info;
    struct game_state Game;
    Info.map = 0;

    Game.my_ants = 0;
    Game.enemy_ants = 0;
    Game.food = 0;
    Game.dead_ants = 0;
	Game.my_hives = 0;
	Game.enemy_hives = 0;

    while (1) 
	{
        int initial_buffer = 100000;

        char *data = malloc(initial_buffer);
        memset(data, 0, initial_buffer);

        *data = '\n';

        char *ins_data = data + 1;

        int i = 0;

        while (1) 
		{
            i++;

            if (i > initial_buffer) 
			{
                initial_buffer *= 2;
                data = realloc(data, initial_buffer);
                memset(ins_data, 0, initial_buffer/2);
            }

            *ins_data = getchar();

            if (*ins_data == '\n') 
			{
                char *backup = ins_data;

                while (*(backup - 1) != '\n') 
				{
                    backup--;
                }

                char *test_cmd = get_line(backup);

                if (strcmp(test_cmd, "go") == 0) 
				{
                    action = 0; 
                    free(test_cmd);
                    break;
                }
                else if (strcmp(test_cmd, "ready") == 0) 
				{
                    action = 1;
                    free(test_cmd);
                    break;
                }
                free(test_cmd);
            }
            
            ins_data++;
        }

        if (action == 0) 
		{
            char *skip_line = data + 1;
            while (*++skip_line != '\n');
            ++skip_line;

            _init_map(skip_line, &Info);
            _init_game(&Info, &Game);
			_init_arrays(&Info);
            do_turn(&Game, &Info);
            fprintf(stdout, "go\n");
            fflush(stdout);
        }
        else if (action == 1) 
		{
            _init_ants(data + 1, &Info);

            Game.my_ant_index = -1;

            fprintf(stdout, "go\n");
            fflush(stdout);
        }

        free(data);
    }
}
