#include "../tic_tac_toe_bdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_tic_tac_toe<sylvan_bdd_adapter>(argc, argv);
}