#include <functional>
#include <vector>

#include "common.cpp"
#include "expected.h"

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

int N = 20;

// =============================================================================
// Label index
inline int label_of_position(int i, int j, int k)
{
  assert(i >= 0 && j >= 0 && k >= 0);

  return (4 * 4 * i) + (4 * j) + k;
}


// =============================================================================
// Constraint lines
std::vector<std::array<int,4>> lines { };

void construct_lines() {
  // 4 planes and the rows in these
  for (int i = 0; i < 4; i++) { // (dist: 4)
    for (int j = 0; j < 4; j++) {
      lines.push_back({ label_of_position(i,j,0), label_of_position(i,j,1), label_of_position(i,j,2), label_of_position(i,j,3) });
    }
  }
  // 4 planes and a diagonal within
  for (int i = 0; i < 4; i++) { // (dist: 10)
    lines.push_back({ label_of_position(i,0,3), label_of_position(i,1,2), label_of_position(i,2,1), label_of_position(i,3,0) });
  }
  // 4 planes... again, now the columns
  for (int i = 0; i < 4; i++) { // (dist: 13)
    for (int k = 0; k < 4; k++) {
      lines.push_back({ label_of_position(i,0,k), label_of_position(i,1,k), label_of_position(i,2,k), label_of_position(i,3,k) });
    }
  }
  // 4 planes and the other diagonal within
  for (int i = 0; i < 4; i++) { // (dist: 16)
    lines.push_back({ label_of_position(i,0,0), label_of_position(i,1,1), label_of_position(i,2,2), label_of_position(i,3,3) });
  }

  // Diagonal of the entire cube (dist: 22)
  lines.push_back({ label_of_position(0,3,3), label_of_position(1,2,2), label_of_position(2,1,1), label_of_position(3,0,0) });

  // Diagonal of the entire cube (dist: 40)
  lines.push_back({ label_of_position(0,3,0), label_of_position(1,2,1), label_of_position(2,1,2), label_of_position(3,0,3) });

  // Diagonals in the vertical planes
  for (int j = 0; j < 4; j++) { // (dist: 46)
    lines.push_back({ label_of_position(0,j,3), label_of_position(1,j,2), label_of_position(2,j,1), label_of_position(3,j,0) });
  }

  // 16 vertical lines (dist: 48)
  for (int j = 0; j < 4; j++) {
    for (int k = 0; k < 4; k++) {
      lines.push_back({ label_of_position(0,j,k), label_of_position(1,j,k), label_of_position(2,j,k), label_of_position(3,j,k) });
    }
  }

  // Diagonals in the vertical planes
  for (int j = 0; j < 4; j++) { // (dist: 49)
    lines.push_back({ label_of_position(0,j,0), label_of_position(1,j,1), label_of_position(2,j,2), label_of_position(3,j,3) });
  }

  for (int k = 0; k < 4; k++) { // (dist: 36)
    lines.push_back({ label_of_position(0,3,k), label_of_position(1,2,k), label_of_position(2,1,k), label_of_position(3,0,k) });
  }
  for (int k = 0; k < 4; k++) { // (dist: 60)
    lines.push_back({ label_of_position(0,0,k), label_of_position(1,1,k), label_of_position(2,2,k), label_of_position(3,3,k) });
  }

  // The 4 diagonals of the entire cube (dist: 61)
  lines.push_back({ label_of_position(0,0,3), label_of_position(1,1,2), label_of_position(2,2,1), label_of_position(3,3,0) });

  // The 4 diagonals of the entire cube (dist: 64)
  lines.push_back({ label_of_position(0,0,0), label_of_position(1,1,1), label_of_position(2,2,2), label_of_position(3,3,3) });
}

// ========================================================================== //
//                           EXACTLY N CONSTRAINT                             //
template<typename adapter_t>
typename adapter_t::dd_t construct_init(adapter_t &adapter);

// ========================================================================== //
//                              LINE CONSTRAINT                               //
template<typename adapter_t>
typename adapter_t::dd_t construct_is_not_winning(adapter_t &adapter,
                                                  std::array<int, 4>& line);

// =============================================================================
template<typename adapter_t>
int run_tictactoe(int argc, char** argv)
{
  no_options option = no_options::NONE;
  bool should_exit = parse_input(argc, argv, option);
  N = input_sizes.empty() ? 20 : input_sizes.at(0);

  if (should_exit) { return -1; }

  // =========================================================================
  std::cout << "Tic-Tac-Toe with " << N << " crosses (" << adapter_t::NAME << " " << M << " MiB):\n";

  time_point t_init_before = get_timestamp();
  adapter_t adapter(64);
  time_point t_init_after = get_timestamp();
  std::cout << "\n"
            << "   " << adapter_t::NAME << " initialisation:\n"
            << "   | time (ms):              " << duration_of(t_init_before, t_init_after) << "\n";

  construct_lines();

  uint64_t solutions;
  {
    // =========================================================================
    // Construct is_equal_N
    std::cout << "\n"
              << "   Initial decision diagram:\n"
              << std::flush;

    time_point t1 = get_timestamp();
    typename adapter_t::dd_t res = construct_init(adapter);
    time_point t2 = get_timestamp();

    const size_t initial_bdd = adapter.nodecount(res);
    const time_duration init_time = duration_of(t1,t2);

#ifdef BDD_BENCHMARK_STATS
    total_nodes += initial_bdd;
#endif // BDD_BENCHMARK_STATS
    std::cout << "   | size (nodes):           " << initial_bdd << "\n"
              << "   | time (ms):              " << init_time << "\n"
              << std::flush;

    // =========================================================================
    // Add constraints lines
    std::cout << "\n"
              << "   Applying constraints:\n"
              << std::flush;

    time_point t3 = get_timestamp();

    for (auto &line : lines) {
      res &= construct_is_not_winning(adapter, line);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(res);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << "   | [" << line[0] << "," << line[1] << "," << line[2] << "] "
                << std::string((line[0] < 10) + (line[1] < 10) + (line[2] < 10), ' ') << ": "
                << nodecount << " DD nodes\n";
#endif // BDD_BENCHMARK_STATS
    }

    time_point t4 = get_timestamp();

    const time_duration constraints_time = duration_of(t3,t4);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n"
              << "   | total no. nodes:        " << total_nodes << "\n"
              << "   | largest size (nodes):   " << largest_bdd << "\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "   | final size (nodes):     " << adapter.nodecount(res) << "\n"
              << "   | time (ms):              " << constraints_time << "\n"
              << std::flush;

    // =========================================================================
    // Count number of solutions
    std::cout << "\n"
              << "   counting solutions:\n"
              << std::flush;

    time_point t5 = get_timestamp();
    solutions = adapter.satcount(res);
    time_point t6 = get_timestamp();

    const time_duration counting_time = duration_of(t5,t6);

    // =========================================================================
    std::cout << "   | number of solutions:    " << solutions << "\n"
              << "   | time (ms):              " << counting_time << "\n"
              << std::flush;

    // =========================================================================
    std::cout << "\n"
              << "   total time (ms):          " << (init_time + constraints_time + counting_time) << "\n"
              << std::flush;
  }

  adapter.print_stats();

  if (N < size(expected_tic_tac_toe) && solutions != expected_tic_tac_toe[N]) {
    return -1;
  }
  return 0;
}
