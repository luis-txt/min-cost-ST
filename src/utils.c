#include "utils.h"
#include "structures/graph.h"

double sumEdgeCosts(int *edgeIndices, int nEdges, Graph *g) {
	double totalCost = 0;
	for (int i = 0; i < nEdges; i++) {
		int globalEdgeI = edgeIndices[i];
		if (globalEdgeI == -1) {
			continue;
		}
		Edge e = g->edges[globalEdgeI];
		totalCost += e.cost;
	}
	return totalCost;
}

int min(int a, int b) {
	if (a <= b) {
		return a;
	}
	return b;
}

int getPredecessor(int u, Edge e) {
	if (e.w == u) {
		return e.v;
	}
	return e.w;
}

void printSteinerTree(SteinerTree st, Graph *g) {
	printf("-------------\n");
	printf("Number of edges in SMT: %d\n", st.n);
	printf("Selected edges:");
	for (int i = 0; i < st.n; i++) {
		int globalEdgeI = st.treeEdgeIndices[i];
		Edge e = g->edges[globalEdgeI];
		printf("{%d %d %.2lf}, ", e.v, e.w, e.cost);
	}
	printf("\n");
}

void printArray(int *arr, int n) {
	for (int i = 0; i < n; i++) {
		printf("%d, ", arr[i]);
	}
	printf("\n");
}

void printEdgeIndices(int *edges, int n, Graph *g) {
	for (int i = 0; i < n; i++) {
		int globalEdgeI = edges[i];
		if (globalEdgeI != -1) {
			printEdge(g->edges[globalEdgeI]);
		}
		else {
			printf("NULL, ");
		}
	}
	printf("\n");
}
