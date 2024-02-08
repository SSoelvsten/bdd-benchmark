#include <array>          // std::array<>
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
template<typename Adapter>
typename Adapter::dd_t construct_init(Adapter &adapter)
{
  std::vector<typename Adapter::build_node_t> init_parts(N+2, adapter.build_node(false));
  init_parts.at(N) = adapter.build_node(true);

  for (int curr_level = 63; curr_level >= 0; curr_level--) {
    int min_idx = curr_level > 63 - N ? N - (63 - curr_level + 1) : 0;
    int max_idx = std::min(curr_level, N);

    for (int curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
      const auto low = init_parts.at(curr_idx);
      const auto high = init_parts.at(curr_idx + 1);

      init_parts.at(curr_idx) = adapter.build_node(curr_level, low, high);
    }
  }

  typename Adapter::dd_t out = adapter.build();
#ifdef BDD_BENCHMARK_STATS
  total_nodes += adapter.nodecount(out);
#endif // BDD_BENCHMARK_STATS
  return out;
}

// ========================================================================== //
//                              LINE CONSTRAINT                               //
template<typename Adapter>
typename Adapter::dd_t construct_is_not_winning(Adapter &adapter,
                                                  std::array<int, 4>& line)
{
  auto root = adapter.build_node(true);

  // Post "don't care" chain
  for (int curr_level = 63; curr_level > line[3]; curr_level--) {
    root = adapter.build_node(curr_level, root, root);
  }

  // Three chains, checking at least one is set to true and one to false
  int line_idx = 4-1;

  auto safe = root;

  auto only_Xs = adapter.build_node(false);
  auto no_Xs = adapter.build_node(false);

  for (int curr_level = line[3]; curr_level > line[0]; curr_level--) {
    if (curr_level == line[line_idx]) {
      no_Xs = adapter.build_node(curr_level, no_Xs, safe);
      only_Xs = adapter.build_node(curr_level, safe, only_Xs);

      line_idx--;
    } else if (curr_level <= line[3]) {
      no_Xs = adapter.build_node(curr_level, no_Xs, no_Xs);
      only_Xs = adapter.build_node(curr_level, only_Xs, only_Xs);
    }

    if (curr_level > line[1]) {
      safe = adapter.build_node(curr_level, safe, safe);
    }
  }

  // Split for both
  root = adapter.build_node(line[0], no_Xs, only_Xs);

  // Pre "don't care" chain
  for (int curr_level = line[0] - 1; curr_level >= 0; curr_level--) {
    root = adapter.build_node(curr_level, root, root);
  }

  typename Adapter::dd_t out = adapter.build();
#ifdef BDD_BENCHMARK_STATS
  total_nodes += adapter.nodecount(out);
#endif // BDD_BENCHMARK_STATS
  return out;
}

// =============================================================================
template<typename Adapter>
int run_tictactoe(int argc, char** argv)
{
  no_options option = no_options::NONE;
  bool should_exit = parse_input(argc, argv, option);
  N = input_sizes.empty() ? 20 : input_sizes.at(0);

  if (should_exit) { return -1; }

  // =========================================================================
  std::cout << "Tic-Tac-Toe with " << N << " crosses\n";

  construct_lines();

  return run<Adapter>(64, [&](Adapter &adapter) {
    uint64_t solutions;

    // =========================================================================
    // Construct is_equal_N
    std::cout << "\n"
              << "  Initial decision diagram\n"
              << std::flush;

    time_point t1 = now();
    typename Adapter::dd_t res = construct_init(adapter);
    time_point t2 = now();

    const size_t initial_bdd = adapter.nodecount(res);
    const time_duration init_time = duration_ms(t1,t2);

#ifdef BDD_BENCHMARK_STATS
    total_nodes += initial_bdd;
#endif // BDD_BENCHMARK_STATS
    std::cout << "  | size (nodes)              " << initial_bdd << "\n"
              << "  | time (ms)                 " << init_time << "\n"
              << std::flush;

    // =========================================================================
    // Add constraints lines
    std::cout << "\n"
              << "  Applying Constraints\n"
              << std::flush;

    time_point t3 = now();

    for (auto &line : lines) {
      res &= construct_is_not_winning(adapter, line);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(res);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << "  | [" << line[0] << "," << line[1] << "," << line[2] << "] "
                << std::string((line[0] < 10) + (line[1] < 10) + (line[2] < 10), ' ') << "               "
                << nodecount
                << "\n"
                << std::flush;
#endif // BDD_BENCHMARK_STATS
    }

    time_point t4 = now();

    const time_duration constraints_time = duration_ms(t3,t4);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "  |\n"
              << "  | total no. nodes           " << total_nodes << "\n"
              << "  | largest size (nodes)      " << largest_bdd << "\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "  | final size (nodes)        " << adapter.nodecount(res) << "\n"
              << "  | time (ms)                 " << constraints_time << "\n"
              << std::flush;

    // =========================================================================
    // Count number of solutions
    std::cout << "\n"
              << "  Counting Solutions\n"
              << std::flush;

    time_point t5 = now();
    solutions = adapter.satcount(res);
    time_point t6 = now();

    const time_duration counting_time = duration_ms(t5,t6);

    // =========================================================================
    std::cout << "  | number of solutions       " << solutions << "\n"
              << "  | time (ms)                 " << counting_time << "\n"
              << std::flush;

    // =========================================================================
    std::cout << "\n"
              << "  total time (ms)             " << (init_time + constraints_time + counting_time) << "\n"
              << std::flush;

    if (N < size(expected_tic_tac_toe) && solutions != expected_tic_tac_toe[N]) {
      return -1;
    }
    return 0;
  });
}
