#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>
#include <stdio.h>

typedef struct Edge Edge;

typedef struct{
	int n;
	int m;
} Sizes;

typedef struct {
	int *vertices;
	int n;
} Terminals;

struct Edge {
	int v;
	int w;
	double cost;
};

typedef struct {
	int *edges;
	int capEdges;
	int deg;
} Vertex;

typedef struct {
	int n;
	int m;
	Vertex *vertices;
	Edge *edges;
} Graph;

typedef struct {
	Graph *graph;
	int *newIDtoOldID;
	int *origEdgeI;
}InducedSubGraph;

Graph *createGraph(int n, int m);

void freeGraph(Graph *g);

void addEdge(int v, int w, double cost, int i, Graph *g);

Edge *getEdge(int v, int edgeI, Graph *g);

int getEdgeIndex(int v, int edgeI, Graph *g);

void scanTerminals(FILE *file, Terminals *terms);

void freeTerminals(Terminals *terms);

Sizes scanGraphStructure(FILE *file, char *line, int lineSize);

Graph *scanGraph(FILE *file, Terminals *terms, bool doubleEdges);

InducedSubGraph createInducedSubGraph(int *selectedVertices, int nSelectedVertices, bool *selectedEdges, Graph *g);

void freeInducedSubGraph(InducedSubGraph indSubG);

int sumOfDegrees(Graph *g);

void printEdge(Edge e);

void printEdges(int v, Graph *g);

void printTerminals(Terminals *terms);

void printGraph(Graph *g);

#endif
