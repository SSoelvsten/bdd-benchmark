#include "../knights_tour.cpp"

#include "zdd_adapter.h"

// ========================================================================== //
//                          Closed Tour Constraints                           //
template<>
adiar::zdd knights_tour_closed(adiar_zdd_adapter &/*adapter*/)
{
  adiar::node_file out;
  adiar::node_writer out_writer(out);

  // Fix t = MAX_TIME() to be (1,2)
  const int stepMax_position = int_of_position(closed_squares[2][0], closed_squares[2][1], MAX_TIME());
  const adiar::node_t stepMax_state = adiar::create_node(stepMax_position, 0,
                                                         adiar::create_sink_ptr(false),
                                                         adiar::create_sink_ptr(true));
  out_writer << stepMax_state;

  adiar::ptr_t root = stepMax_state.uid;

  // All in between is as-is but takes the hamiltonian constraint into account.
  for (int t = MAX_TIME() - 1; t > 1; t--) {
    for (int r = MAX_ROW(); r >= 0; r--) {
      for (int c = MAX_COL(); c >= 0; c--) {
        if (is_closed_square(r,c)) { continue; }

        const adiar::node_t n = adiar::create_node(int_of_position(r,c,t), 0, root, root);
        out_writer << n;

        root = n.uid;
      }
    }
  }

  // Fix t = 1 to be (2,1)
  const int step1_position = int_of_position(closed_squares[1][0], closed_squares[1][1], 1);
  const adiar::node_t step1_state = adiar::create_node(step1_position, 0, root, root);
  out_writer << step1_state;

  root = step1_state.uid;

  // Fix t = 0 to be (0,0)
  const int step0_position = int_of_position(closed_squares[0][0], closed_squares[0][1], 0);
  const adiar::node_t step0_state = adiar::create_node(step0_position, 0, root, root);
  out_writer << step0_state;

  // const size_t nodecount = out_writer.size();
  // largest_bdd = std::max(largest_bdd, nodecount);
  // total_nodes += nodecount;

  return out;
}

// ========================================================================== //
//                 Transition Relation + Hamiltonian Constraint               //
adiar::ptr_t first_legal(int r_from, int c_from, int t)
{
  for (int idx = 0; idx < 8; idx++) {
    const int r_to = r_from + row_moves[idx];
    const int c_to = c_from + column_moves[idx];

    if (!is_legal_position(r_to, c_to)) { continue; }

    const adiar::label_t to_label = int_of_position(r_to, c_to, t);
    const adiar::label_t chain_id = int_of_position(r_from, c_from);

    return adiar::create_node_ptr(to_label, chain_id);
  }
  return adiar::create_sink_ptr(false);
}

adiar::ptr_t next_legal(int r_from, int c_from, int r_to, int c_to, int t)
{
  bool seen_move = false;

  for (int idx = 0; idx < 8; idx++) {
    const int r = r_from + row_moves[idx];
    const int c = c_from + column_moves[idx];

    if (!is_legal_position(r,c)) { continue; }

    if (seen_move) {
      const adiar::label_t to_label = int_of_position(r, c, t);
      const adiar::id_t chain_id = int_of_position(r_from, c_from);

      return adiar::create_node_ptr(to_label, chain_id);
    }
    seen_move |= r == r_to && c == c_to;
  }
  return adiar::create_sink_ptr(false);
}

template<bool incl_hamiltonian>
inline void __knights_tour_rel__post_chain(adiar::node_writer &out_writer,
                                           int time, int row, int col);

template<>
inline void __knights_tour_rel__post_chain<true>(adiar::node_writer &out_writer,
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
      if (time == MAX_TIME() && this_label > this_conflict && !(row_t == 0 && col_t == 0)) {
        continue;
      }

      const int this_id = int_of_position(row_t, col_t);

      // Next cell on board this time step that does match (row_t,col_t).
      // Possibly loops back to (0,0) at the next time step.
      //
      // For the id, we will collapse into the 0-chain if we are past
      // the final time to check for the hamiltonian constraint.
      int next_label = next_reachable_position(row, col, time);
      if (next_label == this_conflict) { next_label++; }
      if (next_label == next_conflict) { next_label++; }
      if (!is_reachable(row_of_position(next_label), col_of_position(next_label))) { next_label++; }

      const int next_id = (MAX_TIME() == time && next_label > this_conflict)
        || (MAX_TIME()-1 == time && next_label > next_conflict)
        ? 0
        : this_id;

      const adiar::ptr_t child = next_label > MAX_POSITION()
        ? adiar::create_sink_ptr(true)
        : adiar::create_node_ptr(next_label, next_id);

      out_writer << adiar::create_node(this_label, this_id, child, child);
    }
  }
}

template<>
inline void __knights_tour_rel__post_chain<false>(adiar::node_writer &out_writer,
                                                  int time, int row, int col)
{
  // Simple "don't care" chain
  const int this_label = int_of_position(row, col, time);
  const int max_reachable = MAX_POSITION();
  const int next_reachable = next_reachable_position(row, col, time);

  const adiar::ptr_t next_ptr = this_label == max_reachable
    ? adiar::create_sink_ptr(true)
    : adiar::create_node_ptr(next_reachable, 0);

  out_writer << adiar::create_node(this_label, 0, next_ptr, next_ptr);
}

template<bool incl_hamiltonian>
inline adiar::ptr_t __knights_tour_rel__post_root(int t, int row, int col, int row_t, int col_t);

