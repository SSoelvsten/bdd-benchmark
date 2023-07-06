cmake_minimum_required(VERSION 3.10)

project (bdd_benchmarks
  VERSION 1
  DESCRIPTION "A collection of comparable BDD Benchmarks"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)


# ============================================================================ #
# Compiler settings
# ============================================================================ #
message(STATUS "CMake build configuration for BDD Benchmarks")
message(STATUS "  OS:          ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:    ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "    C Flags:   ${CMAKE_C_FLAGS}")
message(STATUS "    CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "")

option(BDD_BENCHMARK_STATS "Build with printing of verbose statistics (ruins time measurements)." OFF)
message(STATUS "  Compiled with verbose Statistics: ${BDD_BENCHMARK_STATS}")
if (BDD_BENCHMARK_STATS)
  message(WARN "    NOTE: THIS MAKES ANY REPORTED TIME MEASUREMENTS NOT REPRESENTATIVE!")
endif(BDD_BENCHMARK_STATS)
message(STATUS "")

option(GRENDEL "Build for Grendel-S cluster" OFF)
message(STATUS "  Compiled for Grendel Cluster: ${BDD_BENCHMARK_GRENDEL}")
message(STATUS "")


# ============================================================================ #
# External
# ============================================================================ #

# ---------------------------------------------------------------------------- #
# BDD Packages
# Adiar Package (Steffan Sølvsten - Aarhus University, Denmark)
if (BDD_BENCHMARK_STATS)
  set(ADIAR_STATS ON)
endif(BDD_BENCHMARK_STATS)

add_subdirectory (external/adiar adiar)

# BuDDy Package (Jørn Lind-Nielsen - Copenhagen University, Denmark)
if (BDD_BENCHMARK_STATS)
  # BuDDy statistics: https://github.com/SSoelvsten/buddy/issues/6
  add_definitions(-DCACHESTATS)
endif(BDD_BENCHMARK_STATS)

add_subdirectory (external/BuDDy bdd)
set(BDD_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/BuDDy/src")

# Cal Package (Jagesh Sanghavi, Rajeev Ranjan et al. - University of California, United States of America)
add_subdirectory (external/cal cal)

# Cudd Package (Fabio Somenzi - University of Colorado Boulder, United States of America)
# Build separately in the 'make build' target.
set(CUDD_SRC_DIR "${PROJECT_SOURCE_DIR}/external/cudd")
set(CUDD_BUILD_DIR "${PROJECT_SOURCE_DIR}/build/cudd")
set(CUDD_LIB_FILES "${CUDD_BUILD_DIR}/lib/libcudd.a")

# Sylvan Package (Tom van Dijk - University of Twente, Netherlands)
if (BDD_BENCHMARK_STATS)
  set(SYLVAN_STATS ON)
endif(BDD_BENCHMARK_STATS)

add_subdirectory (external/sylvan sylvan)

# ---------------------------------------------------------------------------- #
# BLIF parser
add_subdirectory (external/blifparse libblifparse)

# ============================================================================ #
# Examples
# ============================================================================ #
add_subdirectory (src)

