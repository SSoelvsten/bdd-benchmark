#include "common.cpp"

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

// TODO: Define 'knights_tour_rel(t)' for regular DFS implementations.
template<typename adapter_t>
typename adapter_t::dd_t knights_tour_rel(adapter_t &adapter, int t);

// ========================================================================== //
//                    Iterate over the above Transition Relation              //
template<typename adapter_t>
typename adapter_t::dd_t knights_tour_iter(adapter_t &adapter)
{
  typename adapter_t::dd_t res = knights_tour_rel<adapter_t>(adapter, MAX_TIME()-1);

  for (int t = MAX_TIME()-2; t >= 0; t--) {
    res &= knights_tour_rel<adapter_t>(adapter, t);

    const size_t nodecount = adapter.nodecount(res);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
  }

  return res;
}

// ========================================================================== //
template<typename adapter_t>
void run_knights_tour(int argc, char** argv)
{
  no_variable_order variable_order = no_variable_order::NO_ORDERING;
  N = 8; // Default N value
  bool should_exit = parse_input(argc, argv, variable_order);
  if (should_exit) { exit(-1); }

  if (rows() == 0 || cols() == 0) {
    ERROR("  Please provide an N > 1 (-N)\n");
    exit(-1);
  }

  // =========================================================================
  INFO("%i x %i - Knight's Tour (%s %i MiB):\n", rows(), cols(), adapter_t::NAME.c_str(), M);

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

    typename adapter_t::dd_t res = rows() == 1 && cols() == 1
      ? adapter.ithvar(int_of_position(0,0,0))
      : knights_tour_iter(adapter);

    time_point t2 = get_timestamp();

    const auto construction_time = duration_of(t1,t2);

    INFO("\n   Decision diagram construction:\n");
    INFO("   | total no. nodes:        %zu\n", total_nodes);
    INFO("   | largest size (nodes):   %zu\n", largest_bdd);
    INFO("   | final size (nodes):     %zu\n", adapter.nodecount(res));
    INFO("   | time (ms):              %zu\n", construction_time);

    // ========================================================================
    // Count number of solutions
    time_point t3 = get_timestamp();
    solutions = adapter.satcount(res);
    time_point t4 = get_timestamp();

    const auto counting_time = duration_of(t3,t4);

    INFO("\n   Counting solutions:\n");
    INFO("   | number of solutions:    %zu\n", solutions);
    INFO("   | time (ms):              %zu\n", counting_time);

    // ========================================================================
    INFO("\n   total time (ms):          %zu\n", construction_time + counting_time);
  }

  adapter.print_stats();

  /* TODO
  if (N < size(expected_queens) && solutions != expected_queens[N]) {
    EXIT(-1);
  }
  */
  FLUSH();
}
