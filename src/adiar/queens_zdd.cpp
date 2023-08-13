#include "../queens_zdd.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_queens<adiar_zdd_adapter>(argc, argv);
}
