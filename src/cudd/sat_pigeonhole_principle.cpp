#include "../sat_pigeonhole_principle.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_sat_pigeonhole_principle<cudd_bdd_adapter>(argc, argv);
}
