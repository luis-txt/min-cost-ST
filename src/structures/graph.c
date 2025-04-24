#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "graph.h"

Graph *createGraph(int n, int m) {
	Graph *g = calloc(1, sizeof(Graph));
	Vertex *vertices = calloc(n, sizeof(Vertex));
	Edge *edges = calloc(m, sizeof(Edge));

	*g = (Graph){n, m, vertices, edges};
	return g;
}

Edge *getEdge(int v, int edgeI, Graph *g) {
	int globalEdgeI = g->vertices[v].edges[edgeI];
	return &g->edges[globalEdgeI];
}

int getEdgeIndex(int v, int edgeI, Graph *g) {
	return g->vertices[v].edges[edgeI];
}

void freeVertex(int id, Graph *g) {
	free(g->vertices[id].edges);
}

void freeGraph(Graph *g) {
	for (int i = 0; i < g->n; i++) {
		freeVertex(i, g);
	}
	free(g->vertices);
	free(g->edges);
	free(g);
}

void freeTerminals(Terminals *terms) {
	free(terms->vertices);
	free(terms);
}

void pushEdge(int id, int edgeI, Graph *g) {
	Vertex *vertex = &g->vertices[id];
	if (vertex->capEdges == 0) {
		vertex->capEdges = 100;
		vertex->edges = calloc(100, sizeof(int));
	}
	if (vertex->deg == vertex->capEdges) {
		vertex->capEdges *= 2;
		vertex->edges = realloc(vertex->edges, vertex->capEdges * sizeof(int));
	}
	vertex->edges[vertex->deg] = edgeI;
	vertex->deg++;
}

void addEdge(int v, int w, double cost, int edgeI, Graph *g) {
	Edge *e = &g->edges[edgeI];
	*e = (Edge){v, w, cost};
	pushEdge(v, edgeI, g);
	pushEdge(w, edgeI, g);
}

Sizes scanGraphStructure(FILE *file, char *line, int lineSize) {
	fgets(line, lineSize, file); // Read header
	
	fgets(line, lineSize, file); // Read |V|
	int n;
	sscanf(line, "Nodes %d", &n);

	fgets(line, lineSize, file); // Read |E|
	int m;
	sscanf(line, "Edges %d", &m);

	return (Sizes){n, m};
}

Graph *scanGraph(FILE *file, Terminals *terms, bool doubleEdges) {
	char line[256];
	Sizes s = scanGraphStructure(file, line, sizeof(line));
	if (doubleEdges) {
		s.m *= 2;
	}
	Graph *g = createGraph(s.n, s.m);

	int v, w;
	double cost;
	int nEdge = 0;
	while (fgets(line, sizeof(line), file)) {
		if (strncmp(line, "END", 3) == 0) {
			break;
		}
		sscanf(line, "E %d %d %lf", &v, &w, &cost);
		// Data is 1-indexed
		v--;
		w--;

		addEdge(v, w, cost, nEdge, g);
		nEdge++;
		if (doubleEdges) {
			addEdge(w, v, cost, nEdge, g);
			nEdge++;
		}
	}
	scanTerminals(file, terms);
	return g;
}

void scanTerminals(FILE *file, Terminals *terms) {
	char line[256];
	int n = 0;

	// Read lines until we find one that starts with "Terminals"
	while (fgets(line, sizeof(line), file)) {
		if (strncmp(line, "Terminals", 9) == 0) {
			sscanf(line, "Terminals %d", &n);
			break;
		}
	}
	
	int *terminals = calloc(n, sizeof(int));

	for (int i = 0; i < n; i++) {
		fgets(line, sizeof(line), file);
		sscanf(line, "T %d", &terminals[i]);
		terminals[i]--; // Data is 1-indexed
	}

	fgets(line, sizeof(line), file); // Read END
	
	terms->n = n;
	terms->vertices = terminals;
}

