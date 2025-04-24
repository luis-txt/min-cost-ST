#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <omp.h>

#include "two-apx.h"
#include "../mst/prim.h"
#include "../dijkstra/dijkstra.h"
#include "../../structures/buffer.h"

static int addEdgeToEdges(int edgeI, int *edges, int nEdges, bool *edgesVisited) {
	if (edgeI != -1 && !edgesVisited[edgeI]) {
		edges[nEdges] = edgeI;
		edgesVisited[edgeI] = true;
		nEdges++;
	}
	return nEdges;
}

static int updateTreeVertices(int v, int *treeVertices, bool *inTree, int nTreeVertices) {
	if (!inTree[v]) {
		inTree[v] = true;
		treeVertices[nTreeVertices] = v;
		nTreeVertices++;
	}
	return nTreeVertices;
}

static void clearFlags(int *edgeIndices, int nEdges, bool *visitedEdges) {
	for (int i = 0; i < nEdges; i++) {
		visitedEdges[edgeIndices[i]] = false;
	}
}

Graph *createMetricClosure(Graph *g, Terminals *terms) {
	int *terminals = terms->vertices;
	int nTerminals = terms->n;

	int nThreads = omp_get_max_threads();
	PathsData **pathsDatas = createMultiPathDatas(nThreads, g->n);
	Buffer *tBuffs = createBuffers(nThreads, sizeof(Edge));

	// Collect edges in parallel
	#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < nTerminals-1; i++) { // Last dijkstra info has already been computed by other runs
		int tID = omp_get_thread_num();
		PathsData *localPathsData = pathsDatas[tID];

		int v = terminals[i];
		dijkstra(v, localPathsData, g);

		for (int j = i+1; j < nTerminals; j++) {
			int w = terminals[j];
			if (localPathsData->dist[w] != DBL_MAX) {
				Edge e = {i, j, localPathsData->dist[w]};
				appendToBuffer(&tBuffs[tID], &e);
			}
		}
		cleanPathsData(localPathsData, g->n);
	}

	// Merge collected edges
	Graph *closure = createGraph(nTerminals, nTerminals * (nTerminals-1)/2);
	int nEdges = 0;
	for (int t = 0; t < nThreads; t++) {
		Edge *edges = (Edge*) tBuffs[t].data;
		for (int i = 0; i < tBuffs[t].n; i++) {
			Edge e = edges[i];
			addEdge(e.v, e.w, e.cost, nEdges, closure);
			nEdges++;
		}
	}

	freeBuffers(tBuffs, nThreads);
	freeMultiPathsDatas(pathsDatas, nThreads);
	return closure;
}

static int mergeCollectedSets(Buffer *edgeBuffs, int nThreads, int *collectedEdgeIndices, int *treeVertices, int *nTreeVertices, bool *inTree, bool *edgesVisited, Graph *g) {
	int nEdges = 0;

	for (int tID = 0; tID < nThreads; tID++) {
		Buffer tBuff = edgeBuffs[tID];
		int *edgeIndices = (int*)tBuff.data;
		
		for (int i = 0; i < tBuff.n; i++) {
			int edgeI = edgeIndices[i];

			if (!edgesVisited[edgeI]) {
				collectedEdgeIndices[nEdges] = edgeI;
				nEdges++;
				edgesVisited[edgeI] = true;

				Edge e = g->edges[edgeI];
				*nTreeVertices = updateTreeVertices(e.v, treeVertices, inTree, *nTreeVertices);
				*nTreeVertices = updateTreeVertices(e.w, treeVertices, inTree, *nTreeVertices);
			}
		}
	}
	return nEdges;
}

