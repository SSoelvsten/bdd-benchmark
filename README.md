# BDD Benchmark
This repository contains multiple examples for use of BDDs to solve various
problems. These are all implemented in exactly the same way, thereby allowing
one to compare implementations.

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [BDD Benchmark](#bdd-benchmark)
    - [Implementations](#implementations)
        - [Enforcing comparability](#enforcing-comparability)
    - [Installation](#installation)
        - [Adiar](#adiar)
        - [Sylvan](#sylvan)
    - [Benchmarks](#benchmarks)
        - [N-Queens](#n-queens)
        - [Tic-Tac-Toe](#tic-tac-toe)
        - [SAT Solver](#sat-solver)
            - [N-Queens](#n-queens-1)
            - [Pigeonhole Principle](#pigeonhole-principle)
    - [License](#license)
    - [References](#references)

<!-- markdown-toc end -->

## Implementations
We provide all the benchmarks described further below for the following
libraries.


- [**Adiar**](https://github.com/ssoelvsten/adiar):
  An I/O Efficient implementation with iterative algorithms using priority
  queues to exploit a special sorting of nodes on disk. These algorithms have no
  need for memoization or garbage collection, but, on the other hand, nodes are
  also not shareable between BDDs.


- [**BuDDy**](http://vlsicad.eecs.umich.edu/BK/Slots/cache/www.itu.dk/research/buddy/):
  An easy-to-use yet extensive implementation with depth-first algorithms using
  a unique node table to share nodes. It uses a memoization table, automated
  garbage collection, and dynamic variable reordering.

  We use the version from [here](https://github.com/jgcoded/BuDDy) that is set
  up for building with CMake.


- [**Sylvan**](https://github.com/trolando/sylvan):
  A parallel (multi-core) implementation with depth-first algorithms using a
  unique node table to share nodes. It also provides a memoization table,
  complement edges, automated garbage collection, and much more.
  
  We will _not_ make use of the multi-core aspect to make the results
  comparable.

We hope to extend the number of packages. See
[issue #12](https://github.com/SSoelvsten/bdd-benchmark/issues/12) for a list
of BDD packages we would like to have added to this set of benchmarks. Any
help to do so is very much appreciated.

### Enforcing comparability
For comparability, we will enforce all packages to follow the same settings.

- Only use a single core

- Packages with a unique node table and a memoization table will have a ratio
  between the two of 16:1.


## Installation
All packages interface with CMake, which makes installation very simple after
having initialised all submodules using the following command.

```
git submodule update --init --recursive
```

This also requires _CMake_ and a _C++_ compiler of your choice.

### Adiar
Adiar also has dependencies on the _Boost Library_, which can be installed as follows
```
apt install libboost-all-dev
```

### Sylvan
Sylvan also needs the _The GNU Multiple Precision Arithmetic_ and _Portable
Hardware Locality_ libraries, which can be installed as follows
```
apt install libgmp-dev libhwloc-dev
```

## Benchmarks

### N-Queens
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


### Tic-Tac-Toe
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


### SAT Solver
We have a few benchmarks using a simple SAT Solver based on the description in
[[Pan04; Section 2](#references)]. 

The SAT solver works in two modes:

- **Counting**:
  Accumulates all clauses together using _apply_ with the _and_ operator and
  then counts the number of solutions

- **Satisfiability**:
  Also accumulates the clauses, but the accumulation is intermixed with
  existential quantification variables, that are not present in any yet-to-be
  accumulated clauses.


#### N-Queens
Computes for the same problem as for [N-Queens](#n-queens) above, but does so
based on a CNF of _at-least-one_ (alo) and _at-most-one_ (amo) constraints.
The number of clauses is some order of O(N³) which also is reflected in the
much worse performance compared to BDD oriented solution above.


#### Pigeonhole Principle
Computes that the following is false

> Given N, does there exist an isomorphism between the sets { 1, 2, ..., N + 1}
> (pigeons) and { 1, 2, ..., N } (holes)?

Based on [[Tveretina10](#references)], this is disproven by use of variables
_P<sub>i,j</sub>_ saying that the _i_'th pigeon is mapped to the _j_'th hole.
This makes for N(N+1) labels and N³ + N(N+1) clauses.


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
