#ifndef BDD_BENCHMARK_COMMON_ADAPTER_H
#define BDD_BENCHMARK_COMMON_ADAPTER_H

#include <iostream>
#include <cassert>
#include <string>

#include "./chrono.h"
#include "./input.h"
#include "./json.h"

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
run(const std::string& benchmark_name, const int varcount, const F& f)
{
  std::cout << json::brace_open << json::endl;

  std::cout
    // Debug mode
    << json::field("debug_mode")
#ifndef NDEBUG
    << json::value(true)
#else
    << json::value(false)
#endif
    << json::comma
    << json::endl
    // Statistics
    << json::field("statistics")
#ifdef BDD_BENCHMARK_STATS
    << json::value(true)
#else
    << json::value(false)
#endif
    << json::comma << json::endl
    << json::endl
    // BDD package substruct
    << json::field("bdd package") << json::brace_open
    << json::endl
    // Name
    << json::field("name") << json::value(Adapter::name) << json::comma
    << json::endl
    // BDD Type
    << json::field("type") << json::value(Adapter::dd) << json::comma << json::endl;

  const time_point t_before = now();
  Adapter adapter(varcount);
  const time_point t_after = now();

  const time_duration t_duration = duration_ms(t_before, t_after);
#ifdef BDD_BENCHMARK_INCL_INIT
  init_time = t_duration;
#endif // BDD_BENCHMARK_INCL_INIT

  std::cout
    // Initialisation Time
    << json::field("init time (ms)") << json::value(t_duration) << json::comma
    << json::endl
    // Memory
    << json::field("memory (MiB)") << json::value(M) << json::comma
    << json::endl
    // Variables
    << json::field("variables") << json::value(varcount)
    << json::endl
    // ...
    << json::brace_close << json::comma << json::endl
    << json::endl;

  std::cout << json::field("benchmark") << json::brace_open << json::endl;
  std::cout << json::field("name") << json::value(benchmark_name) << json::comma << json::endl
            << json::flush;

  const int exit_code = adapter.run([&]() { return f(adapter); });

  std::cout << json::brace_close << json::endl << json::brace_close << json::endl << json::flush;

#ifdef BDD_BENCHMARK_STATS
  if (!exit_code) { adapter.print_stats(); }
#endif

#ifdef BDD_BENCHMARK_WAIT
  // TODO: move to 'std::cerr' to keep 'std::cout' pure JSON?
  std::cout << "\npress any key to exit . . .\n" << std::flush;
  ;
  std::getchar();
  std::cout << "\n";
#endif

  return exit_code;
}

#endif // BDD_BENCHMARK_COMMON_ADAPTER_H