static int processEdgeOfPath(int u, Graph *g, PathsData *pathsData, Buffer *buff, int *nEdges, bool *edgesVisited, int *treeVertices, int *nTreeVertices, bool *inTree) {
	int edgeI = pathsData->preEdgeIndices[u];
	Edge preEdge = g->edges[edgeI];

	appendToBuffer(buff, &edgeI);
	int *edgeIndices = (int*)buff->data;
	*nEdges = addEdgeToEdges(edgeI, edgeIndices, *nEdges, edgesVisited);

	*nTreeVertices = updateTreeVertices(preEdge.v, treeVertices, inTree, *nTreeVertices);
	*nTreeVertices = updateTreeVertices(preEdge.w, treeVertices, inTree, *nTreeVertices);

	return getPredecessor(u, preEdge);
}

static void collectUniqueEdges(Graph *g, Graph *closure, int *terminals, int *closureMSTindices, int nClosure, PathsData *pathsData, Buffer *buff, int *nEdges, int *treeVertices, bool *inTree, int *nTreeVertices, bool *edgesVisited) {
	int lastSource = -1;
	for (int i = 1; i < nClosure; i++) {
		int closureEdgeI = closureMSTindices[i];

		Edge closureEdge = closure->edges[closureEdgeI];
		int t1 = terminals[closureEdge.v];
		int t2 = terminals[closureEdge.w];
	
		if (t1 != lastSource) {
			cleanPathsData(pathsData, g->n);
			dijkstra(t1, pathsData, g);
			lastSource = t1;
		}

		// Set boundaries for the parts of the path to add
		int endOfFirstPart = t2;
		int startOfSecondPart = t2;
		// Process path and find first/last of path that are already in the tree
		int u = t2;
		while (u != -1 && u != t1) {
			int edgeI = pathsData->preEdgeIndices[u];
			Edge preEdge = g->edges[edgeI];

			if (inTree[u]) { // Update directly
				if (endOfFirstPart == t2) {
					endOfFirstPart = u;
				}
				startOfSecondPart = u;
			}
			u = getPredecessor(u, preEdge);
		}

		// Add part of the found path
		// If no in-tree vertices found, add enitre path otherwise add part from t1 to first tree-vertex
		// and from last tree-vertex to t2
		u = endOfFirstPart;
		while (u != -1 && u != t1) {
			u = processEdgeOfPath(u, g, pathsData, buff, nEdges, edgesVisited, treeVertices, nTreeVertices, inTree);
		}
		u = t2;
		while (u != -1 && u != startOfSecondPart) { // Is skipped if no intermediate tree-vertex is found
			u = processEdgeOfPath(u, g, pathsData, buff, nEdges, edgesVisited, treeVertices, nTreeVertices, inTree);
		}
	}
}

static void collectEdgesForParallel(Graph *g, Graph *closure, int *terminals, int *closureMST, int nClosure, PathsData **pathsDatas, Buffer *edgeBuffs) {
	#pragma omp parallel for schedule(dynamic)
	for (int i = 1; i < nClosure; i++) {
		int tID = omp_get_thread_num();
		
		int closureEdgeI = closureMST[i];
		Edge closureEdge = closure->edges[closureEdgeI];
		int t1 = terminals[closureEdge.v];
		int t2 = terminals[closureEdge.w];
		
		dijkstra(t1, pathsDatas[tID], g);

		// Process shortest path and add its edges
		int u = t2;
		while (u != -1 && u != t1) {
			int preEdgeI = pathsDatas[tID]->preEdgeIndices[u];
			Edge preEdge = g->edges[preEdgeI];

			appendToBuffer(&edgeBuffs[tID], &preEdgeI);

			u = getPredecessor(u, preEdge);
		}
		cleanPathsData(pathsDatas[tID], g->n);
	}
}

static bool createClosureMST(Graph *g, Terminals *terms, Graph **closure, int **closureMSTindices) {
	*closure = createMetricClosure(g, terms);
	*closureMSTindices = prim(*closure, 0);
	if (*closureMSTindices == NULL) {
		freeGraph(*closure);
		return false;
	}
	return true;
}

