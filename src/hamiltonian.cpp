// Assertions
#include <cassert>

// Data Structures
#include <array>
#include <sstream>
#include <string>

// Math, e.g. absolute and minimum value
#include <cmath>

// Sorting algorithm
#include <algorithm>

// Types
#include <cstdlib>

// Other
#include <utility>

#include "common/adapter.h"
#include "common/array.h"
#include "common/chrono.h"
#include "common/input.h"

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

////////////////////////////////////////////////////////////////////////////////
//                              Input Parsing                                 //
////////////////////////////////////////////////////////////////////////////////

int N_rows = -1;
int N_cols = -1;

enum encoding
{
  BINARY,
  UNARY,
  CRT__UNARY,
  TIME
};

std::string
to_string(const encoding& e)
{
  switch (e) {
  case encoding::BINARY: return "Binary (Adder)";
  case encoding::UNARY: return "Unary (One-hot)";
  case encoding::CRT__UNARY: return "Chinese Remainder Theorem: Unary (One-hot)";
  case encoding::TIME: return "Time-based";
  default: return "Unknown";
  }
}

encoding enc = encoding::TIME;

class parsing_policy
{
public:
  static constexpr std::string_view name = "Hamiltonian";
  static constexpr std::string_view args = "n:e:";

  static constexpr std::string_view help_text = "        -n n        [4]      Size of grid\n"
                                                "        -e ENC      [time]   Problem encoding";

