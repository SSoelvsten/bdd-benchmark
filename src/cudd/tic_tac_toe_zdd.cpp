#include "../tic_tac_toe_zdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_tic_tac_toe<cudd_zdd_adapter>(argc, argv);
}
