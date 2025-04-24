#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

Queue *createQueue(int cap) {
	Queue *q = calloc(1, sizeof(Queue));
	q->data = calloc(cap, sizeof(int));
	q->frontI = 0;
	q->backI = 0;
	q->cap = cap;
	return q;
}

void freeQueue(Queue *q) {
	free(q->data);
	free(q);
}

bool isEmptyQueue(Queue *q) {
	return q->frontI == q->backI;
}

void enqueue(int val, Queue *q) {
	q->data[q->backI] = val;
	q->backI = (q->backI+1) % q->cap;
}

int dequeue(Queue *q) {
	int val = q->data[q->frontI];
	q->frontI = (q->frontI+1) % q->cap;
	return val;
}

void printQueue(Queue *q) {
	for (int i = 0; i < q->backI; i++) {
		printf("%d, ", q->data[i]);
	}
	printf("\n");
}
