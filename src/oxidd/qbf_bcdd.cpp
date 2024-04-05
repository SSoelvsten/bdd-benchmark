#include "../qbf.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_qbf<oxidd_bcdd_adapter>(argc, argv);
}
