#include "common.cpp"
#include "sat_solver.h"

#include <stdint.h>
#include <utility>

// ========================================================================= //
uint64_t label_of_Pij(uint64_t i, uint64_t j, int N)
{
  assert(0 <= i && i <= N+1);
  assert(0 <= j && j <= N);
  return (N+1)*i + j;
}

// ========================================================================= //
// CNF for the Pigeonhole Principle based on the paper by Olga Tveretina,
// Carsten Sinz, and Hans Zantema "Ordered Binary Decision Diagrams, Pigeonhole
// Formulas and Beyond".
template<typename mgr_t>
void construct_PHP_cnf(sat_solver<mgr_t> &solver, int N)
{
  // PC_n
  for (int i = 1; i <= N+1; i++)
  {
    clause_t clause;
    for (int j = 1; j <= N; j++)
    {
      clause.push_back(literal_t (label_of_Pij(i,j,N), false));
    }
    solver.add_clause(clause);
  }

  // NC_n
  for (int i = 1; i < N+1; i++)
  {
    for (int j = i+1; j <= N+1; j++)
    {
      for (int k = 1; k <= N; k++)
      {
        clause_t clause;
        clause.push_back(literal_t (label_of_Pij(i,k,N), true));
        clause.push_back(literal_t (label_of_Pij(j,k,N), true));
        solver.add_clause(clause);
      }
    }
  }
}

// =============================================================================
template<typename mgr_t>
void run_sat_pigeonhole_principle(int argc, char** argv)
{
  N = 8;
  bool should_exit = parse_input(argc, argv);
  if (should_exit) { exit(-1); }

  bool satisfiable = true;
  {
    // =========================================================================
    std::cout << "Pigeonhole Principle for " << N+1 << " : " << N
              << " (" << mgr_t::NAME << " " << M << " MiB):" << std::endl;

    uint64_t max_var = label_of_Pij(N+1, N, N);

    auto t_init_before = get_timestamp();
    sat_solver<mgr_t> solver(max_var+1);
    auto t_init_after = get_timestamp();
    INFO(" | init time (ms):      %zu\n", duration_of(t_init_before, t_init_after));

    // =========================================================================

    auto t1 = get_timestamp();
    construct_PHP_cnf(solver, N);
    auto t2 = get_timestamp();

    auto t3 = get_timestamp();
    satisfiable = solver.check_satisfiable();
    auto t4 = get_timestamp();

    // =========================================================================
    INFO(" | solution:            %s\n", satisfiable ? "SATISFIABLE" : "UNSATISFIABLE");
    INFO(" | CNF:\n");
    INFO(" | | variables:\n");
    INFO(" | | | total:           %zu\n", solver.var_count());
    INFO(" | | | quantified:      %zu\n", solver.exists_count());
    INFO(" | | clauses:\n");
    INFO(" | | | total:           %zu\n", solver.cnf_size());
    INFO(" | | | done:            %zu\n", solver.apply_count());
    INFO(" | BDD size (nodes):\n");
    INFO(" | | largest size:      %zu\n", solver.bdd_largest_size());
    INFO(" | | final size:        %zu\n", solver.bdd_size());
    INFO(" | time (ms):\n");
    INFO(" | | CNF construction:  %zu\n", duration_of(t1,t2));
    INFO(" | | BDD solving:       %zu\n", duration_of(t3,t4));
  }

  if (satisfiable) { exit(-1); }
}
