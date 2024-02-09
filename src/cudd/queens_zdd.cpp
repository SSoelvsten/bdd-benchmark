#include "../queens.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_queens<cudd_zdd_adapter>(argc, argv);
}
