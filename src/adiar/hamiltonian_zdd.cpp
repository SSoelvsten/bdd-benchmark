#include "../hamiltonian.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_hamiltonian<adiar_zdd_adapter>(argc, argv);
}
