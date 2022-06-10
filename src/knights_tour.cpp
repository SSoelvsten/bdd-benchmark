#include "common.cpp"
#include "expected.h"

size_t largest_bdd = 0;
size_t total_nodes = 0;

// ========================================================================== //
//                             Board Indexation                               //

inline int cols()
{ return N / 2; }

inline int MAX_COL()
{ return cols() - 1; }

inline int rows()
{ return N - cols(); }

inline int MAX_ROW()
{ return rows() - 1; }

inline int MAX_TIME()
{ return rows() * cols() - 1; }

inline int int_of_position(int r, int c, int t = 0)
{ return (rows() * cols() * t) + (cols() * r) + c; }

inline int MAX_POSITION()
{ return int_of_position(MAX_ROW(), MAX_COL(), MAX_TIME()); }

inline int row_of_position(int pos)
{ return (pos / cols()) % rows(); }

inline int col_of_position(int pos)
{ return pos % cols(); }

// ========================================================================== //
//                          Closed Tour Constraints                           //
const int closed_squares [3][2] = {{0,0}, {1,2}, {2,1}};

bool is_closed_square(int r, int c)
{
  return (r == closed_squares[0][0] && c == closed_squares[0][1])
    || (r == closed_squares[1][0] && c == closed_squares[1][1])
    || (r == closed_squares[2][0] && c == closed_squares[2][1]);
}

template<typename adapter_t>
typename adapter_t::dd_t knights_tour_closed(adapter_t &adapter)
{
  // Fix t = MAX_TIME() to be (1,2)
  const int stepMax_position = int_of_position(closed_squares[2][0],
                                               closed_squares[2][1],
                                               MAX_TIME());

  typename adapter_t::dd_t root = adapter.make_node(stepMax_position,
                                                    adapter.leaf_false(),
                                                    adapter.leaf_true());

  // All in between is as-is but takes the hamiltonian constraint into account.
  for (int t = MAX_TIME() - 1; t > 1; t--) {
    for (int r = MAX_ROW(); r >= 0; r--) {
      for (int c = MAX_COL(); c >= 0; c--) {
        if (is_closed_square(r,c)) { continue; }

        root = adapter.make_node(int_of_position(r,c,t), root, root);
      }
    }
  }

  // Fix t = 1 to be (2,1)
  const int step1_position = int_of_position(closed_squares[1][0],
                                             closed_squares[1][1],
                                             1);
  root = adapter.make_node(step1_position, root, root);

  // Fix t = 0 to be (0,0)
  const int step0_position = int_of_position(closed_squares[0][0],
                                             closed_squares[0][1],
                                             0);
  root = adapter.make_node(step0_position, root, root);

  const size_t nodecount = adapter.nodecount(root);
  largest_bdd = std::max(largest_bdd, nodecount);
  total_nodes += nodecount;

  return root;
}

// ========================================================================== //
//                 Transition Relation + Hamiltonian Constraint               //

constexpr int row_moves[8]    = { -2, -2, -1, -1,  1,  1,  2,  2 };
constexpr int column_moves[8] = { -1,  1, -2,  2, -2,  2, -1,  1 };

bool is_legal_move(int r_from, int c_from, int r_to, int c_to)
{
  for (int idx = 0; idx < 8; idx++) {
    if (r_from + row_moves[idx] == r_to &&
        c_from + column_moves[idx] == c_to) {
      return true;
    }
  }
  return false;
}

bool is_legal_position(int r, int c, int t = 0)
{
  if (r < 0 || MAX_ROW() < r)  { return false; }
  if (c < 0 || MAX_COL() < c)  { return false; }
  if (t < 0 || MAX_TIME() < t) { return false; }

  return true;
}

bool is_reachable(int r, int c)
{
  for (int idx = 0; idx < 8; idx++) {
    if (is_legal_position(r + row_moves[idx], c + column_moves[idx])) {
      return true;
    }
  }
  return false;
}

int next_reachable_position(int r, int c, int t)
{
  int postulate = int_of_position(r,c,t);
  bool reachable = false;

  do {
    postulate++;
    const int row = row_of_position(postulate);
    const int col = col_of_position(postulate);

    reachable = is_reachable(row,col);
  } while (!reachable);

  return postulate;
}

constexpr int no_pos = std::numeric_limits<int>::max();

int first_legal(int r_from, int c_from, int t)
{
  for (int idx = 0; idx < 8; idx++) {
    const int r_to = r_from + row_moves[idx];
    const int c_to = c_from + column_moves[idx];

    if (!is_legal_position(r_to, c_to)) { continue; }

    return int_of_position(r_to, c_to, t);
  }
  return no_pos;
}

