#include "../sat_queens.cpp"

#include "package_mgr.h"

// ========================================================================== //
int main(int argc, char** argv)
{
  run_sat_queens<cudd_mgr>(argc, argv);
}
