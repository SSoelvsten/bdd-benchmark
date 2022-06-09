#include <functional>
#include <vector>

#include "common.cpp"
#include "expected.h"

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
typename adapter_t::dd_t construct_init(adapter_t &adapter)
{
  typename adapter_t::dd_t res;

  typename adapter_t::dd_t init_parts[N+1];
  for (int i = 0; i <= N; i++) {
    init_parts[i] = i < N ? adapter.leaf_false() : adapter.leaf_true();
  }

  for (int curr_level = 63; curr_level >= 0; curr_level--) {
    int min_idx = curr_level > 63 - N ? N - (63 - curr_level + 1) : 0;
    int max_idx = std::min(curr_level, N);

    for (int curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
      const typename adapter_t::dd_t low = init_parts[curr_idx];
      const typename adapter_t::dd_t high = curr_idx < N
        ? init_parts[curr_idx + 1]
        : adapter.leaf_false();

      init_parts[curr_idx] = adapter.make_node(curr_level, low, high);
    }
  }

  res = init_parts[0];
  return res;
}

// ========================================================================== //
//                              LINE CONSTRAINT                               //
template<typename adapter_t>
typename adapter_t::dd_t construct_is_not_winning(adapter_t &adapter, std::array<int, 4>& line)
{
  typename adapter_t::dd_t no_Xs = adapter.leaf_false();
  typename adapter_t::dd_t only_Xs = adapter.leaf_false();

  for (int idx = 4-1; idx >= 0; idx--) {
    no_Xs = adapter.make_node(line[idx],
                              no_Xs,
                              idx == 0 ? only_Xs : adapter.leaf_true());

    if (idx > 0) {
      only_Xs = adapter.make_node(line[idx],
                                  adapter.leaf_true(),
                                  only_Xs);
    }
  }

  return no_Xs;
}

// =============================================================================
template<typename adapter_t>
void run_tic_tac_toe(int argc, char** argv)
{
  no_options option = no_options::NONE;
  N = 20;
  bool should_exit = parse_input(argc, argv, option);
  if (should_exit) { exit(-1); }

  // =========================================================================
  INFO("Tic-Tac-Toe with %i crosses (%s %i MiB):\n", N, adapter_t::NAME.c_str(), M);

  time_point t_init_before = get_timestamp();
  adapter_t adapter(64);
  time_point t_init_after = get_timestamp();
  INFO("\n   %s initialisation:\n", adapter_t::NAME.c_str());
  INFO("   | time (ms):              %zu\n", duration_of(t_init_before, t_init_after));

  construct_lines();

  uint64_t solutions;
  {
    // =========================================================================
    // Construct is_equal_N
    INFO("\n   Initial decision diagram:\n");

    time_point t1 = get_timestamp();
    typename adapter_t::dd_t res = construct_init(adapter);
    size_t initial_bdd = adapter.nodecount(res);
    time_point t2 = get_timestamp();

    const time_duration init_time = duration_of(t1,t2);

    INFO("   | size (nodes):           %zu\n", initial_bdd);
    INFO("   | time (ms):              %zu\n", init_time);

    // =========================================================================
    // Add constraints lines
    INFO("\n   Applying constraints:\n");

    size_t largest_bdd = 0;
    size_t total_nodes = initial_bdd;

    time_point t3 = get_timestamp();

    for (auto &line : lines) {
      res &= construct_is_not_winning(adapter, line);

      const size_t nodecount = adapter.nodecount(res);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount + 4 /* from construct_is_not_winning */;
    }

    time_point t4 = get_timestamp();

    const time_duration constraints_time = duration_of(t3,t4);

    INFO("   | total no. nodes:        %zu\n", total_nodes);
    INFO("   | largest size (nodes):   %zu\n", largest_bdd);
    INFO("   | final size (nodes):     %zu\n", adapter.nodecount(res));
    INFO("   | time (ms):              %zu\n", constraints_time);

    // =========================================================================
    // Count number of solutions
    INFO("\n   counting solutions:\n");

    time_point t5 = get_timestamp();
    solutions = adapter.satcount(res);
    time_point t6 = get_timestamp();

    const time_duration counting_time = duration_of(t5,t6);

    // =========================================================================
    INFO("   | number of solutions:    %zu\n", solutions);
    INFO("   | time (ms):              %zu\n", counting_time);

    // =========================================================================
    INFO("\n   total time (ms):          %zu\n", init_time + constraints_time + counting_time);
  }

  adapter.print_stats();

  if (N < size(expected_tic_tac_toe) && solutions != expected_tic_tac_toe[N]) { EXIT(-1); }
  FLUSH();
}
