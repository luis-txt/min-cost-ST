#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include "structures/graph.h"
#include "algorithms/exact/ilp.h"
#include "algorithms/two-apx/two-apx.h"
#include "algorithms/heuristic/heuristic.h"
#include "test/test.h"
#include "utils.h"

typedef enum { NONE, SMALLER_MST, HEURISTIC, EXACT, TWO_APX, MST } Mode;

typedef struct {
	Mode mode;
	bool testFlag;
	bool parallelFlag;
	bool reduceFlag;
	bool upperBoundFlag;
	bool totalCostFlag;
	const char *filePath;
} Options;

Options parse_arguments(int argc, char **argv) {
	Options opts = { NONE, 0, NULL };
	int opt;

	while ((opt = getopt(argc, argv, "hxatspmucr")) != -1) {
		if (opts.mode != NONE && opt != 't' && opt != 'p' && opt != 'u' && opt != 'c' && opt != 'r') {
			fprintf(stderr, "Error: Multiple modes specified. Please select exactly one basis mode (-h, -x, -s, -m, or -a).\n");
			exit(EXIT_FAILURE);
		}
		else if (opt == 's') {
			opts.mode = SMALLER_MST;
		}
		else if (opt == 'h') {
			opts.mode = HEURISTIC;
		}
		else if (opt == 'x') {
			opts.mode = EXACT;
		}
		else if (opt == 'a') {
			opts.mode = TWO_APX;
		}
		else if (opt == 'm') {
			opts.mode = MST;
		}
		else if (opt == 't') {
			opts.testFlag = true;
		}
		else if (opt == 'p') {
			opts.parallelFlag = true;
		}
		else if (opt == 'u') {
			opts.upperBoundFlag = true;
		}
		else if (opt == 'c') {
			opts.totalCostFlag = true;
		}
		else if (opt == 'r') {
			opts.reduceFlag = true;
		}
		else {
			fprintf(stderr, "Usage: %s [-h|-x|-a|-s|-m] [-p] [-t] [-r] [-u] [-c] <filename_of_graph>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if (opts.parallelFlag && opts.mode != TWO_APX) {
		fprintf(stderr, "Error: Parallel flag must only be used with mode '-a'.\n");
		exit(EXIT_FAILURE);
	}
	if ((opts.upperBoundFlag || opts.reduceFlag) && opts.mode != EXACT) {
		fprintf(stderr, "Error: Upper-bound and reduce flags must only be used with mode '-x'.\n");
		exit(EXIT_FAILURE);
	}
	if (opts.mode == NONE) {
		fprintf(stderr, "Error: You must specify exactly one basis mode (-h, -x, -s, -m, or -a).\n");
		exit(EXIT_FAILURE);
	}
	if (optind >= argc) {
		fprintf(stderr, "Error: Expected filename after options.\n");
		exit(EXIT_FAILURE);
	}
	opts.filePath = argv[optind];
	return opts;
}

int main(int argc, char **argv) {
	Graph *g;
	Terminals *terms = calloc(1, sizeof(Terminals));
	SteinerTree st;
	
	Options opts = parse_arguments(argc, argv);

	FILE *graphFile = fopen(opts.filePath, "r");
	if (!graphFile) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	switch (opts.mode) {
		case SMALLER_MST:
			g = scanGraph(graphFile, terms, false);
			fclose(graphFile);
			st = prunedMST(g, terms);
			break; 
		case HEURISTIC:
			g = scanGraph(graphFile, terms, false);
			fclose(graphFile);
			st = takahashiMatsuyama(g, terms);
			break;
		case MST:
			g = scanGraph(graphFile, terms, false);
			fclose(graphFile);
			st = mstST(g, terms);
			break;
		case EXACT:
			g = scanGraph(graphFile, terms, true);
			fclose(graphFile);
			st = ilp(g, terms, opts.upperBoundFlag, opts.reduceFlag);
			break;
		case TWO_APX:
			g = scanGraph(graphFile, terms, false);
			fclose(graphFile);
			st = opts.parallelFlag ? parallelTwoAPX(g, terms) : twoAPX(g, terms);
			break;
		default:
			freeTerminals(terms);
			fprintf(stderr, "Mode not recognized.\n");
			exit(EXIT_FAILURE);
	}

	double totalCost = sumEdgeCosts(st.treeEdgeIndices, st.n, g);

	int exitStatus = EXIT_SUCCESS;

	if (opts.totalCostFlag) {
		fprintf(stdout, "Total cost: %.2lf\n", totalCost);
	}
	else {
		printEdgeIndices(st.treeEdgeIndices, st.n, g);
	}

	// Tests
	if (opts.testFlag) {
		bool isST = isSteinerTree(st, terms, g);
		if (!isST) {
			fprintf(stderr, "Result is not a Steiner Tree :(\n");
			exitStatus = EXIT_FAILURE;
		}
		
		double minCost = 0.0;
		bool hasMinCost = scanMinCost(opts.filePath, &minCost);

		if (isST && hasMinCost) {
			double diff = fabs(totalCost - minCost);
			if (diff == 0.0) {
				fprintf(stdout, "Result is a minimum cost Steiner Tree :)\n");
			}
			else if (opts.mode == EXACT) {
				fprintf(stderr, "Result has not mimimum cost :(\n");
				exitStatus = EXIT_FAILURE;
			}
			else if (opts.mode == TWO_APX && totalCost > 2*minCost) {
				fprintf(stderr, "Result has not two approximation :(\n");
				exitStatus = EXIT_FAILURE;
			}
			else {
				fprintf(stdout, "Result is a Steiner Tree with difference of %.2lf to the minimum :)\n", diff);
			}
		}
	}
	
	freeGraph(g);
	freeTerminals(terms);
	free(st.treeEdgeIndices);
	exit(exitStatus);
}
