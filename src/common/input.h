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
/// \brief Path to temporary files for the BDD package to store data on disk.
///
/// \details This value is provided with `-t`
////////////////////////////////////////////////////////////////////////////////
extern std::string temp_path;

////////////////////////////////////////////////////////////////////////////////
/// \brief List of integer input sizes
///
/// \details This value is provided with `-N`
////////////////////////////////////////////////////////////////////////////////
extern std::vector<int> input_sizes;

////////////////////////////////////////////////////////////////////////////////
/// \brief   Paths for input files
///
/// \details This value is provided with `-f`
////////////////////////////////////////////////////////////////////////////////
extern std::vector<std::string> input_files;

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

////////////////////////////////////////////////////////////////////////////////
/// \brief   Parses a given string into an enum. The error code `should_exit` is
///          made `true` if parsing fails.
///
/// \details When using some option ('-o'), specialize this function to convert
///          the given string to the desired enum value.
////////////////////////////////////////////////////////////////////////////////
template <typename option_enum>
option_enum
parse_option(const std::string& arg, bool& should_exit);

////////////////////////////////////////////////////////////////////////////////
/// \brief   String to be printed as part of '--help' (compile-time derived)
///
/// \details When using some option ('-o'), specialize this function for a
///          short description.
////////////////////////////////////////////////////////////////////////////////
template <typename option_enum>
std::string
option_help_str();

////////////////////////////////////////////////////////////////////////////////
/// \brief Enum type for an empty set of options.
////////////////////////////////////////////////////////////////////////////////
enum no_options
{
  NONE
};

////////////////////////////////////////////////////////////////////////////////
/// \brief Option parsing for `no_option` enum
////////////////////////////////////////////////////////////////////////////////
template <>
inline no_options
parse_option(const std::string&, bool& should_exit)
{
  std::cerr << "Options is undefined for this benchmark\n";
  should_exit = true;
  return no_options::NONE;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief Option parsing for `no_option` enum
////////////////////////////////////////////////////////////////////////////////
template <>
inline std::string
option_help_str<no_options>()
{
  return "Not part of this benchmark";
}

////////////////////////////////////////////////////////////////////////////////
/// \brief   Logic for parsing input values.
////////////////////////////////////////////////////////////////////////////////
template <typename option_enum = no_options>
bool
parse_input(int& argc, char* argv[], option_enum& option)
{
  bool exit = false;
  int c;

  opterr = 0; // Squelch errors for non-common command-line arguments

  while ((c = getopt(argc, argv, "N:M:f:o:t:h")) != -1) {
    try {
      switch (c) {
      case 'N': input_sizes.push_back(std::stoi(optarg)); continue;

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

      case 'o': option = parse_option<option_enum>(optarg, exit); continue;

      case 't': temp_path = optarg; continue;

      case '?': // All parameters not defined above will be overwritten to be the '?' character
        std::cerr << "Undefined flag parameter used\n\n";
        [[fallthrough]]; // Let the compiler know, that we intend to fall through to 'h' case

      case 'h':
        std::cout
          << "Usage:  -flag      [default]  Description" << std::endl
          << std::endl
          << "        -h                    Print this information" << std::endl
          << "        -N SIZE               Size(s) of a problem" << std::endl
          << "        -f FILENAME           Input file to run (use repeatedly for multiple files)"
          << std::endl
          << "        -M MiB      [128]     Amount of memory (MiB) to be dedicated to the BDD "
             "package"
          << std::endl
          << "        -o OPTION             " << option_help_str<option_enum>() << std::endl
          << "        -t TEMP_PTH [/tmp]    Filepath for temporary files on disk" << std::endl;
        return true;
      }
    } catch (const std::invalid_argument& ex) {
      std::cerr << "Invalid number: " << ex.what() << "\n";
      exit = true;
    } catch (const std::out_of_range& ex) {
      std::cerr << "Number out of range: " << ex.what() << "\n";
      exit = true;
    }
  }

  optind = 0; // Reset getopt, such that it can be used again outside
  return exit;
}

#endif // BDD_BENCHMARK_COMMON_INPUT_H
