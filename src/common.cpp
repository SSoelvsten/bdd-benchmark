#include <cmath>
#include <algorithm>
#include <assert.h>

// =============================================================================
// A few chrono wrappers to improve readability
#include <chrono>

typedef std::chrono::steady_clock::time_point time_point;

inline time_point get_timestamp() {
  return std::chrono::steady_clock::now();
}

inline unsigned long int duration_of(time_point &before, time_point &after) {
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

int N = -1;
int M = 128; /* MiB */
std::string temp_path = "";

bool parse_input(int &argc, char* argv[])
{
  bool exit = false;
  int c;

  opterr = 0; // Squelch errors for non-common command-line arguments

  while ((c = getopt(argc, argv, "N:M:t:h")) != -1) {
    try {
      switch(c) {
      case 'N':
        N = std::stoi(optarg);
        continue;

      case 'M':
        M = std::stoi(optarg);
        if (M == 0) {
          std::cout << "  Must specify positive amount of memory (-M)" << std::endl;
        }

        continue;

      case 't':
        temp_path = optarg;
        continue;

      case '?': // All parameters not defined above will be overwritten to be the '?' character
        std::cout << "Undefined flag parameter used" << std::endl << std::endl;

      case 'h':
        std::cout << "Usage:  -flag      [default]  Description" << std::endl
                  << std::endl
                  << "        -h                    Print this information" << std::endl
                  << "        -N SIZE     [" << N
                                        << "]       Specify the size of problem" << std::endl
                  << "        -M MiB      [128]     Specify the amount of memory (MiB) to be dedicated to the BDD package" << std::endl
                  << "        -t TEMP_PTH [/tmp]    Filepath for temporary files on disk (Adiar BDD package)" << std::endl
          ;
        return true;
      }
    } catch (std::invalid_argument const &ex) {
      std::cout << "Invalid number: " << argv[1] << std::endl;
      exit = true;
    } catch (std::out_of_range const &ex) {
      std::cout << "Number out of range: " << argv[1] << std::endl;
      exit = true;
    }
  }

  // optind = 0; // Reset getopt, such that it can be used again outside
  return exit;
}

// =============================================================================
template <class T, size_t N>
constexpr int size(const T (& /*array*/)[N]) noexcept
{
  return N;
}
