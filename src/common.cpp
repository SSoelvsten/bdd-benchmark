#include <algorithm>      // std::sort(), ...
#include <array>          // std::array<>
#include <assert.h>       // Assertions
#include <cmath>          // std::abs(), std::min(), ...
#include <iostream>       // std::cerr
#include <iterator>       // iterators
#include <ostream>        // output streams
#include <functional>     // std::function<>, ...
#include <stdexcept>      // std::invalid_argument
#include <string>         // std::string
#include <sstream>        // std::istringstream
#include <vector>         // std::vector

// =============================================================================
// Global constants
constexpr size_t CACHE_RATIO = 64u;

// Initial size taken from CUDD defaults
constexpr size_t INIT_UNIQUE_SLOTS_PER_VAR = 256u;

// =============================================================================
// Timing
#include <chrono>

using time_point = std::chrono::steady_clock::time_point;

inline time_point now() {
  return std::chrono::steady_clock::now();
}

using time_duration = size_t;

inline time_duration duration_ms(const time_point &begin, const time_point &end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
}

// =============================================================================
// Input parsing

#include <getopt.h>       // argument parsing

int M = 128; /* MiB */
std::string temp_path = "";

std::vector<int> input_sizes = {};
std::vector<std::string> input_files = {};

template<typename option_enum>
option_enum parse_option(const std::string &arg, bool &should_exit);

template<typename option_enum>
std::string option_help_str();

enum no_options { NONE };

template<>
no_options parse_option(const std::string &, bool &should_exit)
{
  std::cerr << "Options is undefined for this benchmark\n";
  should_exit = true;
  return no_options::NONE;
}

template<>
std::string option_help_str<no_options>()
{ return "Not part of this benchmark"; }

template<typename option_enum = no_options>
bool parse_input(int &argc, char* argv[], option_enum &option)
{
  bool exit = false;
  int c;

  opterr = 0; // Squelch errors for non-common command-line arguments

  while ((c = getopt(argc, argv, "N:M:f:o:t:h")) != -1) {
    try {
      switch(c) {
      case 'N':
        input_sizes.push_back(std::stoi(optarg));
        continue;

      case 'M':
        M = std::stoi(optarg);
        if (M == 0) {
          std::cerr << "  Must specify positive amount of memory (-M)\n";
          exit = true;
        }
        continue;

      case 'f': {
        std::string file = optarg;
        if (!file.empty()) { input_files.push_back(file); }
        continue;
      }

      case 'o':
        option = parse_option<option_enum>(optarg, exit);
        continue;

      case 't':
        temp_path = optarg;
        continue;

      case '?': // All parameters not defined above will be overwritten to be the '?' character
        std::cerr << "Undefined flag parameter used\n\n";
        [[fallthrough]]; // Let the compiler know, that we intend to fall through to 'h' case

      case 'h':
        std::cout << "Usage:  -flag      [default]  Description" << std::endl
                  << std::endl
                  << "        -h                    Print this information" << std::endl
                  << "        -N SIZE               Size(s) of a problem" << std::endl
                  << "        -f FILENAME           Input file to run (use repeatedly for multiple files)" << std::endl
                  << "        -M MiB      [128]     Amount of memory (MiB) to be dedicated to the BDD package" << std::endl
                  << "        -o OPTION             " << option_help_str<option_enum>() << std::endl
                  << "        -t TEMP_PTH [/tmp]    Filepath for temporary files on disk" << std::endl
          ;
        return true;
      }
    } catch (std::invalid_argument const &ex) {
      std::cerr << "Invalid number: " << ex.what() << "\n";
      exit = true;
    } catch (std::out_of_range const &ex) {
      std::cerr << "Number out of range: " << ex.what() << "\n";
      exit = true;
    }
  }

  optind = 0; // Reset getopt, such that it can be used again outside
  return exit;
}

// =============================================================================
// based on: https://stackoverflow.com/a/313990/13300643
char ascii_tolower(char in)
{
  if (in <= 'Z' && in >= 'A')
    return in - ('Z' - 'z');
  return in;
}

std::string ascii_tolower(const std::string &in)
{
  std::string out;
  for (auto it = in.begin(); it != in.end(); it++) {
    out.push_back(ascii_tolower(*it));
  }
  return out;
}

// =============================================================================
template <class T, size_t N>
constexpr int size(const T (& /*array*/)[N]) noexcept
{
  return N;
}
