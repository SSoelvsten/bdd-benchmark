#include "common.cpp"
#include "expected.h"

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

// ========================================================================== //
inline int rows()
{ return input_sizes.at(0); }

inline int MAX_ROW()
{ return rows() - 1; }

inline int cols()
{ return input_sizes.at(1); }

inline int MAX_COL()
{ return cols() - 1; }

// =============================================================================
inline int label_of_position(int r, int c)
{
  assert(r >= 0 && c >= 0);
  return (rows() * r) + c;
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

  for (int c = 1; c < cols(); c++) {
    out |= queens_S(adapter, r, c);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "   | | R(" << pos_to_string(r,c) << ") : " << nodecount << " DD nodes\n";
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

// ========================================================================== //
//                              ROW ACCUMULATION                              //
template<typename adapter_t>
typename adapter_t::dd_t queens_B(adapter_t &adapter)
{
  if (rows() == 1 && cols() == 1) {
    return queens_S(adapter, 0, 0);
  }

  typename adapter_t::dd_t out = queens_R(adapter, 0);
  {
#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "   | B(" << 0+1 << ") : " << nodecount << " DD nodes\n";
#endif // BDD_BENCHMARK_STATS
  }

  for (int r = 1; r < rows(); r++) {
    out &= queens_R(adapter, r);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "   | B(" << r+1 << ") : " << nodecount << " DD nodes\n";
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

// ========================================================================== //
template<typename adapter_t>
int run_queens(int argc, char** argv)
{
  no_options option = no_options::NONE;
  bool should_exit = parse_input(argc, argv, option);

  if (input_sizes.size() == 0) { input_sizes.push_back(8); }
  if (input_sizes.size() == 1) { input_sizes.push_back(input_sizes.at(0)); }

  if (should_exit) { return -1; }

  // =========================================================================
  std::cout << "[" << rows() << " x " << cols() << "]-Queens " << " (" << adapter_t::NAME << " " << M << " MiB):\n";

  // ========================================================================
  // Initialise package manager
  const int N = rows() * cols();

  const time_point t_init_before = get_timestamp();
  adapter_t adapter(N);
  const time_point t_init_after = get_timestamp();
  std::cout << "\n"
            << "   " << adapter_t::NAME << " initialisation:\n"
            << "   | variables:              " << N << "\n"
            << "   | time (ms):              " << duration_of(t_init_before, t_init_after) << "\n";

  uint64_t solutions;
  {
    // ========================================================================
    // Compute the bdd that represents the entire board
    std::cout << "\n"
              << "   Decision diagram construction:\n"
              << std::flush;

    const time_point t1 = get_timestamp();
    typename adapter_t::dd_t res = queens_B(adapter);
    const time_point t2 = get_timestamp();

    const time_duration construction_time = duration_of(t1,t2);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n"
              << "   | total no. nodes:        " << total_nodes << "\n"
              << "   | largest size (nodes):   " << largest_bdd << "\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "   | final size (nodes):     " << adapter.nodecount(res) << "\n"
              << "   | time (ms):              " << construction_time << "\n"
              << std::flush;

    // ========================================================================
    // Count number of solutions
    std::cout << "\n"
              << "   Counting solutions:\n"
              << std::flush;

    const time_point t3 = get_timestamp();
    solutions = adapter.satcount(res);
    const time_point t4 = get_timestamp();

    const time_duration counting_time = duration_of(t3,t4);

    std::cout << "   | number of solutions:    " << solutions << "\n"
              << "   | time (ms):              " << counting_time << "\n"
              << std::flush;

    // ========================================================================
    std::cout << "\n"
              << "   total time (ms):          " << (construction_time + counting_time) << "\n"
              << std::flush;
  }

  adapter.print_stats();

  if (rows() == cols() && N < size(expected_queens) && solutions != expected_queens[N]) {
    return -1;
  }
  return 0;
}
