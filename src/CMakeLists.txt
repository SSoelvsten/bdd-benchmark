macro(add_benchmark NAME)
  # Adiar
	add_executable(adiar_${NAME} adiar/${NAME}.cpp) 
  target_link_libraries(adiar_${NAME} adiar)

  # BuDDy
  add_executable(buddy_${NAME} buddy/${NAME}.cpp)
  target_include_directories(buddy_${NAME} PRIVATE "${BDD_SOURCE_DIR}")
  target_link_libraries(buddy_${NAME} bdd)

  # CUDD
  add_executable(cudd_${NAME} cudd/${NAME}.cpp)
  target_link_libraries(cudd_${NAME} PRIVATE "${CUDD_LIB_FILES}")
  target_include_directories(cudd_${NAME} PRIVATE "${CUDD_BUILD_DIR}/include")

  # Sylvan
  add_executable(sylvan_${NAME} sylvan/${NAME}.cpp)
  target_include_directories(sylvan_${NAME} PRIVATE "${SYLVAN_INCLUDE_DIR}")
  target_link_libraries(sylvan_${NAME} sylvan)
endmacro(add_benchmark)

add_benchmark(queens)
add_benchmark(sat_pigeonhole_principle)
add_benchmark(sat_queens)
add_benchmark(tic_tac_toe)
