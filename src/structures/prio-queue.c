#include <stdio.h>
#include <stdlib.h>

#include "prio-queue.h"

PrioQueue *createPrioQueue(int n) {
	PrioQueue *pq = calloc(1, sizeof(PrioQueue));
	Pair *queue = calloc(n, sizeof(Pair));
	
	*pq = (PrioQueue){n, -1, queue};
	return pq;
}

void freePrioQueue(PrioQueue *pq) {
	free(pq->queue);
	free(pq);
}

bool isEmpty(PrioQueue *pq) {
	return pq->indexLastLeaf == -1;
}

void swap(int i, int j, PrioQueue *pq) {
	Pair tmp = pq->queue[i];
	pq->queue[i] = pq->queue[j];
	pq->queue[j] = tmp;
}

void sink(PrioQueue *pq) {
	int i= 0;

	while ((2*i) + 1 <= pq->indexLastLeaf) {
		// There is a left child
		int l = (2*i) + 1;
		int r = l + 1;
		
		if (r <= pq->indexLastLeaf && pq->queue[r].value < pq->queue[l].value) {
			// Both children exist. Select smaller one.
			l = r;
		}
		if (pq->queue[i].value <= pq->queue[l].value) {
			// Parent <= smallest child
			break;
		}
		swap(i, l, pq);
		i = l;
	}
}

void bubbleUp(PrioQueue *pq) {
	int i = pq->indexLastLeaf;

	while (i > 0) {
		int parent = (i-1) / 2;

		if (pq->queue[i].value >= pq->queue[parent].value) {
			break;
		}
		swap(i, parent, pq);
		i = parent;
	}
}

void resizePrioQueue(PrioQueue *pq, int newSize) {
	Pair *newQueue = realloc(pq->queue, newSize * sizeof(Pair));
	pq->queue = newQueue;
	pq->n = newSize;
}

void insert(Pair p, PrioQueue *pq) {
	if (pq->indexLastLeaf+1 == pq->n) {
		resizePrioQueue(pq, pq->n * 2); // Double capacity when full
	}
	pq->indexLastLeaf++;
	pq->queue[pq->indexLastLeaf] = p;
	bubbleUp(pq);
}

Pair extractMin(PrioQueue *pq) {
	if (pq->indexLastLeaf == -1) {
		fprintf(stderr, "Underflow in PrioQueue.\n");
		exit(EXIT_FAILURE);
	}
	Pair min = pq->queue[0];
	
	swap(0, pq->indexLastLeaf, pq);
	pq->indexLastLeaf--;
	sink(pq);
	return min;
}

void printPrioQueue(PrioQueue *pq) {
	for (int i = 0; i <= pq->indexLastLeaf; i++) {
		printf("{key: %d, value: %.2lf}\n", 
			pq->queue[i].key, pq->queue[i].value
		);
	}
}
