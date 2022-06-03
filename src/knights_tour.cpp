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

// TODO: Define 'knights_tour_closed()' for regular DFS implementations.
template<typename adapter_t>
typename adapter_t::dd_t knights_tour_closed(adapter_t &adapter);

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
  for (int r_from = 0; r_from < rows(); r_from++) {
    for (int c_from = 0; c_from < cols(); c_from++) {
      if (is_legal_move(r_from, c_from, r, c)) {
        return true;
      }
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

// TODO: Define 'knights_tour_rel(t)' for regular DFS implementations.
template<typename adapter_t, bool include_hamiltonian_constraint>
typename adapter_t::dd_t knights_tour_rel(adapter_t &adapter, int t);

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
    : knights_tour_rel<adapter_t, incl_hamiltonian>(adapter, t);

  while (t-- > closed) {
    res &= knights_tour_rel<adapter_t, incl_hamiltonian>(adapter, t);

    const size_t nodecount = adapter.nodecount(res);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
  }

  return res;
}

// ========================================================================== //
//                            Add Hamiltonian constraints                     //

// TODO: Define 'knights_tour_ham(t)' for regular DFS implementations.
template<typename adapter_t>
typename adapter_t::dd_t knights_tour_ham(adapter_t &adapter, int r, int c);

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
