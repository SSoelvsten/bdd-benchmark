#include "../apply.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_apply<sylvan_bcdd_adapter>(argc, argv);
}
