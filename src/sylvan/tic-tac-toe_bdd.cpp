#include "../tic-tac-toe_bdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_tictactoe<sylvan_bdd_adapter>(argc, argv);
}
