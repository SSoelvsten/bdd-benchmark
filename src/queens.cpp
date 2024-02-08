#include <sstream>        // std::istringstream

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
template<typename Adapter>
typename Adapter::dd_t queens_S(Adapter &adapter, int i, int j)
{
  auto next = adapter.build_node(true);

  for(int row = MAX_ROW(); row >= 0; row--) {
    for(int col = MAX_COL(); col >= 0; col--) {
      const int label = label_of_position(row, col);

      // Queen must be placed here
      if(row == i && col == j) {
        auto low = adapter.build_node(false);
        auto high = next;
        next = adapter.build_node(label, low, high);

        continue;
      }

      // Conflicting row, column and diagonal with Queen placement
      const int row_diff = std::abs(row - i);
      const int col_diff = std::abs(col - j);

      if((i == row && j != col) || (i != row && j == col) || (col_diff == row_diff)) {
        auto low = next;
        auto high = adapter.build_node(false);
        next = adapter.build_node(label, low, high);

        continue;
      }

      // No in conflicts
      next = adapter.build_node(label, next, next);
    }
  }

  typename Adapter::dd_t out = adapter.build();
#ifdef BDD_BENCHMARK_STATS
  total_nodes += adapter.nodecount(out);
#endif // BDD_BENCHMARK_STATS
  return out;
}


// ========================================================================== //
//                              ROW CONSTRUCTION                              //
template<typename Adapter>
typename Adapter::dd_t queens_R(Adapter &adapter, int r)
{
  typename Adapter::dd_t out = queens_S(adapter, r, 0);

  for (int c = 1; c < cols(); c++) {
    out |= queens_S(adapter, r, c);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "  | | R(" << pos_to_string(r,c) << ")                   " << nodecount << "\n";
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

// ========================================================================== //
//                              ROW ACCUMULATION                              //
template<typename Adapter>
typename Adapter::dd_t queens_B(Adapter &adapter)
{
  if (rows() == 1 && cols() == 1) {
    return queens_S(adapter, 0, 0);
  }

  typename Adapter::dd_t out = queens_R(adapter, 0);
  {
#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "  | B(" << 0+1 << ")                      " << nodecount << "\n"
              << "  |\n";
#endif // BDD_BENCHMARK_STATS
  }

  for (int r = 1; r < rows(); r++) {
    out &= queens_R(adapter, r);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "  | B(" << r+1 << ")                      " << nodecount << "\n"
              << "  |\n";
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

// ========================================================================== //
template<typename Adapter>
int run_queens(int argc, char** argv)
{
  no_options option = no_options::NONE;
  bool should_exit = parse_input(argc, argv, option);

  if (input_sizes.size() == 0) { input_sizes.push_back(8); }
  if (input_sizes.size() == 1) { input_sizes.push_back(input_sizes.at(0)); }

  if (should_exit) { return -1; }

  // =========================================================================
  std::cout << "[" << rows() << " x " << cols() << "]-Queens\n";

  // ========================================================================
  // Initialise package manager
  const int N = rows() * cols();

  return run<Adapter>(N, [&](Adapter &adapter) {
    uint64_t solutions;

    // ========================================================================
    // Compute the bdd that represents the entire board
    std::cout << "\n"
              << "  Decision diagram construction\n"
              << std::flush;

    const time_point t1 = now();
    typename Adapter::dd_t res = queens_B(adapter);
    const time_point t2 = now();

    const time_duration construction_time = duration_ms(t1,t2);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "  | total no. nodes           " << total_nodes << "\n"
              << "  | largest size (nodes)      " << largest_bdd << "\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "  | final size (nodes)        " << adapter.nodecount(res) << "\n"
              << "  | time (ms)                 " << construction_time << "\n"
              << std::flush;

    // ========================================================================
    // Count number of solutions
    std::cout << "\n"
              << "  Counting solutions\n"
              << std::flush;

    const time_point t3 = now();
    solutions = adapter.satcount(res);
    const time_point t4 = now();

    const time_duration counting_time = duration_ms(t3,t4);

    std::cout << "  | number of solutions       " << solutions << "\n"
              << "  | time (ms)                 " << counting_time << "\n"
              << std::flush;

    // ========================================================================
    std::cout << "\n"
              << "  total time (ms)             " << (construction_time + counting_time) << "\n"
              << std::flush;

    if (rows() == cols() && N < size(expected_queens) && solutions != expected_queens[N]) {
      return -1;
    }
    return 0;
  });
}
