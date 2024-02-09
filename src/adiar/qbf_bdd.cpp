#include "../qbf.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_qbf<adiar_bdd_adapter>(argc, argv);
}
