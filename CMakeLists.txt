cmake_minimum_required(VERSION 3.10)

# ============================================================================ #
# Compiler settings
# ============================================================================ #
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-O3 ${CMAKE_C_FLAGS}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")

# ============================================================================ #
# External
# ============================================================================ #
add_subdirectory (external/BuDDy buddy)
add_subdirectory (external/cudd cudd)
add_subdirectory (external/sylvan sylvan)

# ============================================================================ #
# Examples
# ============================================================================ #
add_subdirectory (src)

