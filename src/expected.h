constexpr size_t UNKNOWN = static_cast<size_t>(-1);

// =============================================================================
// Hamiltonian Cycle Problems

// Knight's Tour
//   expected number taken from:
//   [1] https://oeis.org/search?q=knights+tour
//   [2] https://en.wikipedia.org/wiki/Knight%27s_tour#Number_of_tours
//
// If otherwise not stated, the numbers are based on prior runs of our own.
const size_t expected_hamiltonian__knight[17] = {
  0,
  0,
  1,                // 1x1 [1]
  0,                // 2x1 [_]
  0,                // 2x2 [2]
  0,                // 3x2 [_]
  0,                // 3x3 [1]
  0,                // 4x3 [_]
  0,                // 4x4 [1]
  0,                // 5x4 [_]
  0,                // 5x5 [1]
  8,                // 6x5 [_]
  9862,             // 6x6 [2]
  UNKNOWN,          // 7x6 [_]
  0,                // 7x7 [1]
  UNKNOWN,          // 8x7 [_]
  13267364410532    // 8x8 [1]
};

// Grid Graph Tours
//   expected number taken from:
//   [3] https://oeis.org/A003763
//
// If otherwise not stated, the numbers are based on prior runs of our own.
const size_t expected_hamiltonian__grid[13] = {
  0,                   //  0x0  [_]
  1,                   //  1x1  [_]
  1,                   //  2x2  [3]
  0,                   //  3x3  [3]
  6,                   //  4x4  [3]
  0,                   //  5x5  [3]
  1072,                //  6x6  [3]
  0,                   //  7x7  [3]
  4638576,             //  8x8  [3]
  0,                   //  9x9  [3]
  467260456608,        // 10x10 [3]
  0,                   // 11x11 [3]
  1076226888605605706, // 12x12 [3]
  // remaining numbers do not fit into 64 bits
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
