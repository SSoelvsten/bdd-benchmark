#include "common.cpp"
#include "tic_tac_toe.cpp"

#include "buddy_init.cpp"

bdd construct_is_not_winning(std::array<uint64_t, 4>& line)
{
  size_t idx = 4 - 1;

  bdd no_Xs = bddfalse;
  bdd only_Xs = bddfalse;

  do {
    no_Xs = bdd_ite(bdd_ithvar(line[idx]),
                    idx == 0 ? only_Xs : bddtrue,
                    no_Xs);

    if (idx > 0) {
      only_Xs = bdd_ite(bdd_ithvar(line[idx]),
                        only_Xs,
                        bddtrue);
    }
  } while (idx-- > 0);

  return no_Xs;
}

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  // =========================================================================
  INFO("Tic-Tac-Toe with %zu crosses (BuDDy %zu MB):\n", N, M);
  auto t_init_before = get_timestamp();
  BUDDY_INIT(64, M);
  auto t_init_after = get_timestamp();
  INFO(" | init time (ms):         %zu\n", duration_of(t_init_before, t_init_after));

  // =========================================================================
  // Construct is_equal_N
  INFO(" | initial BDD:\n");

  auto t1 = get_timestamp();

  bdd res;

  { // Ensure init_parts is garbage collected
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

    res = init_parts[0];
  }

  int initial_bdd = bdd_nodecount(res);

  auto t2 = get_timestamp();

  INFO(" | | size (nodes):         %i\n", initial_bdd);
  INFO(" | | time (ms):            %zu\n", duration_of(t1,t2));

  // =========================================================================
  // Add constraints lines
  INFO(" | applying constraints:\n");

  construct_lines();
  int largest_bdd = 0;

  auto t3 = get_timestamp();

  for (auto &line : lines) {
    res &= construct_is_not_winning(line);
    largest_bdd = std::max(largest_bdd, bdd_nodecount(res));
  }

  auto t4 = get_timestamp();

  INFO(" | | largest size (nodes): %i\n", largest_bdd);
  INFO(" | | final size (nodes):   %i\n", bdd_nodecount(res));
  INFO(" | | time (ms):            %zu\n", duration_of(t3,t4));

  // =========================================================================
  // Count number of solutions
  INFO(" | counting solutions:\n");

  auto t5 = get_timestamp();
  double solutions = bdd_satcount(res);
  auto t6 = get_timestamp();

  // =========================================================================
  INFO(" | | time (ms):            %zu\n", duration_of(t5,t6));
  INFO(" | | number of solutions:  %.0f\n", solutions);

  // =========================================================================
  INFO(" | total time (ms):        %zu\n", duration_of(t1,t2) + duration_of(t3,t6));

  BUDDY_DEINIT;

  if (N < size(expected_result) && solutions != expected_result[N]) {
    exit(-1);
  }
}
