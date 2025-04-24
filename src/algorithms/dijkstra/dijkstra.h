#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "../../structures/graph.h"
#include "../../utils.h"

typedef struct {
	double *dist;
	int *preEdgeIndices;
} PathsData;

PathsData *createPathsData(int n);

void cleanPathsData(PathsData *data, int n);

void freePathsData(PathsData *data);

void printPathsData(PathsData *pd, int n, Graph *g);

PathsData **createMultiPathDatas(int nPaths, int pathLen);

void freeMultiPathsDatas(PathsData **pathsDatas, int nPaths);

void multiDijkstra(int *sources, int nSources, PathsData *pathsData, Graph *g);

void dijkstra(int s, PathsData *pathsData, Graph *g);

#endif
