#include "queens.cpp"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<typename adapter_t>
typename adapter_t::dd_t queens_S(adapter_t &adapter, int i, int j)
{
  const auto terminal_F = adapter.build_node(false);
  const auto terminal_T = adapter.build_node(true);

  auto latest = terminal_T;

  for (int row = N-1; row >= 0; row--) {
    int row_diff = std::max(row, i) - std::min(row, i);

    if (row_diff == 0) {
      for (int column = N-1; column >= 0; column--) {
        int label = label_of_position(row, column);

        if (column == j) {
          latest = adapter.build_node(label, terminal_F, latest);
        } else {
          latest = adapter.build_node(label, latest, terminal_F);
        }
      }
    } else {
      if (j + row_diff < N) {
        int label = label_of_position(row, j + row_diff);
        latest = adapter.build_node(label, latest, terminal_F);
      }

      int label = label_of_position(row, j);
      latest = adapter.build_node(label, latest, terminal_F);

      if (row_diff <= j) {
        int label = label_of_position(row, j - row_diff);
        latest = adapter.build_node(label, latest, terminal_F);
      }
    }
  }

  typename adapter_t::dd_t out = adapter.build();
  total_nodes += adapter.nodecount(out);
  return out;
}
