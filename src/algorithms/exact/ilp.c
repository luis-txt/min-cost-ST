#include <glpk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <omp.h>

#include "../two-apx/two-apx.h"
#include "../dijkstra/dijkstra.h"
#include "ilp.h"

typedef struct {
	int nArcs;
	int m;
	int root;
	int nX;
	int nFlow;
	int nCols;
	int nEdgeSelectConstr;
	int nFlowConstr;
	int nRows;
} IlpParams;

typedef struct {
	int nnz;
	int* rowInds;
	int* colInds;
	double* coefficients;
} ConstraintMatrix;

static int arcToEdge(int a) {
	return (a / 2)+1; // 1-indexed
}

static int mapToFlowEntry(int a, int t, int nArcs, int nTerminals, int nX) {
	return nX + ((t-1) * nArcs) + a+1; // 1-indexed
}

static IlpParams initIlpParams(Graph *g, Terminals *terms, bool addUpperBound) {
	IlpParams params;

	int nArcs = g->m;
	int m = g->m / 2;
	int nTerminals = terms->n;

	params.nArcs = nArcs; // 2|E|
	params.m = m; // |E|

	params.root = terms->vertices[0];

	// Number of columns (variables)
	params.nX = m; // x_e for each undirected edge
	params.nFlow = nArcs * (nTerminals-1);
	params.nCols = params.nX + params.nFlow;

	// Number of rows (constraints)
	params.nEdgeSelectConstr = (nTerminals-1) * nArcs;
	params.nFlowConstr = (nTerminals-1) * g->n;
	params.nRows = params.nEdgeSelectConstr + params.nFlowConstr;
	if (addUpperBound) {
		params.nRows++; // Add row for additional upper bound constraint
	}
	return params;
}

static ConstraintMatrix createConstraintMatrix(Graph *g, Terminals *terms, IlpParams params, bool addUpperBound) {
	// Sparse matrix (only non-zero values)
	ConstraintMatrix matrix;
	int nTerminals = terms->n;
	int nnzEdgeSelect = 2 * params.nEdgeSelectConstr; //Edge Select. rows have exactly 2 non-zero (f^t_a, x_e)
	int nnzFlow = (nTerminals-1) * sumOfDegrees(g); // Flow rows have have degree(v) non-zero values
	matrix.nnz = nnzEdgeSelect + nnzFlow;
	if (addUpperBound) {
		matrix.nnz += params.m; // Additional nonzero entries for the upper bound row
	}

	matrix.rowInds = calloc(matrix.nnz+1, sizeof(int));
	matrix.colInds = calloc(matrix.nnz+1, sizeof(int));
	matrix.coefficients = calloc(matrix.nnz+1, sizeof(double));

	int rowI = 1;
	int nzI = 1;

	// Edge-Selection Constraints (f^t_a - x_e <= 0)
	for (int t = 1; t < nTerminals; t++) {
		for (int a = 0; a < params.nArcs; a++) {
			int colFlow = mapToFlowEntry(a, t, params.nArcs, nTerminals, params.nX);
			int colEdge = arcToEdge(a);

			// +1 for f^t_a
			matrix.rowInds[nzI] = rowI;
			matrix.colInds[nzI] = colFlow;
			matrix.coefficients[nzI] = 1.0;
			nzI++;

			// -1 for x_{ae(a)}
			matrix.rowInds[nzI] = rowI;
			matrix.colInds[nzI] = colEdge;
			matrix.coefficients[nzI] = -1.0;
			nzI++;

			rowI++;
		}
	}

	// Flow Conservation Constraints
	// sum_{out} f^t_{(v,w)} - sum_{in} f^t_{(w,v)} = b^t(v)
	for (int t = 1; t < nTerminals; t++) {
		for (int v = 0; v < g->n; v++) {
			int deg = g->vertices[v].deg;
			for (int j = 0; j < deg; j++) {
				int a = g->vertices[v].edges[j];
				int colFlow = mapToFlowEntry(a, t, params.nArcs, nTerminals, params.nX);

				// Arc is out => coefficient = +1
				// Arc is in => coefficient = -1
				double coef = (g->edges[a].v == v) ? 1.0 : -1.0;

				matrix.rowInds[nzI] = rowI;
				matrix.colInds[nzI] = colFlow;
				matrix.coefficients[nzI] = coef;
				nzI++;
			}
			rowI++;
		}
	}


	// Optional Upper-Bound Row: sum_{e} c(e) * x_e <= upBound
	if (addUpperBound) {
		int ubRow = params.nRows;
		for (int i = 1; i <= params.m; i++) {
			matrix.rowInds[nzI] = ubRow;
			matrix.colInds[nzI] = i;
			matrix.coefficients[nzI] = g->edges[2 * (i-1)].cost;
			nzI++;
		}
	}
	return matrix;
}

