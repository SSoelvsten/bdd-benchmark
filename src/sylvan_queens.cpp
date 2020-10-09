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
#include "queens.cpp"

using namespace sylvan;

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  size_t M = 128;
  parse_input(argc, argv, N, M);

  size_t largest_bdd = 0;

  // =========================================================================
  SYLVAN_INIT(M)

  // =========================================================================
  // Setup for N Queens
  auto t1 = get_timestamp();

  // =========================================================================
  // Encode the constraint for each field bottom-up.
  //
  //                           x_ij /\ !threats(i,j)
  BDD board[N*N];
  for (size_t i = 0; i < N*N; i++) {
    board[i] = sylvan_true;
    sylvan_protect(board + i);
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
              board[ij_label] = sylvan_and(board[ij_label],
                                           sylvan_ithvar(ij_label));
            } else {
              board[ij_label] = sylvan_and(board[ij_label],
                                           sylvan_not(sylvan_ithvar(label)));
            }
          } while (column-- > 0);
        } else {
          if (j + row_diff < N) {
            size_t label = label_of_position(N, row, j + row_diff);
            board[ij_label] = sylvan_and(board[ij_label],
                                         sylvan_not(sylvan_ithvar(label)));
          }

          size_t label = label_of_position(N, row, j);
          board[ij_label] = sylvan_and(board[ij_label],
                                       sylvan_not(sylvan_ithvar(label)));

          if (row_diff <= j) {
            size_t label = label_of_position(N, row, j - row_diff);
            board[ij_label] = sylvan_and(board[ij_label],
                                         sylvan_not(sylvan_ithvar(label)));
          }
        }
      } while (row-- > 0);

      largest_bdd = std::max(largest_bdd, sylvan_nodecount(board[ij_label]));
    }
  }

  // =========================================================================
  // Accumulate fields into rows and then accumulte rows
  //
  //      Row i:  Field i,1 \/ Field i,2 \/ ... \/ Field i,N
  //
  //      Total:  Row 1 /\ Row 2 /\ ... /\ Row N
  BDD res = sylvan_true, temp = sylvan_true;
  sylvan_protect(&res);
  sylvan_protect(&temp);

  for (size_t i = 0; i < N; i++) {
    temp = board[label_of_position(N, i, 0)];

    for (size_t j = 1; j < N; j++) {
      size_t label = label_of_position(N, i, j);
      temp = sylvan_or(temp, board[label]);
      largest_bdd = std::max(largest_bdd, sylvan_nodecount(temp));
    }

    res = i == 0 ? temp : sylvan_and(res, temp);
    largest_bdd = std::max(largest_bdd, sylvan_nodecount(res));
  }

  auto t2 = get_timestamp();

  // =========================================================================
  // Count number of solutions

  // Old satcount function still requires a silly variables cube
  auto t3 = get_timestamp();

  BDD vars = sylvan_true;
  sylvan_protect(&vars);
  for (size_t i=0; i < N*N; i++) vars = sylvan_and(vars, sylvan_ithvar(i));

  double solutions = sylvan_satcount(res, vars);

  auto t4 = get_timestamp();

  // =========================================================================
  INFO("%zu-Queens (Sylvan %zu MB):\n", N, M);
  INFO(" | number of solutions: %.0f\n", solutions);
  INFO(" | size (nodes):\n");
  INFO(" | | largest size:      %zu\n", largest_bdd);
  INFO(" | | final size:        %zu\n", sylvan_nodecount(res));
  INFO(" | time (ms):\n");
  INFO(" | | construction:      %zu\n", duration_of(t1,t2));
  INFO(" | | counting:          %zu\n", duration_of(t3,t4));
  INFO(" | | total:             %zu\n", duration_of(t1,t4));

  // =========================================================================
  SYLVAN_DEINIT

  if (solutions != expected_result[N]) {
    exit(-1);
  }
}
