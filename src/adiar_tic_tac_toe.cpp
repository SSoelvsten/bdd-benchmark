#include "common.cpp"
#include "tic_tac_toe.cpp"

#include "adiar_init.cpp"

bdd construct_is_not_winning(std::array<uint64_t, 4>& line)
{
  size_t idx = 4 - 1;

  ptr_t no_Xs_false = adiar::create_sink_ptr(false);
  ptr_t no_Xs_true = adiar::create_sink_ptr(true);

  ptr_t some_Xs_true = adiar::create_sink_ptr(false);

  node_file out;
  node_writer out_writer(out);

  do {
    /* Notice, we have to write bottom-up. That is actually more precisely in
     * reverse topological order which also includes the id's. */
    node_t some_Xs = adiar::create_node(line[idx], 1,
                                       adiar::create_sink_ptr(true),
                                       some_Xs_true);

    if (idx != 0) {
      out_writer << some_Xs;
    }

    node_t no_Xs = adiar::create_node(line[idx], 0,
                                     no_Xs_false,
                                     no_Xs_true);

    out_writer << no_Xs;

    no_Xs_false = no_Xs.uid;
    if (idx == 1) { // The next is the root?
      no_Xs_true = some_Xs.uid;
    }

    some_Xs_true = some_Xs.uid;
  } while (idx-- > 0);

  return out;
}

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  // =========================================================================
  INFO("Tic-Tac-Toe with %zu crosses (Adiar %zu MB):\n", N, M);
  auto t_init_before = get_timestamp();
  adiar_init(M);
  auto t_init_after = get_timestamp();
  INFO(" | init time (ms):           %zu\n", duration_of(t_init_before, t_init_after));

  double solutions;

  { // Garbage collect all adiar objects before deinit();
    // =========================================================================
    // Construct is_equal_N
    INFO(" | initial BDD:\n");

    auto t1 = get_timestamp();
    bdd res = bdd_counter(0, 63, N);
    auto t2 = get_timestamp();

    INFO(" | | size (nodes):         %zu\n", bdd_nodecount(res));
    INFO(" | | time (ms):            %zu\n", duration_of(t1,t2));

    // ===========================================================================
    // Add constraints lines
    INFO(" | applying constraints:\n");

    construct_lines();

    size_t largest_bdd = 0;

    auto t3 = get_timestamp();

    for (auto &line : lines) {
      res &= construct_is_not_winning(line);
      largest_bdd = std::max(largest_bdd, bdd_nodecount(res));
    }

    auto t4 = get_timestamp();

    INFO(" | | largest size (nodes): %zu\n", largest_bdd);
    INFO(" | | final size (nodes):   %zu\n", bdd_nodecount(res));
    INFO(" | | time (ms):            %zu\n", duration_of(t3,t4));

    // =========================================================================
    // Count number of solutions
    INFO(" | counting solutions:\n");

    auto t5 = get_timestamp();
    solutions = bdd_satcount(res);
    auto t6 = get_timestamp();

    // =========================================================================
    INFO(" | | time (ms):            %zu\n", duration_of(t5,t6));
    INFO(" | | number of solutions:  %.0f\n", solutions);

    // =========================================================================
    INFO(" | total time (ms):        %zu\n", duration_of(t1,t2) + duration_of(t3,t6));
  }
  adiar_deinit();

  if (solutions != expected_result[N]) {
    exit(-1);
  }
}

