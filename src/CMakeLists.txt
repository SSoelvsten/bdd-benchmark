add_subdirectory (common common)

macro(__add_benchmark LIB LIB_CMAKE NAME TYPE)
  add_executable(${LIB}_${NAME}_${TYPE} ${LIB}/${NAME}_${TYPE}.cpp)

  target_link_libraries(${LIB}_${NAME}_${TYPE} PRIVATE ${LIB_CMAKE})
  target_link_libraries(${LIB}_${NAME}_${TYPE} PRIVATE common)

  if (BDD_BENCHMARK_GRENDEL)
    target_compile_definitions(${LIB}_${NAME}_${TYPE} PRIVATE BDD_BENCHMARK_GRENDEL)
  endif()
  if (BDD_BENCHMARK_INCL_INIT)
    target_compile_definitions(${LIB}_${NAME}_${TYPE} PRIVATE BDD_BENCHMARK_INCL_INIT)
  endif()
  if (BDD_BENCHMARK_STATS)
    target_compile_definitions(${LIB}_${NAME}_${TYPE} PRIVATE BDD_BENCHMARK_STATS)
  endif()
  if (BDD_BENCHMARK_WAIT)
    target_compile_definitions(${LIB}_${NAME}_${TYPE} PRIVATE BDD_BENCHMARK_WAIT)
  endif()
endmacro(__add_benchmark)

macro(add_bdd_benchmark NAME)
  __add_benchmark(adiar  adiar   ${NAME} bdd)
  __add_benchmark(buddy  buddy   ${NAME} bdd)
  __add_benchmark(cudd   cudd    ${NAME} bdd)
  __add_benchmark(libbdd lib-bdd ${NAME} bdd)
  __add_benchmark(oxidd  oxidd   ${NAME} bdd)
endmacro(add_bdd_benchmark)

macro(add_bcdd_benchmark NAME)
  __add_benchmark(cal    cal            ${NAME} bcdd)
  __add_benchmark(cudd   cudd           ${NAME} bcdd)
  __add_benchmark(oxidd  oxidd          ${NAME} bcdd)
  __add_benchmark(sylvan sylvan::sylvan ${NAME} bcdd)
endmacro(add_bcdd_benchmark)

macro(add_zdd_benchmark NAME)
  __add_benchmark(adiar adiar ${NAME} zdd)
  __add_benchmark(cudd  cudd  ${NAME} zdd)
  __add_benchmark(oxidd oxidd ${NAME} zdd)
endmacro(add_zdd_benchmark)

macro(add_benchmark NAME)
  add_bdd_benchmark(${NAME})
  add_bcdd_benchmark(${NAME})
  add_zdd_benchmark(${NAME})
endmacro(add_benchmark)

macro(link_bdd_extra NAME EXTRA_LIB)
  target_link_libraries(adiar_${NAME}_bdd   PRIVATE ${EXTRA_LIB})
  target_link_libraries(buddy_${NAME}_bdd   PRIVATE ${EXTRA_LIB})
  target_link_libraries(cudd_${NAME}_bdd    PRIVATE ${EXTRA_LIB})
  target_link_libraries(libbdd_${NAME}_bdd  PRIVATE ${EXTRA_LIB})
  target_link_libraries(oxidd_${NAME}_bdd   PRIVATE ${EXTRA_LIB})
endmacro(link_bdd_extra)

macro(link_bcdd_extra NAME EXTRA_LIB)
  target_link_libraries(cal_${NAME}_bcdd    PRIVATE ${EXTRA_LIB})
  target_link_libraries(cudd_${NAME}_bcdd   PRIVATE ${EXTRA_LIB})
  target_link_libraries(oxidd_${NAME}_bcdd  PRIVATE ${EXTRA_LIB})
  target_link_libraries(sylvan_${NAME}_bcdd PRIVATE ${EXTRA_LIB})
endmacro(link_bcdd_extra)

macro(link_zdd_extra NAME EXTRA_LIB)
  target_link_libraries(adiar_${NAME}_zdd   PRIVATE ${EXTRA_LIB})
  target_link_libraries(cudd_${NAME}_zdd    PRIVATE ${EXTRA_LIB})
  target_link_libraries(oxidd_${NAME}_zdd   PRIVATE ${EXTRA_LIB})
endmacro(link_zdd_extra)

macro(link_extra NAME EXTRA_LIB)
  link_bdd_extra(${NAME}  ${EXTRA_LIB})
  link_bcdd_extra(${NAME} ${EXTRA_LIB})
  link_zdd_extra(${NAME}  ${EXTRA_LIB})
endmacro(link_extra)

# ---------------------------------------------------------------------------- #
# Combinatorial Benchmarks
add_benchmark(game-of-life)
add_benchmark(hamiltonian)
add_benchmark(queens)
add_benchmark(tic-tac-toe)

# ---------------------------------------------------------------------------- #
# Verification
add_bdd_benchmark(mcnet)
link_bdd_extra(mcnet Boost::boost)
link_bdd_extra(mcnet pugixml)

add_bcdd_benchmark(mcnet)
link_bcdd_extra(mcnet Boost::boost)
link_bcdd_extra(mcnet pugixml)

add_benchmark(picotrav)
link_extra(picotrav libblifparse)

add_bdd_benchmark(qbf)
add_bcdd_benchmark(qbf)

# ---------------------------------------------------------------------------- #
# Single Operation Benchmarks
add_benchmark(apply)

add_bdd_benchmark(relprod)
add_bcdd_benchmark(relprod)

add_benchmark(cnf)
