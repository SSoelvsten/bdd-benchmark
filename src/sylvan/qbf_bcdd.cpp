#include "../qbf.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_qbf<sylvan_bcdd_adapter>(argc, argv);
}
