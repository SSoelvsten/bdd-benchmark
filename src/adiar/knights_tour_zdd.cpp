#include "../knights_tour.cpp"

#include "zdd_adapter.h"

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

template<>
adiar::zdd knights_tour_rel(adiar_zdd_adapter &/*adapter*/, int t) {
  INFO("t = %i\n", t);

  adiar::node_file out;
  adiar::node_writer out_writer(out);

  // Time steps t' > t+1:
  //   Hamiltonian constraint chain for each position reached at time step 't+1'
  //   given some position at time step 't'.
  //
  //   TODO: We do not share subtrees at the bottom-most level, despite the last
  //   time step definitely does share the last few variables. To fix this, we
  //   need to collapse to the '0' chain_id if beyond (row_t,col_t).
  for (int time = MAX_TIME(); time > t+1; time--) {
    for (int row = MAX_ROW(); row >= 0; row--) {
      for (int col = MAX_COL(); col >= 0; col--) {
        for (int row_t = MAX_ROW(); row_t >= 0; row_t--) {
          for (int col_t = MAX_COL(); col_t >= 0; col_t--) {
            // This position matches (row_t, col_t)? Skip it to make this chain
            // enforce it being a hamiltonian path.
            if (row_t == row && col_t == col) { continue; }

            const adiar::label_t this_label = int_of_position(row, col, time);
            const adiar::id_t this_id = int_of_position(row_t, col_t);

            // Find next label, that does match (row_t, col_t)
            const int this_conflict = int_of_position(row_t, col_t, time);
            const int next_conflict = int_of_position(row_t, col_t, time+1);

            int next_label = this_label+1;
            if (next_label == this_conflict) { next_label++; }
            if (next_label == next_conflict) { next_label++; }

            const adiar::ptr_t child = next_label > MAX_POSITION()
              ? adiar::create_sink_ptr(true)
              : adiar::create_node_ptr(next_label, this_id);

            out_writer << adiar::create_node(this_label, this_id, child, child);
          }
        }
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

          const adiar::label_t this_label = int_of_position(row, col, t+1);
          const adiar::id_t chain_id = int_of_position(row_t, col_t);

          const adiar::ptr_t next_this_chain = next_legal(row_t, col_t, row, col, t+1);

          int hamiltonian_legal_root = int_of_position(0, 0, t+2);
          if (row_t == 0 && col_t == 0) { hamiltonian_legal_root++; }

          const adiar::ptr_t hamiltonian_root = t+1 == MAX_TIME()
            ? adiar::create_sink_ptr(true)
            : adiar::create_node_ptr(hamiltonian_legal_root, chain_id);

          out_writer << adiar::create_node(this_label, chain_id,
                                           next_this_chain,
                                           hamiltonian_root);
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

  const size_t nodecount = out_writer.size();
  largest_bdd = std::max(largest_bdd, nodecount);
  total_nodes += nodecount;

  return out;
}

template<>
adiar::zdd knights_tour_rel(adiar_zdd_adapter &/*adapter*/, int t)
{
  return __knights_tour_rel<false>(t);
}

template<>
adiar::zdd knights_tour_ham_rel(adiar_zdd_adapter &/*adapter*/, int t)
{
  return __knights_tour_rel<true>(t);
}


// ========================================================================== //
int main(int argc, char** argv)
{
  run_knights_tour<adiar_zdd_adapter>(argc, argv);
}
