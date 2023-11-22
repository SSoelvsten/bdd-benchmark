#include "../apply.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_apply<adiar_bdd_adapter>(argc, argv);
}
