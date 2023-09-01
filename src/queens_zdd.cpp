#include "queens.cpp"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<typename adapter_t>
typename adapter_t::dd_t queens_S(adapter_t &adapter, int i, int j)
{
  auto next = adapter.build_node(true);

  for(int row = MAX_ROW(); row >= 0; row--) {
    for(int col = MAX_COL(); col >= 0; col--) {
      // Same row or column
      if(i == row && j != col) { continue; }
      if(i != row && j == col) { continue; }

      const int label = label_of_position(row, col);

      // Is the queen
      if(row == i && col == j) {
        auto low = adapter.build_node(false);
        auto high = next;
        next = adapter.build_node(label, low, high);

        continue;
      }

      // Diagonal
      const int row_diff = std::abs(row - i);
      const int col_diff = std::abs(col - j);
      if(col_diff == row_diff) { continue; }

      // Not in conflict
      next = adapter.build_node(label, next, next);
    }
  }

  typename adapter_t::dd_t out = adapter.build();
#ifdef BDD_BENCHMARK_STATS
  total_nodes += adapter.nodecount(out);
#endif // BDD_BENCHMARK_STATS
  return out;
}
