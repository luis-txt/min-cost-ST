#include <stdlib.h>

#include "union-find.h"

UnionFind *createUnionFind(int n) {
	UnionFind *uf = calloc(1, sizeof(UnionFind));
	uf->parent = calloc(n, sizeof(int));
	uf->rank = calloc(n, sizeof(int));

	for (int i = 0; i < n; i++) {
		uf->parent[i] = i;
	}
	return uf;
}

void freeUnionFind(UnionFind *uf) {
	free(uf->parent);
	free(uf->rank);
	free(uf);
}

void unionSet(UnionFind *uf, int x, int y) {
	int setX = findSet(uf, x);
	int setY = findSet(uf, y);

	if (setX == setY) {
		// Already in same set
		return;
	}
	if (uf->rank[setX] < uf->rank[setY]) {
		int tmp = setX;
		setX = setY;
		setY = tmp;
	}
	uf->parent[setY] = setX;
	if (uf->rank[setX] == uf->rank[setY]) {
		uf->rank[setX]++;
	}
}

int findSet(UnionFind *uf, int x) {
	int root = x;
	// Find root of set
	while (uf->parent[root] != root) {
		root = uf->parent[root];
	}
	// Path compression
	while (x != root) {
		int next = uf->parent[x];
		uf->parent[x] = root;
		x = next;
	}
	return root;
}
