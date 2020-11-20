#include <coom/coom.h>

#include "common.cpp"
#include "pigeonhole_principle.cpp"

using namespace coom;

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  size_t largest_bdd = 0;

  // =========================================================================
  INFO("Pigeonhole Principle for %zu : %zu (COOM %zu MB):\n", N+1, N, M);
  coom_init(M);

  // =========================================================================
  bool satisfiable = true;
  {
    bdd sat_acc = bdd_true();

    const auto sat_and_clause = [&](clause_t &clause) -> void
      {
        node_file clause_bdd;

        { // All bdd functions require that no writer is attached to a file. So, we
          // garbage collect the writer before the bdd_apply call.
          node_writer clause_writer(clause_bdd);

          ptr_t next = create_sink_ptr(false);

          for (auto it = clause.rbegin(); it != clause.rend(); it++) {
            literal_t v = *it;

            node n = create_node(v.first, 0,
                                 v.second ? create_sink_ptr(true) : next,
                                 v.second ? next : create_sink_ptr(true));

            next = n.uid;

            clause_writer << n;
          }
        }

        sat_acc &= clause_bdd;

        largest_bdd = std::max(largest_bdd, bdd_nodecount(sat_acc));
      };

    const auto sat_quantify_variable = [&](uint64_t var) -> void
      {
        sat_acc = bdd_exists(sat_acc, var);
        largest_bdd = std::max(largest_bdd, bdd_nodecount(sat_acc));
      };

    const auto sat_is_false = [&]() -> bool
      {
        return is_sink(sat_acc, is_false);
      };

    // =========================================================================
    auto t1 = get_timestamp();

    sat_solver solver;
    construct_PHP_cnf(solver, N);

    auto t2 = get_timestamp();

    INFO(" | CNF:\n");
    INFO(" | | variables:         %zu\n", label_of_Pij(N+1, N, N));
    INFO(" | | clauses:           %zu\n", solver.cnf_size());
    INFO(" | | time (ms):         %zu\n", duration_of(t1,t2));

    // =========================================================================
    INFO(" | BDD Solving:\n");

    auto t3 = get_timestamp();
    satisfiable = solver.is_satisfiable(sat_and_clause,
                                        sat_quantify_variable,
                                        sat_is_false);
    auto t4 = get_timestamp();

    INFO(" | | largest size:      %i\n", largest_bdd);
    INFO(" | | final size:        %i\n", bdd_nodecount(sat_acc));
    INFO(" | | time (ms):         %zu\n", duration_of(t3,t4));

    // =========================================================================
    INFO(" | solution:            %s\n", satisfiable ? "SATISFIABLE" : "UNSATISFIABLE");
  }

  coom_deinit();

  exit(satisfiable ? -1 : 0);
}
