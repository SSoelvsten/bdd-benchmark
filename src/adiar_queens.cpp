#include "common.cpp"
#include "queens.cpp"

#include "adiar_init.cpp"

size_t largest_bdd = 0;

bdd n_queens_S(uint64_t N, uint64_t i, uint64_t j)
{
  node_file out;

  node_writer out_writer(out);

  uint64_t row = N - 1;
  ptr_t next = create_sink_ptr(true);

  do {
    uint64_t row_diff = std::max(row,i) - std::min(row,i);

    if (row_diff == 0) {
      // On row of the queen in question
      uint64_t column = N - 1;
      do {
        label_t label = label_of_position(N, row, column);

        // If (row, column) == (i,j), then the chain goes through high.
        if (column == j) {
          // Node to check whether the queen actually is placed, and if so
          // whether all remaining possible conflicts have to be checked.
          label_t label = label_of_position(N, i, j);
          node_t queen = create_node(label, 0, create_sink_ptr(false), next);

          out_writer << queen;
          next = queen.uid;
          continue;
        }

        node_t out_node = create_node(label, 0, next, create_sink_ptr(false));

        out_writer << out_node;
        next = out_node.uid;
      } while (column-- > 0);
    } else {
      // On another row
      if (j + row_diff < N) {
        // Diagonal to the right is within bounds
        label_t label = label_of_position(N, row, j + row_diff);
        node_t out_node = create_node(label, 0, next, create_sink_ptr(false));

        out_writer << out_node;
        next = out_node.uid;
      }

      // Column
      label_t label = label_of_position(N, row, j);
      node_t out_node = create_node(label, 0, next, create_sink_ptr(false));

      out_writer << out_node;
      next = out_node.uid;

      if (row_diff <= j) {
        // Diagonal to the left is within bounds
        label_t label = label_of_position(N, row, j - row_diff);
        node_t out_node = create_node(label, 0, next, create_sink_ptr(false));

        out_writer << out_node;
        next = out_node.uid;
      }
    }
  } while (row-- > 0);

  return out;
}

bdd n_queens_R(uint64_t N, uint64_t row)
{
  bdd out = n_queens_S(N, row, 0);

  for (uint64_t j = 1; j < N; j++) {
    out |= n_queens_S(N, row, j);
    largest_bdd = std::max(largest_bdd, bdd_nodecount(out));
  }
  return out;
}

bdd n_queens_B(uint64_t N)
{
  if (N == 1) {
    return n_queens_S(N, 0, 0);
  }

  bdd out = n_queens_R(N, 0);

  for (uint64_t i = 1; i < N; i++) {
    out &= n_queens_R(N, i);
    largest_bdd = std::max(largest_bdd, bdd_nodecount(out));
  }
  return out;
}

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  // =========================================================================
  INFO("%zu-Queens (Adiar %zu MB):\n", N, M);
  auto t_init_before = get_timestamp();
  ADIAR_INIT(M);
  auto t_init_after = get_timestamp();
  INFO(" | init time (ms):       %zu\n", duration_of(t_init_before, t_init_after));

  double solutions;

  // =========================================================================
  // Compute board
  { // Garbage collect all adiar objects before calling adiar_deinit();
    auto t1 = get_timestamp();
    bdd res = n_queens_B(N);
    auto t2 = get_timestamp();

    INFO(" | construction:\n");
    INFO(" | | largest size (nodes): %i\n", largest_bdd);
    INFO(" | | final size (nodes):   %i\n", bdd_nodecount(res));
    INFO(" | | time (ms):            %zu\n", duration_of(t1,t2));

    // =========================================================================
    // Count number of solutions
    auto t3 = get_timestamp();
    solutions = bdd_satcount(res);
    auto t4 = get_timestamp();

    // =========================================================================
    INFO(" | counting solutions:\n");
    INFO(" | | counting:             %zu\n", duration_of(t3,t4));
    INFO(" | | number of solutions:  %.0f\n", solutions);

    INFO(" | total time (ms):        %zu\n", duration_of(t1,t4));
  }

  ADIAR_DEINIT;

  if (N < size(expected_result) && solutions != expected_result[N]) {
    exit(-1);
  }
}
