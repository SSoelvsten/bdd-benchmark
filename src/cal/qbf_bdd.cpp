#include "../qbf.cpp"

#include "adapter.h"

int main(int argc, char** argv)
{
  run_qbf<cal_bdd_adapter>(argc, argv);
}
