#include "../queens_bdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_queens<buddy_bdd_adapter>(argc, argv);
}
