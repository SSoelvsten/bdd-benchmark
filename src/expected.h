constexpr size_t UNKNOWN = static_cast<size_t>(-1);

// =============================================================================
// Knight's Tour Problem
//
// expected number taken from:
// [1] https://oeis.org/search?q=knights+tour
// [2] https://en.wikipedia.org/wiki/Knight%27s_tour#Number_of_tours
// [3] Our own numbers
const size_t expected_knights_tour_open[17] = {
  0,
  0,
  1,                // 1x1 [2]
  0,                // 2x1 [3]
  0,                // 2x2 [2]
  0,                // 3x2 [1]
  0,                // 3x3 [2]
  16,               // 4x3 [1]
  0,                // 4x4 [2]
  164,              // 5x4 [3]
  1728,             // 5x5 [2]
  37568,            // 6x5 [3]
  6637920,          // 6x6 [2]
  UNKNOWN,          // 7x6 [?]
  165575218320,     // 7x7 [2]
  UNKNOWN,          // 8x7 [?]
  19591828170979904 // 8x8 [2]
};

const size_t expected_knights_tour_closed[17] = {
  0,
  0,
  1,                // 1x1 [1]
  0,                // 2x1 [3]
  0,                // 2x2 [2]
  0,                // 3x2 [3]
  0,                // 3x3 [1]
  0,                // 4x3 [3]
  0,                // 4x4 [1]
  0,                // 5x4 [3]
  0,                // 5x5 [1]
  UNKNOWN,          // 6x5 [?]
  9862,             // 6x6 [2]
  UNKNOWN,          // 7x6 [?]
  0,                // 7x7 [1]
  UNKNOWN,          // 8x7 [?]
  13267364410532    // 8x8 [1]
};


// =============================================================================
// Queens Problem
//
// expected number taken from:
// https://en.wikipedia.org/wiki/Eight_queens_puzzle#Counting_solutions
const size_t expected_queens[28] = {
  0,
  1,
  0,
  0,
  2,
  10,
  4,
  40,
  92,
  352,
  724,
  2680,
  14200,
  73712,
  365596,
  2279184,
  14772512,
  95815104,
  666090624,
  4968057848,
  39029188884,
  314666222712,
  2691008701644,
  24233937684440,
  227514171973736,
  2207893435808352,
  22317699616364044,
  234907967154122528
};

// =============================================================================
// Tic-Tac-Toe Problem
//
// Expected numbers taken from "Parallel Disk-Based Computation for Large,
// Monolithic Binary Decision Diagrams" by Daniel Kunkle, Vlad Slavici, and Gene
// Cooperman.
uint64_t expected_tic_tac_toe[25] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  304,
  136288,
  9734400,
  296106640,
  5000129244,
};
