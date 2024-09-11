#include "../relprod.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_relprod<cudd_bcdd_adapter>(argc, argv);
}
