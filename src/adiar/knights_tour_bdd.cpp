#include "../knights_tour.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  return run_knights_tour<adiar_bdd_adapter>(argc, argv);
}