  static inline bool
  parse_input(const int c, const char* arg)
  {
    switch (c) {
    case 'n': {
      const int N = std::stoi(arg);
      if (N <= 0) {
        std::cerr << "  Must specify positive board size (-n)\n";
        return true;
      }
      if (N_rows < 0) {
        N_rows = N;
      } else {
        N_cols = N;
      }
      return false;
    }
    case 'e': {
      const std::string lower_arg = ascii_tolower(arg);

      if (lower_arg == "binary") {
        enc = encoding::BINARY;
      } else if (lower_arg == "unary" || lower_arg == "one-hot") {
        enc = encoding::UNARY;
      } else if (lower_arg == "crt_unary" || lower_arg == "crt_one-hot") {
        enc = encoding::CRT__UNARY;
      } else if (lower_arg == "time" || lower_arg == "t") {
        enc = encoding::TIME;
      } else {
        std::cerr << "Undefined option: " << arg << "\n";
        return true;
      }
      return false;
    }
    default: return true;
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
//                           Common board logic                               //
////////////////////////////////////////////////////////////////////////////////

/// \brief Number of rows
inline int
rows()
{
  return N_rows;
}

/// \brief Minimum valid row value
constexpr int
MIN_ROW()
{
  return 0;
}

/// \brief Maximum valid row value
inline int
MAX_ROW()
{
  return rows() - 1;
}

/// \brief Number of columns
inline int
cols()
{
  return N_cols;
}

/// \brief Minimum valid column value
constexpr int
MIN_COL()
{
  return 0;
}

/// \brief Maximum valid column value
inline int
MAX_COL()
{
  return cols() - 1;
}

/// \brief Number of cells on the chess board
inline int
cells()
{
  return rows() * cols();
}

/// \brief Class to encapsulate logic related to a cell and the move relation.
class cell
{
  friend class edge;

  //////////////////////////////////////////////////////////////////////////////
  // Members
private:
  int _r;
  int _c;

  //////////////////////////////////////////////////////////////////////////////
  // Constructors
public:
  /// \brief Default construction of illegal cell [-1,-1] outside the board.
  cell()
    : _r(-1)
    , _c(-1)
  {}

  /// \brief Construction of cell [r,c].
  ///
  /// \remark This does not check whether the cell actually is legal. To do so,
  ///         please use `out_of_range`.
  cell(int r, int c)
    : _r(r)
    , _c(c)
  { /* TODO: throw std::out_of_range if given bad (r,c)? */
  }

  /// \brief Converts back from a diagram variable to the cell.
  ///
  /// \pre The variable `dd_var` must already have been unshifted.
  cell(int dd_var)
    : _r(((dd_var) / cols()) % rows())
    , _c((dd_var) % cols())
  {
    assert(0 <= dd_var && dd_var < cells());
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accessor and DD conversion
public:
  int
  row() const
  {
    return _r;
  }

  int
  col() const
  {
    return _c;
  }

  /// \brief Row-major DD variable name
  ///
  /// \param shift The number of bits to shift.
  ///
  /// \pre `!out_of_range()`
  int
  dd_var(const int shift = 0) const
  {
    if (this->out_of_range()) { throw std::out_of_range("Cell is out-of-range"); }
    return shift + (cols() * _r) + _c;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Quality of life
public:
  bool
  operator==(const cell& o) const
  {
    return this->_r == o._r && this->_c == o._c;
  }

  bool
  operator!=(const cell& o) const
  {
    return !(*this == o);
  }

  bool
  operator<(const cell& o) const
  {
    return this->dd_var() < o.dd_var();
  }

  bool
  operator>(const cell& o) const
  {
    return this->dd_var() > o.dd_var();
  }

  /// \brief Human-friendly string
  std::string
  to_string() const
  {
    std::string res;
    res += '1' + _r;
    res += 'A' + _c;
    return res;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Position and Move logic

  /*
  // ---------------------------------
  // Knight moves ( https://oeis.org/A165134 )
public:
  /// \brief Number of possible neighbours for a knight
  static constexpr int max_moves = 8;

  /// \brief The number of active rows above/below
  static constexpr int active_rows = 2;

private:
  /// \brief Hard coded moves relative to the current cell (following the
  ///        variable ordering as per `dd_var`).
  ///
  /// \details This hardcoding is done with the intention to improve performance.
  static constexpr int moves[max_moves][2] = {
    { -2, -1 },
    { -2,  1 },
    { -1, -2 },
    { -1,  2 },
    {  1, -2 },
    {  1,  2 },
    {  2, -1 },
    {  2,  1 }
  };
  */

  // ---------------------------------
  // Grid Graph moves ( https://oeis.org/A003763 )
public:
  /// \brief Number of possible neighbours
  static constexpr int max_moves = 4;

  /// \brief The number of active rows above/below
  static constexpr int active_rows = 1;

private:
  /// \brief Hard coded moves relative to the current cell (following the
  ///        variable ordering as per `dd_var`).
  ///
  /// \details This hardcoding is done with the intention to improve performance.
  static constexpr int moves[max_moves][2] = { { -1, 0 }, { 0, -1 }, { 0, 1 }, { 1, 0 } };

public:
  /// \brief Whether this cell represents an actual valid position on the board.
  bool
  out_of_range() const
  {
    return row() < 0 || MAX_ROW() < row() || col() < 0 || MAX_COL() < col();
  }

  /// \brief Vertical distance between two cells
  int
  vertical_dist_to(const cell& o) const
  {
    return std::abs(this->row() - o.row());
  }

  /// \brief Horizontal distance between two cells
  int
  horizontal_dist_to(const cell& o) const
  {
    return std::abs(this->col() - o.col());
  }

  /// \brief Manhattan distance to cell `o`
  int
  manhattan_dist_to(const cell& o) const
  {
    return vertical_dist_to(o) + horizontal_dist_to(o);
  }

  /// \brief Whether there is a single move from `this` to `o`
  ///
  /// \details One can move from `this` to `o` if one moves at least one in each
  ///          dimension and by exactly three cells.
  bool
  has_move_to(const cell& o) const
  {
    /*
    // ---------------------------------
    // Knight moves
    return (0 < vertical_dist_to(o))
      && (0 < horizontal_dist_to(o))
      && (this->manhattan_dist_to(o) == 3);
    */

    // ---------------------------------
    // Grid Graph moves
    return this->manhattan_dist_to(o) == 1;
  }

  /// \brief All cells on the board that can be reached from this cell
  std::vector<cell>
  neighbours() const
  {
    std::vector<cell> res;
    for (int i = 0; i < max_moves; ++i) {
      const cell neighbour(this->row() + moves[i][0], this->col() + moves[i][1]);

      if (neighbour.out_of_range()) { continue; }

      res.push_back(neighbour);
    }
    return res;
  }

  /// \brief Whether this cell is reachable from any other cell.
  bool
  has_neighbour() const
  {
    /*
    // ---------------------------------
    // Knight moves

    // For any board larger than 1x1, there is at least one neighbour. For the
    // 3x3 board the center is the only unreachable position.
    return cells() != 9 || (*this != cell(1,1));
    */

    // ---------------------------------
    // Grid Graph moves

    // For any board larger than 1x1, there is at least one neighbour
    return cells() > 1;
  }

public:
  //////////////////////////////////////////////////////////////////////////////
  // Special cell functions

  /// \brief Top-left corner `(0,0)`
  static inline cell
  special_0()
  {
    return cell(0, 0);
  }

  /*
  // ---------------------------------
  // Knight moves

  /// \brief First cell moved to from `(0,0)` (breaking symmetries)
  static inline cell special_1()
  { return cell(1,2); }

  /// \brief Other neighbour encountered at the end (closing the cycle)
  static inline cell special_2()
  { return cell(2,1); }
  */

  // ---------------------------------
  // Grid Graph moves (also known as "closed rook")

  /// \brief First cell moved to from `(0,0)` (breaking symmetries)
  static inline cell
  special_1()
  {
    return cell(1, 0);
  }

  /// \brief Other neighbour encountered at the end (closing the cycle)
  static inline cell
  special_2()
  {
    return cell(0, 1);
  }

  /// \brief Get the three cells involved in the upper-left corner
  static inline std::array<cell, 3>
  specials()
  {
    return { special_0(), special_1(), special_2() };
  }

  /// \brief Whether this cell is special with regards to the upper left corner.
  bool
  is_special() const
  {
    const cell c0 = special_0();
    return (*this == c0 || this->has_move_to(c0));
  }

public:
  //////////////////////////////////////////////////////////////////////////////
  // Minimum and maximum cells with respect to variable ordering

  /// \brief First cell with respect to the variable ordering.
  static inline cell
  first()
  {
    return { MIN_ROW(), MIN_COL() };
  }

  /// \brief Last cell with respect to the variable ordering.
  static inline cell
  last()
  {
    return { MAX_ROW(), MAX_COL() };
  }
};

/// \brief Hash function for `cell` class
template <>
struct std::hash<cell>
{
  std::size_t
  operator()(const cell& c) const
  {
    return std::hash<int>{}(c.dd_var());
  }
};

/// \brief Class to encapsulate logic related to a cell and the move relation.
class edge
{
private:
  cell _u;
  cell _v;

public:
  /// \brief Default construction
  edge() = default;

  /// \brief Construction of an edge given two cells on the board.
  edge(const cell& u, const cell& v)
    : _u(u)
    , _v(v)
  {
    if (u.out_of_range()) {
      throw std::out_of_range("Cell 'u'=" + u.to_string() + " is out of range");
    }
    if (v.out_of_range()) {
      throw std::out_of_range("Cell 'v'=" + v.to_string() + " is out of range");
    }
    if (!u.has_move_to(v)) {
      throw std::out_of_range("Edge " + this->to_string() + " is not a valid move");
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accessor and DD conversion
public:
  /// \brief Source
  const cell&
  u() const
  {
    return _u;
  }

  /// \brief Target
  const cell&
  v() const
  {
    return _v;
  }

  /// \brief The "index" for this edge `u`.
  ///
  /// \remark The index is independent of the edge's direction.
  int
  idx() const
  {
    assert(u() != v());

    const int r_diff = this->v().row() - this->u().row();
    const int c_diff = this->v().col() - this->u().col();

    // Return index in list of relative moves within `cell` class
    for (int i = 0; i < cell::max_moves; ++i) {
      if (cell::moves[i][0] == r_diff && cell::moves[i][1] == c_diff) { return i; }
    }
    return -1;
  }

  /// \brief Whether `u` has an edge to a neighbour with edge-index `i`.
  static bool
  has_idx(const cell& u, const int i)
  {
    for (const cell& v : u.neighbours()) {
      if (edge(u, v).idx() == i) { return true; }
    }
    return false;
  }

  /// \brief Whether the source or the target are invalid values
  bool
  out_of_range() const
  {
    return this->u().out_of_range() || this->v().out_of_range();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Quality of life
public:
  bool
  operator==(const edge& o) const
  {
    return this->u() == o.u() && this->v() == o.v();
  }

  bool
  operator!=(const edge& o) const
  {
    return !(*this == o);
  }

  /// \brief Obtain the reversed directed edge, i.e. from `v` to `u`.
  edge
  reversed() const
  {
    return edge{ this->v(), this->u() };
  }

  /// \brief Human-friendly string
  std::string
  to_string() const
  {
    return this->u().to_string() + "->" + this->v().to_string();
  }
};

/// \brief Hash for `edge` class
template <>
struct std::hash<edge>
{
  std::size_t
  operator()(const edge& e) const
  {
    return std::hash<cell>{}(e.u()) ^ std::hash<cell>{}(e.v());
  }
};

/// \brief Cells in descending order (relative to variable ordering).
std::vector<cell> cells_descending;

/// \brief Initialise the list of all cells on the board (descendingly)
///        following the variable ordering.
void
init_cells_descending()
{
  assert(cells_descending.size() == 0);

  cells_descending.clear();
  cells_descending.reserve(cells());

  for (int row = MAX_ROW(); MIN_ROW() <= row; --row)
    for (int col = MAX_COL(); MIN_COL() <= col; --col) cells_descending.push_back(cell(row, col));

  assert(cells_descending.size() == static_cast<size_t>(cells()));

  // TODO (variable orderings):
  // std::sort<std::greater<cell>>(cells_descending.begin(), cells_descending.end());
}

////////////////////////////////////////////////////////////////////////////////
/// \brief Gadgets for the `encoding::BINARY` and `encoding::(CRT__)UNARY`
///        encodings.
///
/// Simple(ish) encoding with the goal to minimise the number of variables alive
/// at the same time. To this end, we encode the (roughly) 4N edges of the
/// transition relation as variables. If an edge `u->v` is set to true, then we
/// encode that `v` must be the successor of `u` via a gadget.
///
/// We have three different gadgets to pick from:
/// 1. A Binary Adder with an arbitrary modulo value.
/// 2. An Linear-Feedback Shift Register (LFSR) that can only be used with
///    Mersenne Primes.
/// 3. A One-hot encoding that uses linear number of variables instead.
///
/// The special cells have their counters forced to 0, 1, and 63.
///
/// \remark This is expected to work best with BDDs, but also ok for ZDDs.
////////////////////////////////////////////////////////////////////////////////
namespace enc_gadgets
{
  /// \brief Number of undirected edges
  inline int
  edges_undirected()
  {
    return rows() > 1 && cols() > 1 ? 4 * cells() - 6 * (rows() + cols()) + 8 : 0;
  }

  /// \brief Number of (directed) edges
  inline int
  edges()
  {
    return 2 * edges_undirected();
  }

  /// \brief Obtain the ceiling of log2
  inline int
  log2(const int x)
  {
    return static_cast<int>(std::ceil(std::log2(x)));
  }

  /// \brief Possible types of bits for this encoding
  enum class var_t
  {
    in_bit     = 0,
    out_bit    = 1,
    gadget_bit = 2,
  };

  /// \brief Obtain the set of smallest "prime" numbers for gadget
  std::vector<int>
  gadget_moduli(const encoding& opt)
  {
    switch (opt) {
    case encoding::BINARY: {
      return { 1 << log2(cells()) };
    }
    case encoding::UNARY: {
      return { cells() };
    }
    case encoding::CRT__UNARY: {
      // Find the smallest number of prime numbers whose least common multiple
      // is larger than half the number of cells.
      const std::vector<int> candidates[5] = { { 7 }, { 3, 5 }, { 3, 7 }, { 5, 7 }, { 3, 5, 7 } };

      for (const std::vector<int>& candidate : candidates) {
        // Determine the usability and cost of this solution
        int lcm = 1;

        for (const int p : candidate) lcm *= p;

        if (cells() / 2 < lcm) return candidate;
      }
      throw std::out_of_range("No primes available for a chess board this big!");
    }
    case encoding::TIME:
    default: {
      return {};
    }
    }
  }

  /// \brief Number of bits to represent the (directed) in- or out-going edge to
  ///        a single node in the graph.
  inline int
  bits_per_edge(const encoding& opt)
  {
    return opt == encoding::BINARY ? log2(cell::max_moves) : cell::max_moves;
  }

  /// \brief Number of total bits used to identify the chosen edges.
  inline int
  edge_vars(const encoding& opt)
  {
    // For each cell, we have two set of bit-values: in-going and out-going.
    return cells() * 2 * bits_per_edge(opt);
  }

  /// \brief Obtain the dd variable for an in-going or out-going edge at cell `c`.
  inline int
  edge_var(const cell& c, int bit, bool out_going, [[maybe_unused]] const encoding& opt)
  {
    assert(bit < bits_per_edge(opt));
    return (c.dd_var() * 2 * bits_per_edge(opt)) + (2 * bit + out_going);
  }

  /// \brief Decision diagram variable for a bit of the in-going edge to cell `c`.
  inline int
  edge_in_var(const cell& c, int bit, const encoding& opt)
  {
    return edge_var(c, bit, false, opt);
  }

  /// \brief Decision diagram variable for a bit of the out-going edge to cell `c`.
  inline int
  edge_out_var(const cell& c, int bit, const encoding& opt)
  {
    return edge_var(c, bit, true, opt);
  }

  /// \brief Obtain the number of bits per gadget given a certain prime.
  inline int
  bits_per_gadget(const int p, const encoding& opt)
  {
    return opt == encoding::BINARY ? log2(p) : p;
  }

  /// \brief Obtain the largest number of bits per gadget over all primes.
  inline int
  bits_per_gadget(const encoding& opt)
  {
    return bits_per_gadget(gadget_moduli(opt).back(), opt);
  }

  /// \brief Number of total bits used for the gadgets
  inline int
  gadget_vars(const encoding& opt)
  {
    // For each cell, we have a single set of bits for the gadget.
    return cells() * bits_per_gadget(opt);
  }

  /// \brief Obtain the dd variable for a bit in a gadget for cell `c`.
  inline int
  gadget_var(const cell& c, int bit, [[maybe_unused]] const encoding& opt)
  {
    assert(bit < bits_per_gadget(opt));
    return edge_vars(opt) + c.dd_var(cells() * bit);
  }

  inline int
  MIN_CELL_VAR(const encoding& /*opt*/)
  {
    return 0;
  }

  inline int
  MAX_CELL_VAR(const encoding& opt)
  {
    return edge_vars(opt) - 1;
  }

  inline int
  MIN_GADGET_VAR(const encoding& opt)
  {
    return edge_vars(opt);
  }

  inline int
  MAX_GADGET_VAR(const encoding& opt)
  {
    return edge_vars(opt) + gadget_vars(opt) - 1;
  }

  /// \brief Minimum variable
  inline int
  MIN_VAR(const encoding& opt)
  {
    return MIN_CELL_VAR(opt);
  }

  /// \brief Maximum variable
  inline int
  MAX_VAR(const encoding& opt)
  {
    return MAX_GADGET_VAR(opt);
  }

  /// \brief Number of variables used for the encoding.
  inline int
  vars(const encoding& opt)
  {
    return MAX_VAR(opt) + 1;
  }

  /// \brief Number of variables to use for final model count.
  inline int
  satcount_vars(const encoding& opt)
  {
    return cells() * bits_per_edge(opt);
  }

  /// \brief Obtain the cell corresponding to the given DD variable.
  inline cell
  cell_of_var(const int x, const encoding& opt)
  {
    assert(x < vars(opt));

    const int x_unshifted = x < edge_vars(opt) ? x / (2 * bits_per_edge(opt)) : x % cells();

    return cell(x_unshifted);
  }

  /// \brief The bit-index of a variable for some cell c.
  inline int
  bit_of_var(int x, const encoding& opt)
  {
    return x < edge_vars(opt) ? x % (2 * bits_per_edge(opt)) : x / cells();
  }

  /// \brief Obtain the type of a given variable.
  inline var_t
  type_of_var(int x, const encoding& opt)
  {
    return x < edge_vars(opt) ? static_cast<var_t>(bit_of_var(x, opt) % 2) : var_t::gadget_bit;
  }

  /// \brief Obtain the next bit for a fixed integer `x`, depending on encoding.
  ///
  /// As a side-effect the value of `x` is changed accordingly.
  inline bool
  next_fixed_bit(int& x, const encoding& opt)
  {
    switch (opt) {
    case encoding::BINARY: {
      const bool res = x % 2;
      x /= 2;
      return res;
    }
    case encoding::UNARY:
    case encoding::CRT__UNARY: {
      const bool res = x == 0;
      x -= 1; // <-- this is intended to potentially become negative.
      return res;
    }
    default: throw std::out_of_range("Encoding unsupported.");
    }
  }

  /// \brief List of the first few prime numbers.
  constexpr std::array<int, 11> primes = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };

  /// \brief Whether a value (below 32) is a prime
  ///
  /// This algorithm is nothing fancy, but just compares with a pre-compiled
  /// list of numbers.
  ///
  /// \throws std::out_of_range If `i` is larger than the largest "known" prime.
  bool
  is_prime(int i)
  {
    if (i > 32) { throw std::out_of_range("Primes are uncomputed for such large a value"); }
    for (const int p : primes) {
      if (i == p) { return true; }
    }
    return false;
  }

  /// \brief List of all exponents for Mersenne primes that fit into an `int`.
  constexpr std::array<int, 8> mersenne_exponents = { 2, 3, 5, 7, 13, 17, 19, 31 };

  /// \brief Whether a given value is a Mersenne prime
  bool
  is_mersenne_prime(int i)
  {
    for (const int e : mersenne_exponents) {
      const int p = (1 << e) - 1;
      if (i == p) { return true; }
    }
    return false;
  }

  /// \brief Whether a number is a power of two.
  bool
  is_power_of_two(int i)
  {
    return (i & (i - 1)) == 0;
  }

  /// \brief Construct edge-variables with special cells fixed in their choice.
  ///
  /// The gadget is constructed such, that the edge-index is big-endian.
  template <typename Adapter>
  typename Adapter::dd_t
  init_special(Adapter& adapter, const encoding& opt)
  {
    std::array<std::tuple<cell, var_t, int>, 4> fixed_bits{ {
      { cell::special_0(), var_t::in_bit, edge(cell::special_0(), cell::special_2()).idx() },
      { cell::special_0(), var_t::out_bit, edge(cell::special_0(), cell::special_1()).idx() },
      { cell::special_1(), var_t::in_bit, edge(cell::special_1(), cell::special_0()).idx() },
      { cell::special_2(), var_t::out_bit, edge(cell::special_2(), cell::special_0()).idx() },
    } };

    const auto bot = adapter.build_node(false);
    auto root      = adapter.build_node(true);

    for (int x = MAX_CELL_VAR(opt); MIN_CELL_VAR(opt) <= x; --x) {
      const cell c_x  = cell_of_var(x, opt);
      const var_t t_x = type_of_var(x, opt);

      for (auto& [c, t, val] : fixed_bits) {
        if (c == c_x && t == t_x) {
          const bool bit_val = next_fixed_bit(val, opt);
          root               = adapter.build_node(x, bit_val ? bot : root, bit_val ? root : bot);
          goto init_special__loop_end;
        }
      } // for-else:
      root = adapter.build_node(x, root, root);

    init_special__loop_end:
      continue;
    }

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Constraint making sure exactly one of the in-going and out-going
  ///        bits are set to true.
  ///
  /// The gadget is constructed such, that the edge-index is big-endian.
  template <typename Adapter>
  typename Adapter::dd_t
  one_hot_edges(Adapter& adapter, const encoding& opt)
  {
    // TODO: Support for variable orderings that do not have in/out bits
    //       interleaved and each cell being independent.

    int x = MAX_CELL_VAR(opt);

    // Test for each cell
    auto root = adapter.build_node(true);

    while (MIN_CELL_VAR(opt) < x) {
      const cell c_x = cell_of_var(x, opt);

      // Varname choice is whether an In bit and an Out bit already has been set.
      auto __ = adapter.build_node(false);
      auto io = root;
      auto i_ = adapter.build_node(false);
      auto _o = adapter.build_node(false);

      const int max_i = edge_out_var(c_x, 0, opt);
      const int max_o = edge_in_var(c_x, 0, opt);

      for (; 0 <= x && cell_of_var(x, opt) == c_x; --x) {
        const var_t t_x = type_of_var(x, opt);
        assert(t_x != var_t::gadget_bit);

        __ = adapter.build_node(x, __, t_x == var_t::out_bit ? _o : i_);

        if (max_i < x) {
          const auto child = t_x == var_t::in_bit ? io : adapter.build_node(false);
          _o               = adapter.build_node(x, _o, child);
        }
        if (max_o < x) {
          const auto child = t_x == var_t::out_bit ? io : adapter.build_node(false);
          i_               = adapter.build_node(x, i_, child);
        }
        if (max_i < x && max_o < x) { io = adapter.build_node(x, io, adapter.build_node(false)); }
      }

      root = __;
    }

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Constraint excluding picking same in-going as out-going edge.
  ///
  /// This is essentially a long chain that merely checks for every cell `c`
  /// whether at least one bit mismatches between the two. Since the variable
  /// ordering on the cell's indices make them independent, then we can repeat
  /// this test on-top of each other in one long chain.
  ///
  /// The gadget is constructed such, that the edge-index is big-endian.
  template <typename Adapter>
  typename Adapter::dd_t
  unmatch_in_out(Adapter& adapter, const encoding& opt)
  {
    // TODO: Support for variable orderings that do not have in/out bits
    //       interleaved and each cell being independent.

    int x = MAX_CELL_VAR(opt);

    // Test for each cell
    auto root = adapter.build_node(true);

    while (MIN_CELL_VAR(opt) < x) {
      const cell c_x = cell_of_var(x, opt);

      auto success = root;
      auto test    = adapter.build_node(false);
      auto test0   = adapter.build_node(false);
      auto test1   = adapter.build_node(false);

      for (; MIN_CELL_VAR(opt) <= x && cell_of_var(x, opt) == c_x; --x) {
        const var_t t_x = type_of_var(x, opt);

        // Update tests
        assert(t_x != var_t::gadget_bit);
        if (t_x == var_t::out_bit) {
          test0 = adapter.build_node(x, test, success);
          test1 = adapter.build_node(x, success, test);
        } else { // var_t::in_bit
          test = adapter.build_node(x, test0, test1);
        }

        // Update success chain, if there still are possibly succeeding tests for
        // the current cell above this level.
        if (edge_out_var(c_x, 0, opt) < x) { success = adapter.build_node(x, success, success); }
      }

      root = test;
    }

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Constraint excluding non-existent edge of `edge_idx` for cell `c`.
  ///
  /// This essentially is two tests run in parallel: figure out whether there is
  /// at least one mismatching index for both the in-going and the out-going
  /// bits. This is two bits of information, which results in 4 chains in the
  /// diagram.
  ///
  /// The gadget is constructed such, that the edge-index is big-endian.
  template <typename Adapter>
  typename Adapter::dd_t
  remove_illegal(Adapter& adapter, const int edge_idx, const encoding& opt)
  {
    // TODO: Support for variable orderings that do not have in/out bits
    //       interleaved and each cell being independent.

    int x = MAX_CELL_VAR(opt);

    // Test for each cell
    auto root = adapter.build_node(true);

    while (MIN_CELL_VAR(opt) < x) {
      const cell c_x = cell_of_var(x, opt);

      if (edge::has_idx(c_x, edge_idx)) {
        for (; 0 <= x && cell_of_var(x, opt) == c_x; --x) {
          root = adapter.build_node(x, root, root);
        }
      } else { // !edge::has_idx(c_x, edge_idx)
        int c_val_i = edge_idx;
        int c_val_o = edge_idx;

        auto success = root;
        auto test_io = adapter.build_node(false);

        const int max_i = edge_out_var(c_x, 0, opt);
        auto test_i     = adapter.build_node(false);

        const int max_o = edge_in_var(c_x, 0, opt);
        auto test_o     = adapter.build_node(false);

        for (; 0 <= x && cell_of_var(x, opt) == c_x; --x) {
          const var_t t_x = type_of_var(x, opt);
          assert(t_x != var_t::gadget_bit);

          if (t_x == var_t::out_bit) {
            const bool bit_val = next_fixed_bit(c_val_o, opt);

            test_io = adapter.build_node(x, bit_val ? test_i : test_io, bit_val ? test_io : test_i);

            if (max_o < x) {
              test_o =
                adapter.build_node(x, bit_val ? success : test_o, bit_val ? test_o : success);
            }
            if (max_i < x) { test_i = adapter.build_node(x, test_i, test_i); }
          } else { // t_x == var_t::in_bit
            const bool bit_val = next_fixed_bit(c_val_i, opt);

            test_io = adapter.build_node(x, bit_val ? test_o : test_io, bit_val ? test_io : test_o);

            if (max_o < x) { test_o = adapter.build_node(x, test_o, test_o); }
            if (max_i < x) {
              test_i =
                adapter.build_node(x, bit_val ? success : test_i, bit_val ? test_i : success);
            }
          }

          if (max_i < x && max_o < x) { success = adapter.build_node(x, success, success); }
        }

        root = test_io;
      }
    }

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Constraint insisting choice of edge matches at source and target.
  ///
  /// This essentially is an encoding of `u[out].idx() == e.idx() iff
  /// v[in].idx() == e.idx()`. As such a simple *if-then* rather than an *iff*
  /// would suffice. Yet, due to the variable ordering, we may not have `u < v`
  /// and so the don't care else branch is harder to construct. Hence, the *iff*
  /// is in fact easier to construct (and requires at most juts as many diagram
  /// nodes).
  ///
  /// The gadget is constructed such, that the edge-index is big-endian.
  template <typename Adapter>
  typename Adapter::dd_t
  match_u_v(Adapter& adapter, const edge& e, const encoding& opt)
  {
    const int max_bit = bits_per_edge(opt) - 1;
    assert(0 <= max_bit);

    // Essentially, we need to compare two entire bit-vectors: one for `e.u()`
    // and one for `e.v()`. Given the variable ordering, these bit-vectors are
    // completely separate. Hence, we cannot just compare them bit-for-bit like
    // in the constraints above.
    //
    // Yet, we can compensate for the variable ordering by abstracting them into
    // two cells `x < y` with associated index type.
    assert(e.u() != e.v());

    const cell x_c = std::min(e.u(), e.v());

    const int x_min_var = edge_in_var(x_c, 0, opt);
    const int x_max_var = edge_out_var(x_c, max_bit, opt);

    const cell y_c = std::max(e.u(), e.v());

    const int y_min_var = edge_in_var(y_c, 0, opt);
    const int y_max_var = edge_out_var(y_c, max_bit, opt);

    assert(x_min_var < x_max_var);
    assert(x_max_var < y_min_var);
    assert(y_min_var < y_max_var);

    // Variable for chain(s) construction
    int z = MAX_CELL_VAR(opt);

    auto root = adapter.build_node(true);

    // Don't care for everything beyond cell `y`'s edge bits.
    for (; y_max_var < z; --z) { root = adapter.build_node(z, root, root); }

    // Test chain for cell `y` that fails if not `e.idx()`.
    const var_t y_t = y_c == e.u() ? var_t::out_bit : var_t::in_bit;

    int y_val = (y_t == var_t::out_bit ? e : e.reversed()).idx();

    // chain to check `y != y_val`
    auto y_neq = adapter.build_node(false);

    // chain to check `y == y_val`
    auto y_eq = root;

    assert(z == y_max_var);
    for (; y_min_var <= z; --z) {
      if (type_of_var(z, opt) == y_t) {
        const bool bit_val = next_fixed_bit(y_val, opt);

        y_neq = adapter.build_node(z, bit_val ? root : y_neq, bit_val ? y_neq : root);

        y_eq = adapter.build_node(z,
                                  bit_val ? adapter.build_node(false) : y_eq,
                                  bit_val ? y_eq : adapter.build_node(false));
      } else {
        y_neq = adapter.build_node(z, y_neq, y_neq);
        y_eq  = adapter.build_node(z, y_eq, y_eq);
      }

      // Only extend `root` if we still are going to add a `y_t` test above it.
      if (edge_var(y_c, 0, y_t == var_t::out_bit, opt) < z) {
        root = adapter.build_node(z, root, root);
      }
    }

    // Don't care for everything upto the cell `x`'s edge bits.
    for (; x_max_var < z; --z) {
      y_neq = adapter.build_node(z, y_neq, y_neq);
      y_eq  = adapter.build_node(z, y_eq, y_eq);
    }

    // Chain to determine whether `x == e.idx()`. If so, then go to `y_test`,
    // otherwise just go-to `root`.
    const var_t x_t = x_c == e.u() ? var_t::out_bit : var_t::in_bit;
    assert(x_t != y_t);

    int x_val = (x_t == var_t::out_bit ? e : e.reversed()).idx();

    // chain to for `x == x_val ? y_eq : y_neq` decision.
    auto x_chain = y_eq;

    assert(z == x_max_var);
    for (; x_min_var <= z; --z) {
      if (type_of_var(z, opt) == x_t) {
        const bool bit_val = next_fixed_bit(x_val, opt);

        x_chain = adapter.build_node(z, bit_val ? y_neq : x_chain, bit_val ? x_chain : y_neq);
      } else {
        x_chain = adapter.build_node(z, x_chain, x_chain);
      }

      // Only extend `y_neq` if we still are going to add an `x_t` test above it.
      if (edge_var(x_c, 0, x_t == var_t::out_bit, opt) < z) {
        y_neq = adapter.build_node(z, y_neq, y_neq);
      }
    }

    root = x_chain;

    // Don't care for remaining variables
    for (; MIN_CELL_VAR(opt) <= z; --z) { root = adapter.build_node(z, root, root); }

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Helper function for the binary adder where `p` is a power of two.
  template <typename Adapter>
  std::pair<typename Adapter::build_node_t, typename Adapter::build_node_t>
  binary_gadget_levels(Adapter& adapter,
                       const edge& e,
                       [[maybe_unused]] const int p,
                       const encoding& opt)
  {
    assert(opt == encoding::BINARY);
    assert(is_power_of_two(p));

    // Variable for the current level.
    int x = MAX_GADGET_VAR(opt);
    assert(x == MAX_VAR(opt));

    // False terminal for use later
    const auto bot = adapter.build_node(false);

    // Else case is just a long don't care chain.
    auto root_else = adapter.build_node(true);

    // Since our approach is big-endian, we will (when seen top-down) for each
    // `(u,v)` pair check whether they match. If they do not, then the carry
    // must have been propagated up to this point; from here-on forward, the
    // smaller bits of `u` must be 1 and the bits of `v` must be 0.
    const bool u_top = e.u() < e.v();
    const bool v_top = !u_top;

    const cell c_fst = u_top ? e.u() : e.v();
    const cell c_snd = u_top ? e.v() : e.u();

    const int min_uv_var = gadget_var(c_fst, 0, opt);

    const int max_bit    = bits_per_gadget(p, opt) - 1;
    const int max_uv_var = gadget_var(c_snd, max_bit, opt);

    // Don't care nodes below the bottom-most bit
    for (; max_uv_var < x; --x) { root_else = adapter.build_node(x, root_else, root_else); }
    assert(x == max_uv_var);
    assert(cell_of_var(x, opt) == c_snd);

    // Seen from the bottom, we need two "chains":
    //
    // - One for the bit carrying. Yet, this one only exists up to the second
    //   top-most bit.
    const int top_snd_var = gadget_var(c_snd, 0, opt);

    auto carry = cell_of_var(x, opt) == e.u() ? adapter.build_node(x, bot, root_else)
                                              : adapter.build_node(x, root_else, bot);

    // - One for the bits matching up to this point. This chain splits in two to
    //   check whether `c_snd` matches or not. If they match, it merges back
    //   into one node. If they do not match, then this bit must be where the
    //   carry has flipped it; the remaining bits (already processed) must hence
    //   follow the `carry` pattern.
    //
    //   If they match all the way to the very last bit, we force the last bit
    //   of `u` to be 0 and the last bit of `v` to be 1.
    auto match  = bot;
    auto match0 = cell_of_var(x, opt) == e.v() ? adapter.build_node(x, bot, root_else) : bot;
    auto match1 = cell_of_var(x, opt) == e.u() ? adapter.build_node(x, root_else, bot) : bot;

    // Keep track which of the two parts of `match` needs to be extended.
    bool match_latest = false;

    root_else = adapter.build_node(x, root_else, root_else);

    x -= 1;
    assert(x < max_uv_var);

    for (; min_uv_var <= x; --x) {
      const cell c = cell_of_var(x, opt);

      // Further maintain `root_else` don't care nodes for the later edge-bits.
      root_else = adapter.build_node(x, root_else, root_else);

      // Add don't care nodes for other gadgets.
      if (c != c_fst && c != c_snd) {
        if (match_latest) {
          match = adapter.build_node(x, match, match);
        } else {
          match0 = adapter.build_node(x, match0, match0);
          match1 = adapter.build_node(x, match1, match1);
        }
        carry = top_snd_var < x ? adapter.build_node(x, carry, carry) : carry;
        continue;
      }

      // Test for matching values / carry.
      assert(c == c_fst || c == c_snd);

      match_latest = c == c_fst;
      if (match_latest /*, i.e. c_fst*/) {
        match = adapter.build_node(x, match0, match1);
      } else /*!match_latest, i.e. c_snd*/ {
        // Reject u[i] = 1 but v[i] = 0. Since everything above has matched,
        // then doing so would decrease the number. The only exception to this
        // is the top-most bit; this one may decrease into an overflow.

        match0 = v_top && top_snd_var < x ? adapter.build_node(x, match, bot)
                                          : adapter.build_node(x, match, carry);

        match1 = u_top && top_snd_var < x ? adapter.build_node(x, bot, match)
                                          : adapter.build_node(x, carry, match);
      }

      if (top_snd_var < x) {
        const bool bit_val = c == e.u();
        carry              = adapter.build_node(x, bit_val ? bot : carry, bit_val ? carry : bot);
      }
    }
    assert(x < min_uv_var);

    auto root_then = match;

    // Add remaining gadget variables
    for (; MIN_GADGET_VAR(opt) <= x; --x) {
      root_else = adapter.build_node(x, root_else, root_else);
      root_then = adapter.build_node(x, root_then, root_then);
    }
    assert(x == MAX_CELL_VAR(opt));

    return { root_else, root_then };
  }

  /// \brief One-hot encoding with a linear number of variables.
  ///
  /// While we use a linear number of bits, it is technically incorrect to call
  /// this a *unary* encoding; a better word for it might be *one-hot*.
  ///
  /// \remark This is expected to primarily work well with ZDDs.
  template <typename Adapter>
  std::pair<typename Adapter::build_node_t, typename Adapter::build_node_t>
  unary_gadget_levels(Adapter& adapter, const edge& e, const int p, const encoding& opt)
  {
    throw std::invalid_argument("Unary Encoding not yet supported.");

    assert(opt == encoding::UNARY || opt == encoding::CRT__UNARY);
    assert(e.u() != e.v());

    // Variable for the current level.
    int x = gadget_var(cell::last(), p - 1, opt);

    assert(x <= MAX_VAR(opt));
    assert(MIN_GADGET_VAR(opt) < x && x <= MAX_GADGET_VAR(opt));
    assert(MAX_CELL_VAR(opt) < x);

    // False terminal for use later
    const auto bot = adapter.build_node(false);
    const auto top = adapter.build_node(true);

    // -------------------------------------------------------------------------
    // Since the gadget is big-endian and we want to ensure `u = v+1`, then we
    // should always see the true bit of `v` before the one of `u`. Hence, we
    // can build up a chain on `v` that checks with the value of `u`.
    //
    // Let us do so for all but the top-most bit.

    // Chain when the correct values of `u` and `v` are confirmed; from here,
    // both have to be false.
    auto uv_false = top;

    // Chain figuring out which bit of `v` is set. Note, on this chain all `u`
    // must be false (since each failing `v` check must be copied by a failing
    // `u` check).
    auto v_decision = bot;

    // Chain of checking the value of `u` matches `v-1` (obligation from
    // `v_decision`). This either goes to `uv_false` if succesful or fails.
    //
    // To handle the case where `e.v() < e.u()` in the variable ordering, then
    // we need to have two short chains that can run concurrently. The primary
    // chain of interest is `u_obl_next` that includes the obligation for the
    // next bit. Yet, if `e.v() < e.u()` then we need to start creating the
    // chain for testing `u = bit` before we get to check `v = bit` (which in
    // turn needs the `u = bit-1` obligation).
    //
    // The `u_obl_next` chain is `top` for this case, since then `v = 1` will
    // result in checking `u = 0`. Otherwise, `u_obl_curr` will be spawned
    // before the `v = 1` check and is used.
    auto u_obl_curr = bot;
    auto u_obl_next = e.v() < e.u() ? top : bot;

    // Don't Care branch, should the edge not be taken.
    auto root_else = top;

    // For all but the very last bit, update all three chains
    for (int bit = 1; bit < bits_per_gadget(p, opt); ++bit) {
      // For all cells of this bit, i.e. where the gadgets check a certain value.
      assert(p - bit > 0);
      const int min_x = gadget_var(cell::first(), p - bit, opt);
      for (; min_x <= x; --x) {
        const cell c = cell_of_var(x, opt);

        root_else = adapter.build_node(x, root_else, root_else);

        if (c != e.u() && c != e.v()) {
          uv_false   = adapter.build_node(x, uv_false, uv_false);
          v_decision = adapter.build_node(x, v_decision, v_decision);
          u_obl_curr = adapter.build_node(x, u_obl_curr, u_obl_curr);
          u_obl_next = adapter.build_node(x, u_obl_next, u_obl_next);

          continue;
        }

        if (c == e.u()) {
          uv_false   = adapter.build_node(x, uv_false, bot);
          v_decision = adapter.build_node(x, v_decision, bot);

          // Spawn a new obligation for that checks `u = bit`.
          u_obl_curr = adapter.build_node(x, bot, uv_false);

          // Proceed on prior obligation (if any) that checks `u = bit-1`.
          u_obl_next = adapter.build_node(x, u_obl_next, bot);

          continue;
        }

        if (c == e.v()) {
          // If `e.u() < e.v()`, then the `u_obl_curr` chain contains the check
          // for `u = bit-1`; move it into `u_obl_next` to use it with `v = bit`.
          if (e.u() < e.v()) { u_obl_next = u_obl_curr; }

          uv_false   = adapter.build_node(x, uv_false, bot);
          v_decision = adapter.build_node(x, v_decision, u_obl_next);

          // If `e.v() < e.u()`, then `u_obl_curr` contains test for `u = bit`
          // and is going to be reset by `u = bit-1` before we see `v = bit-1`.
          // Hence, we should move `u_obl_curr` into `u_obl_next` to preserve
          // it.
          //
          // Otherwise, set it to `bot` such that no spurious nodes are created
          u_obl_next = e.v() < e.u() ? u_obl_curr : bot;

          // Set `u_obl_curr` to `bot` such that no spurious nodes are created
          u_obl_curr = bot;

          continue;
        }
      }
    }

    // -------------------------------------------------------------------------
    // For the last bit, handle the overflow edge-case of `v = 0` iff `u = p-1`.
    auto root_then = v_decision;

    // The `u = p-2` obligation might still be in `u_obl_curr`.
    if (e.u() < e.v()) { u_obl_next = u_obl_curr; }

    for (; MAX_CELL_VAR(opt) < x; --x) {
      const cell c = cell_of_var(x, opt);

      root_else = adapter.build_node(x, root_else, root_else);

      if (c != e.u() && c != e.v()) {
        root_then = adapter.build_node(x, root_then, root_then);

        // Update `uv_false` until `c == e.u()`
        if (e.u() < c) { uv_false = adapter.build_node(x, uv_false, uv_false); }
        // Update `u_obl_next` until `c == e.v()`
        if (e.v() < c) { u_obl_next = adapter.build_node(x, u_obl_next, u_obl_next); }
        continue;
      }

      if (c == e.u()) {
        // If `u = p-1` then go-to `uv_false` chain where all other bits of `p`
        // and `u` are 0.
        root_then = adapter.build_node(x, root_then, uv_false);

        // Update `u_obl_next` until `c == e.v()`; this includes the check
        // whether `u = p-2`, and so the `u = p-1` bit should be 0.
        if (e.v() < c) { u_obl_next = adapter.build_node(x, u_obl_next, bot); }
        continue;
      }

      if (c == e.v()) {
        // If `v = p-1` then go-to check of `u = p-2` obligation.
        root_then = adapter.build_node(x, root_then, u_obl_next);

        continue;
      }
    }

    // -------------------------------------------------------------------------
    assert(x == MAX_CELL_VAR(opt));

    return { root_else, root_then };
  }

  /// \brief Gadget for increment relation.
  ///
  /// The gadget is constructed such, that the counter is big-endian.
  template <typename Adapter>
  typename Adapter::dd_t
  gadget(Adapter& adapter, const edge& e, const int p, const encoding& opt)
  {
    assert(opt != encoding::TIME);
    assert(e.u() != e.v());

    // -------------------------------------------------------------------------
    // Gadget bits: defer to helper functions for each encoding
    auto [root_else, root_then] = opt == encoding::BINARY ? binary_gadget_levels(adapter, e, p, opt)
                                                          : unary_gadget_levels(adapter, e, p, opt);

    // -------------------------------------------------------------------------
    // Edge bits: check out-bits for `e.u()` has the index.
    int x = MAX_CELL_VAR(opt);

    const int u_max_var = edge_out_var(e.u(), bits_per_edge(opt) - 1, opt);
    const int u_min_var = edge_out_var(e.u(), 0, opt);

    for (; u_max_var < x; --x) {
      root_then = adapter.build_node(x, root_then, root_then);
      root_else = adapter.build_node(x, root_else, root_else);
    }

    auto root = root_then;

    int e_idx = e.idx();
    for (; u_min_var <= x; --x) {
      // Skip in-bits, they should be quantified away at this point.
      if (type_of_var(x, opt) == var_t::in_bit) { continue; }

      const bool bit_val = next_fixed_bit(e_idx, opt);

      root = adapter.build_node(x, bit_val ? root_else : root, bit_val ? root : root_else);
      if (u_min_var < x) { root_else = adapter.build_node(x, root_else, root_else); }
    }

    for (; MIN_CELL_VAR(opt) <= x; --x) { root = adapter.build_node(x, root, root); }
    assert(x == MIN_VAR(opt));

    // -------------------------------------------------------------------------

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    // std::stringstream ss;
    // ss << "gadget_" << e.u().to_string() << "_" << e.v().to_string() << "_" << p << ".dot";

    // adapter.print_dot(out, ss.str());

    return out;
  }

  /// \brief Gadget for a fixed value.
  ///
  /// \details Since we only have to check for a fixed value, then we do not
  ///          need to differentiate between the three types of gadgets; we only
  ///          need to check it has the expected bit-value. Yet, for the LFSR
  ///          gadget this does mean we have to convert `v` into the `v`'th
  ///          iteration of the LFSR before creating the circuit.
  template <typename Adapter>
  typename Adapter::dd_t
  gadget(Adapter& adapter, const cell& c, const int p, int v, const encoding& opt)
  {
    assert(!c.out_of_range());
    assert(p < 1 << bits_per_gadget(p, opt));

    // For all gadgets the value `v` is modulo `p`.
    v = v % p;

    const auto bot = adapter.build_node(false);
    auto root      = adapter.build_node(true);

    // For the `CRT__X` encodings, we use a different number of variables per
    // prime `p`. Hence, for the given prime `p`, we should not create
    // "undefined" bits for the gadget (or prepend with extra bits).
    const int max_bit = log2(p) - 1;
    for (int x = gadget_var(cell::last(), max_bit, opt); MIN_VAR(opt) <= x; --x) {
      // Don't care for anything but gadget bits of `c`
      if (type_of_var(x, opt) != var_t::gadget_bit || cell_of_var(x, opt) != c) {
        root = adapter.build_node(x, root, root);
        continue;
      }

      // Test that bit matches the expected value.
      const bool bit_val = next_fixed_bit(v, opt);
      root               = adapter.build_node(x, bit_val ? bot : root, bit_val ? root : bot);
    }

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Predicate that is true for a given type of bits.
  auto
  bit_pred(const var_t& t, const encoding& opt)
  {
    return [=](const int x) { return type_of_var(x, opt) == t; };
  }

  /// \brief Predicate that is true for a given type of bits for cells on a
  /// specific row.
  auto
  bit_pred(const int row, const var_t& t, const encoding& opt)
  {
    return
      [=](const int x) { return cell_of_var(x, opt).row() == row && type_of_var(x, opt) == t; };
  }

  /// \brief Predicate that is true for a cell's specific given type of bits.
  auto
  bit_pred(const cell& c, const var_t& t, const encoding& opt)
  {
    return [=](const int x) { return cell_of_var(x, opt) == c && type_of_var(x, opt) == t; };
  }

  /// \brief Encoding of the Hamiltonian Cycle problem given a non-zero number
  ///        of modulo values.
  ///
  /// For each modulo value `p`, we enforce that each cycle must have length 0
  /// modulo `p`. The only exception is the cycle that includes the special
  /// top-left corner; this one has to be of length `cells() % p`.
  ///
  /// Notice, if we do this in a row-major order, then we can quantify the
  /// gadgets from row `i-2` away after having finished adding the constraints
  /// of row `i`. Doing so decreases the number of concurrent variables in the
  /// decision diagram.
  ///
  /// If a value of `p > cells()` is used, we are guaranteed the result of the
  /// counting problem is going to be exact. Yet, we can in fact do it with two
  /// much smaller prime numbers. For example, if N=8x8 we can pick prime
  /// factors `p=5` and `p=7`. This eliminates any degenerate case, since a
  /// cycle must have even length but at the same time (due to the Chinese
  /// Remainder Theorem) have a length that is a multiple of 35.
  ///
  /// If a Binary encoding is chosen and `p` is a Mersenne prime, then an LFSR is
  /// used rather than the Binary Adder.
  template <typename Adapter>
  typename Adapter::dd_t
  create(Adapter& adapter, const encoding& opt)
  {
    // -------------------------------------------------------------------------
    // Trivial cases
    if (cells() == 1) { return adapter.ithvar(cell(0, 0).dd_var()); }

    for (int row = 0; row < rows(); ++row) {
      for (int col = 0; col < cols(); ++col) {
        const cell c_from(row, col);

        if (!c_from.has_neighbour()) { return adapter.bot(); }
      }
    }

    assert(3 <= rows() && 3 <= cols());
    assert(3 < rows() || 3 < cols());

    // -------------------------------------------------------------------------
    // Start with all edges (even illegal ones), but '1A -> 2C', '3B -> 1A'
    // already fixed.
    typename Adapter::dd_t paths = init_special(adapter, opt);

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("fix corner") << json::value(adapter.nodecount(paths)) << json::comma
              << json::endl;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Make one-hot for unary
    if (opt == encoding::UNARY || opt == encoding::CRT__UNARY) {
      paths &= one_hot_edges(adapter, opt);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd            = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << json::field("force one-hot") << json::value(nodecount) << json::comma
                << json::endl;
#endif // BDD_BENCHMARK_STATS
    }

    // -------------------------------------------------------------------------
    // Force different choice for in-going and out-going edge
    // Apply constraint
    paths &= unmatch_in_out(adapter, opt);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(paths);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << json::field("in != out") << json::value(nodecount) << json::comma << json::endl;
    std::cout << json::endl;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Remove illegal edges
#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("remove illegal edges") << json::brace_open << json::endl;
#endif // BDD_BENCHMARK_STATS
    for (int edge_idx = cell::max_moves - 1; 0 <= edge_idx; --edge_idx) {
      paths &= remove_illegal(adapter, edge_idx, opt);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd            = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << json::field(std::to_string(edge_idx)) << json::value(nodecount) << json::comma
                << json::endl;
#endif // BDD_BENCHMARK_STATS
    }
#ifdef BDD_BENCHMARK_STATS
    std::cout << json::brace_close << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Force matching choice in in-going and out-going edge
#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("match edge-indices") << json::brace_open << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
    for (int row = MAX_ROW(); 0 <= row; --row) {
      for (int col = MAX_COL(); 0 <= col; --col) {
        const cell u(row, col);

        // Skip (0,0) since both its ingoing and outgoing edges are fixed
        if (u != cell::special_0()) {
          for (const cell v : u.neighbours()) {
            const edge e(u, v);

            // Skip (0,0) since both its ingoing and outgoing edges are fixed
            if (v == cell::special_0()) { continue; }

            paths &= match_u_v(adapter, e, opt);

#ifdef BDD_BENCHMARK_STATS
            const size_t nodecount = adapter.nodecount(paths);
            largest_bdd            = std::max(largest_bdd, nodecount);
            total_nodes += nodecount;

            std::cout << json::field("apply(" + e.to_string() + ")") << json::value(nodecount)
                      << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
          }
        }

        // Quantify the cell cell that is 'active_rows' below and one to the
        // right of the current; this one will never be relevant for later
        // cells.
        const cell q_cell(row + cell::active_rows, col + 1);
        if (!q_cell.out_of_range()) {
          paths = adapter.exists(paths, bit_pred(q_cell, var_t::in_bit, opt));

#ifdef BDD_BENCHMARK_STATS
          const size_t nodecount = adapter.nodecount(paths);
          largest_bdd            = std::max(largest_bdd, nodecount);
          total_nodes += nodecount;

          std::cout << json::field("exists(" + q_cell.to_string() + ")") << json::value(nodecount)
                    << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
        }
      }

      // Quantify the last cell on row+cell::active_rows, since it will not be relevant beyond
      // this point.
      const cell q_cell(row + cell::active_rows, 0);
      if (!q_cell.out_of_range()) {
        paths = adapter.exists(paths, bit_pred(q_cell, var_t::in_bit, opt));

#ifdef BDD_BENCHMARK_STATS
        const size_t nodecount = adapter.nodecount(paths);
        largest_bdd            = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << json::field("exists(" + q_cell.to_string() + ")") << json::value(nodecount)
                  << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
      }
    }

    { // Quantify remaining two rows
      paths = adapter.exists(paths, bit_pred(var_t::in_bit, opt));

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd            = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << json::field("exists(1x)") << json::value(nodecount) << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
    }

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::brace_close << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Add cycle length constraint(s) per modulo value
    const std::vector<int> ps = gadget_moduli(opt);
    for (const int p : ps) {
#ifdef BDD_BENCHMARK_STATS
      std::cout << json::field("path length") << json::brace_open << json::endl;
      std::cout << json::field("modulo") << json::value(p) << json::comma << json::endl;
      std::cout << json::endl;
#endif // BDD_BENCHMARK_STATS

      if constexpr (Adapter::needs_extend) {
        // Establish invariant by extending domain with don't care gadget
        // variables for cells (0,0), (0,1), ... that are active.
        std::vector<int> gadget_vars;
        for (int row = MIN_ROW(); row < MIN_ROW() + cell::active_rows; ++row) {
          for (int col = MIN_COL(); col < cols(); ++col) {
            cell c(row, col);

            for (int bit = 0; bit < bits_per_gadget(opt); ++bit) {
              gadget_vars.push_back(gadget_var(c, bit, opt));
            }
          }
        }

        // Ensure `gadget_vars` actually follows the variable ordering.
        std::sort(gadget_vars.begin(), gadget_vars.end());

        // Finally, add the 2N*bits don't care levels
        paths = adapter.extend(paths, gadget_vars.begin(), gadget_vars.end());

#ifdef BDD_BENCHMARK_STATS
        const size_t nodecount = adapter.nodecount(paths);
        largest_bdd            = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << json::field(std::string("extend(1x") + (cell::active_rows > 1 ? ",2x" : "")
                                 + ")")
                  << json::value(nodecount) << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
      }

      for (int row = MIN_ROW(); row < rows(); ++row) {

        if constexpr (Adapter::needs_extend) {
          // Extend variables to include gadget for cell (row+cell::active_rows,0).

          const cell new_cell(row + cell::active_rows, MIN_COL());
          if (!new_cell.out_of_range()) {
            std::vector<int> gadget_vars;
            for (int bit = 0; bit < bits_per_gadget(opt); ++bit) {
              gadget_vars.push_back(gadget_var(new_cell, bit, opt));
            }

            paths = adapter.extend(paths, gadget_vars.begin(), gadget_vars.end());

#ifdef BDD_BENCHMARK_STATS
            const size_t nodecount = adapter.nodecount(paths);
            largest_bdd            = std::max(largest_bdd, nodecount);
            total_nodes += nodecount;

            std::cout << json::field("extend(" + new_cell.to_string() + ")")
                      << json::value(nodecount) << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
          }
        }

        for (int col = MIN_COL(); col < cols(); ++col) {
          const cell u(row, col);

          if constexpr (Adapter::needs_extend) {
            // Extend variables to include gadget for cell (row+cell::active_rows,col+1).

            const cell new_cell(row + cell::active_rows, col + 1);
            if (!new_cell.out_of_range()) {

              std::vector<int> gadget_vars;
              for (int bit = 0; bit < bits_per_gadget(opt); ++bit) {
                gadget_vars.push_back(gadget_var(new_cell, bit, opt));
              }

              paths = adapter.extend(paths, gadget_vars.begin(), gadget_vars.end());

#ifdef BDD_BENCHMARK_STATS
              const size_t nodecount = adapter.nodecount(paths);
              largest_bdd            = std::max(largest_bdd, nodecount);
              total_nodes += nodecount;

              std::cout << json::field("extend(" + new_cell.to_string() + ")")
                        << json::value(nodecount) << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
            }
          }

          // Add gadget constraint
          if (u.is_special()) {
            const int u_val = u == cell::special_0() ? 0
              : u == cell::special_1()               ? 1
                                                     : /*u == cell::special_2()*/ cells() - 1;

            paths &= gadget(adapter, u, p, u_val, opt);

#ifdef BDD_BENCHMARK_STATS
            const size_t nodecount = adapter.nodecount(paths);
            largest_bdd            = std::max(largest_bdd, nodecount);
            total_nodes += nodecount;

            std::cout << json::field("gadget(" + u.to_string() + ")") << json::value(nodecount)
                      << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
          } else {
            for (const cell v : u.neighbours()) {
              const edge e(u, v);

              paths &= gadget(adapter, e, p, opt);

#ifdef BDD_BENCHMARK_STATS
              const size_t nodecount = adapter.nodecount(paths);
              largest_bdd            = std::max(largest_bdd, nodecount);
              total_nodes += nodecount;

              std::cout << json::field("gadget(" + e.to_string() + ")") << json::value(nodecount)
                        << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
            }

            // Quantify a cell two rows above and one to the left of the
            // current; this one will never be relevant for later cells.
            const cell q_cell(row - cell::active_rows, col - 1);
            if (!q_cell.out_of_range()) {
              paths = adapter.exists(paths, bit_pred(q_cell, var_t::gadget_bit, opt));

#ifdef BDD_BENCHMARK_STATS
              const size_t nodecount = adapter.nodecount(paths);
              largest_bdd            = std::max(largest_bdd, nodecount);
              total_nodes += nodecount;

              std::cout << json::field("exists(" + q_cell.to_string() + ")")
                        << json::value(nodecount) << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
            }
          }
        }

        // Quantify the last cell two rows prior, since it will not be relevant
        // beyond this point.
        const cell q_cell(row - cell::active_rows, MAX_COL());
        if (!q_cell.out_of_range()) {
          paths = adapter.exists(paths, bit_pred(q_cell, var_t::gadget_bit, opt));

#ifdef BDD_BENCHMARK_STATS
          const size_t nodecount = adapter.nodecount(paths);
          largest_bdd            = std::max(largest_bdd, nodecount);
          total_nodes += nodecount;

          std::cout << json::field("exists(" + q_cell.to_string() + ")") << json::value(nodecount)
                    << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
        }
      }

      { // Quantify remaining two rows
        paths = adapter.exists(paths, bit_pred(var_t::gadget_bit, opt));

#ifdef BDD_BENCHMARK_STATS
        const size_t nodecount = adapter.nodecount(paths);
        largest_bdd            = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << json::field("exists(" + std::to_string(MAX_ROW() - 1) + "x,"
                                 + std::to_string(MAX_ROW()) + "x)")
                  << json::value(nodecount) << json::endl;
#endif // BDD_BENCHMARK_STATS
      }
    }
#ifdef BDD_BENCHMARK_STATS
    std::cout << json::brace_close << json::endl;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    return paths;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// \brief Algorithms for the `encoding::TIME` encoding
///
/// A drastically different way to search for Hamiltonian Cycles. Here, a quartic
/// (N^4) number of variables rather than a quadratic(ish) number. To this end,
/// we do not encode edges on the board. Instead, each cell of the board `(r,c)`
/// is associated with a time-step `t` which is up to `r*c`. Each of the variable
/// is `true` if one visits `(r,c)` at time `t`.
///
/// Initially, we accumulate all paths of length `t` before adding a hamiltonian
/// constraint on each cell one-by-one.
///
/// Symmetries are broken by encoding the special starting cell separately and
/// forcing it to visit a pre-determined neighbour at time `1` and the other at
/// `t-1`. While we are at it, we may as well also include the hamiltonian
/// constraint instead of adding it later.
///
/// \remark This is expected to only work well with ZDDs.
////////////////////////////////////////////////////////////////////////////////
namespace enc_time
{
  /// \brief Number of different time-steps
  inline int
  times()
  {
    return cells();
  }

  /// \brief Smallest valid time-step
  constexpr int
  MIN_TIME()
  {
    return 0;
  }

  /// \brief Largest valid time-step
  inline int
  MAX_TIME()
  {
    return times() - 1;
  }

  /// \brief The shift needed for the DD variable for a cell at time-step `t`.
  inline int
  time_shift(int t)
  {
    return cells() * t;
  }

  /// \brief Number of variables used in this encoding.
  inline int
  vars()
  {
    const int shift   = time_shift(MAX_TIME());
    const int max_var = cell(MAX_ROW(), MAX_COL()).dd_var(shift);
    return max_var + 1;
  }

  /// \brief Number of variables to use for final model count
  inline int
  satcount_vars()
  {
    return vars();
  }

  /// \brief Helper function to fix one cell to true and all others to false for
  ///        one time step.
  ///
  /// \see rel_init
  template <typename Adapter>
  void
  rel_0__fix(Adapter& adapter,
             const cell& fixed_cell,
             int time,
             typename Adapter::build_node_t& root)
  {
    const int shift = time_shift(time);

    for (const cell& c : cells_descending) {
      const int var = c.dd_var(shift);

      root = c == fixed_cell ? adapter.build_node(var, adapter.build_node(false), root)
                             : adapter.build_node(var, root, adapter.build_node(false));
    }
  }

  /// \brief Constraint to break symmetries and fix it to be a cycle.
  template <typename Adapter>
  typename Adapter::dd_t
  rel_0(Adapter& adapter)
  {
    auto root = adapter.build_node(true);

    // Fix t = MAX_TIME() to be `cell::special_2()`
    rel_0__fix(adapter, cell::special_2(), MAX_TIME(), root);

    // Set t = MAX_TIME()-1, ..., 3, 2 as don't care (but with hamiltonian
    // constraint for the special cells).
    for (int time = MAX_TIME() - 1; 1 < time; --time) {
      const int shift = time_shift(time);

      for (const cell& c : cells_descending) {
        const int var = c.dd_var(shift);

        root = c.is_special() ? adapter.build_node(var, root, adapter.build_node(false))
                              : adapter.build_node(var, root, root);
      }
    }

    // Fix t = 1, 0 to be `cell::special_1()` and `cell::special_0()`
    rel_0__fix(adapter, cell::special_1(), 1, root);
    rel_0__fix(adapter, cell::special_0(), 0, root);

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    // This will already be accounted for in 'create()' below
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Helper function to fix all time steps in an interval to be "don't
  ///        care" nodes (except for the unreachable ones)
  ///
  /// \see rel_t
  template <typename Adapter>
  void
  rel_t__dont_care(Adapter& adapter, int t_begin, int t_end, typename Adapter::build_node_t& out)
  {
    assert(t_end <= t_begin);

    for (int time = t_begin; t_end < time; --time) {
      const int shift = time_shift(time);

      for (const cell& c : cells_descending) {
        const int var = c.dd_var(shift);

        // Fix unreachable cells to unvisitable.
        out = c.has_neighbour() ? adapter.build_node(var, out, out)
                                : adapter.build_node(var, out, adapter.build_node(false));
      }
    }
  }

  /// \brief Diagram for a transitions at time step `t` to `t+1`.
  template <typename Adapter>
  typename Adapter::dd_t
  rel_t(Adapter& adapter, int t)
  {
    // Time steps: t' > t+1
    //   Chain of "don't cares" for whatever happens after t+1.
    typename Adapter::build_node_t post_chain = adapter.build_node(true);
    rel_t__dont_care(adapter, MAX_TIME(), t + 1, post_chain);

    // Time step: t+1
    //   Chain with decision on where to be at time 't+1' given where one was at time 't'.
    std::vector<typename Adapter::build_node_t> to_chains(cells(), adapter.build_node(false));
    {
      const int shift = time_shift(t + 1);

      for (const cell& to : cells_descending) {
        const int to_var = to.dd_var(shift);

        for (const cell& from : cells_descending) {
          // Do not build the chain for unreachable nodes. Notice, we skip this
          // entire possibility when building the nodes for time step t.
          if (!from.has_neighbour()) { continue; }

          const int idx     = from.dd_var();
          to_chains.at(idx) = from.has_move_to(to)
            ? adapter.build_node(to_var, to_chains.at(idx), post_chain)
            : adapter.build_node(to_var, to_chains.at(idx), adapter.build_node(false));
        }

        // Expand the `post_chain` to include that this cell cannot be taken.
        // Notice in the above, we only go-to the `post_chain` if we set one
        // value to 1. Yet, we cannot go to two different places at once.
        //
        // Yet, to not create any unused nodes, we have to only extend the
        // post_chain if we are yet not done processing.
        for (const cell& o : cells_descending) {
          if (to < o || o == to || !o.has_neighbour()) { continue; }

          post_chain = adapter.build_node(to_var, post_chain, adapter.build_node(false));
          break;
        }
      }
    }

    // Time step: t
    //   For each position at time step 't', check whether we are "here" and go to
    //   the `to_chain` checking "where we go to" at 't+1'.
    auto root = adapter.build_node(false);
    {
      const int shift = time_shift(t);

      for (const cell& c : cells_descending) {
        const int var = c.dd_var(shift);

        // Create next node in chain of choices for "we are here".
        {
          const int idx = c.dd_var();

          root = c.has_neighbour() ? adapter.build_node(var, root, to_chains.at(idx))
                                   : adapter.build_node(var, root, adapter.build_node(false));
        }

        // Expand all `to_chains` that still are of interest, i.e. that will be
        // used later. Here, we should write that they cannot pick this variable.
        for (const cell& o : cells_descending) {
          // Skip cells that already have been or never will be processed
          if (c < o || c == o || !o.has_neighbour()) { continue; }

          const int idx     = o.dd_var();
          to_chains.at(idx) = adapter.build_node(var, to_chains.at(idx), adapter.build_node(false));
        }
      }
    }

    // Time steps: t' < t
    //   Chain of "don't cares" for whatever happens before t.
    rel_t__dont_care(adapter, t - 1, -1, root);

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Diagram for the hamiltonian constraint for all time steps.
  ///
  /// \details Essentially, we have two chains, one for "still not visited" (0)
  /// and the other for "has been visited" (1).
  template <typename Adapter>
  typename Adapter::dd_t
  hamiltonian(Adapter& adapter, const cell& ham_c)
  {
    // TODO: include in the encoding, that one cannot be here at time 0, 1 or MAX

    auto out_0 = adapter.build_node(false);
    auto out_1 = adapter.build_node(true);

    for (int time = MAX_TIME(); MIN_TIME() <= time; --time) {
      const int shift = time_shift(time);

      for (const cell& c : cells_descending) {
        const int var = c.dd_var(shift);

        out_0 = c == ham_c ? adapter.build_node(var, out_0, out_1)
                           : adapter.build_node(var, out_0, out_0);

        if (MIN_TIME() < time || ham_c < c) {
          out_1 = c == ham_c ? adapter.build_node(var, out_1, adapter.build_node(false))
                             : adapter.build_node(var, out_1, out_1);
        }
      }
    }

    typename Adapter::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  template <typename Adapter>
  typename Adapter::dd_t
  create(Adapter& adapter)
  {
    // -------------------------------------------------------------------------
    // Trivial cases
    if (cells() == 1) { return adapter.ithvar(cell(0, 0).dd_var()); }

    for (int row = 0; row < rows(); ++row) {
      for (int col = 0; col < cols(); ++col) {
        const cell c_from(row, col);

        if (!c_from.has_neighbour()) { return adapter.bot(); }
      }
    }

    assert(3 <= rows() && 3 <= cols());
    assert(3 < rows() || 3 < cols());

    // -------------------------------------------------------------------------
    // Accumulate cell-relation constraints
    typename Adapter::dd_t paths = rel_0(adapter);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(paths);
    largest_bdd            = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << json::field("transition relation") << json::brace_open << json::endl;
    std::cout << json::field("t = " + std::to_string(MAX_TIME()) + ", 0") << json::value(nodecount)
              << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS

    // Aggregate transitions backwards in time.
    for (int t = MAX_TIME() - 1; MIN_TIME() < t; --t) {
      paths &= rel_t(adapter, t);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd            = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << json::field("t = " + std::to_string(t)) << json::value(nodecount);
      if (t != MIN_TIME() + 1) { std::cout << json::comma; }
      std::cout << json::endl;
#endif // BDD_BENCHMARK_STATS
    }
#ifdef BDD_BENCHMARK_STATS
    std::cout << json::brace_close << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Accumulate hamiltonian constraints
    //
    // TODO: Follow 'cells_descending' ordering (possibly in reverse)?
#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("hamiltonian constraint") << json::brace_open << json::endl;
#endif // BDD_BENCHMARK_STATS

    for (int row = MIN_ROW(); row <= MAX_ROW(); ++row) {
      for (int col = MIN_COL(); col <= MAX_COL(); ++col) {
        const cell c(row, col);

        // If it is one of the special cells, then this has already been
        // constrained as part of 'rel_init'.
        if (c.is_special()) { continue; }

        paths &= hamiltonian(adapter, c);

#ifdef BDD_BENCHMARK_STATS
        const size_t nodecount = adapter.nodecount(paths);
        largest_bdd            = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << json::field(c.to_string()) << json::value(nodecount);
        if (row != MAX_ROW() || col != MAX_COL()) { std::cout << json::comma; }
        std::cout << json::endl;
#endif // BDD_BENCHMARK_STATS
      }
    }

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::brace_close << json::endl;
#endif // BDD_BENCHMARK_STATS
    return paths;
  }
}

constexpr size_t UNKNOWN = static_cast<size_t>(-1);

////////////////////////////////////////////////////////////////////////////////
/// \brief   Expected number of closed Hamiltonian Knight's Tours.
///
/// \details Numbers are taken from https://oeis.org/search?q=knights+tour and
///          https://en.wikipedia.org/wiki/Knight%27s_tour#Number_of_tours . If
///          otherwise not stated, they are from our own previous runs.
////////////////////////////////////////////////////////////////////////////////
const size_t expected_knight[17] = {
  0,
  0,
  1,             //  1x1 [1]
  0,             //  2x1 [_]
  0,             //  2x2 [2]
  0,             //  3x2 [_]
  0,             //  3x3 [1]
  0,             //  4x3 [_]
  0,             //  4x4 [1]
  0,             //  5x4 [_]
  0,             //  5x5 [1]
  8,             //  6x5 [_]
  9862,          //  6x6 [2]
  UNKNOWN,       //  7x6 [_]
  0,             //  7x7 [1]
  UNKNOWN,       //  8x7 [_]
  13267364410532 //  8x8 [1]
};

////////////////////////////////////////////////////////////////////////////////
/// \brief   Expected number of closed Hamiltonian Grid Graph Tours.
///
/// \details Most numbers are taken from https://oeis.org/A003763 . Otherwise,
///          they are from our previous runs
////////////////////////////////////////////////////////////////////////////////
const size_t expected_grid[13] = {
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

////////////////////////////////////////////////////////////////////////////////
/// \brief Hamiltonian Cycle program: pick encoding and time its execution.
////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
int
run_hamiltonian(int argc, char** argv)
{
  const bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  if (N_rows < 0) { N_rows = 4; }
  if (N_cols < 0) { N_cols = N_rows; }

  // ---------------------------------------------------------------------------

  if (rows() == 0 || cols() == 0) {
    std::cerr << "  | The board has no cells. Please provide Ns > 1 (-N)\n";
    return 1;
  }

  // ---------------------------------------------------------------------------
  // Initialise package manager
  int vars = 0;

  switch (enc) {
  case encoding::BINARY:
  case encoding::UNARY:
  case encoding::CRT__UNARY: {
    vars = enc_gadgets::vars(enc);
    break;
  }
  case encoding::TIME: {
    vars = enc_time::vars();
    break;
  }
  default: { /* ? */
  }
  }

  // -----------------------------------------------------------------------------
  // Initialise cells (i.e. variable ordering)
  if (rows() < cols()) {
    std::cerr << "Note:\n"
              << "|   The variable ordering is designed for 'cols <= rows'.\n"
              << "|   Maybe restart with the dimensions flipped?\n"
              << "\n";
  }

  init_cells_descending();

  return run<Adapter>("hamiltonian", vars, [&](Adapter& adapter) {
    std::cout << json::field("encoding") << json::value(to_string(enc)) << json::comma
              << json::endl;
    std::cout << json::field("rows") << json::value(rows()) << json::comma << json::endl;
    std::cout << json::field("cols") << json::value(cols()) << json::comma << json::endl;
    std::cout << json::endl;

    uint64_t solutions;

    // ---------------------------------------------------------------------------
    // Construct paths based on chosen encoding
    std::cout << json::field(enc == encoding::TIME ? "apply" : "apply+exists") << json::brace_open
              << json::endl;

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("intermediate results") << json::brace_open << json::endl;
#endif // BDD_BENCHMARK_STATS

    typename Adapter::dd_t paths;

    const time_point before_paths = now();
    switch (enc) {
    case encoding::BINARY:
    case encoding::UNARY:
    case encoding::CRT__UNARY: {
      paths = enc_gadgets::create(adapter, enc);
      break;
    }
    case encoding::TIME:
    default: {
      paths = enc_time::create(adapter);
      break;
    }
    }
    const time_point after_paths   = now();
    const time_duration paths_time = duration_ms(before_paths, after_paths);

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::brace_close << json::endl;
    std::cout << json::field("total processed (nodes)") << json::value(total_nodes) << json::comma
              << json::endl;
    std::cout << json::field("largest size (nodes)") << json::value(largest_bdd) << json::comma
              << json::endl;
#endif // BDD_BENCHMARK_STATS
    std::cout << json::field("final size (nodes)") << json::value(adapter.nodecount(paths))
              << json::comma << json::endl;
    std::cout << json::field("time (ms)") << json::value(paths_time) << json::endl;
    std::cout << json::brace_close << json::comma << json::endl << json::flush;

    // -------------------------------------------------------------------------
    // Count number of solutions
    std::cout << json::field("satcount") << json::brace_open << json::endl << json::flush;

    const size_t vc =
      enc == encoding::TIME ? enc_time::satcount_vars() : enc_gadgets::satcount_vars(enc);

    const time_point before_satcount = now();
    solutions                        = adapter.satcount(paths, vc);
    const time_point after_satcount  = now();

    const time_duration satcount_time = duration_ms(before_satcount, after_satcount);

    std::cout << json::field("result") << json::value(solutions) << json::comma << json::endl;
    std::cout << json::field("time (ms)") << json::value(satcount_time) << json::endl;
    std::cout << json::brace_close << json::endl << json::flush;

    // -------------------------------------------------------------------------
    // Print out a solution
    /*
    std::cout << "\n"
              << "  Solution Example:\n"
              << "  | ";

    const auto path = adapter.pickcube(paths);
    for (const auto& [x, v] : path) { std::cout << "x" << x << "=" << v << " "; }
    if (path.empty()) { std::cout << "none..."; }

    std::cout << "\n" << std::flush;
    */

    // -------------------------------------------------------------------------
    std::cout << json::field("total time (ms)")
              << json::value(init_time + paths_time + satcount_time) << json::endl
              << json::flush;

    if (rows() == cols() && rows() < size(expected_grid) && expected_grid[rows()] != UNKNOWN
        && solutions != expected_grid[rows()]) {
      return -1;
    }
    return 0;
  });
}
