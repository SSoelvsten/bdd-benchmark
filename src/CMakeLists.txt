# ============================================================================ #
# BuDDy
# ============================================================================ #

# -- N Queens --
add_executable(buddy_queens buddy_queens.cpp)
target_include_directories(buddy_queens PRIVATE ../external/BuDDy/src)
target_link_libraries(buddy_queens bdd)

# -- N Queens (SAT) --
add_executable(buddy_queens_sat buddy_queens_sat.cpp)
target_include_directories(buddy_queens_sat PRIVATE ../external/BuDDy/src)
target_link_libraries(buddy_queens_sat bdd)

# -- Tic-Tac-Toe --
add_executable(buddy_tic_tac_toe buddy_tic_tac_toe.cpp)
target_include_directories(buddy_tic_tac_toe PRIVATE ../external/BuDDy/src)
target_link_libraries(buddy_tic_tac_toe bdd)

# -- Pigeonhole Principle --
add_executable(buddy_pigeonhole_principle buddy_pigeonhole_principle.cpp)
target_include_directories(buddy_pigeonhole_principle PRIVATE ../external/BuDDy/src)
target_link_libraries(buddy_pigeonhole_principle bdd)

# ============================================================================ #
# COOM
# ============================================================================ #

# -- N Queens --
add_executable(coom_queens coom_queens.cpp)
target_link_libraries(coom_queens coom)
set_target_properties(coom_queens PROPERTIES CXX_STANDARD 17)

# -- N Queens (SAT) --
add_executable(coom_queens_sat coom_queens_sat.cpp)
target_link_libraries(coom_queens_sat coom)
set_target_properties(coom_queens_sat PROPERTIES CXX_STANDARD 17)

# -- Tic-Tac-Toe --
add_executable(coom_tic_tac_toe coom_tic_tac_toe.cpp)
target_link_libraries(coom_tic_tac_toe coom)
set_target_properties(coom_tic_tac_toe PROPERTIES CXX_STANDARD 17)

# -- Pigeonhole Principle --
add_executable(coom_pigeonhole_principle coom_pigeonhole_principle.cpp)
target_link_libraries(coom_pigeonhole_principle coom)
set_target_properties(coom_pigeonhole_principle PROPERTIES CXX_STANDARD 17)

# ============================================================================ #
# Sylvan
# ============================================================================ #

# -- N Queens --
add_executable(sylvan_queens sylvan_queens.cpp)
target_link_libraries(sylvan_queens sylvan)

# -- N Queens (SAT) --
add_executable(sylvan_queens_sat sylvan_queens_sat.cpp)
target_link_libraries(sylvan_queens_sat sylvan)

# -- Tic-Tac-Toe --
add_executable(sylvan_tic_tac_toe sylvan_tic_tac_toe.cpp)
target_link_libraries(sylvan_tic_tac_toe sylvan)

# -- Pigeonhole Principle --
add_executable(sylvan_pigeonhole_principle sylvan_pigeonhole_principle.cpp)
target_link_libraries(sylvan_pigeonhole_principle sylvan)
