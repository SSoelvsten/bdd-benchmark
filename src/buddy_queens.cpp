#include <bdd.h>

#include "common.cpp"
#include "queens.cpp"

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  int largest_bdd = 0;

  // =========================================================================
  // Init BuDDy
  bdd_init(M*47100,10000);
  bdd_setmaxincrease(0);

  bdd_setvarnum(N*N);

  bdd_setcacheratio(64);

  // =========================================================================
  // Setup for N Queens
  auto t1 = get_timestamp();

  // =========================================================================
  // Encode the constraint for each field bottom-up.
  //
  //                           x_ij /\ !threats(i,j)
  bdd board[N*N];
  for (size_t i = 0; i < N*N; i++) {
    board[i] = bddtrue;
  }

  for (size_t i = 0; i < N; i++) {
    for (size_t j = 0; j < N; j++) {
      size_t row = N - 1;

      size_t ij_label = label_of_position(N, i, j);

      do {
        size_t row_diff = std::max(row, i) - std::min(row, i);

        if (row_diff == 0) {
          size_t column = N - 1;

          do {
            size_t label = label_of_position(N, row, column);

            if (column == j) {
              board[ij_label] &= bdd_ithvar(ij_label);
            } else {
              board[ij_label] &= bdd_nithvar(label);
            }
          } while (column-- > 0);
        } else {
          if (j + row_diff < N) {
            size_t label = label_of_position(N, row, j + row_diff);
            board[ij_label] &= bdd_nithvar(label);
          }

          size_t label = label_of_position(N, row, j);
          board[ij_label] &= bdd_nithvar(label);

          if (row_diff <= j) {
            size_t label = label_of_position(N, row, j - row_diff);
            board[ij_label] &= bdd_nithvar(label);
          }
        }
      } while (row-- > 0);

      largest_bdd = std::max(largest_bdd, bdd_nodecount(board[ij_label]));
    }
  }

  // =========================================================================
  // Accumulate fields into rows and then accumulte rows
  //
  //      Row i:  Field i,1 \/ Field i,2 \/ ... \/ Field i,N
  //
  //      Total:  Row 1 /\ Row 2 /\ ... /\ Row N
  bdd res = bddtrue, temp = bddtrue;

  for (size_t i = 0; i < N; i++) {
    temp = board[label_of_position(N, i, 0)];

    for (size_t j = 1; j < N; j++) {
      size_t label = label_of_position(N, i, j);
      temp |= board[label];
      largest_bdd = std::max(largest_bdd, bdd_nodecount(temp));
    }

    res = i == 0 ? temp : res & temp;
    largest_bdd = std::max(largest_bdd, bdd_nodecount(res));
  }

  auto t2 = get_timestamp();

  // =========================================================================
  // Count number of solutions

  // Old satcount function still requires a silly variables cube
  auto t3 = get_timestamp();

  double solutions = bdd_satcount(res);

  auto t4 = get_timestamp();

  // =========================================================================
  INFO("%zu-Queens (BuDDy):\n", N);
  INFO(" | number of solutions: %.0f\n", solutions);
  INFO(" | size (nodes):\n");
  INFO(" | | largest size:      %i\n", largest_bdd);
  INFO(" | | final size:        %i\n", bdd_nodecount(res));
  INFO(" | time (ms):\n");
  INFO(" | | construction:      %zu\n", duration_of(t1,t2));
  INFO(" | | counting:          %zu\n", duration_of(t3,t4));
  INFO(" | | total:             %zu\n", duration_of(t1,t4));

  // =========================================================================
  // Sylvan and LACE deinit
  bdd_done();

  if (solutions != expected_result[N]) {
    exit(-1);
  }
}
