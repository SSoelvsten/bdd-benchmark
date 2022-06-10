#include "../queens.cpp"

#include "zdd_adapter.h"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<>
ZDD queens_S(cudd_zdd_adapter &mgr, int i, int j)
{
  ZDD next = mgr.leaf_true();

  for(int row = N-1; row >= 0; row--) {
    for(int col = N-1; col >= 0; col--) {
      //Same row or column
      if(i == row && j != col) { continue; }
      if(i != row && j == col) { continue; }

      const int label = label_of_position(row, col);

      //Is the queen
      if(row == i && col == j) {
        const ZDD low = mgr.leaf_false();
        const ZDD high = next;
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

// ========================================================================== //
int main(int argc, char** argv)
{
  run_queens<cudd_zdd_adapter>(argc, argv);
}
