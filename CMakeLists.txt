cmake_minimum_required(VERSION 3.10)

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

# BuDDy Package (Copenhagen University, Denmark)
add_subdirectory (external/BuDDy bdd)
set(BDD_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/BuDDy/src")

# Adiar Package (Aarhus University, Denmark)
add_subdirectory (external/adiar adiar)

# Sylvan Package (University of Twente, Netherlands)
add_subdirectory (external/sylvan sylvan)
set(SYLVAN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/build/sylvan/usr/local/include")

# ============================================================================ #
# Examples
# ============================================================================ #
add_subdirectory (src)