InducedSubGraph createInducedSubGraph(int *selectedVertices, int nSelectedVertices, bool *selectedEdges, Graph *g) {
	int sumDegrees = 0;
	bool *isInSub = calloc(g->n, sizeof(bool));
	for (int v = 0; v < nSelectedVertices; v++) {
		int selectedV = selectedVertices[v];
		isInSub[selectedV] = true;
		sumDegrees += g->vertices[selectedV].deg;
	}

	Graph *subG = createGraph(nSelectedVertices, sumDegrees);
	int capOfOrigEdgeI = 100;
	int *origEdgeI = calloc(100, sizeof(int));
	
	int *oldIDtoNewID = calloc(g->n, sizeof(int));
	int *newIDtoOldID = calloc(nSelectedVertices, sizeof(int));
	for (int v = 0; v < nSelectedVertices; v++) {
		oldIDtoNewID[selectedVertices[v]] = v;
		newIDtoOldID[v] = selectedVertices[v];
	}

	// Copy vertices with their induced edges
	int nEdges = 0;
	for (int newI = 0; newI < nSelectedVertices; newI++) {
		int origV = selectedVertices[newI];
		Vertex *vertex = &g->vertices[origV];

		for (int i = 0; i < vertex->deg; i++) {
			int globalEdgeI = vertex->edges[i];
			if (selectedEdges != NULL && !selectedEdges[globalEdgeI]) {
				continue; // Skip not desired edges
			}

			Edge *edge = getEdge(origV, i, g);
			if (isInSub[edge->w]) {
				// Edge belongs to induced subgraph
				if (origV == edge->v) {
					continue; // Add each edge only once
				}
				addEdge(oldIDtoNewID[edge->v], oldIDtoNewID[edge->w], edge->cost, nEdges, subG);

				// Add mapping to original edge index
				if (nEdges == capOfOrigEdgeI) {
					capOfOrigEdgeI *= 2; // Increase capacity of mapping
					origEdgeI = realloc(origEdgeI, sizeof(int) * capOfOrigEdgeI);
				}
				origEdgeI[nEdges] = globalEdgeI;

				nEdges++;
			}
		}
	}

	free(oldIDtoNewID);
	free(isInSub);
	subG->m = nEdges;
	return (InducedSubGraph){subG, newIDtoOldID, origEdgeI};
}

int sumOfDegrees(Graph *g) {
	int sumDegs = 0;
	for (int v = 0; v < g->n; v++) {
		sumDegs += g->vertices[v].deg;
	}
	return sumDegs;
}

void freeInducedSubGraph(InducedSubGraph indSubG) {
	freeGraph(indSubG.graph);
	free(indSubG.origEdgeI);
	free(indSubG.newIDtoOldID);
}

void clearEdgeFlags(bool *edgeFlags, Edge **edges, int nEdges) {
	for (int e = 0; e < nEdges; e++) {
		edgeFlags[e] = false;
	}
}

void printEdge(Edge e) {
	printf("{%d, %.2lf, %d}", e.v, e.cost, e.w);
}

void printEdges(int v, Graph *g) {
	for (int i = 0; i < g->vertices[v].deg; i++) {
		printEdge(*getEdge(v, i, g));
		printf(", ");
	}
}

void printTerminals(Terminals *terms) {
	printf("--- Terminals:\n");
	for (int i = 0; i < terms->n; i++) {
		printf("%d, ", terms->vertices[i]);
	}
	printf("\n");
}

void printGraph(Graph *g) {
	printf("--- IndexedGraph:\n");
	printf("- Vertices (to indices of arcs):\n");
	for (int v = 0; v < g->n; v++) {
		printf("v%d: ", v);
		for (int e = 0; e < g->vertices[v].deg; e++) {
			printf("%d, ", g->vertices[v].edges[e]);
		}
		printf("\n");
	}
	printf("\n- Edges (s, c, t):\n");
	for (int e = 0; e < g->m; e++) {
		printf("(%d, %.2lf, %d), ", g->edges[e].v, g->edges[e].cost, g->edges[e].w);
	}
	printf("\n");
}
