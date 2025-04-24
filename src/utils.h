#ifndef UTILS_H
#define UTILS_H

#include "structures/graph.h"

typedef struct {
	int *treeEdgeIndices;
	int n;
} SteinerTree;

double sumEdgeCosts(int *edgeIndices, int nEdges, Graph *g);

int getPredecessor(int u, Edge e);

void printArray(int *arr, int n);

void printSteinerTree(SteinerTree st, Graph *g);

void printEdgeIndices(int *edges, int n, Graph *g);

#endif
