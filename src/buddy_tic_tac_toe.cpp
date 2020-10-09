#include "buddy_init.cpp"

#include "common.cpp"
#include "tic_tac_toe.cpp"

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  int largest_bdd = 0;

  // =========================================================================
  BUDDY_INIT(64, M)

  // =========================================================================
  // Construct is_equal_N
  auto t1 = get_timestamp();

  bdd init_parts[N+1];
  for (size_t i = 0; i <= N; i++) {
    init_parts[i] = i < N ? bddfalse : bddtrue;
  }

  size_t curr_level = 63;

  do {
    size_t min_idx = curr_level > 63 - N ? N - (63 - curr_level + 1) : 0;
    size_t max_idx = std::min(curr_level, N);

    for (size_t curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
      bdd low = init_parts[curr_idx];
      bdd high = curr_idx == N ? bddfalse : init_parts[curr_idx + 1];

      init_parts[curr_idx] = bdd_ite(bdd_ithvar(curr_level), low, high);
    }
  } while (curr_level-- > 0);

  bdd res = init_parts[0];

  int initial_bdd = bdd_nodecount(res);

  auto t2 = get_timestamp();

  // =========================================================================
  // Add constraints lines
  construct_lines();

  auto t3 = get_timestamp();

  bdd temp_A = bddfalse, temp_B = bddfalse;

  for (auto &line : lines) {
    // Create OBDD for line
    for (auto it = line.rbegin(); it != line.rend(); ++it) {
      temp_A |= bdd_ithvar(*it);
      temp_B |= bdd_nithvar(*it);
    }

    // Add constraint to result
    res &= temp_A & temp_B;
    largest_bdd = std::max(largest_bdd, bdd_nodecount(res));

    // Reset temp_A and temp_B for the next iteration
    temp_A = bddfalse;
    temp_B = bddfalse;
  }

  auto t4 = get_timestamp();

  // =========================================================================
  // Count number of solutions

  auto t5 = get_timestamp();

  double solutions = bdd_satcount(res);

  auto t6 = get_timestamp();

  // =========================================================================
  INFO("Tic-Tac-Toe with %zu crosses (BuDDy %zu MB):\n", N, M);
  INFO(" | number of solutions:    %.0f\n", solutions);
  INFO(" | size (nodes):\n");
  INFO(" | | initial:              %i\n",  initial_bdd);
  INFO(" | | largest:              %i\n",  largest_bdd);
  INFO(" | | final:                %i\n",  bdd_nodecount(res));
  INFO(" | time (ms):\n");
  INFO(" | | intial construction:  %zu\n", duration_of(t1,t2));
  INFO(" | | applying constraints: %zu\n", duration_of(t3,t4));
  INFO(" | | counting:             %zu\n", duration_of(t5,t6));
  INFO(" | | total:                %zu\n", duration_of(t1,t2) + duration_of(t3,t6));

  // =========================================================================
  BUDDY_DEINIT

  if (solutions != expected_result[N]) {
    exit(-1);
  }
}
