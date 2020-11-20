cmake_minimum_required(VERSION 3.10)

# ============================================================================ #
# Compiler settings
# ============================================================================ #
set(CMAKE_C_FLAGS "-O3 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")

# ============================================================================ #
# External
# ============================================================================ #
add_subdirectory (external/BuDDy bdd)
add_subdirectory (external/coom coom)
#add_subdirectory (external/cudd cudd)
add_subdirectory (external/sylvan sylvan)

# ============================================================================ #
# Examples
# ============================================================================ #
add_subdirectory (src)

