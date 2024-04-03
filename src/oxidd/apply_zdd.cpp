#include "../apply.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_apply<oxidd_zdd_adapter>(argc, argv);
}
