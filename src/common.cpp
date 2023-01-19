#include <cmath>
#include <algorithm>
#include <assert.h>

// =============================================================================
// Global constants
constexpr size_t CACHE_RATIO = 64u;

// Initial size taken from CUDD defaults
constexpr size_t INIT_UNIQUE_SLOTS_PER_VAR = 256u;

// =============================================================================
// A few chrono wrappers to improve readability
#include <chrono>

typedef std::chrono::steady_clock::time_point time_point;

inline time_point get_timestamp() {
  return std::chrono::steady_clock::now();
}

typedef unsigned long int time_duration;

inline unsigned long int duration_of(const time_point &before, const time_point &after) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
}

// =============================================================================
// Common printing macros
#define FLUSH() { fflush(stdout); fflush(stderr); }
#define EXIT(e) { FLUSH(); exit(e); }

#define INFO(s, ...) { fprintf(stdout, s, ##__VA_ARGS__); FLUSH() }
#define ERROR(s, ...) { fprintf(stderr, s, ##__VA_ARGS__); FLUSH() }

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

template<typename option_enum>
option_enum parse_option(const std::string &arg, bool &should_exit);

template<typename option_enum>
std::string option_help_str();

enum no_options { NONE };

template<>
no_options parse_option(const std::string &, bool &should_exit)
{
  ERROR("Variable ordering is undefined for this benchmark\n");
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
        N = std::stoi(optarg);
        continue;

      case 'M':
        M = std::stoi(optarg);
        if (M == 0) {
          ERROR("  Must specify positive amount of memory (-M)\n");
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
        ERROR("Undefined flag parameter used\n\n");
        [[fallthrough]]; // Let the compiler know, that we intend to fall through to 'h' case

      case 'h':
        std::cout << "Usage:  -flag      [default]  Description" << std::endl
                  << std::endl
                  << "        -h                    Print this information" << std::endl
                  << "        -N SIZE     [" << N
                                        << "]       Size of a problem" << std::endl
                  << "        -f FILENAME           Input file to run (use repeatedly for multiple files)" << std::endl
                  << "        -M MiB      [128]     Amount of memory (MiB) to be dedicated to the BDD package" << std::endl
                  << "        -o OPTION             " << option_help_str<option_enum>() << std::endl
                  << "        -t TEMP_PTH [/tmp]    Filepath for temporary files on disk" << std::endl
          ;
        return true;
      }
    } catch (std::invalid_argument const &ex) {
      ERROR("Invalid number: %s\n", ex.what());
      exit = true;
    } catch (std::out_of_range const &ex) {
      ERROR("Number out of range: %s", ex.what());
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
