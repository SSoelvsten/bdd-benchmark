cmake_minimum_required(VERSION 3.10)

project (bdd_benchmarks
  VERSION 1
  DESCRIPTION "A collection of comparable BDD Benchmarks"
)


# ============================================================================ #
# Compiler settings
# ============================================================================ #
set(CMAKE_C_FLAGS "-O3 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")

option(GRENDEL "Build for Grendel-S cluster" OFF)
if (GRENDEL)
  add_definitions(-DGRENDEL)
endif()

message(STATUS "CMake build configuration for BDD Benchmarks")
message(STATUS "  OS: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "    C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "    CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "  Grendel: ${GRENDEL}")
message(STATUS "")


# ============================================================================ #
# External
# ============================================================================ #

# ---------------------------------------------------------------------------- #
# BDD Packages
# Adiar Package (Steffan Sølvsten - Aarhus University, Denmark)
add_subdirectory (external/adiar adiar)

# BuDDy Package (Jørn Lind-Nielsen - Copenhagen University, Denmark)
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
add_subdirectory (external/sylvan sylvan)

# ---------------------------------------------------------------------------- #
# BLIF parser
add_subdirectory (external/blifparse libblifparse)

# ============================================================================ #
# Examples
# ============================================================================ #
add_subdirectory (src)

