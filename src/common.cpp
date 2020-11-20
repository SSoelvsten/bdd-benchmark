#include <algorithm>

// =============================================================================
// A few chrono wrappers to improve readability
#include <chrono>

inline std::chrono::steady_clock::time_point get_timestamp() {
  return std::chrono::steady_clock::now();
}

inline unsigned long int duration_of(std::chrono::steady_clock::time_point &before,
                                     std::chrono::steady_clock::time_point &after) {
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

void parse_input(int &argc, char* argv[], size_t &N, size_t &M)
{
  try {
    if (argc > 1) {
      int N_ = std::atoi(argv[1]);
      if (N_ < 0) {
        Abort("N (first argument) should be nonnegative\n");
      }
      N = N_;
    }

    if (argc > 2) {
      int M_ = std::atoi(argv[2]);
      if (M_ <= 0) {
        Abort("M (second argument) should be positive\n");
      }
      M = M_;
    }
  } catch (std::invalid_argument const &ex) {
    Abort("Invalid number: %s\n", argv[1]);
  } catch (std::out_of_range const &ex) {
    Abort("Number out of range: %s\n", argv[1]);
  }
}
