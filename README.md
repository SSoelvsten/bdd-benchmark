# BDD Benchmark
This repository contains multiple examples for use of BDDs to solve various
problems. These are all implemented in exactly the same way, thereby allowing
one to compare implementations.

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [BDD Benchmark](#bdd-benchmark)
    - [Implementations](#implementations)
        - [Enforcing comparability](#enforcing-comparability)
    - [Dependencies](#dependencies)
    - [Usage](#usage)
    - [Combinatorial Benchmarks](#combinatorial-benchmarks)
        - [Knight's Tour [ZDD]](#knights-tour-zdd)
        - [Queens [BDD, ZDD]](#queens-bdd-zdd)
        - [Tic-Tac-Toe [BDD, ZDD]](#tic-tac-toe-bdd-zdd)
    - [Verification](#verification)
        - [Picotrav [BDD]](#picotrav-bdd)
    - [License](#license)
    - [References](#references)

<!-- markdown-toc end -->

## Implementations
We provide all the benchmarks described further below for the following
libraries.

- [**Adiar**](https://github.com/ssoelvsten/adiar):
  An I/O Efficient implementation with iterative algorithms using priority
  queues to exploit a special sorting of streamed from/to disk. These algorithms
  have no need for memoization or garbage collection, but, on the other hand,
  nodes are also not shareable between BDDs.


- [**BuDDy**](http://vlsicad.eecs.umich.edu/BK/Slots/cache/www.itu.dk/research/buddy/):
  An easy-to-use yet extensive implementation with depth-first algorithms using
  a unique node table and memoization. It also supports variable reordering.

  We use the version from [here](https://github.com/jgcoded/BuDDy) that is set
  up for building with CMake.


- **CUDD**:
  The most popular BDD package. It uses depth-first algorithms and a unique node
  table and memoization and also supports complement edges, Zero-suppressed
  Decision Diagrams, and variable reordering.

  We use version 3.0.0 as distributed on the unofficial mirror
  [here](https://github.com/SSoelvsten/cudd).


- [**Sylvan**](https://github.com/trolando/sylvan):
  A parallel (multi-core) implementation with depth-first algorithms using a
  unique node table and memoization. It also uses complement edges and supports
  Zero-suppressed Decision Diagrams.

  We will _not_ make use of the multi-core aspect to make the results
  comparable.

We hope to extend the number of packages. See
[issue #12](https://github.com/SSoelvsten/bdd-benchmark/issues/12) for a list
of BDD packages we would like to have added to this set of benchmarks. Any
help to do so is very much appreciated.

### Enforcing comparability
For comparability, we will enforce all packages to follow the same settings.

- Only use a single core.

- Packages will initialise its unique node table to its full potential size and
  have its operation cache (memoization table) set to of 64:1.

- Dynamic variable reordering is disabled.


## Dependencies
Almost packages interface with CMake or GNU Autotools, which makes installation
very simple after having initialised all submodules using the following command.

```
git submodule update --init --recursive
```

This also requires _CMake_ and a _C++_ compiler of your choice. The _Picotrav_
benchmark requires GNU Bison and Flex, which can be installed with.

```bash
apt install bison flex
```

**Adiar**

Adiar also has dependencies on the _Boost Library_, which can be installed as follows
```bash
apt install libboost-all-dev
```

**CUDD**

The project has been built on Linux and tested on Ubuntu 18.04 and 20.04. On
_Windows Subsystem for Linux_ or _Cygwin_ the automake installation will fail
due to _\r\n_ line endings. To resolve this, first install dos2unix, then convert
the line endings for the relevant files as shown below in the CUDD folder.

```bash
apt install dos2unix
find . -name \*.m4|xargs dos2unix
find . -name \*.ac|xargs dos2unix
find . -name \*.am|xargs dos2unix
```
Alternatively, run `make clean`.

Installation of CUDD seems neither possible without also building the
documentation. For this, you need a local installation of LaTeX.
```bash
sudo apt install texlive texlive-latex-extra
```

**Sylvan**

Sylvan also needs the _The GNU Multiple Precision Arithmetic_ and _Portable
Hardware Locality_ libraries, which can be installed as follows
```bash
apt install libgmp-dev libhwloc-dev
```


## Usage

All interactions have been made easy by use of the _makefile_ at the root.

| Target  | Description                               |
|---------|-------------------------------------------|
| `build` | Build all dependencies and all benchmarks |
| `clean` | Remove all build artifacts                |

Each benchmark below also has its own _make_ target too for ease of use. You may
specify as a make variable the instance size _N_ to solve, the amount of
_M_emory (MiB) to use in, and the _V_ariant (i.e. BDD package). For example, to
solve the combinatorial Queens with CUDD for _N_ = 10 and with 256 MiB of memory
you run the following target.

```bash
make combinatorial/queens V=cudd N=10 M=256
```

We provide benchmarks for both *Binary* and for *Zero-suppressed* Decision
Diagrams. Not all benchmarks support both types of decision diagrams, but if
possible, then the type can be chosen as part of the make target. We will below
mark whether a benchmark supports **BDD**s or **ZDD**s.

```bash
make combinatorial/queens/zdd V=cudd N=10 M=256
```

Some benchmarks allow for choosing between a set of **O**ptions, e.g. variable
ordering or algorithm.

```bash
make verification/picotrav V=cudd O=LEVEL_DFS
```

## Combinatorial Benchmarks

### Knight's Tour [ZDD]
Solves the following problem:

> Given N, then how many hamiltonian paths can a single Knight do across a chess
> board of size (N/2)x(N-N/2)?

Our implementation is based on Bryants implementation
[here](https://github.com/rebryant/Cloud-BDD/blob/conjunction_streamlined/hamiltonian/hpath.py)
using ZDDs. We represent all O(N<sup>4</sup>) states, i.e. position and time, as
a separate variable; a transition relation then encodes the legal moves between
two time steps. By intersecting moves at all time steps we obtain all paths.
On-top of this, hamiltonian constraints are added and finally the size of the
set of Knight's Tours is obtained.

You may choose to include the hamiltonian constraint inside of the transition
relation. This seems to result in slower running times, but slightly smaller
ZDDs.

You may provide an option `-o` of the form `{STRAT}_{TYPE}`

- `{STRAT}`: Whether to use the `SPLIT` approach, where transitions and
  hamiltonian are handled separately. Alternatively, one may use the `COMBINED`
  approach.

- `{TYPE}`: Whether to count the `OPEN` paths, i.e. all hamiltonian paths, or
  only the `CLOSED` ones, i.e. the hamiltonian cycles.

`OPEN` and `CLOSED` are shortcuts for using `SPLIT`.

**Statistics:**

| Variable                | Value         |
|-------------------------|---------------|
| Labels                  | N<sup>4</sup> |
| Intersection operations | 2N<sup>4</sup> |

### Queens [BDD, ZDD]
Solves the following problem:

> Given N, then in how many ways can N queens be placed on an N x N chess board
> without threatening eachother?

Our implementation of these benchmarks are based on the description of
[[Kunkle10](#references)]. We construct an BDD row-by-row that represents
whether the row is in a legal state: is at least one queen placed on each row
and is it also in no conflicts with any other? On the accumulated BDD we then
count the number of satisfying assignments.

**Statistics:**

| Variable         | Value  |
|------------------|--------|
| Labels           | N²     |
| Apply operations | N² + N |


### Tic-Tac-Toe [BDD, ZDD]
Solves the following problem:

> Given N, then in how many ways can Player 1 place N crosses in a 3D 4x4x4 cube
> and have a tie, when Player 2 places noughts in all remaining positions?

This benchmark stems from [[Kunkle10](#references)]. Here we keep an accumulated
BDD on which we add one of the 76 constraints of at least one cross and one
nought after the other. We add these constraints in a different order than
[[Kunkle10](#references)], which does result in an up to 100 times smaller largest
intermediate result.

**Statistics:**

| Variable          |           Value |
|-------------------|-----------------|
| Labels            |              64 |
| Apply operations  |              76 |
| Initial BDD size  | (64-N+1)(N+1)-1 |

## Verification

### Picotrav [BDD]
This benchmark is a small recreation of the _Nanotrav_ example provided with the
CUDD library. Given a hierarchical circuit in (a subset of the) [Berkeley Logic
Interchange Format (BLIF)](https://course.ece.cmu.edu/~ee760/760docs/blif.pdf) a
BDD is created for every net; dereferencing BDDs for intermediate nets after
they have been used for the last time. If two _.blif_ files are given, then BDDs
for both are constructed and every output net is compared for equality.

BDD variables represent the value of inputs. Hence, a good variable ordering is
important for a high performance. To this end, one can use various variable
orderings derived from the given net.

- `INPUT`: Use the order in which they are declared in the input _.blif_ file.

- `DFS`: Variables are ordered based on a DFS traversal where non-input gates
  are recursed to first; thereby favouring deeper nodes.

- `LEVEL`: Variables are ordered based on the deepest reference by another net.
  Ties are broken based on the declaration order in the input (`INPUT`).

- `LEVEL_DFS`: Similar to `LEVEL` but ties are broken based on the ordering in
  `DFS` rather than `INPUT`.

- `RANDOM`: A randomized ordering of variables.

The _.blif_ file(s) is given with the `-f` parameter (_F1_ and _F2_ Make
variables) and the variable order with `-o` (_O_ for Make). You can find
multiple inputs in the _benchmarks/_ folder.

```bash
make verification/picotrav F1=benchmarks/not_a.blif F2=benchmarks/not_b.blif V=cudd O=LEVEL_DFS
```

Note, that the memory you set is only for the BDD package. So, the program will
either use swap or be killed if the BDD package takes up more memory in
conjunction with auxiliary data structures.

## License
The software files in this repository are provided under the
[MIT License](/LICENSE.md).


## References

- [[Kunkle10](https://dl.acm.org/doi/abs/10.1145/1837210.1837222)] Daniel
  Kunkle, Vlad Slavici, Gene Cooperman. “_Parallel Disk-Based Computation for
  Large, Monolithic Binary Decision Diagrams_”. In: _PASCO '10: Proceedings of
  the 4th International Workshop on Parallel and Symbolic Computation_. 2010

- [[Pan04](https://link.springer.com/chapter/10.1007/11527695_19)] Guoqiang
  Pan and Moshe Y. Vardi. “_Search vs. Symbolic Techniques in Satisfiability
  Solving_”. In: _SAT 2004: Theory and Applications of Satisfiability Testing_.
  2004

- [[Tveretina10](https://dl.acm.org/doi/abs/10.1145/1837210.1837222)] Olga
  Tveretina, Carsten Sinz and Hans Zantema. “_Ordered Binary Decision Diagrams,
  Pigeonhole Formulas and Beyond_”. In: _Journal on Satisfiability, Boolean
  Modeling and Computation 1_. 2010
