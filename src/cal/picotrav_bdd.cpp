#include "../picotrav.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_picotrav<cal_bdd_adapter>(argc, argv);
}
