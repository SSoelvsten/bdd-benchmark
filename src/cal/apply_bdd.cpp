#include "../apply.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_apply<cal_bdd_adapter>(argc, argv);
}
