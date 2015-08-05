#include "binheap.h"
#include <stdlib.h>
#include <stdio.h>

#define MinPQSize (10)
#define MinData (-32767)

struct HeapStruct 
{
    int Capacity;
    int Size;
    ElementType *Elements;
};

PriorityQueue Initialize(int MaxElements) 
{
    PriorityQueue H;

    /* 1*/ if (MaxElements < MinPQSize)
	{
		/* 2*/ printf("Priority queue size is too small");
		exit(0);
	}


    /* 3*/ H = malloc(sizeof ( struct HeapStruct));
    /* 4*/ if (H == NULL)
	{
		printf("error: out of space allocating binary heap");
		exit(0);		
	}
	/* 5*/	
    /* Allocate the array plus one extra for sentinel */
    /* 6*/ H->Elements = malloc((MaxElements + 1)
            * sizeof ( ElementType));
    /* 7*/ if (H->Elements == NULL)
	{
/* 8*/ printf("error: out of space allocating elements");
		exit(0);
	}


    /* 9*/ H->Capacity = MaxElements;
    /*10*/ H->Size = 0;
    /*11*/ H->Elements[ 0 ].costPlusEstimate = MinData;

    /*12*/ return H;
}

/* END */

void MakeEmpty(PriorityQueue H) 
{
    H->Size = 0;
	H->Elements[ 0 ].costPlusEstimate = MinData;
}

/* H->Element[ 0 ] is a sentinel */

void Insert(ElementType X, PriorityQueue H) {
    int i;

    if (IsFull(H)) {
        printf("error: attempting to insert element into a full priority queue\n");
		exit(0);
    }

    for (i = ++H->Size; H->Elements[ i / 2 ].costPlusEstimate > X.costPlusEstimate; i /= 2)
        H->Elements[ i ] = H->Elements[ i / 2 ];
    H->Elements[ i ] = X;
}

/* END */


ElementType DeleteMin(PriorityQueue H) 
{
    int i, Child;
    ElementType MinElement, LastElement;

    /* 1*/ if (IsEmpty(H)) 
	{
        /* 2*/ printf("error: attempting to delete min on an empty priority queue\n");
		exit(0);
    }
    /* 4*/ MinElement = H->Elements[ 1 ];
    /* 5*/ LastElement = H->Elements[ H->Size-- ];

    /* 6*/ for (i = 1; i << 1 <= H->Size; i = Child) 
	{
        /* Find smaller child */
        /* 7*/ Child = i << 1;
        /* 8*/ if (Child != H->Size && H->Elements[ Child + 1 ].costPlusEstimate
                /* 9*/ < H->Elements[ Child ].costPlusEstimate)
            /*10*/ Child++;

        /* Percolate one level */
        /*11*/ if (LastElement.costPlusEstimate > H->Elements[ Child ].costPlusEstimate)
            /*12*/ H->Elements[ i ] = H->Elements[ Child ];
        else
            /*13*/ break;
    }
    /*14*/ H->Elements[ i ] = LastElement;
    /*15*/ return MinElement;
}

ElementType FindMin(PriorityQueue H) {
    if (!IsEmpty(H))
        return H->Elements[ 1 ];
    printf("error: attempting to find min on empty priority queue\n");
	exit(0);
}

int IsEmpty(PriorityQueue H) {
    return H->Size == 0;
}

int IsFull(PriorityQueue H) {
    return H->Size == H->Capacity;
}

void Destroy(PriorityQueue H) {
    free(H->Elements);
    free(H);
}

#if 0

for (i = N / 2; i > 0; i--)
    PercolateDown(i);

#endif
