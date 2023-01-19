#include "../qbf.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_qbf<buddy_bdd_adapter>(argc, argv);
}
