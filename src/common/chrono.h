#ifndef BDD_BENCHMARK_COMMON_CHRONO_H
#define BDD_BENCHMARK_COMMON_CHRONO_H

#include <chrono>
#include <cstdlib>

using time_point = std::chrono::steady_clock::time_point;

inline time_point now() {
  return std::chrono::steady_clock::now();
}

using time_duration = size_t;

inline time_duration duration_ms(const time_point &begin, const time_point &end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
}

#endif // BDD_BENCHMARK_COMMON_CHRONO_H
