# BDD Benchmark

This is a collection of benchmarks for Binary Decision Diagrams (BDDs)
[[Bryant1986](#references)] and some of its variants. These are useful to survey
the strengths and weaknesses of different implementations and to guide their
developers. To this end, each benchmark is implemented by use of C++ templates
such that they are agnostic of the BDD package used.

This project has been developed at the
[Logic and Semantics](https://logsem.github.io/) group at
[Aarhus University](https://cs.au.dk).

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [Decision Diagrams](#decision-diagrams)
- [BDD Packages](#bdd-packages)
    - [Dependencies](#dependencies)
- [Usage](#usage)
    - [Build](#build)
    - [Run](#run)
- [Benchmarks](#benchmarks)
    - [Apply](#apply)
    - [Game of Life](#game-of-life)
    - [Hamiltonian Cycle](#hamiltonian-cycle)
    - [McNet](#mcnet)
    - [Picotrav](#picotrav)
    - [QBF Solver](#qbf-solver)
    - [Queens](#queens)
    - [RelProd](#relprod)
    - [Tic-Tac-Toe](#tic-tac-toe)
- [Performance Regression Testing](#performance-regression-testing)
- [License](#license)
- [Citation](#citation)
- [References](#references)

<!-- markdown-toc end -->

## Decision Diagrams

Our benchmarks target the following types of decision diagrams.

- [**Binary Decision Diagrams**](https://en.wikipedia.org/wiki/Binary_decision_diagram)
  **(BDDs)**
  [[Bryant1986](#references)]

- [**Binary Decision Diagrams with Complemented Edges**](https://en.wikipedia.org/wiki/Binary_decision_diagram#Complemented_edges)
  **(BCDDs)**
  [[Brace1990](#references)]

- [**Zero-suppressed Decision Diagrams**](https://en.wikipedia.org/wiki/Zero-suppressed_decision_diagram)
  **(ZDDs)**
  [[Minato1993](#references)]

## BDD Packages

The benchmarks are implemented using C++ templates to make them agnostic of the
BDD package used. To do so, each implementation has an *adapter* in-between
which is fully inlined at compile time. Currently, we support the following BDD
packages.

- [**Adiar**](https://github.com/ssoelvsten/adiar):

  An I/O-efficient implementation with iterative algorithms using time-forward
  processing to exploit a special sorting of BDD nodes streamed from and to the
  disk. These algorithms have no need for memoization or garbage collection.
  But, on the other hand, nodes are also not shareable between BDDs.


- [**BuDDy**](https://buddy.sourceforge.net/manual/main.html):

  An easy-to-use yet extensive implementation of shared BDDs with depth-first
  algorithms using a unique node table and memoization.

  We use [this](https://github.com/SSoelvsten/BuDDy) version that has been
  updated and builds using CMake.


- [**CAL**](https://github.com/SSoelvsten/cal):

  A breadth-first implementation that exploits a specific level-by-level
  locality of nodes on disk to improve performance when dealing with large BDDs.
  Unlike Adiar it also supports sharing of nodes between BDDs at the cost of
  memoization and garbage collection.

  We use the [revived version](https://github.com/SSoelvsten/cal) with an
  extended C API, CMake support and a C++ API.


- **CUDD**:

  Probably the most popular BDD package of all. It uses depth-first algorithms
  and a unique node table and memoization.

  We use [this modified v3.0](https://github.com/SSoelvsten/cudd) with CMake
  support and an extended C++ API.


- [**LibBDD**](https://github.com/sybila/biodivine-lib-bdd):

  A thread-safe implementation with depth-first algorithms and memoization. Yet
  unlike others, it does not implement *shared* BDDs, i.e., two diagrams do not
  share common subgraphs. Hence, the unique and memoization tables are neither
  reused between operations.

  We use [this unofficial Rust-to-C FFI](https://github.com/nhusung/lib-bdd-ffi).


- [**OxiDD**](https://github.com/OxiDD/oxidd):

  A multi-threaded (and thread-safe) framework for the implementation of
  decision diagrams and their algorithms. Currently, its algorithms are
  depth-first on a unique node table and memoization.


- [**Sylvan**](https://github.com/trolando/sylvan):

  A multi-threaded (and thread-safe) implementation with depth-first algorithms
  using a unique node table and memoization.


|                    | **Adiar** | **BuDDy** | **CAL** | **CUDD** | **LibBDD** | **OxiDD** | **Sylvan** |
|--------------------|-----------|-----------|---------|----------|------------|-----------|------------|
| **Language**       | C++       | C         | C       | C        | Rust       | Rust      | C          |
|                    |           |           |         |          |            |           |            |
| **BDD**            | ✓         | ✓         |         | (✓)      | ✓          | ✓         | (✓)        |
| **BCDD**           |           |           | ✓       | ✓        |            | ✓         | ✓          |
| **MTBDD**          |           |           |         | ✓        |            | ✓         | ✓          |
| **ZDD**            | ✓         |           |         | ✓        |            | ✓         | ✓          |
|                    |           |           |         |          |            |           |            |
| **Thread Safe**    |           |           |         |          | ✓          | ✓         | ✓          |
| **Multi-threaded** |           |           |         |          |            | ✓         | ✓          |
|                    |           |           |         |          |            |           |            |
| **Reordering**     |           | ✓         | ✓       | ✓        |            | (✓)       | (✓)        |
|                    |           |           |         |          |            |           |            |
| **Shared Nodes**   |           | ✓         | ✓       | ✓        |            | ✓         | ✓          |
| **Ext. Memory**    | ✓         |           | ✓       |          |            |           |            |


We hope to extend the number of packages. See [issue
#12](https://github.com/SSoelvsten/bdd-benchmark/issues/12) for a list of BDD
packages we would like to have added to this set of benchmarks. Any help to do
so is very much appreciated.


### Dependencies
All packages use CMake to build. This makes compilation and linking very simple.
One merely has to initialize all submodules (recursively) using the following
command.

```
git submodule update --init --recursive
```

To build, one needs *CMake* and a *C++* compiler of your choice. On Ubuntu, one
can obtain these with the following command:

```bash
apt install cmake g++
```

The *Picotrav* benchmark requires GNU Bison and Flex. On Ubuntu, these can be
installed with.

```bash
apt install bison flex
```

The *McNet* benchmark requires the *Boost Library*. On Ubuntu, these can be
installed as follows

```bash
apt install libboost-all-dev
```

**Adiar**

Adiar also has a transitive dependency on the *Boost Library*. On Ubuntu, these
can be installed as follows

```bash
apt install libboost-all-dev
```

**LibBDD** and **OxiDD**

These libraries are implemented in Rust and interact with C/C++ via an FFI.
Hence, one needs to use *cargo* which in turn requires an internet connection to
download the dependency crates during the first build ("crate" is the Rust term
for a package).

There are different ways to install Rust. On all platforms, it is possible to
use the [official installer script](https://www.rust-lang.org/tools/install) for
*rustup*, the toolchain manager. Some OSes also provide a *rustup* or
*rustup-init* package. If you have *rustup* installed, then you can install the
latest stable Rust toolchain (including *cargo*) using `rustup default stable`.
Some OSes also provide a fairly recent Rust package. On Ubuntu, you can simply
install *cargo* via:

```bash
apt install cargo
```

Furthermore, the libraries depend on *cbindgen* to generate a C header from Rust
code. *cbindgen* can either be installed through Cargo or your OS package
manager. Note however, that a relatively recent version of *cbindgen* is
required (we tested 0.26.0, which is *not* provided by Ubuntu versions prior to
24.04).

```bash
cargo install --force cbindgen
```

> [!NOTE]
> If installing *cbindgen* through cargo, remember to update your *PATH*.
> Otherwise, CMake will abort with an "*Could not find toolchain ''*" error. You
> can obtain the path from the print message during *cargo install* which states
> "*warning: be sure to add <THE_PATH> to your PATH ...*"


## Usage

When all dependencies have been resolved (see [above](#dependencies)), you can
build and run the benchmarks as follows.

### Build

To configure the CMake project run:
```bash
cmake -B build
```

The default settings can be changed by also parsing various parameters to CMake.
Here are the values that might be relevant.

- **`-D CMAKE_BUILD_TYPE=<Release|Debug|RelWithDebInfo|...>`** (default: *Release*)

  Change the build type. Except for development purposes, leaving it at *Release*
  is propaly the most correct choice.

- **`-D CMAKE_C_COMPILER=<...>`, `-D CMAKE_CXX_COMPILER=<...>`**

  Select a specific C/C++ compiler.

- **`-G <Generator>`**

  Select the build generator; on most systems, `Makefile` would be the default.
  To speed up the build process you may want to try to use `Ninja` instead.


Features of the benchmarks and BDD packages can also be turned on and off as follows

- **`-D BDD_BENCHMARK_INCL_INIT=<OFF|ON>`** (default: *OFF*)

  If `ON`, includes the initialization time of the BDD Package in the total running
  time.

- **`-D BDD_BENCHMARK_STATS=<OFF|ON>`** (default: *OFF*)

  If `ON`, build with statistics. This might affect performance.

- **`-D BDD_BENCHMARK_WAIT=<OFF|ON>`** (default: *OFF*)

  If `ON`, pause before deinitialising the BDD package and exiting. This can be
  used to investigate its state with external tools, e.g., use of Hugepages.

After configuring, you can build the benchmarks:
```bash
cmake --build build
```

### Run

After building, the *build/src* folder contains one executable for every
possible combination of BDD library, benchmark, and DD kind. Not all libraries
support every kind of DD (see [above](#bdd-packages)) and not all benchmarks
are available for BDDs/BCDDs or ZDDs.

The executables follows a `<Library>_<Benchmark>_<DD>` naming scheme (each of
the three components in *nocase*). All executables have a common command line
interface to configure the BDD library in question:

- **`-h`**

  Print a help text with all options (including the ones particular to that
  particular benchmark).

- **`-M <int>`** (default: *128*)

  Amount of memory (in MiB) to be dedicated to the BDD library.

  Note, that the memory you set is only for the BDD package. So, the program
  will either use swap or be killed if the BDD package takes up more memory in
  conjunction with the benchmark's auxiliary data structures.

- **`-P <int>`** (default: *1*)

  (Maximum) worker thread count for multi-threaded BDD libraries, e.g., OxiDD
  and Sylvan.

- **`-R`**

  Enable dynamic variable reordering (if available with said BDD package).

- **`-T <path>`** (default: */tmp*, */usr/tmp/*, ...)

  Filepath for temporary files on disk for external memory libraries, e.g.,
  Adiar.

For example, you can run the Queens benchmark on Sylvan with 1024 MiB
of memory and 4 threads as follows:
```bash
./build/src/sylvan_queens_bcdd -M 1024 -P 4
```

Furthermore, each benchmark requires options. See `-h` or the [Benchmarks
Section](#benchmarks) for details.

## Benchmarks

### Apply

Based on [[Pastva2023](#references)], this benchmark loads two (or more) decision
diagrams stored in a *binary* format (as they are serialized by the
[LibBDD](https://github.com/sybila/biodivine-lib-bdd)) and then combines them
with an *Apply* operation (or more).

The benchmark can be configured with the following options:

- **`-f <path>`**

  Path to a *.bdd* / *.zdd* file. Use this once for each input. You can find
  some inputs in the *benchmarks/apply* folder together with links to larger
  and more interesting inputs.

- **`-o <and|or>`** (default: *and*)

  Specify the operator to be used to combine the two decision diagrams.

```bash
./build/src/${LIB}_apply_${KIND} -f benchmarks/apply/x0.bdd -f benchmarks/apply/x1.bdd -o and
```


### CNF Construction

This benchmark loads a DIMACS CNF file, constructs its clauses and conjoins them.
The benchmark uses the variable order specified in the CNF file and interprets
the clause list as an approximately balanced tree, e.g.
(c<sub>0</sub> c<sub>1</sub>) (c<sub>2</sub> (c<sub>3</sub> c<sub>4</sub>)). It
makes sense to preprocess raw CNF files using external tools and infer good
variable and clause orders.

```bash
./build/src/${LIB}_cnf_${KIND} -f benchmarks/cnf/sample.cnf
```


### Game Of Life

Solves the following problem:

> Given N<sub>1</sub> and N<sub>2</sub>, how many *Garden of Edens* exist of size
> N<sub>1</sub>xN<sub>2</sub> in Conway's Game of Life?

The benchmark can be configured with the following options:

- **`-n <int>`**

  The size of the sub-game. Use twice for non-quadratic grids.

- **`-s <...>`**

  Restrict the search to *symmetrical* Garden of Edens:

  - `none`: Search through all solutions (default), i.e., no symmetries are
    introduced.

  - `mirror`/`mirror-vertical`: Solutions that are reflected vertically.

  - `mirror-quadrant`/`mirror-quad`: Solutions that are reflected both
    horizontally and vertically.

  - `mirror-diagonal`/`mirror-diag`: Solutions that are reflected across one
    diagonal (requires a square grid).

  - `mirror-double_diagonal`/`mirror-double_diag`: Solutions that are reflected
    across both diagonals (requires a square grid).

  - `rotate`/`rotate-90`: Solutions that are rotated by 90 degrees (requires a
    square grid).

  - `rotate-180`: Solutions that are rotated by 180 degrees.

All symmetries use a variable order where the pre/post variables are zipped and
and follow a row-major ordering.

```bash
./build/src/${LIB}_game-of-life_${KIND} -n 5 -n 4 -o rotate-180
```


### Hamiltonian Cycle

Solves the following problem:

> Given N<sub>1</sub> and N<sub>2</sub>, how many hamiltonian cycles exist on a
> Grid Graph size N<sub>1</sub>xN<sub>2</sub>?

The benchmark can be configured with the following options:

- **`-n <int>`**

  The size of the grid; use twice for non-quadratic grids.

- **`-e <...>`** (default: *time*)

  Pick the encoding/algorithm to solve the problem with:

  - `time`/`t`: Based on [[Bryant2021](#references)], all O(N<sup>4</sup>)
    states are represented as individual variables, i.e., each variable
    represents the position and time. A transition relation then encodes the
    legal moves between two time steps. By intersecting moves at all time steps
    we obtain all paths. On-top of this, hamiltonian constraints are added and
    finally the size of the set of cycles is obtained.

  - `binary`: Based on [[Marijn2021](#references)] with multiple tweaks by
    Randal E. Bryant to adapt it to decision diagrams. Each cell's choice of
    move, i.e., each edge, is represented by a binary number with
    *log<sub>2</sub>(4) = 2* variables. Yet, this does not enforce the choice of
    edges correspond to a Hamiltonian Cycle. Hence, we further add
    *log<sub>2</sub>(N<sup>2</sup>)*-bit binary counter gadgets. These encode
    *if u->v then v=u+1 % N<sup>2</sup>*.

<!--
  - `unary`/`one-hot`: Similar to `binary` but the edges and the gadgets of *b*
    values use a one-hot encoding with *b* variables. Only one out of the *b*
    variables are ever set to true at the same time; the value of the gadget is
    the variable set to true.

  - `crt_one-hot`/`crt`: Similar to `unary` but one or more prime numbers are
    used for the gadgets added at the end. By use of the Chinese Remainder
    Theorem, we can still be sure, we only have valid cycles at the end. One
    hopes this decreases the size of the diagram, since the number of possible
    values for each gadget are much smaller.
-->

  The `time` <!-- and the `unary`/`crt_unary` --> encoding are designed with ZDDs
  in mind whereas the `binary` encoding is designed for BDDs. That is, using the
  `time` encoding with BDDs does not give you great, i.e., small and fast, results.

```bash
./build/src/${LIB}_hamiltonian_${KIND} -n 6 -n 5
```

<!--
> [!IMPORTANT]
> The gadget for the *unary* encodings is not yet properly implemented. Hence,
> this variant has been disabled.
-->


### McNet

> [!IMPORTANT]
> ZDDs are not supported for this benchmark (yet)!

This simple *Model Checker for Nets* (pronounced "*Mac Net*", "*magnet*" or
maybe "*Monet*"?) provides algorithms for the exploration of various transition
system formats.

- **`-f <path>`**

  Path to a file with a Petri Net (*.pnml*) or a Boolean Network (*.aeon*,
  *.bnet*, *.sbml*). You can find multiple inputs in the *benchmarks/mcnet*
  folder together with links to larger and more interesting inputs.

- **`-a <...>`**

  The algorithm(s) to run. All algorithms depend on having computed the
  reachable states.

  - `reachable`/`reachability`: Compute the set of reachable states.

  - `deadlocks`: Compute the set of reachable deadlock states.

  - `scc`: Compute each Strongly Connected Component using the Chain
    algorithm from [[Larsen2023](#references)]

- **`-o <...>`** (default: *input*)

  Each BDD variable represents a location in the net. Hence, the variable
  ordering can have a major impact on the diagrams' size.

  - `cuthill-mckee`: Use the Cuthill-McKee order derived from the incidence
    graph, i.e. where variables are connected if they occur together in the
    same transition.

  - `input`: Use the order in which the locations are declared in the file.

  - `random`: A randomized ordering of variables.

  - `sloan`: Use Sloan's ordering derived from the incidence graph, where
    variables are connected if they occur together in the same transition.

- **`-s <...>`**

  Merges the transition relation together into one with the given semantics.
  If not set, then the model is explored with *asynchronous* semantics and a
  *disjoint* relation.

  - `asynchronous`: Explore the model instead with *asynchronous* update
    semantics and a single *joined* relation.

  - `synchronous`: Explore the model instead with *synchronous* update
    semantics.

```bash
./build/src/${LIB}_mcnet_${KIND} -f benchmarks/mcnet/split.pnml
```


### Picotrav
This benchmark is a small recreation of the *Nanotrav* example provided with the
CUDD library [[Somenzi2015](#references)]. Given a hierarchical circuit in (a
subset of the) [Berkeley Logic Interchange Format
(BLIF)](https://course.ece.cmu.edu/~ee760/760docs/blif.pdf) a BDD is created for
every net; dereferencing BDDs for intermediate nets after they have been used
for the last time. If two *.blif* files are given, then BDDs for both are
constructed and every output net is compared for equality.

The benchmark can be configured with the following options:

- **`-f <path>`**

  Path to a *.blif* file. You can find multiple inputs in the
  *benchmarks/picotrav* folder together with links to larger and more
  interesting inputs.

  If used twice, the BDDs for both circuits' output gates are constructed and
  checked for logical equality.

- **`-o <...>`** (default: *input*)

  BDD variables represent the value of inputs. Hence, a good variable ordering
  is important for a high performance. To this end, one can use various variable
  orderings derived from the given net.

  - `input`: Use the order in which they are declared in the input *.blif* file.

  - `df`/`depth-first`: Variables are ordered based on a depth-first traversal
    where non-input gates are recursed to first; thereby favouring deeper nodes.

  - `level`: Variables are ordered based on the deepest reference by another
    net. Ties are broken based on the declaration order in the input (`input`).

  - `level_df`: Similar to `level` but ties are broken based on the ordering in
    `df` rather than `input`.

  - `random`: A randomized ordering of variables.

```bash
./build/src/${LIB}_picotrav_${KIND} -f benchmarks/picotrav/not_a.blif -f benchmarks/picotrav/not_b.blif -o level_df
```


### QBF Solver

> [!IMPORTANT]
> ZDDs are not supported for this benchmark (yet)!

Based on Jaco van de Pol's Christmas holiday hobbyproject, this is an
implementation of a QBF solver. Given an input in the
[*qcir*](https://www.qbflib.org/qcir.pdf) format, the decision diagram
representing each gate is computed bottom-up. The outermost quantifier in the
*prenex* is not resolved with BDD operations. Instead, if the decision diagram
has not already collapsed to a terminal, a witness/counter-example is obtained
from the diagram.

- **`-f <path>`**

  Path to a *.qcir* file. You can find multiple inputs in the *benchmarks/qbf*
  folder together with links to larger and more interesting inputs.

- **`-o <...>`** (default: *input*)

  Each BDD variable corresponds to an input variables, i.e., a literal, of the
  *qcir* circuit. To keep the decision diagrams small, one can choose from using
  different variable orders; each of these are derived from the given circuit
  during initialisation.

  - `input`: Use the order in which variables are introduced in the *.qcir*
    file.

  - `df`/`depth-first`: Order variables based on their first occurence in a
    depth-first traversal of the circuit.

  - `df_rtl`/`depth-first_rtl`: Same as `df` but the depth-first traversal is
    *right-to-left* for each gate in the circuit.

  - `level`/`level_df`: Order variables first based on their deepest reference
    by another gate. Ties are broken based on the depth-first (`df`) order.

  If the `input` ordering is used, then the gates of the circuit are also
  resolved in the order they were declared. Otherwise, for `df` and `level`,
  gates are resolved in a bottom-up order based on their depth within the
  circuit.

```bash
./build/src/${LIB}_qbf_${KIND} -f benchmarks/qbf/example_a.qcir -o df
```


### Queens
Solves the following problem:

> Given N, in how many ways can N queens be placed on an N x N chess board
> without threatening eachother?

Our implementation of these benchmarks are based on the description of
[[Kunkle2010](#references)]. Row by row, we construct an BDD that represents
whether the row is in a legal state: is at least one queen placed on each row
and is it also in no conflicts with any other? On the accumulated BDD we then
count the number of satisfying assignments.

The benchmark can be configured with the following options:

- **`-n <int>`**

  The size of the chess board.

```bash
./build/src/${LIB}_queens_${KIND} -n 8
```


### RelProd

> [!IMPORTANT]
> ZDDs are not supported for this benchmark (yet)!

Building on-top of the [Apply](#apply) benchmark, this benchmark loads a
*relation* and a set of *states* stored in a *binary* format (as they are
serialized by the
[LibBDD](https://github.com/sybila/biodivine-lib-bdd)) and then combines them
with a *Relational Product* operation in either direction.

The benchmark can be configured with the following options:

- **`-o <next|prev>`** (default: *next*)

  Specify whether the transition relation should be traversed forwards
  (*next*) or backwards (*prev*).

- **`-r <path>`**

  Path to a *.bdd* / *.zdd* file that contains the relation. It is also assumed,
  that this relation includes the *frame rule*.

- **`-s <path>`**

  Path to a *.bdd* / *.zdd* file that contains the set of states.

```bash
./build/src/${LIB}_relprod_${KIND} -r benchmarks/relprod/self-loop/relation.bdd -s benchmarks/relprod/self-loop/states_all.bdd -o next
```


### Tic-Tac-Toe
Solves the following problem:

> Given N, in how many ways can Player 1 place N crosses in a 3D 4x4x4 cube and
> have a tie, when Player 2 places noughts in all remaining positions?

This benchmark stems from [[Kunkle2010](#references)]. Here we keep an
accumulated BDD on which we add one of the 76 constraints of at least one cross
and one nought after the other. We add these constraints in a different order
than [[Kunkle2010](#references)], which does result in an up to 100 times
smaller largest intermediate result.

The interesting thing about this benchmark is, that even though the BDDs grow
near-exponentially, the initial BDD size grows polynomially with N, it always
uses 64 variables number and 76 Apply operations.

- **`-n <int>`**

  The number of crosses are placed in the 64 positions.

```bash
./build/src/${LIB}_tic-tac-toe_${KIND} -n 20
```


## Performance Regression Testing

This collection of benchmarks is not only a good way to compare implementations
of BDD packages. Assuming there are no breaking changes in a pull request, this
also provides a way to test for *performance regression*.

To this end, *regression.py* provides an interactive python program that fully
automates downloading inputs and running and analysing the benchmark timings of
two branches. For a non-interactive context, e.g., continuous integration, all
arguments can be parsed through *stdin*. For example, the following goes through
all the prompts for using the *8*-Queens problem to test Adiar with *128* MiB
`origin/main` (against itself) with *no* verbose build or runtime output and
with at least 3 and at most 10 samples.

```bash
python regression.py <<< "queens
8
adiar
128
origin
main
origin
main
no
no
3
10
"
```

Between *min* and *max* number of samples are collected, until both the
*baseline* and the *under test* branches have a standard deviation below 5%. For
smaller instances, samples are collected for at least 30 minutes whereas for
larger instances, the last sampling may start after 90 minutes.

The python script exits with a non-zero code, if there is a statistically
relevant slowdown. A short and concise report is placed in
*regression_{package}.out* which can be posted as a comment on the pull request.


## License
The software files in this repository are provided under the
[MIT License](/LICENSE.md).


## Citation
If you use this repository in your work, we sadly do not yet have written a
paper on this repository alone (this will be done though). In the meantime,
please cite the initial paper on *Adiar*.

```bibtex
@InProceedings{soelvsten2022:TACAS,
  title         = {Adiar: Binary Decision Diagrams in External Memory},
  author        = {S{\o}lvsten, Steffan Christ
               and van de Pol, Jaco
               and Jakobsen, Anna Blume
               and Thomasen, Mathias Weller Berg},
  year          = {2022},
  booktitle     = {Tools and Algorithms for the Construction and Analysis of Systems},
  editor        = {Fisman, Dana
               and Rosu, Grigore},
  pages         = {295--313},
  numPages      = {19},
  publisher     = {Springer},
  series        = {Lecture Notes in Computer Science},
  volume        = {13244},
  isbn          = {978-3-030-99527-0},
  doi           = {10.1007/978-3-030-99527-0\_16},
}
```


## References

- [[Brace1990](https://doi.org/10.1109/DAC.1990.114826)]
  K. Brace, R. Rudell, R. E. Bryant: “*Efficient implementation of a BDD package*”.
  In: 27th ACM/IEEE Design Automation Conference. pp. 40–45 (1990).

- [[Bryant1986](https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=1676819)]
  R. E. Bryant. “*Graph-Based Algorithms for Boolean Function Manipulation*”.
  In: *IEEE Transactions on Computers*. (1986)

- [[Bryant2021](https://github.com/rebryant/Cloud-BDD/blob/conjunction_streamlined/hamiltonian/hpath.py)]
  R. E. Bryant. “*hpath.py*”. In: *Cloud-BDD* (GitHub). 2021

- [[Larsen2023](https://link.springer.com/chapter/10.1007/978-3-031-30820-8_22)]
  C. A. Larsen, S. M. Schmidt, J. Steensgaard, A. B. Jakobsen, J. van de Pol,
  and A. Pavlogiannis: “*A Truly Symbolic Linear-Time Algorithm for SCC
  Decomposition*”. In: 27th ACM/IEEE Design Automation Conference. pp. 40–45
  (1990).

- [[Kunkle2010](https://dl.acm.org/doi/abs/10.1145/1837210.1837222)] Daniel
  Kunkle, Vlad Slavici, Gene Cooperman. “*Parallel Disk-Based Computation for
  Large, Monolithic Binary Decision Diagrams*”. In: *PASCO '10: Proceedings of
  the 4th International Workshop on Parallel and Symbolic Computation*. 2010

- [[Marijn2021](https://link.springer.com/chapter/10.1007/978-3-030-80223-3_15)]
  Heule, Marijn J. H. “*Chinese Remainder Encoding for Hamiltonian Cycles*”. In:
  *Theory and Applications of Satisfiability Testing*. 2021

- [[Minato1993](https://dl.acm.org/doi/10.1145/157485.164890)]
  S. Minato. “*Zero-suppressed BDDs for Set Manipulation in Combinatorial
  Problems*”. In: *International Design Automation Conference*. 1993

- [[Pastva2023](https://ieeexplore.ieee.org/document/10329375)]
  S. Pastva and T. Henzinger. “*Binary Decision Diagrams on Modern Hardware*”.
  In: *Proceedings of the 23rd Conference on Formal Methods in Computer-Aided
  Design* (2023)

- [[Somenzi2015](https://github.com/ssoelvsten/cudd)]
  Somenzi, Fabio: *CUDD: CU decision diagram package, 3.0*. University
  of Colorado at Boulder. 2015
