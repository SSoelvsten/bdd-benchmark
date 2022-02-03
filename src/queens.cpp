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
template<typename adapter_t>
typename adapter_t::bdd_t queens_S(adapter_t &adapter, int i, int j)
{
  int row = N - 1;

  typename adapter_t::bdd_t out = adapter.leaf_true();

  do {
    int row_diff = std::max(row, i) - std::min(row, i);

    if (row_diff == 0) {
      int column = N - 1;

      do {
        int label = label_of_position(row, column);

        if (column == j) {
          out &= adapter.ithvar(label);
        } else {
          out &= adapter.nithvar(label);
        }
      } while (column-- > 0);
    } else {
      if (j + row_diff < N) {
        int label = label_of_position(row, j + row_diff);
        out &= adapter.nithvar(label);
      }

      int label = label_of_position(row, j);
      out &= adapter.nithvar(label);

      if (row_diff <= j) {
        int label = label_of_position(row, j - row_diff);
        out &= adapter.nithvar(label);
      }
    }
  } while (row-- > 0);

  total_nodes += adapter.nodecount(out);

  return out;
}

// ========================================================================== //
//                              ROW CONSTRUCTION                              //
template<typename adapter_t>
typename adapter_t::bdd_t queens_R(adapter_t &adapter, int row)
{
  typename adapter_t::bdd_t out = queens_S(adapter, row, 0);

  for (int j = 1; j < N; j++) {
    out |= queens_S(adapter, row, j);

    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
  }
  return out;
}

// ========================================================================== //
//                              ROW ACCUMULATION                              //
template<typename adapter_t>
typename adapter_t::bdd_t queens_B(adapter_t &adapter)
{
  if (N == 1) {
    return queens_S(adapter, 0, 0);
  }

  typename adapter_t::bdd_t out = queens_R(adapter, 0);

  for (int i = 1; i < N; i++) {
    out &= queens_R(adapter, i);

    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
  }
  return out;
}

// ========================================================================== //
template<typename adapter_t>
void run_queens(int argc, char** argv)
{
  no_variable_order variable_order = no_variable_order::NO_ORDERING;
  N = 8; // Default N value
  bool should_exit = parse_input(argc, argv, variable_order);
  if (should_exit) { exit(-1); }

  // =========================================================================
  INFO("%i-Queens (%s %i MiB):\n", N, adapter_t::NAME.c_str(), M);

  // ========================================================================
  // Initialise package manager
  time_point t_init_before = get_timestamp();
  adapter_t adapter(N*N);
  time_point t_init_after = get_timestamp();
  INFO("\n   %s initialisation:\n", adapter_t::NAME.c_str());
  INFO("   | time (ms):              %zu\n", duration_of(t_init_before, t_init_after));

  uint64_t solutions;
  {
    // ========================================================================
    // Compute the bdd that represents the entire board
    time_point t1 = get_timestamp();
    typename adapter_t::bdd_t res = queens_B(adapter);
    time_point t2 = get_timestamp();

    const auto construction_time = duration_of(t1,t2);

    INFO("\n   BDD construction:\n");
    INFO("   | total no. nodes:        %zu\n", total_nodes);
    INFO("   | largest size (nodes):   %zu\n", largest_bdd);
    INFO("   | final size (nodes):     %zu\n", adapter.nodecount(res));
    INFO("   | time (ms):              %zu\n", construction_time);

    // ========================================================================
    // Count number of solutions
    time_point t3 = get_timestamp();
    solutions = adapter.satcount(res);
    time_point t4 = get_timestamp();

    const auto counting_time = duration_of(t3,t4);

    INFO("\n   Counting solutions:\n");
    INFO("   | number of solutions:    %zu\n", solutions);
    INFO("   | time (ms):              %zu\n", counting_time);

    // ========================================================================
    INFO("\n   total time (ms):          %zu\n", construction_time + counting_time);
  }

  adapter.print_stats();

  if (N < size(expected_queens) && solutions != expected_queens[N]) {
    EXIT(-1);
  }
  FLUSH();
}

