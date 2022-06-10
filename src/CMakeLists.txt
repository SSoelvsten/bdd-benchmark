macro(add_bdd_benchmark NAME)
  # Adiar
  add_executable(adiar_${NAME} adiar/${NAME}.cpp)
  target_link_libraries(adiar_${NAME} PRIVATE adiar)

  # BuDDy
  add_executable(buddy_${NAME} buddy/${NAME}.cpp)
  target_include_directories(buddy_${NAME} PRIVATE "${BDD_SOURCE_DIR}")
  target_link_libraries(buddy_${NAME} PRIVATE bdd)

  # CUDD
  add_executable(cudd_${NAME} cudd/${NAME}.cpp)
  target_link_libraries(cudd_${NAME} PRIVATE "${CUDD_LIB_FILES}")
  target_include_directories(cudd_${NAME} PRIVATE "${CUDD_BUILD_DIR}/include")

  # Sylvan
  add_executable(sylvan_${NAME} sylvan/${NAME}.cpp)
  target_link_libraries(sylvan_${NAME} PRIVATE Sylvan::Sylvan)
endmacro(add_bdd_benchmark)

macro(add_zdd_benchmark NAME)
  # Adiar
  add_executable(adiar_${NAME} adiar/${NAME}.cpp)
  target_link_libraries(adiar_${NAME} PRIVATE adiar)

  # CUDD
  add_executable(cudd_${NAME} cudd/${NAME}.cpp)
  target_link_libraries(cudd_${NAME} PRIVATE "${CUDD_LIB_FILES}")
  target_include_directories(cudd_${NAME} PRIVATE "${CUDD_BUILD_DIR}/include")
endmacro(add_zdd_benchmark)

macro(link_extra NAME LIB_NAME)
  target_link_libraries(adiar_${NAME}  PRIVATE ${LIB_NAME})
  target_link_libraries(buddy_${NAME}  PRIVATE ${LIB_NAME})
  target_link_libraries(cudd_${NAME}   PRIVATE ${LIB_NAME})
  target_link_libraries(sylvan_${NAME} PRIVATE ${LIB_NAME})
endmacro(link_extra)

# ---------------------------------------------------------------------------- #
# Combinatorial Benchmarks
add_bdd_benchmark(queens)
add_zdd_benchmark(queens_zdd)

add_bdd_benchmark(tic_tac_toe)

add_zdd_benchmark(knights_tour_zdd)

# ---------------------------------------------------------------------------- #
# SAT Solver benchmarks
add_bdd_benchmark(sat_pigeonhole_principle)

add_bdd_benchmark(sat_queens)

# ---------------------------------------------------------------------------- #
# Verification
add_bdd_benchmark(picotrav)
link_extra(picotrav libblifparse)
