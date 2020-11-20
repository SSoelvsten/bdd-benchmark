/**
 * N-queens example.
 *
 * Created from the original example for Sylvan based on the work by Robert
 * Meolic, released by him into the public domain. Further modified based on a
 * paper of Daniel Kunkle, Vlad Slavici, and Gene Cooperman to improve
 * performance manyfold.
 */

#include "sylvan_init.cpp"

#include "common.cpp"
#include "tic_tac_toe.cpp"

using namespace sylvan;

Bdd construct_is_not_winning(std::array<uint64_t, 4>& line)
{
  size_t idx = 4 - 1;

  BDD no_Xs = sylvan_false;
  BDD only_Xs = sylvan_false;

  do {
    no_Xs = sylvan_makenode(line[idx], no_Xs, idx == 0 ? only_Xs : sylvan_true);

    if (idx > 0) {
      only_Xs = sylvan_makenode(line[idx], sylvan_true, only_Xs);
    }
  } while (idx-- > 0);

  return no_Xs;
}

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 20;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  // =========================================================================
  INFO("Tic-Tac-Toe with %zu crosses (Sylvan %zu MB):\n", N, M);
  SYLVAN_INIT(M)

  // =========================================================================
  // Construct is_equal_N
  INFO(" | initial BDD:\n");

  auto t1 = get_timestamp();

  Bdd res;

  { // Garbage collect init_parts to stop claiming 'dead' nodes
    BDD init_parts[N+1];
    for (size_t i = 0; i <= N; i++) {
      init_parts[i] = i < N ? sylvan_false : sylvan_true;
    }

    size_t curr_level = 63;

    do {
      size_t min_idx = curr_level > 63 - N ? N - (63 - curr_level + 1) : 0;
      size_t max_idx = std::min(curr_level, N);

      for (size_t curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
        BDD low = init_parts[curr_idx];
        BDD high = curr_idx == N ? sylvan_false : init_parts[curr_idx + 1];

        init_parts[curr_idx] = sylvan_makenode(curr_level, low, high);
      }
    } while (curr_level-- > 0);

    res = init_parts[0];
  }

  auto t2 = get_timestamp();

  INFO(" | | size (nodes):         %zu\n", res.NodeCount());
  INFO(" | | time (ms):            %zu\n", duration_of(t1,t2));

  // =========================================================================
  // Add constraints lines
  INFO(" | applying constraints:\n");

  construct_lines();
  size_t largest_bdd = 0;

  auto t3 = get_timestamp();

  for (auto &line : lines) {
    res &= construct_is_not_winning(line);
    largest_bdd = std::max(largest_bdd, res.NodeCount());
  }

  auto t4 = get_timestamp();

  INFO(" | | largest size (nodes): %zu\n", largest_bdd);
  INFO(" | | final size (nodes):   %zu\n", res.NodeCount());
  INFO(" | | time (ms):            %zu\n", duration_of(t3,t4));

  // =========================================================================
  // Count number of solutions
  INFO(" | counting solutions:\n");

  // Old satcount function still requires a silly variables cube
  auto t5 = get_timestamp();
  double solutions = res.SatCount(64);
  auto t6 = get_timestamp();

  INFO(" | | time (ms):            %zu\n", duration_of(t5,t6));
  INFO(" | | number of solutions:  %.0f\n", solutions);

  // =========================================================================
  INFO(" | total time (ms):        %zu\n", duration_of(t1,t2) + duration_of(t3,t6));

  SYLVAN_DEINIT

  if (solutions != expected_result[N]) {
    exit(-1);
  }
}
