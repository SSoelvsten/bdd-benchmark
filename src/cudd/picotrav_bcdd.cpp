#include "../picotrav.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_picotrav<cudd_bcdd_adapter>(argc, argv);
}