int next_legal(int r_from, int c_from, int r_to, int c_to, int t)
{
  bool seen_move = false;

  for (int idx = 0; idx < 8; idx++) {
    const int r = r_from + row_moves[idx];
    const int c = c_from + column_moves[idx];

    if (!is_legal_position(r,c)) { continue; }

    if (seen_move) { return int_of_position(r, c, t); }
    seen_move |= r == r_to && c == c_to;
  }
  return no_pos;
}

template<typename adapter_t>
void __knights_tour_rel__post_chain__simp(adapter_t &adapter,
                                          std::vector<typename adapter_t::dd_t> &post_chains,
                                          int time, int row, int col)
{
  const int this_label = int_of_position(row, col, time);

  const typename adapter_t::dd_t res =
    adapter.make_node(this_label, post_chains.at(0), post_chains.at(0));

  for (int idx = 0; idx < N*N; idx++) {
    post_chains.at(idx) = res;
  }
}

template<typename adapter_t>
void __knights_tour_rel__post_chain__ham(adapter_t &adapter,
                                         std::vector<typename adapter_t::dd_t> &post_chains,
                                         int time, int row, int col)
{
  // Hamiltonian constraint chain for each position reached at time step 't+1'
  // given some position at time step 't'.
  const int this_label = int_of_position(row, col, time);

  for (int row_t = MAX_ROW(); row_t >= 0; row_t--) {
    for (int col_t = MAX_COL(); col_t >= 0; col_t--) {
      // This position matches (row_t, col_t)? Skip it to make this
      // chain enforce it being a hamiltonian path.
      if (row_t == row && col_t == col) { continue; }

      // Missing node for this and the next time step
      const int this_conflict = int_of_position(row_t, col_t, time);
      const int next_conflict = int_of_position(row_t, col_t, time+1);

      // If past this time step's conflict, then do not output
      // something, since we will merge with the (0,0) chain
      if (time == MAX_TIME()
          && this_label > this_conflict
          && !(row_t == 0 && col_t == 0)) {
        continue;
      }

      // Next cell on board this time step that does match (row_t,col_t).
      // Possibly loops back to (0,0) at the next time step.
      //
      // For the id, we will collapse into the 0-chain if we are past
      // the final time to check for the hamiltonian constraint.
      int next_label = next_reachable_position(row, col, time);
      if (next_label == this_conflict) { next_label++; }
      if (next_label == next_conflict) { next_label++; }
      if (!is_reachable(row_of_position(next_label), col_of_position(next_label))) { next_label++; }

      const int chain_idx = int_of_position(row_t, col_t);
      const int child_idx = (MAX_TIME() == time && next_label > this_conflict)
        || (MAX_TIME()-1 == time && next_label > next_conflict)
        ? 0
        : chain_idx;

      post_chains.at(chain_idx) = adapter.make_node(this_label,
                                                    post_chains.at(child_idx),
                                                    post_chains.at(child_idx));
    }
  }
}

template<typename adapter_t, bool incl_hamiltonian>
typename adapter_t::dd_t __knights_tour_rel(adapter_t &adapter, int t)
{
  std::vector<typename adapter_t::dd_t> post_chains(N*N, adapter.leaf_true());

  // Time steps t' > t+1:
  for (int time = MAX_TIME(); time > t+1; time--) {
    for (int row = MAX_ROW(); row >= 0; row--) {
      for (int col = MAX_COL(); col >= 0; col--) {
        if (!is_reachable(row, col)) { continue; }

        if constexpr (incl_hamiltonian) {
          __knights_tour_rel__post_chain__ham(adapter, post_chains, time, row, col);
        } else {
          __knights_tour_rel__post_chain__simp(adapter, post_chains, time, row, col);
        }
      }
    }
  }

  // Time step t+1:
  //   Chain with each possible position reachable from some position at time 't'.
  std::vector<typename adapter_t::dd_t> to_chains(N*N, adapter.leaf_false());

  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      for (int row_t = MAX_ROW(); row_t >= 0; row_t--) {
        for (int col_t = MAX_COL(); col_t >= 0; col_t--) {
          if (!is_legal_move(row_t, col_t, row, col)) { continue; }

          const int this_label = int_of_position(row, col, t+1);
          const int vector_idx = int_of_position(row_t, col_t);

          to_chains.at(vector_idx) = adapter.make_node(this_label,
                                                       to_chains.at(vector_idx),
                                                       post_chains.at(vector_idx));
        }
      }
    }
  }

  // Time step t:
  //   For each position at time step 't', check whether we are "here" and go to
  //   the chain checking "where we go to" at 't+1'.
  typename adapter_t::dd_t root = adapter.leaf_false();

  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      const int this_label = int_of_position(row, col, t);
      root = adapter.make_node(this_label, root, to_chains.at(int_of_position(row,col)));
    }
  }

  // Time-step t' < t:
  //   Just allow everything, i.e. add no constraints
  if (t > 0) {
    for (int pos = int_of_position(MAX_ROW(), MAX_COL(), t-1); pos >= 0; pos--) {
      root = adapter.make_node(pos, root, root);
    }
  }

  // Finalize
  return root;
}

