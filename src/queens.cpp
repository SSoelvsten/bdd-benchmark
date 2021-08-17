#include "common.cpp"
#include "expected.h"

size_t largest_bdd = 0;
size_t total_nodes = 0;

// =============================================================================
inline int label_of_position(int i, int j)
{
  assert(i >= 0 && j >= 0);

  return (N * i) + j;
}

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<typename mgr_t>
typename mgr_t::bdd_t queens_S(mgr_t &mgr, int i, int j)
{
  int row = N - 1;

  typename mgr_t::bdd_t out = mgr.leaf_true();

  do {
    int row_diff = std::max(row, i) - std::min(row, i);

    if (row_diff == 0) {
      int column = N - 1;

      do {
        int label = label_of_position(row, column);

        if (column == j) {
          out &= mgr.ithvar(label);
        } else {
          out &= mgr.nithvar(label);
        }
      } while (column-- > 0);
    } else {
      if (j + row_diff < N) {
        int label = label_of_position(row, j + row_diff);
        out &= mgr.nithvar(label);
      }

      int label = label_of_position(row, j);
      out &= mgr.nithvar(label);

      if (row_diff <= j) {
        int label = label_of_position(row, j - row_diff);
        out &= mgr.nithvar(label);
      }
    }
  } while (row-- > 0);

  total_nodes += mgr.nodecount(out);

  return out;
}

// ========================================================================== //
//                              ROW CONSTRUCTION                              //
template<typename mgr_t>
typename mgr_t::bdd_t queens_R(mgr_t &mgr, int row)
{
  typename mgr_t::bdd_t out = queens_S(mgr, row, 0);

  for (int j = 1; j < N; j++) {
    out |= queens_S(mgr, row, j);

    const size_t nodecount = mgr.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
  }
  return out;
}

// ========================================================================== //
//                              ROW ACCUMULATION                              //
template<typename mgr_t>
typename mgr_t::bdd_t queens_B(mgr_t &mgr)
{
  if (N == 1) {
    return queens_S(mgr, 0, 0);
  }

  typename mgr_t::bdd_t out = queens_R(mgr, 0);

  for (int i = 1; i < N; i++) {
    out &= queens_R(mgr, i);

    const size_t nodecount = mgr.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
  }
  return out;
}

// ========================================================================== //
template<typename mgr_t>
void run_queens(int argc, char** argv)
{
  no_variable_order variable_order = no_variable_order::NO_ORDERING;
  N = 8; // Default N value
  bool should_exit = parse_input(argc, argv, variable_order);
  if (should_exit) { exit(-1); }

  // =========================================================================
  std::cout << N << "-Queens"
            << " (" << mgr_t::NAME << " " << M << " MiB):" << std::endl;

  // ========================================================================
  // Initialise package manager
  time_point t_init_before = get_timestamp();
  mgr_t mgr(N*N);
  time_point t_init_after = get_timestamp();
  INFO(" | package init (ms):      %zu\n", duration_of(t_init_before, t_init_after));

  uint64_t solutions;
  {
    // ========================================================================
    // Compute the bdd that represents the entire board
    time_point t1 = get_timestamp();
    typename mgr_t::bdd_t res = queens_B(mgr);
    time_point t2 = get_timestamp();

    INFO(" | construction:\n");
    INFO(" | | total no. nodes:      %zu\n", total_nodes);
    INFO(" | | largest size (nodes): %zu\n", largest_bdd);
    INFO(" | | final size (nodes):   %zu\n", mgr.nodecount(res));
    INFO(" | | time (ms):            %zu\n", duration_of(t1,t2));

    // ========================================================================
    // Count number of solutions
    time_point t3 = get_timestamp();
    solutions = mgr.satcount(res);
    time_point t4 = get_timestamp();

    // ========================================================================
    INFO(" | counting solutions:\n");
    INFO(" | | counting:             %zu\n", duration_of(t3,t4));
    INFO(" | | number of solutions:  %zu\n", solutions);

    INFO(" | total time (ms):        %zu\n", duration_of(t1,t4));
  }

  if (N < size(expected_queens) && solutions != expected_queens[N]) {
    exit(-1);
  }
}

