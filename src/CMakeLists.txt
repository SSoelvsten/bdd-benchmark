# ============================================================================ #
# BuDDy
# ============================================================================ #

# -- N Queens --
add_executable(buddy_queens buddy_queens.cpp)
target_include_directories(buddy_queens PRIVATE ../external/BuDDy/src)
target_link_libraries(buddy_queens bdd)

# -- Tic-Tac-Toe --
add_executable(buddy_tic_tac_toe buddy_tic_tac_toe.cpp)
target_include_directories(buddy_tic_tac_toe PRIVATE ../external/BuDDy/src)
target_link_libraries(buddy_tic_tac_toe bdd)


# ============================================================================ #
# Sylvan
# ============================================================================ #

# -- N Queens --
add_executable(sylvan_queens sylvan_queens.cpp)
target_link_libraries(sylvan_queens sylvan)

# -- Tic-Tac-Toe --
add_executable(sylvan_tic_tac_toe sylvan_tic_tac_toe.cpp)
target_link_libraries(sylvan_tic_tac_toe sylvan)
