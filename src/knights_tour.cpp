#include "common.cpp"
#include "expected.h"

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

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

inline std::string pos_to_string(int r, int c)
{
  std::stringstream ss;
  ss << (r+1) << (char) ('A'+c);
  return ss.str();
}

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
typename adapter_t::dd_t knights_tour_rel(adapter_t &adapter, int t);

// ========================================================================== //
//                    Iterate over the above Transition Relation              //
bool closed = false;

template<typename adapter_t>
typename adapter_t::dd_t knights_tour_iter_rel(adapter_t &adapter)
{
  // Reset 'largest_bdd'
#ifdef BDD_BENCHMARK_STATS
  largest_bdd = 0;
#endif // BDD_BENCHMARK_STATS

  int t = MAX_TIME()-1;

  // Initial aggregator value at final time step
  typename adapter_t::dd_t res = closed
    ? knights_tour_closed<adapter_t>(adapter)
    : knights_tour_rel<adapter_t>(adapter, t);

#ifdef BDD_BENCHMARK_STATS
  std::cout << "   | [t = " << t << "] : ??? DD nodes\n"; // TODO
#endif // BDD_BENCHMARK_STATS

  // Go backwards in time, aggregating all legal paths
  for (; closed <= t ; t--) {
    res &= knights_tour_rel<adapter_t>(adapter, t);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(res);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "   | [t = " << t << "] : " << nodecount<< " DD nodes\n";
#endif // BDD_BENCHMARK_STATS
  }

#ifdef BDD_BENCHMARK_STATS
  std::cout << "   |\n";
#endif // BDD_BENCHMARK_STATS
  return res;
}

// ========================================================================== //
//                            Add Hamiltonian constraints                     //
template<typename adapter_t>
typename adapter_t::dd_t knights_tour_ham(adapter_t &adapter, int r, int c);

template<typename adapter_t>
void knights_tour_iter_ham(adapter_t &adapter, typename adapter_t::dd_t &paths)
{
  // Reset 'largest_bdd'
#ifdef BDD_BENCHMARK_STATS
  largest_bdd = 0;
#endif // BDD_BENCHMARK_STATS

  // Add hamiltonian constraints
  for (int r = 0; r < rows(); r++) {
    for (int c = 0; c < cols(); c++) {
      if (closed && is_closed_square(r,c)) { continue; }

      paths &= knights_tour_ham<adapter_t>(adapter, r, c);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << "   | " << pos_to_string(r,c) << " : %zu DD nodes\n" << nodecount;
#endif // BDD_BENCHMARK_STATS
    }
  }
#ifdef BDD_BENCHMARK_STATS
  std::cout << "   |\n";
#endif // BDD_BENCHMARK_STATS
}

// ========================================================================== //
enum iter_opt { OPEN, CLOSED };

template<>
std::string option_help_str<iter_opt>()
{ return "Desired Variable ordering"; }

template<>
iter_opt parse_option(const std::string &arg, bool &should_exit)
{
  if (arg == "OPEN" || arg == "O")
  { return iter_opt::OPEN; }

  if (arg == "CLOSED" || arg == "C")
  { return iter_opt::CLOSED; }

  std::cerr << "Undefined option: " << arg << "\n";
  should_exit = true;

  return iter_opt::OPEN;
}

// ========================================================================== //

template<typename adapter_t>
int run_knights_tour(int argc, char** argv)
{
  iter_opt opt = iter_opt::OPEN; // Default strategy
  N = 12; // Default N value for a 6x6 sized chess board

  bool should_exit = parse_input(argc, argv, opt);
  if (should_exit) { return -1; }

  closed  = opt == iter_opt::CLOSED;

  // =========================================================================
  std::cout << rows() << " x " << cols() << " - Knight's Tour (" << adapter_t::NAME << " " << M << " MiB):\n"
            << "   | Tour type:              " << (closed ? "Closed tours only" : "Open (all) tours") << "\n";

  if (rows() == 0 || cols() == 0) {
    std::cout << "\n"
              << "  The board has no cells. Please provide an N > 1 (-N)\n";
    return 0;
  }

  if (closed && (rows() < 3 || cols() < 3) && (rows() != 1 || cols() != 1)) {
    std::cout << "\n"
              << "  There cannot exist closed tours on boards smaller than 3 x 3\n"
              << "  Aborting computation...\n";
    return 0;
  }

  // ========================================================================
  // Initialise package manager
  time_point t_init_before = get_timestamp();
  adapter_t adapter(MAX_POSITION()+1);
  time_point t_init_after = get_timestamp();

  std::cout << "\n   " << adapter_t::NAME << " initialisation:\n"
            << "   | time (ms):              " << duration_of(t_init_before, t_init_after)
            << std::flush;

  uint64_t solutions;
  {
    // ========================================================================
    // Compute the decision diagram that represents all hamiltonian paths
    std::cout << "\n"
              << "   Paths construction:\n"
              << std::flush;

    const time_point t1 = get_timestamp();

    typename adapter_t::dd_t res = rows() == 1 && cols() == 1
      ? adapter.ithvar(int_of_position(0,0,0))
      : knights_tour_iter_rel<adapter_t>(adapter);

    const time_point t2 = get_timestamp();

    const time_duration paths_time = duration_of(t1,t2);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "   | total no. nodes:        " << total_nodes << "\n"
              << "   | largest size (nodes):   " << largest_bdd << "\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "   | final size (nodes):     " << adapter.nodecount(res) << "\n"
              << "   | time (ms):              " << paths_time << "\n"
              << std::flush;

    // ========================================================================
    // Hamiltonian constraints
    std::cout << "\n"
              << "  Applying Hamiltonian constraints:\n"
              << std::flush;

    const time_point t3 = get_timestamp();

    knights_tour_iter_ham(adapter, res);

    const time_point t4 = get_timestamp();

    const time_duration hamiltonian_time = duration_of(t3,t4);

#ifdef BDD_BENCHMARK_STATS
      std::cout << "   | total no. nodes:        " << total_nodes << "\n"
                << "   | largest size (nodes):   " << largest_bdd << "\n";
#endif // BDD_BENCHMARK_STATS
      std::cout << "   | final size (nodes):     " << adapter.nodecount(res) << "\n"
                << "   | time (ms):              " << hamiltonian_time << "\n"
                << std::flush;

    // ========================================================================
    // Count number of solutions
    const time_point t5 = get_timestamp();
    solutions = adapter.satcount(res);
    const time_point t6 = get_timestamp();

    const time_duration counting_time = duration_of(t5,t6);

    std::cout << "\n"
              << "   Counting solutions:\n"
              << "   | number of solutions:    " << solutions << "\n"
              << "   | time (ms):              " << counting_time << "\n"
              << std::flush;

    // ========================================================================
    std::cout << "\n"
              << "total time (ms):          " << (paths_time + hamiltonian_time + counting_time) << "\n"
              << std::flush;
  }

  adapter.print_stats();

  if (!closed && N < size(expected_knights_tour_open)
      && expected_knights_tour_open[N] != UNKNOWN && solutions != expected_knights_tour_open[N]) {
    return -1;
  }

  if (closed && N < size(expected_knights_tour_closed)
      && expected_knights_tour_closed[N] != UNKNOWN && solutions != expected_knights_tour_closed[N]) {
    return -1;
  }
  return 0;
}
