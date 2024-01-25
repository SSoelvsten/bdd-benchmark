#include "../queens.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_queens<libbdd_bdd_adapter>(argc, argv);
}
