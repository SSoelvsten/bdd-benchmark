#include "../cnf.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_cnf<cudd_zdd_adapter>(argc, argv);
}
