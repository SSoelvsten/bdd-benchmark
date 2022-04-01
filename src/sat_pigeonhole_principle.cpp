#include "common.cpp"
#include "sat_solver.h"

#include <stdint.h>
#include <utility>

// ========================================================================= //
uint64_t label_of_Pij(int i, int j)
{
  assert(0 <= i && i <= N+1);
  assert(0 <= j && j <= N);
  return (N+1)*i + j;
}

// ========================================================================= //
// CNF for the Pigeonhole Principle based on the paper by Olga Tveretina,
// Carsten Sinz, and Hans Zantema "Ordered Binary Decision Diagrams, Pigeonhole
// Formulas and Beyond".
template<typename adapter_t>
void construct_PHP_cnf(sat_solver<adapter_t> &solver)
{
  // PC_n
  for (int i = 1; i <= N+1; i++)
  {
    clause_t clause;
    for (int j = 1; j <= N; j++)
    {
      clause.push_back(literal_t (label_of_Pij(i,j), false));
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
        clause.push_back(literal_t (label_of_Pij(i,k), true));
        clause.push_back(literal_t (label_of_Pij(j,k), true));
        solver.add_clause(clause);
      }
    }
  }
}

// =============================================================================
template<typename adapter_t>
void run_sat_pigeonhole_principle(int argc, char** argv)
{
  no_variable_order variable_order = no_variable_order::NO_ORDERING;
  N = 8;
  bool should_exit = parse_input(argc, argv, variable_order);
  if (should_exit) { exit(-1); }

  bool satisfiable = true;
  {
    // =========================================================================
    INFO("Pigeonhole Principle for %i : %i (%s %i MiB):\n", N+1, N, adapter_t::NAME.c_str(), M);

    uint64_t max_var = label_of_Pij(N+1, N);

    auto t_init_before = get_timestamp();
    sat_solver<adapter_t> solver(max_var+1);
    auto t_init_after = get_timestamp();
    INFO("\n   %s initialisation:\n", adapter_t::NAME.c_str());
    INFO("   | time (ms):                %zu\n", duration_of(t_init_before, t_init_after));

    // =========================================================================
    INFO("\n   CNF construction:\n");

    auto t1 = get_timestamp();
    construct_PHP_cnf(solver);
    auto t2 = get_timestamp();

    INFO("   | variables:                %zu\n", solver.var_count());
    INFO("   | clauses:                  %zu\n", solver.cnf_size());
    INFO("   | time (ms):                %zu\n", duration_of(t1,t2));

    // =========================================================================
    INFO("\n   Decision diagram satisfiability solving:\n");

    auto t3 = get_timestamp();
    satisfiable = solver.check_satisfiable();
    auto t4 = get_timestamp();

    INFO("   | operations:\n");
    INFO("   | | exists:                 %zu\n", solver.exists_count());
    INFO("   | | apply:                  %zu\n", solver.apply_count());
    INFO("   | DD size (nodes):\n");
    INFO("   | | largest:                %zu\n", solver.bdd_largest_size());
    INFO("   | | final:                  %zu\n", solver.bdd_size());
    INFO("   | time (ms):                %zu\n", duration_of(t3,t4));
  }

  if (satisfiable) { EXIT(-1); }
  FLUSH();
}
