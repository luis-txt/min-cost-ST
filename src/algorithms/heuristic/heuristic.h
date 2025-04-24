#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "../../structures/graph.h"
#include "../../utils.h"

SteinerTree prunedMST(Graph *g, Terminals *terms);

SteinerTree mstST(Graph *g, Terminals *terms);

SteinerTree takahashiMatsuyama(Graph *g, Terminals *terms);

#endif
