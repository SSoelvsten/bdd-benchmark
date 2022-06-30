#include "knights_tour.cpp"

// ========================================================================== //
//                          Closed Tour Constraints                           //

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
template<typename adapter_t>
void __knights_tour_rel__post_chain__simp(adapter_t &adapter,
                                          std::vector<typename adapter_t::dd_t> &post_chains,
                                          int time, int row, int col)
{
  const int this_label = int_of_position(row, col, time);

  const typename adapter_t::dd_t res =
    adapter.make_node(this_label, post_chains.at(0), post_chains.at(0));

  for (int idx = 0; idx < rows() * cols(); idx++) {
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
  std::vector<typename adapter_t::dd_t> post_chains(cols() * rows(), adapter.leaf_true());

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
  std::vector<typename adapter_t::dd_t> to_chains(cols() * rows(), adapter.leaf_false());

  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      const int this_label = int_of_position(row, col, t+1);

      for (int row_t = MAX_ROW(); row_t >= 0; row_t--) {
        for (int col_t = MAX_COL(); col_t >= 0; col_t--) {
          if (!is_reachable(row_t, col_t)) { continue; }

          if (!is_legal_move(row_t, col_t, row, col)) { continue; }

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
      if (!is_reachable(row, col)) { continue; }

      const int this_label = int_of_position(row, col, t);
      const int to_chain_idx = int_of_position(row, col);
      root = adapter.make_node(this_label, root, to_chains.at(to_chain_idx));
    }
  }

  // Time-step t' < t:
  //   Just allow everything, i.e. add no constraints
  for (int pos = int_of_position(MAX_ROW(), MAX_COL(), t-1); pos >= 0; pos--) {
    root = adapter.make_node(pos, root, root);
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
