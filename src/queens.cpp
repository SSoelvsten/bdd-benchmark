// Assertions
#include <cassert>

// Data Structures
#include <sstream>
#include <string>

// Types
#include <cstdlib>

#include "common/adapter.h"
#include "common/array.h"
#include "common/chrono.h"
#include "common/input.h"
#include "common/json.h"

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

// ========================================================================== //
int N = 8;

class parsing_policy
{
public:
  static constexpr std::string_view name = "Queens";
  static constexpr std::string_view args = "N:";

  static constexpr std::string_view help_text =
    "        -N n        [8]      Size of board";

  static inline bool
  parse_input(const int c, const char* arg)
  {
    switch (c) {
    case 'N': {
      N = std::stoi(arg);
      if (N <= 0) {
        std::cerr << "  Must specify positive board size (-N)\n";
        return true;
      }
      return false;
    }
    default:
      return true;
    }
  }
};

// ========================================================================== //
inline int
rows()
{
  return N;
}

inline int
MAX_ROW()
{
  return rows() - 1;
}

inline int
cols()
{
  return N;
}

inline int
MAX_COL()
{
  return cols() - 1;
}

// =============================================================================
inline int
label_of_position(int r, int c)
{
  assert(r >= 0 && c >= 0);
  return (rows() * r) + c;
}

inline std::string
row_to_string(int r)
{
  return std::to_string(r + 1);
}

inline std::string
col_to_string(int c)
{
  return std::string(1, (char)('A' + c));
}

inline std::string
pos_to_string(int r, int c)
{
  return row_to_string(r) + col_to_string(c);
}

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template <typename Adapter>
typename Adapter::dd_t
queens_S(Adapter& adapter, int i, int j)
{
  auto next = adapter.build_node(true);

  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      const int label = label_of_position(row, col);

      // Queen must be placed here
      if (row == i && col == j) {
        auto low  = adapter.build_node(false);
        auto high = next;
        next      = adapter.build_node(label, low, high);

        continue;
      }

      // Conflicting row, column and diagonal with Queen placement
      const int row_diff = std::abs(row - i);
      const int col_diff = std::abs(col - j);

      if ((i == row && j != col) || (i != row && j == col) || (col_diff == row_diff)) {
        auto low  = next;
        auto high = adapter.build_node(false);
        next      = adapter.build_node(label, low, high);

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
template <typename Adapter>
typename Adapter::dd_t
queens_R(Adapter& adapter, int r)
{
  typename Adapter::dd_t out = queens_S(adapter, r, 0);

#ifdef BDD_BENCHMARK_STATS
  std::cout << json::field("R(" + pos_to_string(r, 0) + ")") << json::value(adapter.nodecount(out))
            << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS

  for (int c = 1; c < cols(); c++) {
    out |= queens_S(adapter, r, c);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << json::field("R(" + pos_to_string(r, c) + ")") << json::value(nodecount)
              << json::comma << json::endl
              << json::flush;
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

// ========================================================================== //
//                              ROW ACCUMULATION                              //
template <typename Adapter>
typename Adapter::dd_t
queens_B(Adapter& adapter)
{
  if (rows() == 1 && cols() == 1) { return queens_S(adapter, 0, 0); }

  typename Adapter::dd_t out = queens_R(adapter, 0);
  {
#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << json::field("B(" + row_to_string(0) + ")") << json::value(nodecount) << json::comma
              << json::endl
              << json::endl;
#endif // BDD_BENCHMARK_STATS
  }

  for (int r = 1; r < rows(); r++) {
    out &= queens_R(adapter, r);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << json::field("B(" + row_to_string(r) + ")") << json::value(nodecount);
    if (r != MAX_ROW()) { std::cout << json::comma << json::endl; }
    std::cout << json::endl << json::flush;
#endif // BDD_BENCHMARK_STATS
  }
  return out;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief   Number of solutions for the Queens Problem
///
/// \details Number taken from https://en.wikipedia.org/wiki/Eight_queens_puzzle
////////////////////////////////////////////////////////////////////////////////
const size_t expected[28] = {
  0,                 //  0x0
  1,                 //  1x1
  0,                 //  2x2
  0,                 //  3x3
  2,                 //  4x4
  10,                //  5x5
  4,                 //  6x6
  40,                //  7x7
  92,                //  8x8
  352,               //  9x9
  724,               // 10x10
  2680,              // 11x11
  14200,             // 12x12
  73712,             // 13x13
  365596,            // 14x14
  2279184,           // 15x15
  14772512,          // 16x16
  95815104,          // 17x17
  666090624,         // 18x18
  4968057848,        // 19x19
  39029188884,       // 20x20
  314666222712,      // 21x21
  2691008701644,     // 22x22
  24233937684440,    // 23x23
  227514171973736,   // 24x24
  2207893435808352,  // 25x25
  22317699616364044, // 26x26
  234907967154122528 // 27x27
};

// ========================================================================== //
template <typename Adapter>
int
run_queens(int argc, char** argv)
{
  bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  // =========================================================================
  // Initialise package manager
  return run<Adapter>("queens", N*N, [&](Adapter& adapter) {
    uint64_t solutions;

    std::cout << json::field("N") << json::value(N) << json::comma << json::endl;
    std::cout << json::endl << json::flush;

    // ========================================================================
    // Compute the bdd that represents the entire board
    std::cout << json::field("apply") << json::brace_open << json::endl << json::flush;

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("intermediate results") << json::brace_open << json::endl;
#endif

    const time_point t1        = now();
    typename Adapter::dd_t res = queens_B(adapter);
    const time_point t2        = now();

    const time_duration construction_time = duration_ms(t1, t2);

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::brace_close << json::comma << json::endl;
    std::cout << json::field("total processed (nodes)") << json::value(total_nodes) << json::comma
              << json::endl;
    std::cout << json::field("largest size (nodes)") << json::value(largest_bdd) << json::comma
              << json::endl;
#endif // BDD_BENCHMARK_STATS
    std::cout << json::field("final size (nodes)") << json::value(adapter.nodecount(res)) << json::comma
              << json::endl;
    std::cout << json::field("time (ms)") << json::value(construction_time) << json::endl;
    std::cout << json::brace_close << json::comma << json::endl << json::flush;

    // ========================================================================
    // Count number of solutions
    std::cout << json::field("satcount") << json::brace_open << json::endl << json::flush;

    const time_point t3 = now();
    solutions           = adapter.satcount(res);
    const time_point t4 = now();

    const time_duration counting_time = duration_ms(t3, t4);

    std::cout << json::field("result") << json::value(solutions) << json::comma << json::endl;
    std::cout << json::field("time (ms)") << json::value(counting_time) << json::endl;
    std::cout << json::brace_close << json::endl << json::flush;

    // ========================================================================
    std::cout << json::field("total time (ms)") << json::value(init_time + construction_time + counting_time)
              << json::endl
              << json::flush;

    if (rows() == cols() && cols() < size(expected) && solutions != expected[cols()]) { return -1; }
    return 0;
  });
}
