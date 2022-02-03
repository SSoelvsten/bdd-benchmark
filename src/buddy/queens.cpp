#include "../queens.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_queens<buddy_bdd_adapter>(argc, argv);
}
