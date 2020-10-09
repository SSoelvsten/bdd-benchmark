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

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 20;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  size_t largest_bdd = 0;
  size_t initial_bdd = 0;

  // =========================================================================
  SYLVAN_INIT(M)

  // =========================================================================
  // Construct is_equal_N
  auto t1 = get_timestamp();

  BDD init_parts[N+1];
  for (size_t i = 0; i <= N; i++) {
    init_parts[i] = i < N ? sylvan_false : sylvan_true;
    sylvan_protect(init_parts + i);
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

  BDD res = init_parts[0];
  sylvan_protect(&res);

  initial_bdd = sylvan_nodecount(res);

  auto t2 = get_timestamp();

  // =========================================================================
  // Add constraints lines
  construct_lines();

  auto t3 = get_timestamp();

  BDD temp_A = sylvan_false, temp_B = sylvan_false;
  sylvan_protect(&temp_A);
  sylvan_protect(&temp_B);

  for (auto &line : lines) {
    // Create OBDD for line
    for (auto it = line.rbegin(); it != line.rend(); ++it) {
      temp_A = sylvan_or(temp_A, sylvan_ithvar(*it));
      temp_B = sylvan_or(temp_B, sylvan_not(sylvan_ithvar(*it)));
    }

    // Add constraint to result
    temp_A = sylvan_and(temp_A, temp_B);
    res = sylvan_and(res, temp_A);
    largest_bdd = std::max(largest_bdd, sylvan_nodecount(res));

    // Reset temp_A and temp_B for the next iteration
    temp_A = sylvan_false;
    temp_B = sylvan_false;
  }

  auto t4 = get_timestamp();

  // =========================================================================
  // Count number of solutions

  // Old satcount function still requires a silly variables cube
  auto t5 = get_timestamp();

  BDD vars = sylvan_true;
  sylvan_protect(&vars);
  for (size_t i=0; i < 64; i++) vars = sylvan_and(vars, sylvan_ithvar(i));

  double solutions = sylvan_satcount(res, vars);

  auto t6 = get_timestamp();

  // =========================================================================
  INFO("Tic-Tac-Toe with %zu crosses (Sylvan %zu MB):\n", N, M);
  INFO(" | number of solutions:    %.0f\n", solutions);
  INFO(" | size (nodes):\n");
  INFO(" | | initial:              %zu\n", initial_bdd);
  INFO(" | | largest:              %zu\n", largest_bdd);
  INFO(" | | final:                %zu\n", sylvan_nodecount(res));
  INFO(" | time (ms):\n");
  INFO(" | | intial construction:  %zu\n", duration_of(t1,t2));
  INFO(" | | applying constraints: %zu\n", duration_of(t3,t4));
  INFO(" | | counting:             %zu\n", duration_of(t5,t6));
  INFO(" | | total:                %zu\n", duration_of(t1,t2) + duration_of(t3,t6));

  // =========================================================================
  SYLVAN_DEINIT

  if (solutions != expected_result[N]) {
    exit(-1);
  }
}