static int *prune(Graph *g, InducedSubGraph indSubG, int *nFinalEdges, bool *edgesVisited) {
	// Creating induced subgraph from selected vertices and edges
	int *mstEdgeIndicesInIndG = prim(indSubG.graph, 0);

	// Build final Steiner Tree from MST edges
	int *stEdges = calloc(g->n, sizeof(int));
	int nEdges = 0;
	for (int i = 1; i < indSubG.graph->n; i++) {
		int iInSubG = mstEdgeIndicesInIndG[i];
		int origI = indSubG.origEdgeI[iInSubG];
		nEdges = addEdgeToEdges(origI, stEdges, nEdges, edgesVisited);
	}
	
	free(mstEdgeIndicesInIndG);

	*nFinalEdges = nEdges;
	return stEdges;
}

static SteinerTree pruneAndCleanup(Graph *g, int *edgeIndices, int nEdges, bool *edgesVisited, int *treeVertices, int nTreeVertices) {
	int nFinalEdges = 0;
	InducedSubGraph indSubG = createInducedSubGraph(treeVertices, nTreeVertices, edgesVisited, g);
	
	clearFlags(edgeIndices, nEdges, edgesVisited);
	
	int *stEdgeIndices = prune(g, indSubG, &nFinalEdges, edgesVisited);
	freeInducedSubGraph(indSubG);
	
	return (SteinerTree){stEdgeIndices, nFinalEdges};
}

SteinerTree twoAPX(Graph *g, Terminals *terms) {
	int *terminals = terms->vertices;
	PathsData *pathsData = createPathsData(g->n);

	Buffer *edgesBuff = createBuffer(sizeof(int));
	int nEdges = 0;

	Graph *closure;
	int *closureMSTindices;
	if (!createClosureMST(g, terms, &closure, &closureMSTindices)) {
		freePathsData(pathsData);
		return (SteinerTree){NULL, 0};
	}

	int nTreeVertices = 0;
	int *treeVertices = calloc(g->n, sizeof(int));
	bool *inTree = calloc(g->n, sizeof(bool));
	bool *edgesVisited = calloc(g->m, sizeof(bool));

	collectUniqueEdges(g, closure, terminals, closureMSTindices, closure->n, pathsData, edgesBuff, &nEdges, treeVertices, inTree, &nTreeVertices, edgesVisited);

	freePathsData(pathsData);
	free(closureMSTindices);
	freeGraph(closure);

	int *edgeIndices = (int*)edgesBuff->data;
	SteinerTree st = pruneAndCleanup(g, edgeIndices, nEdges, edgesVisited, treeVertices, nTreeVertices);

	freeBuffer(edgesBuff);
	free(treeVertices);
	free(edgesVisited);
	free(inTree);

	return st;
}

SteinerTree parallelTwoAPX(Graph *g, Terminals *terms) {
	int *terminals = terms->vertices;

	Graph *closure = NULL;
	int *closureMSTindices = NULL;
	if (!createClosureMST(g, terms, &closure, &closureMSTindices)) {
		return (SteinerTree){NULL, 0};
	}

	int nThreads = omp_get_max_threads();
	PathsData **pathsDatas = createMultiPathDatas(nThreads, g->n);
	Buffer *edgeBuffs = createBuffers(nThreads, sizeof(int));
	bool *edgesVisited = calloc(g->m, sizeof(bool));

	collectEdgesForParallel(g, closure, terminals, closureMSTindices, closure->n, pathsDatas, edgeBuffs);

	int nTreeVertices = 0;
	int *treeVertices = calloc(g->n, sizeof(int));
	bool *inTree = calloc(g->n, sizeof(bool));
	int *collectedEdges = calloc(g->m, sizeof(int));

	int nEdges = mergeCollectedSets(edgeBuffs, nThreads, collectedEdges, treeVertices, 
	&nTreeVertices, inTree, edgesVisited, g);

	freeMultiPathsDatas(pathsDatas, nThreads);
	freeBuffers(edgeBuffs, nThreads);
	free(closureMSTindices);
	freeGraph(closure);

	SteinerTree st = pruneAndCleanup(g, collectedEdges, nEdges, edgesVisited, treeVertices, nTreeVertices);

	free(collectedEdges);
	free(edgesVisited);
	free(treeVertices);
	free(inTree);

	return st;
}