static void freeConstraintMatrix(ConstraintMatrix *matrix) {
	free(matrix->rowInds);
	free(matrix->colInds);
	free(matrix->coefficients);
}

static glp_prob *createGLPKProblem(Graph *g, Terminals *terms, IlpParams params) {
	glp_prob *lp = glp_create_prob();
	glp_set_prob_name(lp, "SMT ILP Classical Formulation");
	glp_set_obj_dir(lp, GLP_MIN);
	glp_term_out(GLP_OFF); // Not verbose
	
	glp_add_rows(lp, params.nRows); // Create all row placeholders
	return lp;
}

static void addConstraints(glp_prob *lp, Graph *g, Terminals *terms, IlpParams params, bool addUpperBound) {
	// Edge-Selection Constraints
	for (int i = 1; i <= params.nEdgeSelectConstr; i++) {	
		glp_set_row_bnds(lp, i, GLP_UP, 0.0, 0.0); // <= 0
	}

	// Flow-Conservation Constraints
	int flowRowStart = params.nEdgeSelectConstr + 1;
	int rowI = flowRowStart;
	for (int t = 1; t < terms->n; t++) {
		int terminal = terms->vertices[t];
		for (int v = 0; v < g->n; v++) {
			double rhs = 0.0;
			if (v == params.root) {
				rhs = 1.0;
			}
			else if (v == terminal) {
				rhs = -1.0;
			}
			glp_set_row_bnds(lp, rowI, GLP_FX, rhs, rhs); // must equal rhs
			rowI++;
		}
	}

	if (addUpperBound) {
		// Compute upper bound using 2-APX algorithm
		SteinerTree stApprox = parallelTwoAPX(g, terms);
		double upBound = sumEdgeCosts(stApprox.treeEdgeIndices, stApprox.n, g);

		glp_set_row_bnds(lp, params.nRows, GLP_UP, 0.0, upBound);
		free(stApprox.treeEdgeIndices);
	}
}

static void addCoefficients(glp_prob *lp, Graph *g, IlpParams params) {
	glp_add_cols(lp, params.nCols); // Columns for x_e and for flow f^t_a
	
	// x_e columns (binary, cost in objective)
	for (int i = 1; i <= params.nX; i++) {
		glp_set_col_bnds(lp, i, GLP_DB, 0.0, 1.0);
		// Use cost of one arc as representative
		glp_set_obj_coef(lp, i, g->edges[2 * (i-1)].cost);
		glp_set_col_kind(lp, i, GLP_BV); // in {0, 1}
	}
	// f^t_a columns (continuous, 0 cost)
	for (int i = params.nX+1; i <= params.nCols; i++) {
		glp_set_col_bnds(lp, i, GLP_DB, 0.0, 1.0);
		glp_set_obj_coef(lp, i, 0.0); // not in objective
		glp_set_col_kind(lp, i, GLP_CV); // in [0, 1]
	}
}

static void solveProblem(glp_prob *lp, ConstraintMatrix matrix) {
	glp_iocp parm;
	glp_init_iocp(&parm);
	parm.presolve = GLP_ON;
	int ret = glp_intopt(lp, &parm);
	if (ret != 0) {
		fprintf(stderr, "Error solving ILP: %d\n", ret);
		glp_delete_prob(lp);
		free(matrix.rowInds);
		free(matrix.colInds);
		free(matrix.coefficients);
		exit(EXIT_FAILURE);
	}
}

