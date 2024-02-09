#include "../queens.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_queens<adiar_bdd_adapter>(argc, argv);
}
