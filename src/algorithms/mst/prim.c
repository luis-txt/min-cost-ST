#include <stdlib.h>
#include <float.h>
#include <stdbool.h>

#include "prim.h"
#include "../../structures/prio-queue.h"
#include "../../utils.h"

int *prim(Graph *g, int root) {
	PrioQueue *pq = createPrioQueue(g->n);
	double *dist = calloc(g->n, sizeof(double));
	int *preEdgeIndices = calloc(g->n, sizeof(int));
	for (int i = 0; i < g->n; i++) {
		preEdgeIndices[i] = -1; // Init all indices to no valid value (representing NULL)
	}
	bool *inPQ = calloc(g->n, sizeof(bool));

	// Init priority-queue and distances
	for (int i = 0; i < g->n; i++) {
		dist[i] = DBL_MAX;
		inPQ[i] = true;
	}
	dist[root] = 0;
	insert((Pair){root, 0}, pq);

	// Create MST
	while (!isEmpty(pq)) {
		Pair v = extractMin(pq);
		
		if (!inPQ[v.key]) {
			continue;
		}
		inPQ[v.key] = false;

		// Update keys of neighborhood
		for (int i = 0; i < g->vertices[v.key].deg; i++) {
			Edge *e = getEdge(v.key, i, g);
			int w = getPredecessor(v.key, *e);
			double cost = e->cost;

			if (inPQ[w] && cost < dist[w]) {
				dist[w] = cost;
				preEdgeIndices[w] = getEdgeIndex(v.key, i, g);
				insert((Pair){w, cost}, pq);
			}
		}
	}

	freePrioQueue(pq);
	free(dist);
	free(inPQ);
	return preEdgeIndices;
}
