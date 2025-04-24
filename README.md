# min-cost-ST
Implementation of multiple algorithms for calculating a *undirected (minimum cost) steiner tree*.

## Installation
Clone the reposetory along with the [PACE instances from 2018](https://github.com/PACE-challenge/SteinerTree-PACE-2018-instances) (using SSH) by running:
```
git clone git@gitlab.inf.uni-konstanz.de:ag-storandt/ae-24/luis.flak.git
```

For creation of an executable, a modern C compiler as GCC 14.2.1 20250110 (Red Hat 14.2.1-7) or Clang 19.1.7 (Fedora 19.1.7-2.fc41) is needed.
Since most modern C compilers (such as GCC or Clang) come with built-in support for OpenMP, it normally does not have to be installed seperately.

Then, on Fedora use the following command to install GNU Linear Programming Kit (GLPK):
```
sudo dnf install glpk glpk-devel
```
or on Ubuntu/Debian use (not tested):
```
sudo apt install glpk libglpk-dev
```

Afterwards, the complete C program can be compiled by typing `make` from the *Exam* directory.

## Algorithms
After compilation, the resulting *min-cost-ST* executable is able to execute the following algorithms:
- `-m` "MST" using Prim's algorithm is created without any additions
- `-s` "Smaller MST" creates a MST with pruned leafs
- `-h` "Heuristic" executes the Takahashi-Matsuyama heuristic
- `-a` "Approximate" executes the 2-APX algorithm
- `-x` "Exact" executes the flow-based ILP formulation using GLPK

Note that only one algorithm can be selected.

## Improvements
- `-p` "Parallel" runs the algorithm in parallel using OpenMP (only usable with -a)
- `-u` "Upper" adds an upper bound as additional constraint to the ILP (only usable with -x)
- `-r` "Reduce" reduces the number of edges as a preprocessing step in parallel (only usable with -x)

Note that -u and -r can be used together. 

## Modes
- With no additional flag: Returns the computed, 0-indexed steiner tree
- `-c` "Cost" only returns total cost of the computed steiner tree
- `-t` "Test" returns the resulting 0-indexed steiner tree and result of the executed tests according to the next subsection *Unit-Tests*

## Unit-Tests
If the -t flag is set, we conduct tests the following tests.
For the PACE instances, there are optimum costs provided for Track 1 and 2. For Track 3 there are only upper bounds provided.
The program searches if the selected instance under *graphs/Track<NR>/instance<ID>.gr* is in Track 1 or 2 and if so, it compares the resulting cost of the selected algorithm to the optimum cost for the given instance. Depending on the selected algorithm, it then outputs the difference in cost to the optimum or that the result is an optimum steiner tree.

Furthermore, the computed steiner tree is examined to be a valid steiner tree by checking
- Is the resulting structure a tree (connected, acyclic)?
- Is the resulting structure containing all given terminals?
The result of this check is then output aswell.

## Usage
To use the program, the compiled *min-cost-ST* executable can be executed along with flags for determining the desired algorithm and mode. Additionally, the path to a file containing the input graph (represented in the [PACE format](https://pacechallenge.org/2018/steiner-tree/)) must be provided:
```
./min-cost-ST [-h|-x|-a|-s|-m] [-p] [-r] [-u] [-c] [-t] <PATH_TO_INPUT_GRAPH>
```
Example for executing the parallel 2-APX implementation on instance 5 of Track 1 and after computing the steiner tree, executing tests (as described in subsection *test-graphs*):
```
./min-cost-ST -a -p -t graphs/Track1/instance005.gr
```
We also provide two smaller, comprehensible graph instances for testing in *test-graphs*.

## Scripts
Additionally to the program, we provide a multitude of *Python3* scripts for analysing and visualising the collected benchmark data.

To use the provided scripts, some python3 packages are needed. These can be installed using:
```
pip install numpy matplotlib pandas networkx
```
The following scripts are provided.

### create_characteristic_data
Processes the directory *graphs* (the PACE instances) and computes the respective characteristics in the *graph_characteristics* directory for each graph instance. This includes the number of vertices |V|, the number of edges |E|, the number of terminals |R|, the density of the graph (computed by |E| / (|V| choose 2)), the maximum degree and the standard-deviation of the edge costs.
Run with the following command from the *Exam* directory.
```
python3 scripts/create_characteristic_data.py
```

### create_plots
Without arguments, a selection of plots in the *.pgd* format is created. Optional arguments are:
- `--tracks` lets you select specific tracks from the PACE data (default is all three tracks).
- `--output_dir` lets you specify an output directory (default is *plots*).
- `--pdf` creates pdf plots instead of using the file *.pgd* format.
Run with the following command from the *Exam* directory.
```
python3 scripts/create_plots.py [FLAGS]
```

### read_extreme_values
Reads the graph characteristics (under *graph_characteristics*), the optimum costs (in *graphs*) and the benchmark data (under *benches*) and computes:
- Maximum number of edges |E| in any instance
- Minimum number of edges |E| on which for the respective algorithms failed to compute a result
- Average and maximum running time for each algorithm
- Average and maximum cost difference for each algorithm
Run with the following command from the *Exam* directory.
```
python3 scripts/read_extreme_values.py
```

### run_benches
- Executes the algorithms on the PACE instances and collects the benchmarking data (memory consumption, running time and cost of the resulting steiner tree).
- Runs each algorithm on each instance at most 1,000 times and at least once if the five minutes time constraint is enough. If another run is estimated to fit in the remaining time of the five minutes constraint, the algorithm is executed multiple times again and the resulting data is averaged across runs.
Run with the following command from the *Exam* directory.
```
python3 scripts/run_benches.py
```
