cmake_minimum_required(VERSION 3.10)

# ============================================================================ #
# Compiler settings
# ============================================================================ #
set(CMAKE_C_FLAGS "-O3 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")

# ============================================================================ #
# External
# ============================================================================ #

# BuDDy Package (Copenhagen University, Denmark)
add_subdirectory (external/BuDDy bdd)
set(BDD_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/BuDDy/src")

# COOM Package (Aarhus University, Denmark)
add_subdirectory (external/coom coom)

# Sylvan Package (University of Twente, Netherlands)
add_subdirectory (external/sylvan sylvan)
set(SYLVAN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/build/sylvan/usr/local/include")

# ============================================================================ #
# Examples
# ============================================================================ #
add_subdirectory (src)

