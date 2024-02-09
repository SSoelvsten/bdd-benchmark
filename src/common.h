#ifndef BDD_BENCHMARK_COMMON_H
#define BDD_BENCHMARK_COMMON_H

#include <cstdlib>
#include <cassert>
#include <string>

////////////////////////////////////////////////////////////////////////////////
// Global constants

/// Value based on recommendation for BuDDy
constexpr size_t CACHE_RATIO = 64u;

/// Initial size taken from CUDD defaults
constexpr size_t INIT_UNIQUE_SLOTS_PER_VAR = 256u;

////////////////////////////////////////////////////////////////////////////////
// Input parsing

extern int M; /* MiB */
extern std::string temp_path;

////////////////////////////////////////////////////////////////////////////////
// Utility functions

/// \brief Integer logarithm floor(log2(n))
///
/// \param n  Must not be 0
constexpr unsigned
ilog2(unsigned long long n)
{
  assert(n > 0);

#ifdef __GNUC__ // GCC and Clang support `__builtin_clzll`
  // "clz" stands for count leading zero bits. The builtin function may be
  // implemented more efficiently than the loop below.
  return sizeof(unsigned long long) * 8 - __builtin_clzll(n) - 1;
#else
  unsigned exp           = 1u;
  unsigned long long val = 2u; // 2^1
  while (val < n) {
    val <<= 1u;
    exp++;
  }

  return exp;
#endif
}

#endif // BDD_BENCHMARK_COMMON_H
