/**
 * N-queens example.
 *
 * Created from the original example for Sylvan based on the work by Robert
 * Meolic, released by him into the public domain. Further modified based on a
 * paper of Daniel Kunkle, Vlad Slavici, and Gene Cooperman to improve
 * performance manyfold.
 */

#include <sylvan.h>
#include <sylvan_table.h>

#include "common.cpp"
#include "queens.cpp"

using namespace sylvan;

// =============================================================================
int main(int argc, char** argv)
{
  size_t N = 8;
  parse_input(argc, argv, N);

  size_t largest_bdd = 0;

  // =========================================================================
  // Init Lace
  lace_init(0, 1000000); // auto-detect number of workers, use a 1,000,000 size task queue
  lace_startup(0, NULL, NULL); // auto-detect program stack, do not use a callback for startup

  // Lace is initialized, now set local variables
  LACE_ME;

  // =========================================================================
  // Init Sylvan
  // Nodes table size of 1LL<<20 is 1048576 entries
  // Cache size of 1LL<<18 is 262144 entries
  // Nodes table size: 24 bytes * nodes
  // Cache table size: 36 bytes * cache entries
  // With 2^20 nodes and 2^18 cache entries, that's 33 MB
  // With 2^24 nodes and 2^22 cache entries, that's 528 MB
  sylvan_set_sizes(1LL<<20, 1LL<<24, 1LL<<18, 1LL<<22);
  sylvan_init_package();
  sylvan_set_granularity(3); // granularity 3 is decent value for this small problem - 1 means "use cache for every operation"
  sylvan_init_bdd();

  // =========================================================================
  // Setup for N Queens
  auto t1 = get_timestamp();

  // =========================================================================
  // Encode the constraint for each field bottom-up.
  //
  //                           x_ij /\ !threats(i,j)
  BDD board[N*N];
  for (size_t i=0; i<N*N; i++) {
    board[i] = sylvan_true;
    sylvan_protect(board+i);
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
    largest_bdd = std::max(largest_bdd, sylvan_nodecount(temp));

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
  BDD vars = sylvan_true;
  sylvan_protect(&vars);
  for (size_t i=0; i < N*N; i++) vars = sylvan_and(vars, sylvan_ithvar(i));

  auto t3 = get_timestamp();

  double solutions = sylvan_satcount(res, vars);

  auto t4 = get_timestamp();

  // =========================================================================
  INFO("%zu-Queens (Sylvan):\n", N);
  INFO(" | number of solutions: %.0f\n", solutions);
  INFO(" | size (nodes):\n");
  INFO(" | | largest size:      %zu\n", largest_bdd);
  INFO(" | | final size:        %zu\n", sylvan_nodecount(res));
  INFO(" | time (ms):\n");
  INFO(" | | construction:      %zu\n", duration_of(t1,t2));
  INFO(" | | counting:          %zu\n", duration_of(t3,t4));
  INFO(" | | total:             %zu\n", duration_of(t1,t4));

  // =========================================================================
  // Sylvan and LACE deinit
  sylvan_quit();
  lace_exit();
}
