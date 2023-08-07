#include "common.cpp"
#include "expected.h"

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

// =============================================================================
inline int label_of_position(int r, int c)
{
  assert(r >= 0 && c >= 0);
  return (N * r) + c;
}

inline std::string pos_to_string(int r, int c)
{
  std::stringstream ss;
  ss << (r+1) << (char) ('A'+c);
  return ss.str();
}

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<typename adapter_t>
typename adapter_t::dd_t queens_S(adapter_t &mgr, int i, int j);

// ========================================================================== //
//                              ROW CONSTRUCTION                              //
template<typename adapter_t>
typename adapter_t::dd_t queens_R(adapter_t &adapter, int r)
{
  typename adapter_t::dd_t out = queens_S(adapter, r, 0);

  for (int c = 1; c < N; c++) {
    out |= queens_S(adapter, r, c);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    INFO("   | | R(%s) : %zu DD nodes\n", pos_to_string(r,c).c_str(), nodecount);
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

// ========================================================================== //
//                              ROW ACCUMULATION                              //
template<typename adapter_t>
typename adapter_t::dd_t queens_B(adapter_t &adapter)
{
  if (N == 1) {
    return queens_S(adapter, 0, 0);
  }

  typename adapter_t::dd_t out = queens_R(adapter, 0);
  {
#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    INFO("   | B(%i) : %zu DD nodes\n", 0+1, nodecount);
#endif // BDD_BENCHMARK_STATS
  }

  for (int r = 1; r < N; r++) {
    out &= queens_R(adapter, r);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    INFO("   | B(%i) : %zu DD nodes\n", r+1, nodecount);
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

// ========================================================================== //
template<typename adapter_t>
void run_queens(int argc, char** argv)
{
  no_options option = no_options::NONE;
  N = 8; // Default N value
  bool should_exit = parse_input(argc, argv, option);
  if (should_exit) { exit(-1); }

  // =========================================================================
  INFO("%i-Queens (%s %i MiB):\n", N, adapter_t::NAME.c_str(), M);

  // ========================================================================
  // Initialise package manager
  const time_point t_init_before = get_timestamp();
  adapter_t adapter(N*N);
  const time_point t_init_after = get_timestamp();
  INFO("\n   %s initialisation:\n", adapter_t::NAME.c_str());
  INFO("   | time (ms):              %zu\n", duration_of(t_init_before, t_init_after));

  uint64_t solutions;
  {
    // ========================================================================
    // Compute the bdd that represents the entire board
    INFO("\n   Decision diagram construction:\n");
    const time_point t1 = get_timestamp();
    typename adapter_t::dd_t res = queens_B(adapter);
    const time_point t2 = get_timestamp();

    const time_duration construction_time = duration_of(t1,t2);

#ifdef BDD_BENCHMARK_STATS
    INFO("   |\n");
    INFO("   | total no. nodes:        %zu\n", total_nodes);
    INFO("   | largest size (nodes):   %zu\n", largest_bdd);
#endif // BDD_BENCHMARK_STATS
    INFO("   | final size (nodes):     %zu\n", adapter.nodecount(res));
    INFO("   | time (ms):              %zu\n", construction_time);

    // ========================================================================
    // Count number of solutions
    INFO("\n   Counting solutions:\n");
    const time_point t3 = get_timestamp();
    solutions = adapter.satcount(res);
    const time_point t4 = get_timestamp();

    const time_duration counting_time = duration_of(t3,t4);

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

