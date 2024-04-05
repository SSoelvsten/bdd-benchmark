#include "../qbf.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_qbf<cal_bcdd_adapter>(argc, argv);
}
