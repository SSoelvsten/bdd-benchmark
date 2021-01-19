#include "common.cpp"
#include "queens.cpp"

#include "sylvan_init.cpp"

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  // =========================================================================
  INFO("%zu-Queens SAT (BuDDy %zu MB):\n", N, M);
  auto t_init_before = get_timestamp();
  SYLVAN_INIT(M);
  auto t_init_after = get_timestamp();
  INFO(" | init time (ms):        %zu\n", duration_of(t_init_before, t_init_after));

  // =========================================================================
  uint64_t varcount = label_of_position(N, N-1, N-1)+1;
  SYLVAN_SOLVER_INIT(solver, varcount);

  auto t1 = get_timestamp();
  construct_Queens_cnf(solver, N);
  auto t2 = get_timestamp();

  INFO(" | CNF:\n");
  INFO(" | | clauses:             %zu\n", solver.cnf_size());
  INFO(" | | variables:           %zu\n", solver.var_count());
  INFO(" | | time (ms):           %zu\n", duration_of(t1,t2));
  INFO(" |\n");

  // =========================================================================
  auto t3 = get_timestamp();
  bool satisfiable = solver.check_satisfiable();
  auto t4 = get_timestamp();
  INFO(" | Satisfiability:\n");
  INFO(" | | solution:            %s\n", satisfiable ? "SATISFIABLE" : "UNSATISFIABLE");
  INFO(" | statistics:\n");
  INFO(" | | operations:\n");
  INFO(" | | | exists:            %zu\n", solver.exists_count());
  INFO(" | | | apply:             %zu\n", solver.apply_count());
  INFO(" | | BDD size (nodes):\n");
  INFO(" | | | largest size:      %zu\n", solver.bdd_largest_size());
  INFO(" | | | final size:        %zu\n", solver.bdd_size());
  INFO(" | | time (ms):           %zu\n", duration_of(t3,t4));
  INFO(" |\n");

  // =========================================================================
  auto t5 = get_timestamp();
  uint64_t solutions = solver.check_satcount();
  auto t6 = get_timestamp();
  INFO(" | Counting:\n");
  INFO(" | | solutions:           %zu\n", solutions);
  INFO(" | statistics:\n");
  INFO(" | | operations:\n");
  INFO(" | | | apply:             %zu\n", solver.apply_count());
  INFO(" | | BDD size (nodes):\n");
  INFO(" | | | largest size:      %zu\n", solver.bdd_largest_size());
  INFO(" | | | final size:        %zu\n", solver.bdd_size());
  INFO(" | | time (ms):           %zu\n", duration_of(t5,t6));

  // =========================================================================
  SYLVAN_DEINIT;

  if (solutions != expected_result[N] && satisfiable) {
    exit(-1);
  }
}
