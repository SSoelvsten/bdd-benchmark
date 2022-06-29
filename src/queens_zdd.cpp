#include "queens.cpp"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<typename adapter_t>
typename adapter_t::dd_t queens_S(adapter_t &mgr, int i, int j)
{
  typename adapter_t::dd_t next = mgr.leaf_true();

  for(int row = N-1; row >= 0; row--) {
    for(int col = N-1; col >= 0; col--) {
      //Same row or column
      if(i == row && j != col) { continue; }
      if(i != row && j == col) { continue; }

      const int label = label_of_position(row, col);

      //Is the queen
      if(row == i && col == j) {
        typename adapter_t::dd_t low = mgr.leaf_false();
        typename adapter_t::dd_t high = next;
        next = mgr.make_node(label, low, high);

        continue;
      }

      //Diagonal
      const int row_diff = std::abs(row - i);
      const int col_diff = std::abs(col - j);
      if(col_diff == row_diff) { continue; }

      //Not in conflict
      next = mgr.make_node(label, next, next);
    }
  }
  return next;
}