template<typename adapter_t>
typename adapter_t::dd_t knights_tour_rel(adapter_t &adapter, int t)
{
  return __knights_tour_rel<adapter_t, false>(adapter, t);
}

template<typename adapter_t>
typename adapter_t::dd_t knights_tour_ham_rel(adapter_t &adapter, int t)
{
  return __knights_tour_rel<adapter_t, true>(adapter, t);
}

// ========================================================================== //
//                    Iterate over the above Transition Relation              //
bool closed = false;
bool ham_rel = false;

template<typename adapter_t, bool incl_hamiltonian>
typename adapter_t::dd_t knights_tour_iter_rel(adapter_t &adapter)
{
  largest_bdd = 0;

  typename adapter_t::dd_t res;

  int t = MAX_TIME()-1;
  res = closed
    ? knights_tour_closed<adapter_t>(adapter)
    : knights_tour_rel<adapter_t>(adapter, t);

  while (t-- > closed) {
    res &= incl_hamiltonian
      ? knights_tour_ham_rel<adapter_t>(adapter, t)
      : knights_tour_rel<adapter_t>(adapter, t);

    const size_t nodecount = adapter.nodecount(res);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
  }

  return res;
}

// ========================================================================== //
//                            Add Hamiltonian constraints                     //
template<typename adapter_t>
typename adapter_t::dd_t knights_tour_ham(adapter_t &adapter, int r, int c)
{
  typename adapter_t::dd_t out_never = adapter.leaf_false();
  typename adapter_t::dd_t out_once = adapter.leaf_true();

  for (int this_t = MAX_TIME(); this_t >= 0; this_t--) {
    for (int this_r = MAX_ROW(); this_r >= 0; this_r--) {
      for (int this_c = MAX_COL(); this_c >= 0; this_c--) {
        const int this_label = int_of_position(this_r, this_c, this_t);
        const bool is_rc = r == this_r && c == this_c;

        if (!is_rc && (this_t > 0 || this_r > r)) {
          out_once = adapter.make_node(this_label, out_once, out_once);
        }

        out_never = adapter.make_node(this_label,
                                      out_never,
                                      is_rc ? out_once : out_never);
      }
    }
  }

  const size_t nodecount = adapter.nodecount(out_never);
  largest_bdd = std::max(largest_bdd, nodecount);
  total_nodes += nodecount;

  return out_never;
}

template<typename adapter_t>
void knights_tour_iter_ham(adapter_t &adapter, typename adapter_t::dd_t &paths)
{
  largest_bdd = 0;

  for (int r = 0; r < rows(); r++) {
    for (int c = 0; c < cols(); c++) {
      if (closed && is_closed_square(r,c)) { continue; }

      paths &= knights_tour_ham<adapter_t>(adapter, r, c);

      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;
    }
  }
}

// ========================================================================== //
enum iter_opt { SPLIT_OPEN, SPLIT_CLOSED, COMBINED_OPEN, COMBINED_CLOSED };

template<>
std::string option_help_str<iter_opt>()
{ return "Desired Variable ordering"; }

template<>
iter_opt parse_option(const std::string &arg, bool &should_exit)
{
  if (arg == "SPLIT_OPEN" || arg == "OPEN" || arg == "SPLIT")
  { return iter_opt::SPLIT_OPEN; }

  if (arg == "SPLIT_CLOSED" || arg == "CLOSED")
  { return iter_opt::SPLIT_CLOSED; }

  if (arg == "COMBINED_OPEN" || arg == "COMBINED")
  { return iter_opt::COMBINED_OPEN; }

  if (arg == "COMBINED_CLOSED")
  { return iter_opt::COMBINED_CLOSED; }

  ERROR("Undefined option: %s\n", arg.c_str());
  should_exit = true;

  return iter_opt::SPLIT_OPEN;
}

