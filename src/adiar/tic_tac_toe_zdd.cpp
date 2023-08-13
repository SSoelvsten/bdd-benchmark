#include "../tic_tac_toe_zdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_tic_tac_toe<adiar_zdd_adapter>(argc, argv);
}
