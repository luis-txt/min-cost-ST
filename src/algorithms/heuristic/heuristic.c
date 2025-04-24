#include <stdlib.h>
#include <float.h>

#include "heuristic.h"
#include "../mst/prim.h"
#include "../dijkstra/dijkstra.h"
#include "../../structures/queue.h"

static int collectEdgeIndices(int *mstIndices, int n, int *edgeIndices) {
	int nEdges = 0;
	for (int i = 0; i < n; i++) {
		if (mstIndices[i] != -1) {
			edgeIndices[nEdges] = mstIndices[i];
			nEdges++;
		}
	}
	return nEdges;
}

SteinerTree prunedMST(Graph *g, Terminals *terms) {
	int *mstIndices = prim(g, terms->vertices[0]); // Returns for each vertex the edge from the predecessor

	// Counting degrees in MST
	int *degree = calloc(g->n, sizeof(int));
	for (int v = 0; v < g->n; v++) {
		if (mstIndices[v] != -1) {
			Edge *e = &g->edges[mstIndices[v]];
			degree[e->v]++;
			degree[e->w]++;
		}
	}

	bool *isTerminal = calloc(g->n, sizeof(bool));
	for (int i = 0; i < terms->n; i++) {
		isTerminal[terms->vertices[i]] = true;
	}

	Queue *q = createQueue(g->n);
	for (int i = 0; i < g->n; i++) {
		if (mstIndices[i] == -1) {
			continue; // Skip root or not existing edges
		}
		Edge e = g->edges[mstIndices[i]];
		if ((degree[e.v] == 1 && !isTerminal[e.v]) || (degree[e.w] == 1 && !isTerminal[e.w])) {
			enqueue(i, q);
		}
	}

	// Prune edges
	while (!isEmptyQueue(q)) {
		int leaf = dequeue(q);
		if (mstIndices[leaf] == -1) {
			continue; // Skip already processed edges or root
		}

		// Remove edge
		Edge e = g->edges[mstIndices[leaf]];
		int w = getPredecessor(leaf, e);
		mstIndices[leaf] = -1;
		degree[leaf] = 0;
		degree[w]--;
		
		if (degree[w] == 1 && !isTerminal[w]) {
			enqueue(w, q);
		}
	}

	free(degree);
	free(isTerminal);
	freeQueue(q);

	// Create final tree
	int *remainingEdgeIndices = calloc(g->m, sizeof(int));
	int nRemainingEdges = collectEdgeIndices(mstIndices, g->n, remainingEdgeIndices);
	remainingEdgeIndices = realloc(remainingEdgeIndices, nRemainingEdges * sizeof(int)); // Reduce array capacity

	free(mstIndices);
	return (SteinerTree){remainingEdgeIndices, nRemainingEdges};
}

SteinerTree mstST(Graph *g, Terminals *terms) {
	int *mstIndices = prim(g, terms->vertices[0]); // Returns for each vertex the edge from the predecessor
	int *selectedEdgeIndices = calloc(g->m, sizeof(int));
	int nEdges = collectEdgeIndices(mstIndices, g->n, selectedEdgeIndices);
	free(mstIndices);
	return (SteinerTree){selectedEdgeIndices, nEdges};
}

SteinerTree takahashiMatsuyama(Graph *g, Terminals *terms) {
	int *treeVertices = calloc(g->n, sizeof(int));
	treeVertices[0] = terms->vertices[0];
	int nTreeVertices = 1;

	bool *isInTree = calloc(g->n, sizeof(bool));
	isInTree[terms->vertices[0]] = true;

	int *treeEdgeIndices = calloc(g->m, sizeof(int));
	int nTreeEdges = 0;
	
	int nNotInclTerms = terms->n-1;
	int *notInclTerms = calloc(nNotInclTerms, sizeof(int));
	for (int i = 1; i < terms->n; i++) {
		notInclTerms[i-1] = terms->vertices[i];
	}

	PathsData *pathsData = createPathsData(g->n);
	bool *edgesVisited = calloc(g->m, sizeof(bool));

	while (nNotInclTerms > 0) {
		double shortestDist = DBL_MAX;
		int nearestTermI = -1;
		int nearestTerm = -1;

		multiDijkstra(treeVertices, nTreeVertices, pathsData, g);
	
		// Find terminal not already in tree with shortest distance to tree
		for (int i = 0; i < nNotInclTerms; i++) {
			int notIncTerm = notInclTerms[i];
			if (pathsData->dist[notIncTerm] < shortestDist) {
				shortestDist = pathsData->dist[notIncTerm];
				nearestTerm = notIncTerm;
				nearestTermI = i;
			}
		}

		if (nearestTermI == -1) {
			break; // No reachable terminal is found
		}

		notInclTerms[nearestTermI] = notInclTerms[nNotInclTerms-1];
		nNotInclTerms--;

		// Add shortest path to tree
		int vI = nearestTerm;
		int eI = pathsData->preEdgeIndices[vI];
		while (eI != -1) {
			Edge e = g->edges[eI];
			
			if (edgesVisited[eI]) {
				// Edge has already been added
				int pred = getPredecessor(vI, e);
				vI = pred;
				eI = pathsData->preEdgeIndices[vI];
				continue;
			}
			edgesVisited[eI] = true;

			// Add edge
			treeEdgeIndices[nTreeEdges] = eI;
			nTreeEdges++;
			
			// Add vertices
			if (!isInTree[e.v]) {
				isInTree[e.v] = true;
				treeVertices[nTreeVertices] = e.v;
				nTreeVertices++;
			}
			if (!isInTree[e.w]) {
				isInTree[e.w] = true;
				treeVertices[nTreeVertices] = e.w;
				nTreeVertices++;
			}
			
			vI = getPredecessor(vI, e);
			eI = pathsData->preEdgeIndices[vI];
		}
		cleanPathsData(pathsData, g->n);
	}

	freePathsData(pathsData);
	free(edgesVisited);
	free(treeVertices);
	free(notInclTerms);
	free(isInTree);

	return (SteinerTree){treeEdgeIndices, nTreeEdges};
}
