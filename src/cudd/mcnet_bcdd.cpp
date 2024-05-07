#include "../mcnet.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_mcnet<cudd_bcdd_adapter>(argc, argv);
}
