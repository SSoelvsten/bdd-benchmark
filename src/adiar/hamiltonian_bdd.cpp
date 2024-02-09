#include "../hamiltonian.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_hamiltonian<adiar_bdd_adapter>(argc, argv);
}
