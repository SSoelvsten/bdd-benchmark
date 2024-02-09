#ifndef BDD_BENCHMARK_COMMON_ADAPTER_H
#define BDD_BENCHMARK_COMMON_ADAPTER_H

#include <iostream>
#include <cassert>

#include "./chrono.h"
#include "./input.h"

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \brief   Number of slots in the unique node table per single slot in the
///          computation cache.
///
/// \details Value based on recommendation in documentation of BuDDy.
////////////////////////////////////////////////////////////////////////////////
constexpr size_t CACHE_RATIO = 64u;

////////////////////////////////////////////////////////////////////////////////
/// \brief   Initial number of entries in the unique table per variable.
///
/// \details Value take from CUDD defaults
////////////////////////////////////////////////////////////////////////////////
constexpr size_t INIT_UNIQUE_SLOTS_PER_VAR = 256u;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \brief Integer logarithm floor(log2(n))
///
/// \param n  Must not be 0
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
/// \brief Initializes the BDD package and runs the given benchmark
////////////////////////////////////////////////////////////////////////////////
template <typename Adapter, typename F>
int
run(const int varcount, const F& f)
{
  std::cout << "  " << Adapter::NAME << ":\n";

  const time_point t_before = now();
  Adapter adapter(varcount);
  const time_point t_after = now();

  std::cout << "  | init time (ms)            " << duration_ms(t_before, t_after) << "\n"
            << "  | memory (MiB)              " << M << "\n"
            << "  | variables                 " << varcount << "\n"
            << std::flush;

  const int exit_code = adapter.run([&]() { return f(adapter); });

  if (!exit_code) { adapter.print_stats(); }

#ifdef BDD_BENCHMARK_WAIT
  std::cout << "\npress any key to exit . . .\n" << std::flush;
  ;
  std::getchar();
  std::cout << "\n";
#endif

  return exit_code;
}

#endif // BDD_BENCHMARK_COMMON_ADAPTER_H
