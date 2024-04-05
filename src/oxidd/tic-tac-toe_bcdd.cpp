#include "../tic-tac-toe.cpp"

#include "adapter.h"

int
main(int argc, char** argv)
{
  return run_tictactoe<oxidd_bcdd_adapter>(argc, argv);
}
