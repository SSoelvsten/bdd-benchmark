#include "../queens_zdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_queens<cudd_zdd_adapter>(argc, argv);
}
