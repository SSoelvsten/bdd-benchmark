#include "../queens.cpp"

#include "zdd_adapter.h"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<>
adiar::zdd queens_S(adiar_zdd_adapter &/*mgr*/, int i, int j)
{
  adiar::node_file out;
  adiar::node_writer out_writer(out);
  adiar::ptr_t next = adiar::create_sink_ptr(true);

  for(int row = N-1; row >= 0; row--) {
    for(int col = N-1; col >= 0; col--) {
      //Same row or column
      if(i == row && j != col) { continue; }
      if(i != row && j == col) { continue; }

      adiar::label_t label = label_of_position(row, col);

      //Is the queen
      if(row == i && col == j) {
        adiar::ptr_t low = adiar::create_sink_ptr(false);
        adiar::ptr_t high = next;
        adiar::node_t out_node = adiar::create_node(label, 0, low, high);

        out_writer << out_node;
        next = out_node.uid;
        continue;
      }

      //Diagonal
      int row_diff = std::abs(row - i);
      int col_diff = std::abs(col - j);
      if(col_diff == row_diff) { continue; }

      //Not in conflict
      adiar::node_t out_node = adiar::create_node(label, 0, next, next);

      out_writer << out_node;
      next = out_node.uid;
    }
  }
  return out;
}

// ========================================================================== //
int main(int argc, char** argv)
{
  run_queens<adiar_zdd_adapter>(argc, argv);
}
