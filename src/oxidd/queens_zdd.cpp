#include "../queens.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_queens<oxidd_zdd_adapter>(argc, argv);
}
