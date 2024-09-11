#include "../cnf.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_cnf<oxidd_bcdd_adapter>(argc, argv);
}
