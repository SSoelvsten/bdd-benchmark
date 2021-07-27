#include "common.cpp"
#include "expected.h"

#include "sat_solver.h"

// =============================================================================
inline size_t label_of_position(size_t i, size_t j)
{
  return (N * i) + j;
}

// =============================================================================
template<typename mgr_t>
void construct_Queens_cnf(sat_solver<mgr_t> &solver)
{
  // ALO on rows
  for (size_t i = 0; i < N; i++) {
    clause_t clause;

    for (size_t j = 0; j < N; j++) {
      clause.push_back(literal_t (label_of_position(i,j), false));
    }

    solver.add_clause(clause);
  }

  // ALO on columns

  // Strictly not necessary, as the ALO on rows together with the AMO
  // constrasize_ts below already enforce this. What we do gain is hopefully an
  // earlier pruning of the search-tree (reflected in the BDD).
  for (size_t j = 0; j < N; j++) {
    clause_t clause;

    for (size_t i = 0; i < N; i++) {
      clause.push_back(literal_t (label_of_position(i,j), false));
    }

    solver.add_clause(clause);
  }

  // AMO on rows
  for (size_t row = 0; row < N; row++) {
    for (size_t i = 0; i < N-1; i++) {
      for (size_t j = i+1; j < N; j++) {
        clause_t clause;
        clause.push_back(literal_t (label_of_position(row,i), true));
        clause.push_back(literal_t (label_of_position(row,j), true));
        solver.add_clause(clause);
      }
    }
  }

  // AMO on columns
  for (size_t col = 0; col < N; col++) {
    for (size_t i = 0; i < N-1; i++) {
      for (size_t j = i+1; j < N; j++) {
        clause_t clause;
        clause.push_back(literal_t (label_of_position(i,col), true));
        clause.push_back(literal_t (label_of_position(j,col), true));
        solver.add_clause(clause);
      }
    }
  }

  // AMO on diagonals
  for (size_t d = 0; d < N; d++) {
    // Diagonal, that touches the left side of the board
    for (size_t i_col = 0; i_col + d < N; i_col++) {
      size_t i_row = i_col + d;
      for (size_t j_offset = 1; i_row + j_offset < N; j_offset++) {
        size_t j_row = i_row + j_offset;
        size_t j_col = i_col + j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }

  for (size_t d = 1; d < N; d++) {
    // Diagonal, that touches the right side of the board
    // d starts at 1 to skip the diagonal that touches both left and right
    for (size_t i_row = 0; i_row + d < N; i_row++) {
      size_t i_col = i_row + d;
      for (size_t j_offset = 1; i_col + j_offset < N; j_offset++) {
        size_t j_row = i_row + j_offset;
        size_t j_col = i_col + j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }

  // AMO on anti-diagonals
  for (size_t d = 0; d < N; d++) {
    // Diagonal, that touches the top
    for (int i_row = 0; i_row < N && N-1 - i_row - d >= 0; i_row++) {
      size_t i_col = N - 1 - i_row - d;
      for (int j_offset = 1; i_col - j_offset >= 0 && i_row + j_offset < N; j_offset++) {
        size_t j_row = i_row + j_offset;
        size_t j_col = i_col - j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }

  for (size_t d = 1; d < N; d++) {
    // Anti-diagonal, that touches the bottom
    for (int i_col = 0; i_col < N && N-1 - i_col + d >= 0; i_col++) {
      size_t i_row = N - 1 - i_col + d;
      for (int j_offset = 1; i_col - j_offset >= 0 && i_row + j_offset < N; j_offset++) {
        size_t j_row = i_row + j_offset;
        size_t j_col = i_col - j_offset;

        clause_t clause;
        clause.push_back(literal_t (label_of_position(i_row,i_col), true));
        clause.push_back(literal_t (label_of_position(j_row,j_col), true));
        solver.add_clause(clause);
      }
    }
  }
}

// =============================================================================
template<typename mgr_t>
void run_sat_queens(int argc, char** argv)
{
  N = 6;
  bool should_exit = parse_input(argc, argv);
  if (should_exit) { exit(-1); }

  bool satisfiable = true;
  uint64_t solutions = -1;

  {
    // =========================================================================
    std::cout << N << "-Queens SAT"
              << " (" << mgr_t::NAME << " " << M << " MiB):" << std::endl;

    uint64_t varcount = label_of_position(N-1, N-1)+1;

    auto t_init_before = get_timestamp();
    sat_solver<mgr_t> solver(varcount);
    auto t_init_after = get_timestamp();
    INFO(" | init time (ms):        %zu\n", duration_of(t_init_before, t_init_after));

    // =========================================================================
    auto t1 = get_timestamp();
    construct_Queens_cnf(solver);
    auto t2 = get_timestamp();

    INFO(" | CNF:\n");
    INFO(" | | clauses:             %zu\n", solver.cnf_size());
    INFO(" | | variables:           %zu\n", solver.var_count());
    INFO(" | | time (ms):           %zu\n", duration_of(t1,t2));
    INFO(" |\n");

    // =========================================================================
#ifndef GRENDEL
    auto t3 = get_timestamp();
    satisfiable = solver.check_satisfiable();
    auto t4 = get_timestamp();
    INFO(" | Satisfiability:\n");
    INFO(" | | solution:            %s\n", satisfiable ? "SATISFIABLE" : "UNSATISFIABLE");
    INFO(" | statistics:\n");
    INFO(" | | operations:\n");
    INFO(" | | | exists:            %zu\n", solver.exists_count());
    INFO(" | | | apply:             %zu\n", solver.apply_count());
    INFO(" | | BDD size (nodes):\n");
    INFO(" | | | largest size:      %zu\n", solver.bdd_largest_size());
    INFO(" | | | final size:        %zu\n", solver.bdd_size());
    INFO(" | | time (ms):           %zu\n", duration_of(t3,t4));
    INFO(" |\n");
#endif

    // =========================================================================
    auto t5 = get_timestamp();
    solutions = solver.check_satcount();
    auto t6 = get_timestamp();
    INFO(" | Counting:\n");
    INFO(" | | solutions:           %zu\n", solutions);
    INFO(" | statistics:\n");
    INFO(" | | operations:\n");
    INFO(" | | | apply:             %zu\n", solver.apply_count());
    INFO(" | | BDD size (nodes):\n");
    INFO(" | | | largest size:      %zu\n", solver.bdd_largest_size());
    INFO(" | | | final size:        %zu\n", solver.bdd_size());
    INFO(" | | time (ms):           %zu\n", duration_of(t5,t6));
  }

  if ((N >= size(expected_queens) || solutions != expected_queens[N])
      && ((N != 2 && N != 3) && satisfiable)
      && ((N == 2 || N == 3) && !satisfiable)) {
    exit(-1);
  }
}
