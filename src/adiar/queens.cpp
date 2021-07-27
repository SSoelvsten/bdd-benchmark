#include "../queens.cpp"

#include "package_mgr.h"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<>
adiar::bdd queens_S(adiar_mgr &/*mgr*/, uint64_t i, uint64_t j)
{
  adiar::node_file out;
  adiar::node_writer out_writer(out);

  uint64_t row = N - 1;
  adiar::ptr_t next = adiar::create_sink_ptr(true);

  do {
    uint64_t row_diff = std::max(row,i) - std::min(row,i);

    if (row_diff == 0) {
      // On row of the queen in question
      uint64_t column = N - 1;
      do {
        adiar::label_t label = label_of_position(row, column);

        // If (row, column) == (i,j), then the chain goes through high.
        if (column == j) {
          // Node to check whether the queen actually is placed, and if so
          // whether all remaining possible conflicts have to be checked.
          adiar::label_t label = label_of_position(i, j);
          adiar::node_t queen = adiar::create_node(label, 0, adiar::create_sink_ptr(false), next);

          out_writer << queen;
          next = queen.uid;
          continue;
        }

        adiar::node_t out_node = adiar::create_node(label, 0, next, adiar::create_sink_ptr(false));

        out_writer << out_node;
        next = out_node.uid;
      } while (column-- > 0);
    } else {
      // On another row
      if (j + row_diff < N) {
        // Diagonal to the right is within bounds
        adiar::label_t label = label_of_position(row, j + row_diff);
        adiar::node_t out_node = adiar::create_node(label, 0, next, adiar::create_sink_ptr(false));

        out_writer << out_node;
        next = out_node.uid;
      }

      // Column
      adiar::label_t label = label_of_position(row, j);
      adiar::node_t out_node = adiar::create_node(label, 0, next, adiar::create_sink_ptr(false));

      out_writer << out_node;
      next = out_node.uid;

      if (row_diff <= j) {
        // Diagonal to the left is within bounds
        adiar::label_t label = label_of_position(row, j - row_diff);
        adiar::node_t out_node = adiar::create_node(label, 0, next, adiar::create_sink_ptr(false));

        out_writer << out_node;
        next = out_node.uid;
      }
    }
  } while (row-- > 0);

  return out;
}

// ========================================================================== //
int main(int argc, char** argv)
{
  run_queens<adiar_mgr>(argc, argv);
}
