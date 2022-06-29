macro(add_bdd_benchmark NAME)
  # Adiar
  add_executable(adiar_${NAME}_bdd adiar/${NAME}_bdd.cpp)
  target_link_libraries(adiar_${NAME}_bdd PRIVATE adiar)

  # BuDDy
  add_executable(buddy_${NAME}_bdd buddy/${NAME}_bdd.cpp)
  target_include_directories(buddy_${NAME}_bdd PRIVATE "${BDD_SOURCE_DIR}")
  target_link_libraries(buddy_${NAME}_bdd PRIVATE bdd)

  # CUDD
  add_executable(cudd_${NAME}_bdd cudd/${NAME}_bdd.cpp)
  target_link_libraries(cudd_${NAME}_bdd PRIVATE "${CUDD_LIB_FILES}")
  target_include_directories(cudd_${NAME}_bdd PRIVATE "${CUDD_BUILD_DIR}/include")

  # Sylvan
  add_executable(sylvan_${NAME}_bdd sylvan/${NAME}_bdd.cpp)
  target_link_libraries(sylvan_${NAME}_bdd PRIVATE Sylvan::Sylvan)
endmacro(add_bdd_benchmark)

macro(add_zdd_benchmark NAME)
  # Adiar
  add_executable(adiar_${NAME}_zdd adiar/${NAME}_zdd.cpp)
  target_link_libraries(adiar_${NAME}_zdd PRIVATE adiar)

  # CUDD
  add_executable(cudd_${NAME}_zdd cudd/${NAME}_zdd.cpp)
  target_link_libraries(cudd_${NAME}_zdd PRIVATE "${CUDD_LIB_FILES}")
  target_include_directories(cudd_${NAME}_zdd PRIVATE "${CUDD_BUILD_DIR}/include")
endmacro(add_zdd_benchmark)

macro(link_bdd_extra NAME LIB_NAME)
  target_link_libraries(adiar_${NAME}_bdd  PRIVATE ${LIB_NAME})
  target_link_libraries(buddy_${NAME}_bdd  PRIVATE ${LIB_NAME})
  target_link_libraries(cudd_${NAME}_bdd   PRIVATE ${LIB_NAME})
  target_link_libraries(sylvan_${NAME}_bdd PRIVATE ${LIB_NAME})
endmacro(link_bdd_extra)

# ---------------------------------------------------------------------------- #
# Combinatorial Benchmarks
add_bdd_benchmark(queens)
add_zdd_benchmark(queens)

add_bdd_benchmark(tic_tac_toe)
add_zdd_benchmark(tic_tac_toe)

add_zdd_benchmark(knights_tour)

# ---------------------------------------------------------------------------- #
# Verification
add_bdd_benchmark(picotrav)
link_bdd_extra(picotrav libblifparse)
