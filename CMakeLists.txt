cmake_minimum_required(VERSION 3.10)

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
message(STATUS "CMake build configuration for BDD Benchmarks")
message(STATUS "  OS:                       ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:                 ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "    C Flags:                ${CMAKE_C_FLAGS}")
message(STATUS "    CXX Flags:              ${CMAKE_CXX_FLAGS}")
message(STATUS "")

option(BDD_BENCHMARK_STATS "Build with printing of verbose statistics (ruins time measurements)." OFF)
message(STATUS "  Compiled with Statistics: ${BDD_BENCHMARK_STATS}")
if (BDD_BENCHMARK_STATS)
  message(WARN "    NOTE: THIS MAKES ANY REPORTED TIME MEASUREMENTS NOT REPRESENTATIVE!")
endif(BDD_BENCHMARK_STATS)
message(STATUS "")

option(BDD_BENCHMARK_WAIT "Wait to exit program until enter has been pressed." OFF)
message(STATUS "  Compiled with Waiting:    ${BDD_BENCHMARK_WAIT}")
message(STATUS "")

option(GRENDEL "Build for Grendel cluster" OFF)
message(STATUS "  Compiled for Grendel:     ${BDD_BENCHMARK_GRENDEL}")
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

add_subdirectory (external/BuDDy buddy EXCLUDE_FROM_ALL)

# Cal Package (Jagesh Sanghavi, Rajeev Ranjan et al. - University of California, United States of America)
add_subdirectory (external/cal cal EXCLUDE_FROM_ALL)

# CUDD Package (Fabio Somenzi - University of Colorado Boulder, United States of America)
set(CUDD_SRC_DIR ${PROJECT_SOURCE_DIR}/external/cudd)
set(CUDD_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/cudd-build)
set(CUDD_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/cudd-install)
if (BDD_BENCHMARK_STATS)
  string(APPEND CUDD_CPPFLAGS -DDD_STATS)
endif(BDD_BENCHMARK_STATS)

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
set(CUDD_CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}} -Wall -Wextra -std=c11")
set(CUDD_CXXFLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}} -Wall -Wextra -std=c++11")
# `cudd-build` is the target that builds CUDD and installs it in `CUDD_INSTALL_DIR`
ExternalProject_Add(cudd-build
  PREFIX ${CUDD_BUILD_DIR}
  SOURCE_DIR ${CUDD_SRC_DIR}
  DOWNLOAD_COMMAND ${AUTORECONF_EXE} ${CUDD_SRC_DIR}
  CONFIGURE_COMMAND ${CUDD_SRC_DIR}/configure --srcdir=${CUDD_SRC_DIR} --prefix=${CUDD_INSTALL_DIR} --enable-obj CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CPPFLAGS=${CUDD_CPPFLAGS} CFLAGS=${CUDD_CFLAGS} CXXFLAGS=${CUDD_CXXFLAGS}
  BUILD_COMMAND ${MAKE_EXE} install
  BUILD_BYPRODUCTS ${CUDD_INSTALL_DIR}/lib/libcudd.a ${CUDD_INSTALL_DIR}/include/cudd.h ${CUDD_INSTALL_DIR}/include/cuddObj.hh
  USES_TERMINAL_CONFIGURE ON
  USES_TERMINAL_BUILD ON)
# `cudd` is the target used for `target_link_libraries(… cudd)`
add_library(cudd STATIC IMPORTED)
add_dependencies(cudd cudd-build)
# Already create the include directory now such that we can add it to the `cudd` target
file(MAKE_DIRECTORY ${CUDD_INSTALL_DIR}/include)
set_target_properties(cudd PROPERTIES
  IMPORTED_LOCATION ${CUDD_INSTALL_DIR}/lib/libcudd.a
  INTERFACE_INCLUDE_DIRECTORIES ${CUDD_INSTALL_DIR}/include)

# Biodivine/LibBDD (Samuel Pastva et al. - Institute of Science and Technology Austria)
add_subdirectory(external/lib-bdd-ffi lib-bdd-ffi EXCLUDE_FROM_ALL)

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

