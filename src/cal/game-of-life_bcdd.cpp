#include "../game-of-life.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_gameoflife<cal_bcdd_adapter>(argc, argv);
}
