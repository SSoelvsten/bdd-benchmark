#include "knights_tour.cpp"

// ========================================================================== //
//                            Life Time Statistics                            //
#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

// ========================================================================== //
//                          Closed Tour Constraints                           //

template<typename adapter_t>
typename adapter_t::dd_t knights_tour_closed(adapter_t &adapter)
{
  // Fix t = MAX_TIME() to be (1,2)
  const int stepMax_position = int_of_position(closed_squares[2][0],
                                               closed_squares[2][1],
                                               MAX_TIME());

  auto root = adapter.build_node(stepMax_position,
                                 adapter.build_node(false),
                                 adapter.build_node(true));

  // All in between is as-is but takes the hamiltonian constraint into account.
  for (int t = MAX_TIME() - 1; t > 1; t--) {
    for (int r = MAX_ROW(); r >= 0; r--) {
      for (int c = MAX_COL(); c >= 0; c--) {
        if (is_closed_square(r,c)) { continue; }

        root = adapter.build_node(int_of_position(r,c,t), root, root);
      }
    }
  }

  // Fix t = 1 to be (2,1)
  const int step1_position = int_of_position(closed_squares[1][0],
                                             closed_squares[1][1],
                                             1);
  root = adapter.build_node(step1_position, adapter.build_node(false), root);

  // Fix t = 0 to be (0,0)
  const int step0_position = int_of_position(closed_squares[0][0],
                                             closed_squares[0][1],
                                             0);
  root = adapter.build_node(step0_position, adapter.build_node(false), root);

  typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
  const size_t nodecount = adapter.nodecount(out);
  largest_bdd = std::max(largest_bdd, nodecount);
  total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

  return out;
}

// ========================================================================== //
//                               Transition Relation                          //
template<typename adapter_t>
typename adapter_t::dd_t knights_tour_rel(adapter_t &adapter, int t)
{
  std::vector<typename adapter_t::build_node_t> post_chains(cols() * rows(), adapter.build_node(true));

  // Time steps t' > t+1:
  for (int time = MAX_TIME(); time > t+1; time--) {
    for (int row = MAX_ROW(); row >= 0; row--) {
      for (int col = MAX_COL(); col >= 0; col--) {
        if (!is_reachable(row, col)) { continue; }

        const int this_label = int_of_position(row, col, time);

        const auto res =
          adapter.build_node(this_label, post_chains.at(0), post_chains.at(0));

        for (int idx = 0; idx < rows() * cols(); idx++) {
          post_chains.at(idx) = res;
        }
      }
    }
  }

  // Time step t+1:
  //   Chain with each possible position reachable from some position at time 't'.
  std::vector<typename adapter_t::build_node_t> to_chains(cols() * rows(), adapter.build_node(false));

  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      const int this_label = int_of_position(row, col, t+1);

      for (int row_t = MAX_ROW(); row_t >= 0; row_t--) {
        for (int col_t = MAX_COL(); col_t >= 0; col_t--) {
          if (!is_reachable(row_t, col_t)) { continue; }

          if (!is_legal_move(row_t, col_t, row, col)) { continue; }

          const int vector_idx = int_of_position(row_t, col_t);

          to_chains.at(vector_idx) = adapter.build_node(this_label,
                                                        to_chains.at(vector_idx),
                                                        post_chains.at(vector_idx));
        }
      }
    }
  }

  // Time step t:
  //   For each position at time step 't', check whether we are "here" and go to
  //   the chain checking "where we go to" at 't+1'.
  auto root = adapter.build_node(false);

  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      if (!is_reachable(row, col)) { continue; }

      const int this_label = int_of_position(row, col, t);
      const int to_chain_idx = int_of_position(row, col);
      root = adapter.build_node(this_label, root, to_chains.at(to_chain_idx));
    }
  }

  // Time-step t' < t:
  //   Just allow everything, i.e. add no constraints
  for (int pos = int_of_position(MAX_ROW(), MAX_COL(), t-1); pos >= 0; pos--) {
    root = adapter.build_node(pos, root, root);
  }

  typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
  const size_t nodecount = adapter.nodecount(out);
  largest_bdd = std::max(largest_bdd, nodecount);
  total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

  return out;
}

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
typename adapter_t::dd_t knights_tour_ham(adapter_t &adapter, int r, int c)
{
  auto out_once = adapter.build_node(true);
  auto out_never = adapter.build_node(false);

  for (int this_t = MAX_TIME(); this_t >= 0; this_t--) {
    for (int this_r = MAX_ROW(); this_r >= 0; this_r--) {
      for (int this_c = MAX_COL(); this_c >= 0; this_c--) {
        const int this_label = int_of_position(this_r, this_c, this_t);
        const bool is_rc = r == this_r && c == this_c;

        if (!is_rc && (this_t > 0 || this_r > r)) {
          out_once = adapter.build_node(this_label, out_once, out_once);
        }

        out_never = adapter.build_node(this_label,
                                       out_never,
                                       is_rc ? out_once : out_never);
      }
    }
  }

  typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
  const size_t nodecount = adapter.nodecount(out);
  largest_bdd = std::max(largest_bdd, nodecount);
  total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

  return out;
}

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
