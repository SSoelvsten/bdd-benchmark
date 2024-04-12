cmake_minimum_required(VERSION 3.15)

project (bdd_benchmarks
  VERSION 1
  DESCRIPTION "A collection of comparable BDD Benchmarks"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default build type: Release" FORCE)
endif()

# ============================================================================ #
# Compiler settings
# ============================================================================ #
message(STATUS "")
message(STATUS "CMake build configuration for BDD Benchmarks")
message(STATUS "  OS:                   ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:             ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  |  C Flags:           ${CMAKE_C_FLAGS}")
message(STATUS "  |  CXX Flags:         ${CMAKE_CXX_FLAGS}")
message(STATUS "  |  EXE Linker Flags:  ${CMAKE_EXE_LINKER_FLAGS}")

message(STATUS "  Options:")
option(BDD_BENCHMARK_STATS "Build with printing of verbose statistics (ruins time measurements)." OFF)
message(STATUS "  |  Statistics:        ${BDD_BENCHMARK_STATS}")
if (BDD_BENCHMARK_STATS)
  message(WARN " | NOTE: THIS MAKES ANY REPORTED TIME MEASUREMENTS NOT REPRESENTATIVE!")
endif(BDD_BENCHMARK_STATS)

option(BDD_BENCHMARK_WAIT "Wait to exit program until enter has been pressed." OFF)
message(STATUS "  |  Waiting:           ${BDD_BENCHMARK_WAIT}")

option(BDD_BENCHMARK_GRENDEL "Build for Grendel cluster" OFF)
message(STATUS "  |  Grendel:           ${BDD_BENCHMARK_GRENDEL}")
message(STATUS "")

# ============================================================================ #
# External
# ============================================================================ #

include(ExternalProject)
find_program(AUTORECONF_EXE NAMES autoreconf DOC "autoreconf is required to build CUDD")
find_program(MAKE_EXE NAMES gmake make DOC "make is required to build CUDD")

# ---------------------------------------------------------------------------- #
# BDD Packages
# Adiar Package (Steffan Sølvsten - Aarhus University, Denmark)
if (BDD_BENCHMARK_STATS)
  set(ADIAR_STATS ON)
endif(BDD_BENCHMARK_STATS)

add_subdirectory (external/adiar adiar EXCLUDE_FROM_ALL)

# BuDDy Package (Jørn Lind-Nielsen - Copenhagen University, Denmark)
set(BUDDY_EXAMPLES OFF)
if (BDD_BENCHMARK_STATS)
  set(BUDDY_STATS ON)
endif(BDD_BENCHMARK_STATS)

add_subdirectory (external/buddy buddy EXCLUDE_FROM_ALL)

# Cal Package (Jagesh Sanghavi, Rajeev Ranjan et al. - University of California, United States of America)
add_subdirectory (external/cal cal EXCLUDE_FROM_ALL)

# CUDD Package (Fabio Somenzi - University of Colorado Boulder, United States of America)
add_subdirectory(external/cudd cudd EXCLUDE_FROM_ALL)

# Biodivine/LibBDD (Samuel Pastva et al. - Institute of Science and Technology Austria)
if (NOT BDD_BENCHMARK_GRENDEL)
  add_subdirectory(external/lib-bdd-ffi lib-bdd-ffi EXCLUDE_FROM_ALL)
endif(NOT BDD_BENCHMARK_GRENDEL)

# OxiDD (Nils Husung et al. - Saarland University, Germany and Eindhoven University of Technology, The Netherlands)
if (NOT BDD_BENCHMARK_GRENDEL)
  add_subdirectory(external/oxidd oxidd EXCLUDE_FROM_ALL)
endif(NOT BDD_BENCHMARK_GRENDEL)

# Sylvan Package (Tom van Dijk - University of Twente, Netherlands)
if (BDD_BENCHMARK_STATS)
  set(SYLVAN_STATS ON)
endif(BDD_BENCHMARK_STATS)

add_subdirectory (external/sylvan sylvan EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------- #
# BLIF parser
add_subdirectory (external/blifparse libblifparse EXCLUDE_FROM_ALL)

# ============================================================================ #
# Benchmarks
# ============================================================================ #
add_subdirectory (src)

# ============================================================================ #
# Formatting
# ============================================================================ #
include(cmake/clang-cxx-dev-tools.cmake)

