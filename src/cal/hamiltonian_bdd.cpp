#include "../hamiltonian.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_hamiltonian<cal_bdd_adapter>(argc, argv);
}
