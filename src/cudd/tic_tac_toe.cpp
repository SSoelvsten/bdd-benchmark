#include "../tic_tac_toe.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_tic_tac_toe<cudd_bdd_adapter>(argc, argv);
}