template<>
inline adiar::ptr_t __knights_tour_rel__post_root<true>(int t, int /*row*/, int /*col*/, int row_t, int col_t)
{
  int hamiltonian_legal_root = int_of_position(0, 0, t+2);
  if (row_t == 0 && col_t == 0) { hamiltonian_legal_root++; }

  const int hamiltonian_legal_id = hamiltonian_legal_root > int_of_position(row_t, col_t, MAX_TIME())
    ? 0
    : int_of_position(row_t, col_t); // chain_id;

  return t+1 == MAX_TIME()
    ? adiar::create_sink_ptr(true)
    : adiar::create_node_ptr(hamiltonian_legal_root, hamiltonian_legal_id);
}

template<>
inline adiar::ptr_t __knights_tour_rel__post_root<false>(int t, int /*row*/, int /*col*/, int /*row_t*/, int /*col_t*/)
{
  return t+1 == MAX_TIME()
    ? adiar::create_sink_ptr(true)
    : adiar::create_node_ptr(int_of_position(0,0,t+2), 0);
}

template<bool incl_hamiltonian>
inline adiar::zdd knights_tour_rel(int t)
{
  adiar::node_file out;
  adiar::node_writer out_writer(out);

  // Time steps t' > t+1:
  for (int time = MAX_TIME(); time > t+1; time--) {
    for (int row = MAX_ROW(); row >= 0; row--) {
      for (int col = MAX_COL(); col >= 0; col--) {
        if (!is_reachable(row, col)) { continue; }

        __knights_tour_rel__post_chain<incl_hamiltonian>(out_writer, time, row, col);
      }
    }
  }

  // Time-step t+1:
  //   Chain with each possible position reachable from some position at time 't'.
  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      for (int row_t = MAX_ROW(); row_t >= 0; row_t--) {
        for (int col_t = MAX_COL(); col_t >= 0; col_t--) {
          if (!is_legal_move(row_t, col_t, row, col)) { continue; }

          const int this_label = int_of_position(row, col, t+1);
          const int chain_id = int_of_position(row_t, col_t);

          const adiar::ptr_t next_this_chain = next_legal(row_t, col_t, row, col, t+1);
          const adiar::ptr_t chain_root = __knights_tour_rel__post_root<incl_hamiltonian>(t, row, col, row_t, col_t);

          out_writer << adiar::create_node(this_label, chain_id, next_this_chain, chain_root);
        }
      }
    }
  }

  // Time-step t:
  //   For each position at time step 't', check whether we are "here" and go to
  //   the chain checking "where we go to" at 't+1'.
  adiar::ptr_t root = adiar::create_sink_ptr(false);

  for (int row = MAX_ROW(); row >= 0; row--) {
    for (int col = MAX_COL(); col >= 0; col--) {
      const adiar::label_t this_label = int_of_position(row, col, t);
      const adiar::ptr_t move_chain = first_legal(row, col, t+1);

      const adiar::node_t n = adiar::create_node(this_label, 0, root, move_chain);
      root = n.uid;
      out_writer << n;
   }
  }

  // Time-step t' < t:
  //   Just allow everything, i.e. add no constraints
  if (t > 0) {
    for (int pos = int_of_position(MAX_ROW(), MAX_COL(), t-1); pos >= 0; pos--) {
      adiar::node_t n = adiar::create_node(pos, 0, root, root);
      root = n.uid;
      out_writer << n;
    }
  }

  // const size_t nodecount = out_writer.size();
  // largest_bdd = std::max(largest_bdd, nodecount);
  // total_nodes += nodecount;

  return out;
}

template<>
adiar::zdd knights_tour_rel<adiar_zdd_adapter, false>(adiar_zdd_adapter &/*adapter*/, int t)
{
  const adiar::zdd res = knights_tour_rel<false>(t);
  return res;
}

template<>
adiar::zdd knights_tour_rel<adiar_zdd_adapter, true>(adiar_zdd_adapter &/*adapter*/, int t)
{
  const adiar::zdd res = knights_tour_rel<true>(t);
  return res;
}

// ========================================================================== //
template<>
adiar::zdd knights_tour_ham(adiar_zdd_adapter &/*adapter*/, int r, int c)
{
  adiar::node_file out;
  adiar::node_writer out_writer(out);

  adiar::ptr_t root_never = adiar::create_sink_ptr(false);
  adiar::ptr_t root_once = adiar::create_sink_ptr(true);

  for (int this_t = MAX_TIME(); this_t >= 0; this_t--) {
    for (int this_r = MAX_ROW(); this_r >= 0; this_r--) {
      for (int this_c = MAX_COL(); this_c >= 0; this_c--) {
        int this_label = int_of_position(this_r, this_c, this_t);

        bool is_rc = r == this_r && c == this_c;

        if (!is_rc && (this_t > 0 || this_r > r)) {
          adiar::node_t out_once = adiar::create_node(this_label, 1, root_once, root_once);
          out_writer << out_once;
          root_once = out_once.uid;
        }

        adiar::node_t out_never = adiar::create_node(this_label, 0,
                                                     root_never,
                                                     is_rc ? root_once : root_never);
        out_writer << out_never;
        root_never = out_never.uid;
      }
    }
  }

  // const size_t nodecount = out_writer.size();
  // largest_bdd = std::max(largest_bdd, nodecount);
  // total_nodes += nodecount;

  return out;
}

// ========================================================================== //
int main(int argc, char** argv)
{
  run_knights_tour<adiar_zdd_adapter>(argc, argv);
}
