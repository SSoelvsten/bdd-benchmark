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
template<typename mgr_t>
typename mgr_t::bdd_t construct_init(mgr_t &mgr)
{
  typename mgr_t::bdd_t res;

  typename mgr_t::bdd_t init_parts[N+1];
  for (int i = 0; i <= N; i++) {
    init_parts[i] = i < N ? mgr.leaf_false() : mgr.leaf_true();
  }

  int curr_level = 63;

  do {
    int min_idx = curr_level > 63 - N ? N - (63 - curr_level + 1) : 0;
    int max_idx = std::min(curr_level, N);

    for (int curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
      typename mgr_t::bdd_t low = init_parts[curr_idx];
      typename mgr_t::bdd_t high = curr_idx == N ? mgr.leaf_false() : init_parts[curr_idx + 1];

      init_parts[curr_idx] = mgr.ite(mgr.ithvar(curr_level), low, high);
    }
  } while (curr_level-- > 0);

  res = init_parts[0];

  return res;
}

// ========================================================================== //
//                              LINE CONSTRAINT                               //
template<typename mgr_t>
typename mgr_t::bdd_t construct_is_not_winning(mgr_t &mgr, std::array<int, 4>& line)
{
  size_t idx = 4 - 1;

  typename mgr_t::bdd_t no_Xs = mgr.leaf_false();
  typename mgr_t::bdd_t only_Xs = mgr.leaf_false();

  do {
    no_Xs = mgr.ite(mgr.ithvar(line[idx]),
                    idx == 0 ? only_Xs : mgr.leaf_true(),
                    no_Xs);

    if (idx > 0) {
      only_Xs = mgr.ite(mgr.ithvar(line[idx]),
                        only_Xs,
                        mgr.leaf_true());
    }
  } while (idx-- > 0);

  return no_Xs;
}

// =============================================================================
template<typename mgr_t>
void run_tic_tac_toe(int argc, char** argv)
{
  no_variable_order variable_order = no_variable_order::NO_ORDERING;
  N = 20;
  bool should_exit = parse_input(argc, argv, variable_order);
  if (should_exit) { exit(-1); }

  // =========================================================================

  std::cout << "Tic-Tac-Toe with " << N << " crosses "
            << "(" << mgr_t::NAME << " " << M << " MiB):" << std::endl;

  auto t_init_before = get_timestamp();
  mgr_t mgr(64);
  auto t_init_after = get_timestamp();
  INFO(" | init time (ms):         %zu\n", duration_of(t_init_before, t_init_after));

  construct_lines();

  uint64_t solutions;
  {
    // =========================================================================
    // Construct is_equal_N
    INFO(" | initial BDD:\n");

    auto t1 = get_timestamp();
    typename mgr_t::bdd_t res = construct_init(mgr);
    size_t initial_bdd = mgr.nodecount(res);
    auto t2 = get_timestamp();

    INFO(" | | size (nodes):         %zu\n", initial_bdd);
    INFO(" | | time (ms):            %zu\n", duration_of(t1,t2));

    // =========================================================================
    // Add constraints lines
    INFO(" | applying constraints:\n");

    size_t largest_bdd = 0;
    size_t total_nodes = initial_bdd;

    auto t3 = get_timestamp();

    for (auto &line : lines) {
      res &= construct_is_not_winning(mgr, line);

      const size_t nodecount = mgr.nodecount(res);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount + 4 /* from construct_is_not_winning */;
    }

    auto t4 = get_timestamp();

    INFO(" | | total no. nodes:      %zu\n", total_nodes);
    INFO(" | | largest size (nodes): %zu\n", largest_bdd);
    INFO(" | | final size (nodes):   %zu\n", mgr.nodecount(res));
    INFO(" | | time (ms):            %zu\n", duration_of(t3,t4));

    // =========================================================================
    // Count number of solutions
    INFO(" | counting solutions:\n");

    auto t5 = get_timestamp();
    solutions = mgr.satcount(res);
    auto t6 = get_timestamp();

    // =========================================================================
    INFO(" | | time (ms):            %zu\n", duration_of(t5,t6));
    INFO(" | | number of solutions:  %zu\n", solutions);

    // =========================================================================
    INFO(" | total time (ms):        %zu\n", duration_of(t1,t2) + duration_of(t3,t6));
  }

  mgr.print_stats();

  if (N < size(expected_tic_tac_toe) && solutions != expected_tic_tac_toe[N]) {
    exit(-1);
  }
}
