#ifndef TEST_H
#define TEST_H

#include "../utils.h"

bool scanMinCost(const char *filePath, double *minCost);

bool isSteinerTree(SteinerTree st, Terminals *terms, Graph *g);

#endif
