#include "../game-of-life.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_gameoflife<adiar_zdd_adapter>(argc, argv);
}
