#include "../tic-tac-toe_zdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_tictactoe<cudd_zdd_adapter>(argc, argv);
}
