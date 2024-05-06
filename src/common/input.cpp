#include "input.h"

int M = 128; /* MiB */

bool enable_reordering = false;

int threads = 1;

std::string temp_path = "";

std::vector<int> input_sizes = {};

std::vector<std::string> input_files = {};
