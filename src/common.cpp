#include <cmath>
#include <algorithm>
#include <assert.h>

// =============================================================================
// Global constants
constexpr size_t CACHE_RATIO = 64u;

// Initial size taken from CUDD defaults
constexpr size_t INIT_UNIQUE_SLOTS_PER_VAR = 256;

// =============================================================================
// A few chrono wrappers to improve readability
#include <chrono>

typedef std::chrono::steady_clock::time_point time_point;

inline time_point get_timestamp() {
  return std::chrono::steady_clock::now();
}

inline unsigned long int duration_of(const time_point &before, const time_point &after) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
}

// =============================================================================
// Common printing macros
#define INFO(s, ...) fprintf(stdout, s, ##__VA_ARGS__)
#define Abort(...) { fprintf(stderr, __VA_ARGS__); exit(-1); }


// =============================================================================
// Input parsing
#include <getopt.h>       // argument parsing
#include <iostream>       // std::cerr
#include <stdexcept>      // std::invalid_argument
#include <string>         // std::string
#include <sstream>        // std::istringstream
#include <vector>         // std::vector
#include <iterator>       // iterators

int N = -1;
int M = 128; /* MiB */
std::string temp_path = "";

std::vector<std::string> input_files = {};

enum no_variable_order { NO_ORDERING };

template<typename variable_order_enum>
variable_order_enum parse_variable_ordering(const std::string &arg, bool &should_exit);

template<>
no_variable_order parse_variable_ordering(const std::string &, bool &should_exit)
{
  std::cerr << "Variable ordering is undefined for this benchmark" << std::endl;
  should_exit = true;
  return no_variable_order::NO_ORDERING;
}

template<typename variable_order_enum = no_variable_order>
bool parse_input(int &argc, char* argv[], variable_order_enum &variable_order)
{
  bool exit = false;
  int c;

  opterr = 0; // Squelch errors for non-common command-line arguments

  while ((c = getopt(argc, argv, "N:M:f:o:t:h")) != -1) {
    try {
      switch(c) {
      case 'N':
        N = std::stoi(optarg);
        continue;

      case 'M':
        M = std::stoi(optarg);
        if (M == 0) {
          std::cerr << "  Must specify positive amount of memory (-M)" << std::endl;
          exit = true;
        }
        continue;

      case 'f': {
        std::string file = optarg;
        if (!file.empty()) { input_files.push_back(file); }
        continue;
      }

      case 'o':
        variable_order = parse_variable_ordering<variable_order_enum>(optarg, exit);
        continue;

      case 't':
        temp_path = optarg;
        continue;

      case '?': // All parameters not defined above will be overwritten to be the '?' character
        std::cerr << "Undefined flag parameter used" << std::endl << std::endl;
        [[fallthrough]]; // Let the compiler know, that we intend to fall through to 'h' case

      case 'h':
        std::cout << "Usage:  -flag      [default]  Description" << std::endl
                  << std::endl
                  << "        -h                    Print this information" << std::endl
                  << "        -N SIZE     [" << N
                                        << "]       Size of a problem" << std::endl
                  << "        -f FILENAME           Input file to run (use repeatedly for multiple files)" << std::endl
                  << "        -M MiB      [128]     Amount of memory (MiB) to be dedicated to the BDD package" << std::endl
                  << "        -o ORDERING           Variable ordering to use" << std::endl
                  << "        -t TEMP_PTH [/tmp]    Filepath for temporary files on disk" << std::endl
          ;
        return true;
      }
    } catch (std::invalid_argument const &ex) {
      std::cerr << "Invalid number: " << argv[1] << std::endl;
      exit = true;
    } catch (std::out_of_range const &ex) {
      std::cerr << "Number out of range: " << argv[1] << std::endl;
      exit = true;
    }
  }

  optind = 0; // Reset getopt, such that it can be used again outside
  return exit;
}

// =============================================================================
template <class T, size_t N>
constexpr int size(const T (& /*array*/)[N]) noexcept
{
  return N;
}
