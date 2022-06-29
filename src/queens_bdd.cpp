#include "queens.cpp"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<typename adapter_t>
typename adapter_t::dd_t queens_S(adapter_t &adapter, int i, int j)
{
  int row = N - 1;

  typename adapter_t::dd_t out = adapter.leaf_true();

  for (int row = N-1; row >= 0; row--) {
    int row_diff = std::max(row, i) - std::min(row, i);

    if (row_diff == 0) {
      for (int column = N-1; column >= 0; column--) {
        int label = label_of_position(row, column);

        if (column == j) {
          out &= adapter.ithvar(label);
        } else {
          out &= adapter.nithvar(label);
        }
      }
    } else {
      if (j + row_diff < N) {
        int label = label_of_position(row, j + row_diff);
        out &= adapter.nithvar(label);
      }

      int label = label_of_position(row, j);
      out &= adapter.nithvar(label);

      if (row_diff <= j) {
        int label = label_of_position(row, j - row_diff);
        out &= adapter.nithvar(label);
      }
    }
  }

  total_nodes += adapter.nodecount(out);

  return out;
}
