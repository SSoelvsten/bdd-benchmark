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

size_t largest_bdd = 0;

Bdd n_queens_S(uint64_t N, uint64_t i, uint64_t j)
{
  size_t row = N - 1;
  Bdd out = sylvan_true;

  do {
    size_t row_diff = std::max(row, i) - std::min(row, i);

    if (row_diff == 0) {
      size_t column = N - 1;

      do {
        size_t label = label_of_position(N, row, column);

        if (column == j) {
          out &= sylvan_ithvar(label);
        } else {
          out &= sylvan_nithvar(label);
        }
      } while (column-- > 0);
    } else {
      if (j + row_diff < N) {
        size_t label = label_of_position(N, row, j + row_diff);
        out &= sylvan_nithvar(label);
      }

      size_t label = label_of_position(N, row, j);
      out &= sylvan_nithvar(label);

      if (row_diff <= j) {
        size_t label = label_of_position(N, row, j - row_diff);
        out &= sylvan_nithvar(label);
      }
    }
  } while (row-- > 0);

  largest_bdd = std::max(largest_bdd, out.NodeCount());

  return out;
}

Bdd n_queens_R(uint64_t N, uint64_t row)
{
  Bdd out = n_queens_S(N, row, 0);

  for (uint64_t j = 1; j < N; j++) {
    out |= n_queens_S(N, row, j);
    largest_bdd = std::max(largest_bdd, out.NodeCount());
  }
  return out;
}

Bdd n_queens_B(uint64_t N)
{
  if (N == 1) {
    return n_queens_S(N, 0, 0);
  }

  Bdd out = n_queens_R(N, 0);

  for (uint64_t i = 1; i < N; i++) {
    out &= n_queens_R(N, i);
    largest_bdd = std::max(largest_bdd, out.NodeCount());
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
  INFO("%zu-Queens (Sylvan %zu MB):\n", N, M);
  SYLVAN_INIT(M);

  // =========================================================================
  // Compute board

  auto t1 = get_timestamp();
  Bdd res = n_queens_B(N);
  auto t2 = get_timestamp();

  INFO(" | construction:\n");
  INFO(" | | largest size (nodes): %zu\n", largest_bdd);
  INFO(" | | final size (nodes):   %zu\n", res.NodeCount());
  INFO(" | | time (ms):            %zu\n", duration_of(t1,t2));

  // =========================================================================
  // Count number of solutions

  auto t3 = get_timestamp();
  double solutions = res.SatCount(label_of_position(N,N-1,N-1)+1);
  auto t4 = get_timestamp();

  INFO(" | counting solutions:\n");
  INFO(" | | counting:             %zu\n", duration_of(t3,t4));
  INFO(" | | number of solutions:  %.0f\n", solutions);

  INFO(" | total time (ms):        %zu\n", duration_of(t1,t4));

  // =========================================================================
  SYLVAN_DEINIT;

  if (solutions != expected_result[N]) {
    exit(-1);
  }
}
