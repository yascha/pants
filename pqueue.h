/*	pqueue.h

	Header file for pqueue implementation

	by: Jonathan haywod
*/

#include <stdio.h>

#define PQUEUESIZE       1000
#define PARENT(i)		((i-1)/2)
#define LEFT(i)			((2*i+1))
#define RIGHT(i)		(2*(i+1))
#define COST(i)			(i.costPlusEstimate)

#define TRUE 1
#define FALSE 0


struct node
{
	int costPlusEstimate;
	int costToGetHere;
	int row;
	int col;
	struct node* parent;
} ;

typedef struct node ElementType;


typedef struct 
{
        ElementType q[PQUEUESIZE+1];			/* body of pqueue */
        int count;                      /* number of pqueue elements */
} PriorityQueue;