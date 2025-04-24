#include <stdlib.h>
#include <float.h>

#include "dijkstra.h"
#include "../../structures/prio-queue.h"
#include "../../structures/graph.h"

PathsData *createPathsData(int n) {
	PathsData *pathsData = calloc(1, sizeof(PathsData));
	pathsData->dist = calloc(n, sizeof(double));
	pathsData->preEdgeIndices = calloc(n, sizeof(int));

	for (int i = 0; i < n; i++) {
		pathsData->dist[i] = DBL_MAX;
		pathsData->preEdgeIndices[i] = -1;
	}
	return pathsData;
}

void cleanPathsData(PathsData *pathsData, int n) {
	for (int i = 0; i < n; i++) {
		pathsData->preEdgeIndices[i] = -1;
		pathsData->dist[i] = DBL_MAX;
	}
}

void freePathsData(PathsData *pathsData) {
	free(pathsData->dist);
	free(pathsData->preEdgeIndices);
	free(pathsData);
}

void printPathsData(PathsData *pd, int n, Graph *g) {
	printf("\ndist: ");
	for (int v = 0; v < n; v++) {
		printf("%.2lf; ", pd->dist[v]);
	}
	printf("\npre-edges: ");
	printEdgeIndices(pd->preEdgeIndices, n, g);
}

PathsData **createMultiPathDatas(int nPaths, int pathLen) {
	PathsData **pathsDatas = calloc(nPaths, sizeof(PathsData*));
	for (int t = 0; t < nPaths; t++) {
		pathsDatas[t] = createPathsData(pathLen);
	}
	return pathsDatas;
}

void freeMultiPathsDatas(PathsData **pathsDatas, int nPaths) {
	for (int p = 0; p < nPaths; p++) {
		freePathsData(pathsDatas[p]);
	}
	free(pathsDatas);
}

void multiDijkstra(int *sources, int nSources, PathsData *pathsData, Graph *g) {
	PrioQueue *pq = createPrioQueue(g->n);

	for (int i = 0; i < nSources; i++) {
		int s = sources[i];
		pathsData->dist[s] = 0;
		insert((Pair){s, 0}, pq);
	}

	while (!isEmpty(pq)) {
		Pair v = extractMin(pq);

		if (v.value != pathsData->dist[v.key]) {
			continue; // Skip old entry
		}

		for (int i = 0; i < g->vertices[v.key].deg; i++) {
			Edge *e = getEdge(v.key, i, g);

			int w = getPredecessor(v.key, *e);
			double newDist = pathsData->dist[v.key] + e->cost;
			
			if (pathsData->dist[w] > newDist) {
				pathsData->dist[w] = newDist;
				pathsData->preEdgeIndices[w] = getEdgeIndex(v.key, i, g);
				insert((Pair){w, newDist}, pq);
			}
		}
	}
	freePrioQueue(pq);
}

void dijkstra(int s, PathsData *pathsData, Graph *g) {
	multiDijkstra(&s, 1, pathsData, g);
}