static SteinerTree extractSolution(glp_prob *lp, Graph *g, IlpParams params, ConstraintMatrix matrix) {
	// Extract values of vector x which represent selected edges
	SteinerTree st;
	if (params.nX <= 0) {
		fprintf(stderr, "Underflow Error for arcs.\n");
		exit(EXIT_FAILURE);
	}
	st.treeEdgeIndices = calloc(params.nX, sizeof(int));
	st.n = 0;
	for (int i = 1; i <= params.nX; i++) {
		// Collect respective undirected edge
		int val = (int) glp_mip_col_val(lp, i);
		if (val == 1) {
			st.treeEdgeIndices[st.n] = 2 * (i-1); // Choose first arc of pair
			st.n++;
		}
	}
	return st;
}

static void reduceGraph(Graph *g) {
	int nEdges = g->m / 2;
	bool *removeEdges = calloc(nEdges, sizeof(bool));
	int *newPos = calloc(nEdges, sizeof(int));
	for (int i = 0; i < nEdges; i++) {
		newPos[i] = i;
	}

	// Run dijkstra for each undirected edge
	int nThreads = omp_get_max_threads();
	PathsData **pathsDatas = createMultiPathDatas(nThreads, g->n);

	#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < nEdges; i++) {
		int tID = omp_get_thread_num();
		Edge e = g->edges[2*i];
		
		dijkstra(e.v, pathsDatas[tID], g);
		
		int costSum = sumEdgeCosts(pathsDatas[tID]->preEdgeIndices, g->n, g);
		if (costSum < e.cost) {
			// There is a shorter path which makes the edge not used in any min ST
			removeEdges[i] = true;
		}
		cleanPathsData(pathsDatas[tID], g->n);
	}

	int left = 0;
	int right = nEdges-1;
	while (left < right) {
		while (left < nEdges && !removeEdges[left]) {
			// Find next edge to remove (swap to the end)
			left++;
		}
		while (right >= 0 && removeEdges[right]) {
			// Find next staying edge index (swap with edge to remove)
			right--;
		}
		if (left < right) {
			// Swap edge pair of left with right
			Edge temp1 = g->edges[2*left];
			Edge temp2 = g->edges[2*left + 1];
			
			g->edges[2*left] = g->edges[2*right];
			g->edges[2*left + 1] = g->edges[2*right + 1];
			g->edges[2*right] = temp1;
			g->edges[2*right + 1]= temp2;

			newPos[right] = left; // Update mapping of kept edge
			removeEdges[left] = false;
			removeEdges[right] = true;
			left++;
			right--;
		}
	}

	// Count kept edges
	int newCount = 0;
	for (int i = 0; i < nEdges; i++) {
		if (!removeEdges[i]) {
			newCount++;
		}
	}

	g->m = newCount * 2;
	g->edges = realloc(g->edges, g->m * sizeof(Edge));

	// Update adjacency array using new positions
	for (int v = 0; v < g->n; v++) {
		for (int j = 0; j < g->vertices[v].deg; j++) {
			int oldArcI = g->vertices[v].edges[j];
			int undirectedI = oldArcI / 2;
			int rem = oldArcI % 2;
			g->vertices[v].edges[j] = newPos[undirectedI]*2 + rem;
		}
	}

	free(removeEdges);
	free(newPos);
	freeMultiPathsDatas(pathsDatas, nThreads);
}

SteinerTree ilp(Graph *g, Terminals *terms, bool addUpperBound, bool reduceG) {
	if (reduceG) {
		reduceGraph(g);
	}

	IlpParams params = initIlpParams(g, terms, addUpperBound);

	ConstraintMatrix matrix = createConstraintMatrix(g, terms, params, addUpperBound);

	glp_prob *lp = createGLPKProblem(g, terms, params);

	addConstraints(lp, g, terms, params, addUpperBound);
	addCoefficients(lp, g, params);

	glp_load_matrix(lp, matrix.nnz, matrix.rowInds, matrix.colInds, matrix.coefficients);
	
	solveProblem(lp, matrix);

	SteinerTree st = extractSolution(lp, g, params, matrix);

	freeConstraintMatrix(&matrix);
	glp_delete_prob(lp);

	return st;
}
