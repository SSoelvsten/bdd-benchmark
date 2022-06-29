#include "../queens_bdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_queens<sylvan_bdd_adapter>(argc, argv);
}