// ========================================================================== //

template<typename adapter_t>
void run_knights_tour(int argc, char** argv)
{
  iter_opt opt = iter_opt::SPLIT_OPEN; // Default strategy
  N = 12; // Default N value for a 6x6 sized chess board

  bool should_exit = parse_input(argc, argv, opt);
  if (should_exit) { exit(-1); }

  closed  = opt == iter_opt::SPLIT_CLOSED || opt == iter_opt::COMBINED_CLOSED;
  ham_rel = opt == iter_opt::COMBINED_OPEN || opt == iter_opt::COMBINED_CLOSED;

  // =========================================================================
  INFO("%i x %i - Knight's Tour (%s %i MiB):\n", rows(), cols(), adapter_t::NAME.c_str(), M);
  INFO("   | Tour type:              %s\n", closed ? "Closed tours only" : "Open (all) tours");
  INFO("   | Computation pattern:    Transitions %s Hamiltonian\n", ham_rel ? "||" : ";");

  if (rows() == 0 || cols() == 0) {
    INFO("\n  The board has no cells. Please provide an N > 1 (-N)\n");
    exit(0);
  }

  if (closed && (rows() < 3 || cols() < 3) && rows() != 1 && cols() != 1) {
    INFO("\n  There cannot exist closed tours on boards smaller than 3 x 3\n");
    INFO("  Aborting computation...\n");
    exit(0);
  }

  // ========================================================================
  // Initialise package manager
  time_point t_init_before = get_timestamp();
  adapter_t adapter(MAX_POSITION()+1);
  time_point t_init_after = get_timestamp();
  INFO("\n   %s initialisation:\n", adapter_t::NAME.c_str());
  INFO("   | time (ms):              %zu\n", duration_of(t_init_before, t_init_after));

  uint64_t solutions;
  {
    // ========================================================================
    // Compute the decision diagram that represents all hamiltonian paths
    time_point t1 = get_timestamp();

    if (ham_rel) {
      INFO("\n   Paths + Hamiltonian construction:\n");
    } else {
      INFO("\n   Paths construction:\n");
    }

    typename adapter_t::dd_t res = rows() == 1 && cols() == 1
      ? adapter.ithvar(int_of_position(0,0,0))
      : (ham_rel
         ? knights_tour_iter_rel<adapter_t, true>(adapter)
         : knights_tour_iter_rel<adapter_t, false>(adapter));

    time_point t2 = get_timestamp();

    const time_duration paths_time = duration_of(t1,t2);

    INFO("   | total no. nodes:        %zu\n", total_nodes);
    INFO("   | largest size (nodes):   %zu\n", largest_bdd);
    INFO("   | final size (nodes):     %zu\n", adapter.nodecount(res));
    INFO("   | time (ms):              %zu\n", paths_time);

    // ========================================================================
    // Hamiltonian constraints (if requested seperately)
    time_duration hamiltonian_time = 0;
    if (!ham_rel) {
      INFO("\n   Applying Hamiltonian constraints:\n");

      time_point t3 = get_timestamp();
      knights_tour_iter_ham(adapter, res);
      time_point t4 = get_timestamp();
      hamiltonian_time = duration_of(t3,t4);

      INFO("   | total no. nodes:        %zu\n", total_nodes);
      INFO("   | largest size (nodes):   %zu\n", largest_bdd);
      INFO("   | final size (nodes):     %zu\n", adapter.nodecount(res));
      INFO("   | time (ms):              %zu\n", hamiltonian_time);
    }

    // ========================================================================
    // Count number of solutions
    time_point t5 = get_timestamp();
    solutions = adapter.satcount(res);
    time_point t6 = get_timestamp();

    const time_duration counting_time = duration_of(t5,t6);

    INFO("\n   Counting solutions:\n");
    INFO("   | number of solutions:    %zu\n", solutions);
    INFO("   | time (ms):              %zu\n", counting_time);

    // ========================================================================
    INFO("\n   total time (ms):          %zu\n", paths_time + hamiltonian_time + counting_time);
  }

  adapter.print_stats();

  if (!closed && N < size(expected_knights_tour_open)
      && expected_knights_tour_open[N] != UNKNOWN && solutions != expected_knights_tour_open[N]) {
    EXIT(-1);
  }

  if (closed && N < size(expected_knights_tour_closed)
      && expected_knights_tour_closed[N] != UNKNOWN && solutions != expected_knights_tour_closed[N]) {
    EXIT(-1);
  }
  FLUSH();
}
