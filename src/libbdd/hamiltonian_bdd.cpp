#include "../hamiltonian.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_hamiltonian<libbdd_bdd_adapter>(argc, argv);
}
