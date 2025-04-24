#ifndef PRIOQUEUE_H
#define PRIOQUEUE_H

#include <stdbool.h>

#include "graph.h"

typedef struct {
	int key;
	double value;
} Pair;

typedef struct {
	int n;
	int indexLastLeaf;
	Pair *queue;
} PrioQueue;

PrioQueue *createPrioQueue(int n);

void freePrioQueue(PrioQueue *pq);

bool isEmpty(PrioQueue *pq);

void insert(Pair p, PrioQueue *pq);

// for efficiency, only insert is used
// void decreaseKey(int value, int newPrio, PrioQueue *pq);

Pair extractMin(PrioQueue *pq);

void printQueue(PrioQueue *pq);

#endif
