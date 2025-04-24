#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

#include "graph.h"

typedef struct {
	int n;
	int cap;
	int *data;
	int frontI;
	int backI;
} Queue;

Queue *createQueue(int cap);

void freeQueue(Queue *q);

bool isEmptyQueue(Queue *q);

void enqueue(int val, Queue *q);

int dequeue(Queue *q);

void printQueue(Queue *q);

#endif
