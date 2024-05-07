#include "../mcnet.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_mcnet<buddy_bdd_adapter>(argc, argv);
}
