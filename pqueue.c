/*	pqueue.c

	Implementation of an array-based heap for abstract
	priority pqueue data type.

	by: Jonathan Haywood
	begun: March 19, 2006
*/

#include "pqueue.h"



void Initialize(PriorityQueue *q)
{
        q->count = 0;
}

void Insert(PriorityQueue *q, ElementType * x)
{
		int parent, leaf;
		ElementType value, temp;

        if (q->count >= PQUEUESIZE)
		{
			printf("Warning: pqueue overflow enpqueue");
			return;
		}
       
		q->count++;
		leaf = q->count - 1;		
		q->q[leaf] = *x;			

		// percolate up
		parent = PARENT(leaf);
		value = q->q[leaf];
		while(leaf > 0 && (COST(q->q[leaf]) < COST(q->q[parent])))
		{
			temp = q->q[leaf];
			q->q[leaf] = q->q[parent];
			q->q[parent] = temp;
			leaf = parent;
			parent = PARENT(leaf);
		}
		q->q[leaf] = value;
}

ElementType FindMin(PriorityQueue *q)
{
		int heapsize, root, childpos;
		ElementType minVal, temp;
		ElementType value;

		minVal = q->q[0];
		q->q[0] = q->q[q->count-1];		// take last element and put on root of tree
		q->count--;						// adjust size

		root = 0;
		if(q->count > 1)
		{
			// percolate down
			heapsize = q->count;
			value = q->q[root];			// get root
			while(root < heapsize)
			{
				childpos = LEFT(root);
				if(childpos < heapsize)
				{
					if((RIGHT(root) < heapsize) &&
						(COST(q->q[childpos+1]) <
						 COST(q->q[childpos])))
					{
						childpos++;
					}
					if(COST(q->q[childpos]) < COST(q->q[root]))
					{
						temp = q->q[root];
						q->q[root] = q->q[childpos];
						q->q[childpos] = temp;
						root = childpos;
					}
					else
					{
						q->q[root] = value;
						break;
					}
				}
				else
				{
					q->q[root] = value;
					break;
				}
			}
		}
		return minVal;
}

int isEmpty(PriorityQueue *q)
{
        if (q->count == 0) return (TRUE);
        else return (FALSE);
}



void printQueue(PriorityQueue *q)
{
	int i;
	printf("{ ");
	for(i=0; i < q->count; i++) printf("%d ",q->q[i].costPlusEstimate);
	printf("}n");
}

//
//int main()
//{
//	// Example use of priority queue
//	pqueue queue;
//	node n1, n2, n3;
//	n1.data = 1;
//	n1.cost = 1;
//	n2.data = 0;
//	n2.cost = 0;
//	n3.data = 3;
//	n3.cost = 3;
//
//	init_queue(&queue);
//	enqueue(&queue, &n1);
//	enqueue(&queue, &n2);
//	enqueue(&queue, &n3);
//
//	print_queue(&queue);
//	return 0;
//}

