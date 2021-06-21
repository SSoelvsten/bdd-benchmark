# ============================================================================ #
# BuDDy
# ============================================================================ #

# -- Queens --
add_executable(buddy_queens buddy_queens.cpp)
target_include_directories(buddy_queens PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_queens bdd)

# -- SAT Solver (Pigeonhole Principle) --
add_executable(buddy_sat_pigeonhole_principle buddy_sat_pigeonhole_principle.cpp)
target_include_directories(buddy_sat_pigeonhole_principle PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_sat_pigeonhole_principle bdd)

# -- SAT Solver (Queens) --
add_executable(buddy_sat_queens buddy_sat_queens.cpp)
target_include_directories(buddy_sat_queens PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_sat_queens bdd)

# -- Tic-Tac-Toe --
add_executable(buddy_tic_tac_toe buddy_tic_tac_toe.cpp)
target_include_directories(buddy_tic_tac_toe PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_tic_tac_toe bdd)

# ============================================================================ #
# Adiar
# ============================================================================ #

# -- Queens --
add_executable(adiar_queens adiar_queens.cpp)
target_link_libraries(adiar_queens adiar)

# -- SAT Solver (Pigeonhole Principle) --
add_executable(adiar_sat_pigeonhole_principle adiar_sat_pigeonhole_principle.cpp)
target_link_libraries(adiar_sat_pigeonhole_principle adiar)

# -- SAT Solver (Queens) --
add_executable(adiar_sat_queens adiar_sat_queens.cpp)
target_link_libraries(adiar_sat_queens adiar)

# -- Tic-Tac-Toe --
add_executable(adiar_tic_tac_toe adiar_tic_tac_toe.cpp)
target_link_libraries(adiar_tic_tac_toe adiar)

# ============================================================================ #
# Sylvan
# ============================================================================ #

# -- Queens --
add_executable(sylvan_queens sylvan_queens.cpp)
target_include_directories(sylvan_queens PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_queens sylvan)

# -- SAT Solver (Pigeonhole Principle) --
add_executable(sylvan_sat_pigeonhole_principle sylvan_sat_pigeonhole_principle.cpp)
target_include_directories(sylvan_sat_pigeonhole_principle PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_sat_pigeonhole_principle sylvan)

# -- SAT Solver (Queens) --
add_executable(sylvan_sat_queens sylvan_sat_queens.cpp)
target_include_directories(sylvan_sat_queens PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_sat_queens sylvan)

# -- Tic-Tac-Toe --
add_executable(sylvan_tic_tac_toe sylvan_tic_tac_toe.cpp)
target_include_directories(sylvan_tic_tac_toe PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_tic_tac_toe sylvan)
