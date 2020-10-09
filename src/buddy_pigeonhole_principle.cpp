#include "buddy_init.cpp"

#include "common.cpp"
#include "pigeonhole_principle.cpp"

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  int largest_bdd = 0;

  // =========================================================================
  BUDDY_INIT(label_of_Pij(N+1, N, N)+1, M)

  // =========================================================================
  bdd sat_acc = bddtrue;

  const auto sat_and_clause = [&](clause_t &clause) -> void
  {
    bdd c = bddfalse;

    for (auto it = clause.rbegin(); it != clause.rend(); it++)
    {
      bdd v = (*it).second ? bdd_nithvar((*it).first) : bdd_ithvar((*it).first);
      c = bdd_ite(v, bddtrue, c);
    }
    sat_acc &= c;

    largest_bdd = std::max(largest_bdd, bdd_nodecount(c));
    largest_bdd = std::max(largest_bdd, bdd_nodecount(sat_acc));
  };

  const auto sat_quantify_variable = [&](uint64_t var) -> void
  {
    sat_acc = bdd_exist(sat_acc, bdd_ithvar(var));
  };

  const auto sat_is_false = [&]() -> bool
  {
    return sat_acc == bddfalse;
  };

  // =========================================================================
  auto t1 = get_timestamp();

  sat_solver solver;
  construct_PHP_cnf(solver, N);

  auto t2 = get_timestamp();

  // =========================================================================
  auto t3 = get_timestamp();

  bool satisfiable = solver.is_satisfiable(sat_and_clause,
                                           sat_quantify_variable,
                                           sat_is_false);

  auto t4 = get_timestamp();

  // =========================================================================
  INFO("Pigeonhole Principle for %zu : %zu (BuDDy %zu MB):\n", N+1, N, M);
  INFO(" | solution:            %s\n", satisfiable ? "SATISFIABLE" : "UNSATISFIABLE");
  INFO(" | CNF:\n");
  INFO(" | | variables:         %zu\n", label_of_Pij(N+1, N, N));
  INFO(" | | clauses:           %zu\n", solver.cnf_size());
  INFO(" | OBDD size (nodes):\n");
  INFO(" | | largest size:      %i\n", largest_bdd);
  INFO(" | | final size:        %i\n", bdd_nodecount(sat_acc));
  INFO(" | time (ms):\n");
  INFO(" | | CNF construction:  %zu\n", duration_of(t1,t2));
  INFO(" | | OBDD solving:      %zu\n", duration_of(t3,t4));


  // =========================================================================
  BUDDY_DEINIT

  exit(satisfiable ? -1 : 0);
}
