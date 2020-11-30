// =============================================================================
inline size_t label_of_position(size_t N, size_t i, size_t j)
{
  return (N * i) + j;
}


// =============================================================================
// expected number taken from:
//  https://en.wikipedia.org/wiki/Eight_queens_puzzle#Counting_solutions
size_t expected_result[28] = {
  0,
  1,
  0,
  0,
  2,
  10,
  4,
  40,
  92,
  352,
  724,
  2680,
  14200,
  73712,
  365596,
  2279184,
  14772512,
  95815104,
  666090624,
  4968057848,
  39029188884,
  314666222712,
  2691008701644,
  24233937684440,
  227514171973736,
  2207893435808352,
  22317699616364044,
  234907967154122528
};

////////////////////////////////////////////////////////////////////////////////
/// Constructs the CNF for the Queens problem.
////////////////////////////////////////////////////////////////////////////////
template<typename bdd_policy>
void construct_Queens_cnf(sat_solver<bdd_policy> &solver, int N)
{
  // ALO on rows
  for (int i = 0; i < N; i++) {
    clause_t clause;

    for (int j = 0; j < N; j++) {
      clause.push_back(literal_t (label_of_position(N,i,j), false));
    }

    solver.add_clause(clause);
  }

  // ALO on columns

  // Strictly not necessary, as the ALO on rows together with the AMO
  // constraints below already enforce this. What we do gain is hopefully an
  // earlier pruning of the search-tree (reflected in the BDD).
  for (int j = 0; j < N; j++) {
    clause_t clause;

    for (int i = 0; i < N; i++) {
      clause.push_back(literal_t (label_of_position(N,i,j), false));
    }

    solver.add_clause(clause);
  }

  // AMO on rows
  for (int row = 0; row < N; row++) {
    for (int i = 0; i < N-1; i++) {
      for (int j = i+1; j < N; j++) {
        clause_t clause;
        clause.push_back(literal_t (label_of_position(N,row,i), true));
        clause.push_back(literal_t (label_of_position(N,row,j), true));
        solver.add_clause(clause);
      }
    }
  }

  // AMO on columns
  for (int col = 0; col < N; col++) {
    for (int i = 0; i < N-1; i++) {
      for (int j = i+1; j < N; j++) {
        clause_t clause;
        clause.push_back(literal_t (label_of_position(N,i,col), true));
        clause.push_back(literal_t (label_of_position(N,j,col), true));
        solver.add_clause(clause);
      }
    }
  }

  // AMO on diagonals
  for (int d = 0; d < N; d++) {
    // Diagonal, that touches the left side of the board
    for (int i_col = 0; i_col + d < N; i_col++) {
      int i_row = i_col + d;
      for (int j_offset = 1; i_row + j_offset < N; j_offset++) {
        int j_row = i_row + j_offset;
        int j_col = i_col + j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(N,i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(N,j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }

  for (int d = 1; d < N; d++) {
    // Diagonal, that touches the right side of the board
    // d starts at 1 to skip the diagonal that touches both left and right
    for (int i_row = 0; i_row + d < N; i_row++) {
      int i_col = i_row + d;
      for (int j_offset = 1; i_col + j_offset < N; j_offset++) {
        int j_row = i_row + j_offset;
        int j_col = i_col + j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(N,i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(N,j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }

  // AMO on anti-diagonals
  for (int d = 0; d < N; d++) {
    // Diagonal, that touches the top
    for (int i_row = 0; i_row < N && N-1 - i_row - d >= 0; i_row++) {
      int i_col = N - 1 - i_row - d;
      for (int j_offset = 1; i_col - j_offset >= 0 && i_row + j_offset < N; j_offset++) {
        int j_row = i_row + j_offset;
        int j_col = i_col - j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(N,i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(N,j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }

  for (int d = 1; d < N; d++) {
    // Anti-diagonal, that touches the bottom
    for (int i_col = 0; i_col < N && N-1 - i_col + d >= 0; i_col++) {
      int i_row = N - 1 - i_col + d;
      for (int j_offset = 1; i_col - j_offset >= 0 && i_row + j_offset < N; j_offset++) {
        int j_row = i_row + j_offset;
        int j_col = i_col - j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(N,i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(N,j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }
}
