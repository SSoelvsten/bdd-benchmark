#include "../cnf.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_cnf<adiar_bdd_adapter>(argc, argv);
}