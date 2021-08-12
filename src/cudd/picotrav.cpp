#include "../picotrav.cpp"

#include "package_mgr.h"

int main(int argc, char** argv)
{
  run_picotrav<cudd_mgr>(argc, argv);
}
