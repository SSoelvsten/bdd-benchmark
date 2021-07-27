#include "../tic_tac_toe.cpp"

#include "package_mgr.h"

int main(int argc, char** argv)
{
  run_tic_tac_toe<cudd_mgr>(argc, argv);
}
