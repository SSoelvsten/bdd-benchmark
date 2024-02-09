#ifndef BDD_BENCHMARK_COMMON_ARRAY_H
#define BDD_BENCHMARK_COMMON_ARRAY_H

#include <cstdlib>

/// \brief Reobtain size of an array with a compile-time known size.
template <class T, size_t N>
constexpr int size(const T (& /*array*/)[N]) noexcept
{
  return N;
}

#endif // BDD_BENCHMARK_COMMON_ARRAY_H
