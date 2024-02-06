# BDD Benchmark

This repository contains multiple examples for use of Binary Decision Diagrams
(BDDs) to solve various problems. These are all implemented in exactly the same
way, thereby allowing one to compare implementations.

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [BDD Packages](#bdd-packages)
    - [Enforcing comparability](#enforcing-comparability)
    - [Dependencies](#dependencies)
- [Usage](#usage)
- [Benchmarks](#benchmarks)
    - [Apply](#apply)
    - [Game of Life](#game-of-life)
    - [Hamiltonian Cycle](#hamiltonian-cycle)
    - [Picotrav](#picotrav)
    - [QBF Solver](#qbf-solver)
    - [Queens](#queens)
    - [Tic-Tac-Toe](#tic-tac-toe)
- [Performance Regression Testing](#performance-regression-testing)
- [License](#license)
- [Citation](#citation)
- [References](#references)

<!-- markdown-toc end -->


## BDD Packages
We provide all the benchmarks described further below for the following
libraries.

- [**Adiar**](https://github.com/ssoelvsten/adiar):
  An I/O Efficient implementation with iterative algorithms using priority
  queues to exploit a special sorting of BDD nodes streamed from and to the
  disk. These algorithms have no need for memoization or garbage collection.
  But, on the other hand, nodes are also not shareable between BDDs. It also
  supports Zero-suppressed Decision Diagrams.


- [**BuDDy**](http://vlsicad.eecs.umich.edu/BK/Slots/cache/www.itu.dk/research/buddy/):
  An easy-to-use yet extensive implementation with depth-first algorithms using
  a unique node table and memoization. It also supports variable reordering.

  We use [this](https://github.com/SSoelvsten/BuDDy) version that builds using
  CMake and has been updated slightly.


- [**CAL**](https://github.com/SSoelvsten/cal):
  A breadth-first implementation that exploits a specific level-by-level
  locality of nodes on disk to improve performance when dealing with large BDDs.
  Unlike Adiar it also supports sharing of nodes between BDDs at the cost of
  memoization and garbage collection.

  We use the [revived version](https://github.com/SSoelvsten/cal) with an
  extended C API, CMake support and a C++ API.


- **CUDD**:
  Probably the most popular BDD package of all. It uses depth-first algorithms
  and a unique node table, memoization and complement edges, variable reordering,
  and supports Zero-suppressed Decision Diagrams.

  We use [this modified v3.0](https://github.com/SSoelvsten/cudd) in which its
  C++ API has been extended.


- [**Sylvan**](https://github.com/trolando/sylvan):
  A parallel (multi-core) implementation with depth-first algorithms using a
  unique node table and memoization. It also uses complement edges and supports
  Zero-suppressed Decision Diagrams.


We hope to extend the number of packages. See
[issue #12](https://github.com/SSoelvsten/bdd-benchmark/issues/12) for a list
of BDD packages we would like to have added to this set of benchmarks. Any
help to do so is very much appreciated.


### Enforcing comparability
For comparability, we will enforce all packages to follow the same settings.

- Packages will initialise its unique node table (if any) to its full potential
  size and have its operation cache (if any) set to of 64:1.

- Dynamic variable reordering is disabled.

- Multi-threaded libraries use (except otherwise requested) only a single core.


### Dependencies
All packages, but CUDD, use CMake to build. This makes compilation and linking
very simple. One merely has to initialise all submodules (recursively) using the
following command.

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

**Adiar**

Adiar also has dependencies on the *Boost Library*. On Ubuntu, these can be
installed as follows
```bash
apt install libboost-all-dev
```

**CUDD**

The project has been built on Linux and tested on Ubuntu 18.04 through 22.04
and Fedora 36+.

On *Windows Subsystem for Linux* or *Cygwin* the automake installation will
fail due to *\r\n* line endings. To resolve this, first install *dos2unix*,
then convert the line endings for the relevant files as shown below in the CUDD
folder.

```bash
apt install dos2unix
find . -name \*.m4|xargs dos2unix
find . -name \*.ac|xargs dos2unix
find . -name \*.am|xargs dos2unix
```
Alternatively, run the `make clean` target in this repositories root folder.

Installation of CUDD seems neither possible without also building the
documentation. For this, you need a local installation of LaTeX.
```bash
apt install texlive texlive-latex-extra
```

Building and linking CUDD in spite of its lack of CMake support is handled in
the `make build` script (see below). If you want to do it manually instead,
write the following lines to build and install it to CMake's *build* folder:
```bash
cd external/cudd
autoreconf
./configure --prefix ../../<cmake-build-folder>/cudd/ --enable-obj
make MAKEINFO=true
make install
```

**Sylvan**

Sylvan also needs the *The GNU Multiple Precision Arithmetic* and *Portable
Hardware Locality* libraries, which can be installed as follows
```bash
apt install libgmp-dev libhwloc-dev
```


## Usage

All interactions have been made easy by use of the *makefile* at the root.

| Target    | Description                               |
|-----------|-------------------------------------------|
| `build`   | Build all dependencies and all benchmarks |
| `clean`   | Remove all build artifacts                |
| `run/...` | Run a single instance of a benchmark      |

The benchmarks can be built with multiple options:

| CMake Variable        | Make Variable | Description                                                       |
|-----------------------|---------------|-------------------------------------------------------------------|
| `BDD_BENCHMARK_STATS` | `STATS`       | If *ON*, build with statistics (default is *OFF*)                 |
| `BDD_BENCHMARK_WAIT`  | `WAIT`        | If *ON*, pause before deinitialising the BDD package and exiting. |

Each benchmark below also has its own *make* target too for ease of use. You may
specify as a make variable the amount of *M*emory (MiB) to use by the *V*ariant\
(i.e. BDD package) to be tested. For example, to solve the combinatorial Queens
problem with BuDDy with 256 MiB of memory you run the following target.

```bash
make run/queens V=buddy M=256
```

Note, that the memory you set is only for the BDD package. So, the program will
either use swap or be killed if the BDD package takes up more memory in
conjunction with the benchmark's auxiliary data structures.

We provide benchmarks for both *Binary* and for *Zero-suppressed* Decision
Diagrams. Not all benchmarks or BDD packages support both types of decision
diagrams, but if possible, then the type can be chosen as part of the make
target.

```bash
make run/queens/zdd V=adiar M=256
```

Some benchmarks allow for choosing between a set of *O*ptions, e.g. variable
ordering, encoding, or algorithm to use.

```bash
make run/picotrav O=level_df
```

All Make variables have default values when unspecified.


## Benchmarks

### Apply

Based on [[Pastva2023](#references)], this benchmark loads two decision diagrams
stored in a *binary* format (as they are serialized by the
[lib-bdd](https://github.com/sybila/biodivine-lib-bdd) BDD package) and then
combines them with a single *Apply* operation.

As an option, you can specify the operator to be used to combine the decision
diagrams.

- `and`
- `or`

The *.bdd* / *.zdd* file(s) is given with the `-f` parameter (*F1* and *F2*
Make variables) and the apply operand with `-o` (*O* for Make). You can find
some inputs in the *benchmarks/apply* folder together with links to larger and
more interesting inputs.

```bash
make run/apply F1=benchmarks/apply/x0.bdd F2=benchmarks/apply/x1.bdd O=and
```

### Game Of Life

Solves the following problem:

> Given N<sub>1</sub> and N<sub>2</sub>, how many *Garden of Edens* exist of size
> N<sub>1</sub>xN<sub>2</sub> in Conway's Game of Life?

The search can optionally be restricted to *symmetrical* Garden of Edens:

- `none`: Search through all solutions (default), i.e. no symmetries are
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
make run/game-of-life NR=5 NC=4 O=mirror-diagonal
```


### Hamiltonian Cycle

Solves the following problem:

> Given N<sub>1</sub> and N<sub>2</sub>, how many hamiltonian cycles exist on a
> Grid Graph size N<sub>1</sub>xN<sub>2</sub>?

*Optionally*, you can pick the encoding/algorithm to solve the problem with:

- `time`/`t`: Based on [[Bryant2021](#references)], all O(N<sup>4</sup>) states
  are represented as individual variables, i.e. each variable represents the
  position and time. A transition relation then encodes the legal moves between
  two time steps. By intersecting moves at all time steps we obtain all paths.
  On-top of this, hamiltonian constraints are added and finally the size of the
  set of cycles is obtained.

- `binary`: Based on [[Marijn2021](#references)] with multiple tweaks by Randal
  E. Bryant to adapt it to decision diagrams. Each cell's choice of move, i.e.
  each edge, is represented by a binary number with *log<sub>2</sub>(4) = 2*
  variables. Yet, this does not enforce the choice of edges correspond to a
  Hamiltonian Cycle. Hence, we further add *log<sub>2</sub>(N<sup>2</sup>)*-bit
  binary counter gadgets. These encode *if u->v then v=u+1 % N<sup>2</sup>*.

- `unary`/`one-hot`: Similar to `binary` but the edges and the gadgets of *b*
  values use a one-hot encoding with *b* variables. Only one out of the *b*
  variables are ever set to true at the same time; the value of the gadget is
  the variable set to true.

- `crt_one-hot`/`crt`: Similar to `unary` but one or more prime numbers are used
  for the gadgets added at the end. By use of the Chinese Remainder Theorem, we
  can still be sure, we only have valid cycles at the end. One hopes this
  decreases the size of the diagram, since the number of possible values for
  each gadget are much smaller.

The `time` and the `unary`/`crt_unary` encoding are designed with ZDDs in mind
whereas the `binary` encoding is designed for BDDs. That is, using the `time`
encoding with BDDs does not give you great, i.e. small and fast, results.

```bash
make run/hamiltonian NR=6 NC=5
```

> [!IMPORTANT]
> The gadget for the `one-hot` encodings is not yet properly implemented. Hence,
> this variant has been disabled.

### Picotrav
This benchmark is a small recreation of the *Nanotrav* example provided with the
CUDD library [[Somenzi2015](#references)]. Given a hierarchical circuit in (a
subset of the) [Berkeley Logic Interchange Format
(BLIF)](https://course.ece.cmu.edu/~ee760/760docs/blif.pdf) a BDD is created for
every net; dereferencing BDDs for intermediate nets after they have been used
for the last time. If two *.blif* files are given, then BDDs for both are
constructed and every output net is compared for equality.

BDD variables represent the value of inputs. Hence, a good variable ordering is
important for a high performance. To this end, one can use various variable
orderings derived from the given net.

- `input`: Use the order in which they are declared in the input *.blif* file.

- `df`/`depth-first`: Variables are ordered based on a depth-first traversal
  where non-input gates are recursed to first; thereby favouring deeper nodes.

- `level`: Variables are ordered based on the deepest reference by another net.
  Ties are broken based on the declaration order in the input (`input`).

- `level_df`: Similar to `level` but ties are broken based on the ordering in
  `df` rather than `input`.

- `random`: A randomized ordering of variables.

The *.blif* file(s) is given with the `-f` parameter (*F1* and *F2* Make
variables) and the variable order with `-o` (*O* for Make). You can find
multiple inputs in the *benchmarks/picotrav* folder together with links to
larger and more interesting inputs.

```bash
make run/picotrav F1=benchmarks/picotrav/not_a.blif F2=benchmarks/picotrav/not_b.blif O=level_df
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

Each BDD variable corresponds to an input variables, i.e. a literal, of the
*qcir* circuit. To keep the decision diagrams small, one can choose from using
different variable orders; each of these are derived from the given circuit
during initialisation.

- `input`: Use the order in which variables are introduced in the *.qcir* file.

- `df`/`depth-first`: Order variables based on their first occurence in a
  depth-first traversal of the circuit.

- `df_rtl`/`depth-first_rtl`: Same as `df` but the depth-first traversal is
  *right-to-left* for each gate in the circuit.

- `level`/`level_df`: Order variables first based on their deepest reference by
  another gate. Ties are broken based on the depth-first (`df`) order.

If the `input` ordering is used, then the gates of the circuit are also resolved
in the order they were declared. Otherwise for `df` and `level`, gates are
resolved in a bottom-up order based on their depth within the circuit.

The *.qcir* file is given with the `-f` parameter (*F* Make variable) and the
ordering with `-o` (*O* for Make).

```bash
make run/qbf F=benchmarks/qcir/example_a.blif O=df
```


### Queens
Solves the following problem:

> Given N, in how many ways can N queens be placed on an N x N chess board
> without threatening eachother?

Our implementation of these benchmarks are based on the description of
[[Kunkle10](#references)]. Row by row, we construct an BDD that represents
whether the row is in a legal state: is at least one queen placed on each row
and is it also in no conflicts with any other? On the accumulated BDD we then
count the number of satisfying assignments.

```bash
make run/queens N=8
```


### Tic-Tac-Toe
Solves the following problem:

> Given N, in how many ways can Player 1 place N crosses in a 3D 4x4x4 cube and
> have a tie, when Player 2 places noughts in all remaining positions?

This benchmark stems from [[Kunkle10](#references)]. Here we keep an accumulated
BDD on which we add one of the 76 constraints of at least one cross and one
nought after the other. We add these constraints in a different order than
[[Kunkle10](#references)], which does result in an up to 100 times smaller largest
intermediate result.

The interesting thing about this benchmark is, that even though the BDDs grow
near-exponentially, the initial BDD size grows polynomially with N, it always uses
64 variables number and 76 Apply operations.

```bash
make run/tic-tac-toe N=20
```


## Performance Regression Testing

This collection of benchmarks is not only a good way to compare implementations
of BDD packages. Assuming there are no breaking changes in a pull request, this
also provides a way to test for *performance regression*.

To this end, *regression.py* provides an interactive python program that fully
automates downloading inputs and running and analysing the benchmark timings of
two branches. For a non-interactive context, e.g. continuous integration, all
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

- [[Bryant2021](https://github.com/rebryant/Cloud-BDD/blob/conjunction_streamlined/hamiltonian/hpath.py)]
  Bryant, Randal E. “*hpath.py*”. In: *Cloud-BDD* (GitHub). 2021

- [[Kunkle10](https://dl.acm.org/doi/abs/10.1145/1837210.1837222)] Daniel
  Kunkle, Vlad Slavici, Gene Cooperman. “*Parallel Disk-Based Computation for
  Large, Monolithic Binary Decision Diagrams*”. In: *PASCO '10: Proceedings of
  the 4th International Workshop on Parallel and Symbolic Computation*. 2010

- [[Marijn2021](https://link.springer.com/chapter/10.1007/978-3-030-80223-3_15)]
  Heule, Marijn J. H. “*Chinese Remainder Encoding for Hamiltonian Cycles*”. In:
  *Theory and Applications of Satisfiability Testing*. 2021

- [[Pastva2023](https://repositum.tuwien.at/bitstream/20.500.12708/188807/1/Pastva-2023-Binary%20decision%20diagrams%20on%20modern%20hardware-vor.pdf)]
  Samuel Pastva and Thomas Henzinger. “*Binary Decision Diagrams on Modern
  Hardware*”. In: *Proceedings of the 23rd Conference on Formal Methods in
  Computer-Aided Design* (2023)

- [[Somenzi2015](https://github.com/ssoelvsten/cudd)]
  Somenzi, Fabio: *CUDD: CU decision diagram package, 3.0*. University
  of Colorado at Boulder. 2015
