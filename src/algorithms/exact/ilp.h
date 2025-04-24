#ifndef ILP_H
#define ILP_H

#include "../../utils.h"
#include "../../structures/graph.h"

SteinerTree ilp(Graph *g, Terminals *terms, bool addUpperBound, bool reduceG);

#endif
