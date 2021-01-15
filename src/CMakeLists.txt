# ============================================================================ #
# BuDDy
# ============================================================================ #

# -- N Queens --
add_executable(buddy_queens buddy_queens.cpp)
target_include_directories(buddy_queens PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_queens bdd)

# -- N Queens (SAT) --
add_executable(buddy_queens_sat buddy_queens_sat.cpp)
target_include_directories(buddy_queens_sat PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_queens_sat bdd)

# -- Tic-Tac-Toe --
add_executable(buddy_tic_tac_toe buddy_tic_tac_toe.cpp)
target_include_directories(buddy_tic_tac_toe PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_tic_tac_toe bdd)

# -- Pigeonhole Principle --
add_executable(buddy_pigeonhole_principle buddy_pigeonhole_principle.cpp)
target_include_directories(buddy_pigeonhole_principle PRIVATE "${BDD_SOURCE_DIR}")
target_link_libraries(buddy_pigeonhole_principle bdd)

# ============================================================================ #
# Adiar
# ============================================================================ #

# -- N Queens --
add_executable(adiar_queens adiar_queens.cpp)
target_link_libraries(adiar_queens adiar)
set_target_properties(adiar_queens PROPERTIES CXX_STANDARD 17)

# -- N Queens (SAT) --
add_executable(adiar_queens_sat adiar_queens_sat.cpp)
target_link_libraries(adiar_queens_sat adiar)
set_target_properties(adiar_queens_sat PROPERTIES CXX_STANDARD 17)

# -- Tic-Tac-Toe --
add_executable(adiar_tic_tac_toe adiar_tic_tac_toe.cpp)
target_link_libraries(adiar_tic_tac_toe adiar)
set_target_properties(adiar_tic_tac_toe PROPERTIES CXX_STANDARD 17)

# -- Pigeonhole Principle --
add_executable(adiar_pigeonhole_principle adiar_pigeonhole_principle.cpp)
target_link_libraries(adiar_pigeonhole_principle adiar)
set_target_properties(adiar_pigeonhole_principle PROPERTIES CXX_STANDARD 17)

# ============================================================================ #
# Sylvan
# ============================================================================ #

# -- N Queens --
add_executable(sylvan_queens sylvan_queens.cpp)
target_include_directories(sylvan_queens PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_queens sylvan)

# -- N Queens (SAT) --
add_executable(sylvan_queens_sat sylvan_queens_sat.cpp)
target_include_directories(sylvan_queens_sat PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_queens_sat sylvan)

# -- Tic-Tac-Toe --
add_executable(sylvan_tic_tac_toe sylvan_tic_tac_toe.cpp)
target_include_directories(sylvan_tic_tac_toe PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_tic_tac_toe sylvan)

# -- Pigeonhole Principle --
add_executable(sylvan_pigeonhole_principle sylvan_pigeonhole_principle.cpp)
target_include_directories(sylvan_pigeonhole_principle PRIVATE "${SYLVAN_INCLUDE_DIR}")
target_link_libraries(sylvan_pigeonhole_principle sylvan)
