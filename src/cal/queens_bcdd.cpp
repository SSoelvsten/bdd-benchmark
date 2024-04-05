#include "../queens.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_queens<cal_bcdd_adapter>(argc, argv);
}
