// =============================================================================
// A few chrono wrappers to improve readability
#include <chrono>

inline std::chrono::high_resolution_clock::time_point get_timestamp() {
  return std::chrono::high_resolution_clock::now();
}

inline unsigned long int duration_of(std::chrono::high_resolution_clock::time_point &before,
                                     std::chrono::high_resolution_clock::time_point &after) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
}

// =============================================================================
// Common printing macros
#define INFO(s, ...) fprintf(stdout, s, ##__VA_ARGS__)
#define Abort(...) { fprintf(stderr, __VA_ARGS__); exit(-1); }


// =============================================================================
// Input parsing
#include <iostream>       // std::cerr
#include <stdexcept>      // std::invalid_argument

void parse_input(int &argc, char* argv[], size_t &N)
{
  try {
    if (argc == 1) {
      return;
    } else {
      N = std::atoi(argv[1]);
      if (N < 0) {
        Abort("N (first argument) should be nonnegative\n");
      }
    }
  } catch (std::invalid_argument const &ex) {
    Abort("Invalid number: %s\n", argv[1]);
  } catch (std::out_of_range const &ex) {
    Abort("Number out of range: %s\n", argv[1]);
  }
}
