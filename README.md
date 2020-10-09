# OBDD Benchmark
This repository contains multiple examples for use of OBDDs to solve various
problems. These are all implemented in exactly the same way, thereby allowing
one to compare implementations.

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [OBDD Benchmark](#obdd-benchmark)
    - [Implementations](#implementations)
    - [Installation](#installation)
        - [Sylvan](#sylvan)
    - [Benchmarks](#benchmarks)
        - [N-Queens](#n-queens)
        - [Tic-Tac-Toe](#tic-tac-toe)
        - [Pigeonhole Principle](#pigeonhole-principle)
    - [License](#license)
    - [References](#references)

<!-- markdown-toc end -->

## Implementations
We aim to provide all the benchmarks described below for the following
libraries. For comparability, we will set them up to only utilise a single core.

- **CUDD**
  A popular implementation using a _unique node table_. For speedup it uses
  a memoization table, and it also provides (automatic) dynamic variable
  reordering.

  We use the version from [here](https://github.com/mballance/cudd) that is set
  up for building with CMake.


- [**BuDDy**](http://vlsicad.eecs.umich.edu/BK/Slots/cache/www.itu.dk/research/buddy/):
  An easy-to-use yet extensive implementation build on a _unique node table_. It
  uses a memoization table for speedup, and it provides automated garbage
  collection and dynamic variable reordering.

  We use the version from [here](https://github.com/jgcoded/BuDDy) that is set
  up for building with CMake.


- [**Sylvan**](https://github.com/trolando/sylvan):
  A parallel (multi-core) implementation using a _unique node table_, complement
  edges, automated garbage collection, and much more. We will *not* make use of
  the multi-core aspect to make sure the results are comparable.


## Installation
All packages interface with CMake, which makes installation very simple after
having initialised all submodules using the following command.

```
git submodule update --init --recursive
```

This also requires _CMake_ and a _C++_ compiler of your choice.

### Sylvan
Sylvan also needs to following other dependencies, which we merely will write up
as the ubuntu apt command.
```
apt install libgmp-dev
```

Furthermore, Sylvan cannot simply be built by CMake, but one has to first
_install_ it as follows (see [issue #10](https://github.com/trolando/sylvan/issues/10)
on the Sylvan project)
```
mkdir external/sylvan/build && cd external/sylvan/build
cmake ..
sudo make
sudo make install
```

## Benchmarks

### N-Queens
Solves the following problem:

> Given N, then in how many ways can N queens be placed on an N x N chess board
> without threatening eachother?

Our implementation of these benchmarks are based on the description of
[[Kunkle10](#references)]. We construct an OBDD row-by-row that represents
whether the row is in a legal state: is at least one queen placed on each row
and is it also in no conflicts with any other? On the accumulated OBDD we then
counts the number of satisfying assignments.

**Notice**: This is a pretty simple example and has all of the normal
shortcomings for OBDDs trying to solve the N-Queens problem. At around N = 14
the intermediate sizes explodes a lot. One can with about 100 GB of disk space
or memory available to compute the 15-Queens problem. Presumably, one needs
around 1 TB for the 16-Queens problem.

**Statistics:**

| Variable         | Value  |
|------------------|--------|
| Labels           | N²     |
| Apply operations | N² + N |


### Tic-Tac-Toe
Solves the following problem:

> Given N, then in how many ways can Player 1 place N crosses in a 3D 4x4x4 cube
> and have a tie, when Player 2 places noughts on all remaining

This benchmark stems from [[Kunkle10](#references)]. Here we keep an accumulated
OBDD on which we add one of the 76 constraints of at least one cross and one
nought after the other. We add these constraints in a different order than
[[Kunkle10](#references)], which does result in an up to 100 times smaller largest
intermediate result.

**Statistics:**

| Variable          |           Value |
|-------------------|-----------------|
| Labels            |              64 |
| Apply operations  |              76 |
| Initial OBDD size | (64-N+1)(N+1)-1 |


### Pigeonhole Principle
Proves the following is not true

> Given N, does there exist an isomorphism between the sets { 1, 2, ..., N + 1}
> (pigeons) and { 1, 2, ..., N } (holes)?

Based on [[Tveretina10](#references)], this is disproven by use of variables
_P<sub>i,j</sub>_ saying that pigeon _i_ is mapped to the _j_'th hole. These are
rewritten into a CNF formula, which is then solved in a simple way as described
in [[Pan04](#references)]: sort all clauses by the maximal occuring literal into
clusters and then interchangably accumulate clauses and existentially quantify
variables.

**Statistics:**

| Variable | Value       |
|----------|-------------|
| Labels   | N(N+1)      |
| Clauses  | N³ + N(N+1) |

Which makes for _N(N+1)_ existential quantifications and _N³ + N(N+1)_ apply
operations.


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
