#include "../sat_pigeonhole_principle.cpp"

#include "package_mgr.h"

// ========================================================================== //
int main(int argc, char** argv)
{
  run_sat_pigeonhole_principle<cudd_mgr>(argc, argv);
}
