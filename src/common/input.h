#ifndef BDD_BENCHMARK_COMMON_INPUT_H
#define BDD_BENCHMARK_COMMON_INPUT_H

#include <iostream>  // std::cout, std::cerr, ...
#include <getopt.h>  // getopt
#include <stdexcept> // std::invalid_argument
#include <string>    // std::string, std::stoi, ...
#include <vector>    // std::vector

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \brief Amount of Mebibytes (MiB) of memory to dedicate to the BDD package.
///
/// \details This value is provided with `-M`
////////////////////////////////////////////////////////////////////////////////
extern int M;

////////////////////////////////////////////////////////////////////////////////
/// \brief Whether *dynamic variable reordering* should be enabled.
///
/// \details This value is provided with `-r`
////////////////////////////////////////////////////////////////////////////////
extern bool enable_reordering;

////////////////////////////////////////////////////////////////////////////////
/// \brief Worker thread count for multi-threaded BDD packages
///
/// \details This value is provided with `-P`
////////////////////////////////////////////////////////////////////////////////
extern int threads;

////////////////////////////////////////////////////////////////////////////////
/// \brief Path to temporary files for the BDD package to store data on disk.
///
/// \details This value is provided with `-t`
////////////////////////////////////////////////////////////////////////////////
extern std::string temp_path;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \brief Lowercase a single ASCII character.
///
/// \details Based on https://stackoverflow.com/a/313990/13300643
////////////////////////////////////////////////////////////////////////////////
inline char
ascii_tolower(char in)
{
  if (in <= 'Z' && in >= 'A') return in - ('Z' - 'z');
  return in;
}

/// \brief Lowercase an entire string of ASCII characters.
inline std::string
ascii_tolower(const std::string& in)
{
  std::string out;
  for (auto it = in.begin(); it != in.end(); it++) { out.push_back(ascii_tolower(*it)); }
  return out;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief Values to be parsed particular to a benchmark.
////////////////////////////////////////////////////////////////////////////////
std::string
parse_args();

////////////////////////////////////////////////////////////////////////////////
/// \brief Logic for parsing input values particular to a benchmark.
////////////////////////////////////////////////////////////////////////////////
bool
parse_input(const int c, const char* arg);

////////////////////////////////////////////////////////////////////////////////
/// \brief Title to be displayed for benchmark help prompt
////////////////////////////////////////////////////////////////////////////////
constexpr std::string_view
parse_help_title();

////////////////////////////////////////////////////////////////////////////////
/// \brief Logic for parsing input values particular to a benchmark.
////////////////////////////////////////////////////////////////////////////////
constexpr std::string_view
parse_help_args();

////////////////////////////////////////////////////////////////////////////////
/// \brief Logic for parsing input values.
///
/// \details Implement the above functions to specify parsing of .
////////////////////////////////////////////////////////////////////////////////
template <class Policy>
inline bool
parse_input(int& argc, char* argv[])
{
  bool exit = false;
  int c;

  opterr = 0; // Squelch errors for non-common command-line arguments

  const std::string args = std::string("h" "M:P:rt:") + std::string(Policy::args);
  while ((c = getopt(argc, argv, args.data())) != -1) {
    try {
      switch (c) {
      case 'M': {
        M = std::stoi(optarg);
        if (M <= 0) {
          std::cerr << "  Must specify positive amount of memory (-M)\n";
          exit = true;
        }
        continue;
      }
      case 'P': {
        threads = std::stoi(optarg);
        if (threads <= 0) {
          std::cerr << "  Must specify a positive thread count (-P)\n";
          exit = true;
        }
        continue;
      }
      case 'r': {
        enable_reordering = true;
        continue;
      }
      case 't': {
        temp_path = optarg;
        continue;
      }

      case '?': // All parameters not defined above will be overwritten to be the '?' character
        [[fallthrough]];
      case 'h': {
        std::cout
          << Policy::name << " Benchmark\n"
          << "-------------------------------------------------------------------------------\n"
          << "Usage:  -flag      [default] Description\n"
          << "-------------------------------------------------------------------------------\n"
          << "        -h                   Print this information\n"
          << "\n"
          << "-------------------------------------------------------------------------------\n"
          << "BDD Package options:\n"
          << "        -M MiB      [128]    Amount of memory (MiB)\n"
          << "        -t TEMP_PTH [/tmp]   Filepath for temporary files on disk\n"
          << "        -P THREADS  [1]      Worker thread count\n"
          << "        -r                   Enable dynamic variable reordering\n"
          << "\n"
          << "-------------------------------------------------------------------------------\n"
          << "Benchmark options:\n"
          << Policy::help_text << "\n"
          << std::flush;
        return true;
      }

      default: {
        exit |= Policy::parse_input(c, optarg);
      }
      }
    } catch (const std::invalid_argument& ex) {
      std::cerr << "Invalid number: " << ex.what() << "\n";
      exit = true;
    } catch (const std::out_of_range& ex) {
      std::cerr << "Number out of range: " << ex.what() << "\n";
      exit = true;
    }
  }

  //optind = 0; // Reset getopt, such that it can be used again outside
  return exit;
}


#endif // BDD_BENCHMARK_COMMON_INPUT_H
