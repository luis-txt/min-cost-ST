#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "test.h"
#include "../structures/graph.h"
#include "../structures/union-find.h"

bool scanMinCost(const char *filePath, double *minCost) {
	// Extract instance file name from the path
	const char *lastPart = strrchr(filePath, '/');
	const char *instanceFileName = lastPart ? lastPart + 1 : filePath;

	// Extract track directory name
	const char *start = strstr(filePath, "graphs/");
	if (start == NULL) {
		return false; // No valid path
	}
	start += strlen("graphs/"); // Move past "graphs/"
	
	const char *secondPart = strchr(start, '/');
	if (secondPart == NULL) {
		return false; // No valid path
	}
	
	int trackLen = secondPart - start;
	char trackDir[256];
	
	strncpy(trackDir, start, trackLen);
	trackDir[trackLen] = '\0';

	trackDir[0] = tolower(trackDir[0]);

	// Create CSV file path as "graphs/[trackDir].csv"
	char csvFilePath[512];
	snprintf(csvFilePath, sizeof(csvFilePath), "graphs/%s.csv", trackDir);

	FILE *csvFile = fopen(csvFilePath, "r");

	char line[256];
	
	fgets(line, sizeof(line), csvFile); // Skip header

	double cost = -1.0;
	while (fgets(line, sizeof(line), csvFile)) {
		char name[256];
		double currCost;

		if (sscanf(line, "%s%*[ ,]%lf", name, &currCost) == 2) {
			if (strcmp(name, instanceFileName) == 0) {
				cost = currCost;
				break;
			}
		}
		else {
			// Handle parsing error if necessary
			printf("Failed to parse line: %s", line);
			exit(EXIT_FAILURE);
		}
	}
	if (cost == -1.0) {
		//No minimum value found in csv-file
		return false;
	}

	fclose(csvFile);
	*minCost = cost;
	return true;
}

bool isSteinerTree(SteinerTree st, Terminals *terms, Graph *g) {
	int nVertices = g->n;

	if (terms->n<= 1) {
		// Only one terminal
		return st.n == 0; // Single vertex is valid
	}

	// Mark vertices in ST and union their sets in Union-Find
	UnionFind *uf = createUnionFind(nVertices);
	bool *inTree = calloc(nVertices, sizeof(bool));
	for (int i = 0; i < st.n; i++) {
		int globalEdgeI = st.treeEdgeIndices[i];
		Edge e = g->edges[globalEdgeI];
		inTree[e.v] = true;
		inTree[e.w] = true;
		unionSet(uf, e.v, e.w);
	}

	// Count vertices in st
	int verticesInTree = 0;
	for (int i = 0; i < nVertices; i++) {
		if (inTree[i]) {
			verticesInTree++;
		}
	}

	// Check number of edges in st
	if (st.n != verticesInTree - 1) {
		printf("Structure is not a tree (edge count mismatch).\n");
		free(inTree);
		freeUnionFind(uf);
		return false;
	}

	// Check inclusion of al terminals
	for (int i = 0; i < terms->n; i++) {
		int t = terms->vertices[i];
		if (!inTree[t]) {
			printf("Terminal %d is not in the tree.\n", t);
			free(inTree);
			freeUnionFind(uf);
			return false;
		}
	}

	// Check connectivity
	int comp = -1;
	for (int v = 0; v < nVertices; v++) {
		if (inTree[v]) {
			if (comp == -1) {
				comp = findSet(uf, v);
			}
			else if (findSet(uf, v) != comp) {
				printf("Vertices are not connected.\n");
				free(inTree);
				freeUnionFind(uf);
				return false;
			}
		}
	}

	free(inTree);
	freeUnionFind(uf);
	return true;
}
