macro(add_benchmark NAME)
  # Adiar
	add_executable(adiar_${NAME} adiar/${NAME}.cpp)
  #set_target_properties(adiar_${NAME} PROPERTIES CXX_STANDARD 17)
  target_link_libraries(adiar_${NAME} PRIVATE adiar)

  # BuDDy
  add_executable(buddy_${NAME} buddy/${NAME}.cpp)
  #set_target_properties(buddy_${NAME} PROPERTIES CXX_STANDARD 17)
  target_include_directories(buddy_${NAME} PRIVATE "${BDD_SOURCE_DIR}")
  target_link_libraries(buddy_${NAME} PRIVATE bdd)

  # CUDD
  add_executable(cudd_${NAME} cudd/${NAME}.cpp)
  #set_target_properties(cudd_${NAME} PROPERTIES CXX_STANDARD 17)
  target_link_libraries(cudd_${NAME} PRIVATE "${CUDD_LIB_FILES}")
  target_include_directories(cudd_${NAME} PRIVATE "${CUDD_BUILD_DIR}/include")

  # Sylvan
  add_executable(sylvan_${NAME} sylvan/${NAME}.cpp)
  #set_target_properties(sylvan_${NAME} PROPERTIES CXX_STANDARD 17)
  target_include_directories(sylvan_${NAME} PRIVATE "${SYLVAN_INCLUDE_DIR}")
  target_link_libraries(sylvan_${NAME} PRIVATE sylvan)
endmacro(add_benchmark)

macro(link_extra NAME LIB_NAME)
  target_link_libraries(adiar_${NAME}  PRIVATE ${LIB_NAME})
  target_link_libraries(buddy_${NAME}  PRIVATE ${LIB_NAME})
  target_link_libraries(cudd_${NAME}   PRIVATE ${LIB_NAME})
  target_link_libraries(sylvan_${NAME} PRIVATE ${LIB_NAME})
endmacro(link_extra)

# ---------------------------------------------------------------------------- #
# Combinatorial Benchmarks
add_benchmark(picotrav)
link_extra(picotrav libblifparse)

add_benchmark(queens)

add_benchmark(tic_tac_toe)

# ---------------------------------------------------------------------------- #
# SAT Solver benchmarks
add_benchmark(sat_pigeonhole_principle)

add_benchmark(sat_queens)
