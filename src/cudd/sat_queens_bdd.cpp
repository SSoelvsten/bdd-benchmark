#include "../sat_queens.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_sat_queens<cudd_bdd_adapter>(argc, argv);
}
