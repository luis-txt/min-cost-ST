#ifndef UNIONFIND_H
#define UNIONFIND_H

typedef struct {
	int *parent;
	int *rank;
} UnionFind;

UnionFind *createUnionFind(int n);

void freeUnionFind(UnionFind *uf);

void unionSet(UnionFind *uf, int x, int y);

int findSet(UnionFind *uf, int x);

#endif
