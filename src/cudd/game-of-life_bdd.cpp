#include "../game-of-life.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_gameoflife<cudd_bdd_adapter>(argc, argv);
}
