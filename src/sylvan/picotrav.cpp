#include "../picotrav.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_picotrav<sylvan_bdd_adapter>(argc, argv);
}
