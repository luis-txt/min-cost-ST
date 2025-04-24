#ifndef TWOAPX_H
#define TWOAPX_H

#include "../../structures/graph.h"
#include "../../utils.h"

SteinerTree twoAPX(Graph *g, Terminals *terms);

SteinerTree parallelTwoAPX(Graph *g, Terminals *terms);

#endif
