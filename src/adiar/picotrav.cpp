#include "../picotrav.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_picotrav<adiar_bdd_adapter>(argc, argv);
}
