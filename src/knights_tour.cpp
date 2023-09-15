////////////////////////////////////////////////////////////////////////////////
// TODO: Simpler alternative than the Knight's Graph
//       https://oeis.org/A003763
////////////////////////////////////////////////////////////////////////////////

#include "common.cpp"
#include "expected.h"

#include <unordered_map>
#include <utility>

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

////////////////////////////////////////////////////////////////////////////////
/// \brief Enum for choosing the encoding.
////////////////////////////////////////////////////////////////////////////////
enum enc_opt { BINARY, UNARY, CRT__BINARY, CRT__UNARY, TIME };

////////////////////////////////////////////////////////////////////////////////
template<>
std::string option_help_str<enc_opt>()
{ return "Desired problem encoding"; }

////////////////////////////////////////////////////////////////////////////////
template<>
enc_opt parse_option(const std::string &arg, bool &should_exit)
{
  const std::string lower_arg = ascii_tolower(arg);

  if (lower_arg == "binary")
    { return enc_opt::BINARY; }

  if (lower_arg == "unary" || lower_arg == "one-hot")
    { return enc_opt::UNARY; }

  if (lower_arg == "crt_binary" || lower_arg == "crt")
    { return enc_opt::CRT__BINARY; }

  if (lower_arg == "crt_unary" || lower_arg == "crt_one-hot")
    { return enc_opt::CRT__UNARY; }

  if (lower_arg == "time" || lower_arg == "t")
    { return enc_opt::TIME; }

  std::cerr << "Undefined option: " << arg << "\n";
  should_exit = true;

  return enc_opt::TIME;
}

////////////////////////////////////////////////////////////////////////////////
std::string option_str(const enc_opt& enc)
{
  switch (enc) {
  case enc_opt::BINARY:
    return "Binary (Adder)";
  case enc_opt::UNARY:
    return "Unary (One-hot)";
  case enc_opt::CRT__BINARY:
    return "Chinese Remainder Theorem: Binary (Adder / LFSR)";
  case enc_opt::CRT__UNARY:
    return "Chinese Remainder Theorem: Unary (One-hot)";
  case enc_opt::TIME:
    return "Time-based";
  default:
    return "Unknown";
  }
}

////////////////////////////////////////////////////////////////////////////////
//                           Common board logic                               //
////////////////////////////////////////////////////////////////////////////////

/// \brief Number of rows
inline int rows()
{ return input_sizes.at(0); }

/// \brief Minimum valid row value
constexpr int MIN_ROW()
{ return 0; }

/// \brief Maximum valid row value
inline int MAX_ROW()
{ return rows() - 1; }

/// \brief Number of columns
inline int cols()
{ return input_sizes.at(1); }

/// \brief Minimum valid column value
constexpr int MIN_COL()
{ return 0; }

/// \brief Maximum valid column value
inline int MAX_COL()
{ return cols() - 1; }

/// \brief Number of cells on the chess board
inline int cells()
{ return rows() * cols(); }

/// \brief Class to encapsulate logic related to a cell and the Knight's move.
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
    : _r(-1), _c(-1)
  { }

  /// \brief Construction of cell [r,c].
  ///
  /// \remark This does not check whether the cell actually is legal. To do so,
  ///         please use `out_of_range`.
  cell(int r, int c)
    : _r(r), _c(c)
  { /* TODO: throw std::out_of_range if given bad (r,c)? */ }

  /// \brief Converts back from a diagram variable to the cell.
  ///
  /// \pre The variable `dd_var` must already have been unshifted.
  cell(int dd_var)
    : _r(((dd_var) / cols()) % rows())
    , _c((dd_var) % cols())
  { assert(0 <= dd_var && dd_var < cells()); }

  //////////////////////////////////////////////////////////////////////////////
  // Accessor and DD conversion
public:
  int row() const
  { return _r; }

  int col() const
  { return _c; }

  /// \brief Row-major DD variable name
  ///
  /// \param shift The number of bits to shift.
  int dd_var(const int shift = 0) const
  {
    return shift + (cols() * _r) + _c;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Quality of life
public:
  bool operator==(const cell& o) const
  { return this->_r == o._r && this->_c == o._c; }

  bool operator!=(const cell& o) const
  { return !(*this == o); }

  bool operator<(const cell& o) const
  { return this->dd_var() < o.dd_var(); }

  bool operator>(const cell& o) const
  { return this->dd_var() > o.dd_var(); }

  /// \brief Human-friendly string
  std::string to_string() const
  {
    std::string res;
    res += '1'+_r;
    res += 'A'+_c;
    return res;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Position and Move logic
public:
  /// \brief Number of possible neighbours for a knight
  static constexpr int max_moves = 8;

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

public:
  /// \brief Whether this cell represents an actual valid position on the board.
  bool out_of_range() const
  { return row() < 0 || MAX_ROW() < row() || col() < 0 || MAX_COL() < col(); }

  /// \brief Vertical distance between two cells
  int vertical_dist_to(const cell &o) const
  { return std::abs(this->row() - o.row()); }

  /// \brief Horizontal distance between two cells
  int horizontal_dist_to(const cell &o) const
  { return std::abs(this->col() - o.col()); }

  /// \brief Manhattan distance to cell `o`
  int manhattan_dist_to(const cell& o) const
  { return vertical_dist_to(o) + horizontal_dist_to(o); }

  /// \brief Whether there is a single knight-move from `this` to `o`
  ///
  /// \details One can move from `this` to `o` if one moves at least one in each
  ///          dimension and by exactly three cells.
  bool has_move_to(const cell& o) const
  {
    return (0 < vertical_dist_to(o))
      && (0 < horizontal_dist_to(o))
      && (this->manhattan_dist_to(o) == 3);
  }

  /// \brief All cells on the board that can be reached from this cell
  std::vector<cell> neighbours() const
  {
    std::vector<cell> res;
    for (int i = 0; i < max_moves; ++i) {
      const cell neighbour(this->row() + moves[i][0],
                           this->col() + moves[i][1]);

      if (neighbour.out_of_range()) { continue; }

      res.push_back(neighbour);
    }
    return res;
  }

  /// \brief Whether this cell is reachable from any other cell.
  bool has_neighbour() const
  {
    // For any board larger than 3x3, there is at least one neighbour. For the
    // 3x3 board the center is the only unreachable position.
    return cells() != 9 || (*this != cell(1,1));
  }

public:
  //////////////////////////////////////////////////////////////////////////////
  // Special cell functions

  /// \brief Top-left corner `(0,0)`
  static inline cell special_0()
  { return cell(0,0); }

  /// \brief First cell moved to from `(0,0)` (breaking symmetries)
  static inline cell special_1()
  { return cell(1,2); }

  /// \brief Cell `(2,1)` encountered at the end (closing the cycle)
  static inline cell special_2()
  { return cell(2,1); }

  /// \brief Get the three cells involved in the upper-left corner
  static inline std::array<cell,3> specials()
  { return { special_0(), special_1(), special_2() }; }

  /// \brief Whether this cell is special with regards to the upper left corner.
  bool is_special() const
  {
    const cell c0 = special_0();
    return (*this == c0 || this->has_move_to(c0));
  }
};

/// \brief Hash function for `cell` class
template<>
struct std::hash<cell>
{
  std::size_t operator()(const cell &c) const
  { return std::hash<int>{}(c.dd_var()); }
};

/// \brief Class to encapsulate logic related to a cell and the Knight's move.
class edge
{
private:
  cell _u;
  cell _v;

public:
  /// \brief Default construction
  edge() = default;

  /// \brief Construction of an edge given two cells on the board.
  edge(const cell &u, const cell &v)
    : _u(u), _v(v)
  {
    if (u.out_of_range()) {
      throw std::out_of_range("Cell 'u'="+u.to_string()+" is out of range");
    }
    if (v.out_of_range()) {
      throw std::out_of_range("Cell 'v'="+v.to_string()+" is out of range");
    }
    if (!u.has_move_to(v)) {
      throw std::out_of_range("Edge "+this->to_string()+" is not a knight's move");
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accessor and DD conversion
public:
  /// \brief Source
  const cell& u() const
  { return _u; }

  /// \brief Target
  const cell& v() const
  { return _v; }

  /// \brief The "index" for this edge `u`.
  ///
  /// \remark The index is independent of the edge's direction.
  int idx() const
  {
    assert(u() != v());

    const int r_diff = this->v().row() - this->u().row();
    const int c_diff = this->v().col() - this->u().col();

    // Return index in list of relative moves within `cell` class
    for (int i = 0; i < cell::max_moves; ++i) {
      if (cell::moves[i][0] == r_diff && cell::moves[i][1] == c_diff) {
        return i;
      }
    }
    return -1;
  }

  /// \brief Whether `u` has an edge to a neighbour with edge-index `i`.
  static bool has_idx(const cell &u, const int i)
  {
    for (const cell &v : u.neighbours()) {
      if (edge(u,v).idx() == i) {
        return true;
      }
    }
    return false;
  }

  /// \brief Whether the source or the target are invalid values
  bool out_of_range() const
  { return this->u().out_of_range() || this->v().out_of_range(); }

  //////////////////////////////////////////////////////////////////////////////
  // Quality of life
public:
  bool operator==(const edge& o) const
  { return this->u() == o.u() && this->v() == o.v(); }

  bool operator!=(const edge& o) const
  { return !(*this == o); }

  /// \brief Obtain the reversed directed edge, i.e. from `v` to `u`.
  edge reversed() const
  { return edge{this->v(), this->u()}; }

  /// \brief Human-friendly string
  std::string to_string() const
  { return this->u().to_string()+"->"+this->v().to_string(); }
};

/// \brief Hash for `edge` class
template<> struct std::hash<edge>
{
  std::size_t operator()(const edge &e) const
  { return std::hash<cell>{}(e.u()) ^ std::hash<cell>{}(e.v()); }
};

/// \brief Cells in descending order (relative to variable ordering).
std::vector<cell> cells_descending;

/// \brief Initialise the list of all cells on the board (descendingly)
///        following the variable ordering.
void init_cells_descending()
{
  assert(cell_descending.size() == 0);

  cells_descending.clear();
  cells_descending.reserve(cells());

  for (int row = MAX_ROW(); MIN_ROW() <= row; --row)
    for (int col = MAX_COL(); MIN_COL() <= col; --col)
      cells_descending.push_back(cell(row, col));

  assert(cell_descending.size() == cells());

  // TODO (variable orderings):
  // std::sort<std::greater<cell>>(cells_descending.begin(), cells_descending.end());
}

////////////////////////////////////////////////////////////////////////////////
/// \brief Gadgets for the `enc_opt::(CRT__)BINARY` and `enc_opt::(CRT__)UNARY`
///        encodings.
///
/// Simple(ish) encoding with the goal to minimise the number of variables alive
/// at the same time. To this end, we encode the (roughly) 4N edges of the
/// knight-graph as variables. If an edge `u->v` is set to true, then we encode
/// that `v` must be the successor of `u` via a gadget.
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
  inline int edges_undirected()
  {
    return rows() > 1 && cols() > 1
      ? 4 * cells() - 6 * (rows() + cols()) + 8
      : 0;
  }

  /// \brief Number of (directed) edges
  inline int edges()
  {
    return 2 * edges_undirected();
  }

  /// \brief Obtain the ceiling of log2
  inline int log2(const int x)
  {
    return static_cast<int>(std::ceil(std::log2(x)));
  }

  /// \brief Possible types of bits for this encoding
  enum class var_t {
    in_bit     = 0,
    out_bit    = 1,
    gadget_bit = 2,
  };

  /// \brief Obtain the set of smallest "prime" numbers for gadget
  std::vector<int> gadget_moduli(const enc_opt &opt)
  {
    switch (opt) {
    case enc_opt::BINARY:
    case enc_opt::UNARY: {
      return { cells() };
    }
    case enc_opt::CRT__BINARY:
    case enc_opt::CRT__UNARY: {
      // Find the smallest number of prime numbers whos least common multiple is
      // larger than half the number of cells.
      const std::vector<int> candidates[5] = {
        { 7 },
        { 3, 5 },
        { 3, 7 },
        { 5, 7 },
        { 3, 5, 7 }
      };

      for (const std::vector<int> &candidate : candidates) {
        // Determine the usability and cost of this solution
        int lcm = 1;

        for (const int p : candidate)
          lcm  *= p;

        if (cells() / 2 < lcm)
          return candidate;
      }
      throw std::out_of_range("No primes available for a chess board this big!");
    }
    case enc_opt::TIME:
    default:
      { return {}; }
    }
  }

  /// \brief Number of bits to represent the (directed) in- or out-going edge to
  ///        a single node in the graph.
  inline int bits_per_edge(const enc_opt &opt)
  {
    return opt == enc_opt::BINARY || opt == enc_opt::CRT__BINARY
      ? log2(cell::max_moves) : cell::max_moves;
  }

  /// \brief Number of total bits used to identify the chosen edges.
  inline int edge_vars(const enc_opt &opt)
  {
    // For each cell, we have two set of bit-values: in-going and out-going.
    return cells() * 2 * bits_per_edge(opt);
  }

  /// \brief Obtain the dd variable for an in-going or out-going edge at cell `c`.
  inline int edge_var(const cell &c, int bit, bool out_going,
                      [[maybe_unused]] const enc_opt &opt)
  {
    assert(bit < bits_per_edge(opt));
    return (c.dd_var() * 2 * bits_per_edge(opt)) + (2 * bit + out_going);
  }

  /// \brief Decision diagram variable for a bit of the in-going edge to cell `c`.
  inline int edge_in_var(const cell &c, int bit, const enc_opt &opt)
  { return edge_var(c, bit, false, opt); }

  /// \brief Decision diagram variable for a bit of the out-going edge to cell `c`.
  inline int edge_out_var(const cell &c, int bit, const enc_opt &opt)
  { return edge_var(c, bit, true, opt); }

  /// \brief Obtain the number of bits per gadget given a certain prime.
  inline int bits_per_gadget(const enc_opt &opt, const int p)
  {
    return opt == enc_opt::BINARY || opt == enc_opt::CRT__BINARY ? log2(p) : p;
  }

  /// \brief Obtain the largest number of bits per gadget over all primes.
  inline int bits_per_gadget(const enc_opt &opt)
  {
    return bits_per_gadget(opt, gadget_moduli(opt).back());
  }

  /// \brief Number of total bits used for the gadgets
  inline int gadget_vars(const enc_opt &opt)
  {
    // For each cell, we have a single set of bits for the gadget.
    return cells() * bits_per_gadget(opt);
  }

  /// \brief Obtain the dd variable for a bit in a gadget for cell `c`.
  inline int gadget_var(const cell &c, int bit, [[maybe_unused]] const enc_opt &opt)
  {
    assert(bit < bits_per_gadget(opt));
    return edge_vars(opt) + c.dd_var(cells() * bit);
  }

  inline int MIN_CELL_VAR(const enc_opt &/*opt*/)
  { return 0; }

  inline int MAX_CELL_VAR(const enc_opt &opt)
  { return edge_vars(opt)-1; }

  inline int MIN_GADGET_VAR(const enc_opt &opt)
  { return edge_vars(opt); }

  inline int MAX_GADGET_VAR(const enc_opt &opt)
  { return edge_vars(opt) + gadget_vars(opt)-1; }

  /// \brief Minimum variable
  inline int MIN_VAR(const enc_opt &opt)
  { return MIN_CELL_VAR(opt); }

  /// \brief Maximum variable
  inline int MAX_VAR(const enc_opt &opt)
  { return MAX_GADGET_VAR(opt); }

  /// \brief Number of variables used for the encoding.
  inline int vars(const enc_opt &opt)
  { return MAX_VAR(opt) + 1; }

  /// \brief Number of variables to use for final model count.
  inline int satcount_vars(const enc_opt &opt)
  { return cells() * bits_per_edge(opt); }

  inline cell cell_of_var(const int x, const enc_opt &opt)
  {
    assert(x < vars(opt));

    const int x_unshifted = x < edge_vars(opt)
      ? x / (2 * bits_per_edge(opt))
      : x % cells();

    return cell(x_unshifted);
  }

  /// \brief The bit-index of a variable for some cell c.
  inline int bit_of_var(int x, const enc_opt &opt)
  {
    return x < edge_vars(opt)
      ? x % (2 * bits_per_edge(opt))
      : x / cells();
  }

  /// \brief Obtain the type of a given variable.
  inline var_t type_of_var(int x, const enc_opt &opt)
  {
    return x < edge_vars(opt)
      ? static_cast<var_t>(bit_of_var(x, opt) % 2)
      : var_t::gadget_bit;
  }

  /// \brief Obtain the next bit for a fixed integer `x`, depending on encoding.
  ///
  /// As a side-effect the value of `x` is changed accordingly.
  inline bool next_fixed_bit(int &x, const enc_opt &opt)
  {
    switch (opt) {
    case enc_opt::BINARY:
    case enc_opt::CRT__BINARY: {
      const bool res = x % 2;
      x /= 2;
      return res;
    }
    case enc_opt::UNARY:
    case enc_opt::CRT__UNARY: {
      const bool res = x == 0;
      x -= 1; // <-- this is intended to potentially become negative.
      return res;
    }
    default:
      throw std::out_of_range("Encoding unsupported.");
    }
  }

  /// \brief List of the first few prime numbers.
  constexpr std::array<int, 11> primes =
    { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };

  /// \brief Whether a value (below 32) is a prime
  ///
  /// This algorithm is nothing fancy, but just compares with a pre-compiled
  /// list of numbers.
  ///
  /// \throws std::out_of_range If `i` is larger than the largest "known" prime.
  bool is_prime(int i)
  {
    if (i > 32) {
      throw std::out_of_range("Primes are uncomputed for such large a value");
    }
    for (const int p : primes) {
      if (i == p) { return true; }
    }
    return false;
  }

  /// \brief List of all exponents for Mersenne primes that fit into an `int`.
  constexpr std::array<int, 8> mersenne_exponents =
    { 2, 3, 5, 7, 13, 17, 19, 31 };

  /// \brief Whether a given value is a Mersenne prime
  bool is_mersenne_prime(int i)
  {
    for (const int e : mersenne_exponents) {
      const int p = (1 << e) - 1;
      if (i == p) { return true; }
    }
    return false;
  }

  /// \brief Construct edge-variables with special cells fixed in their choice.
  ///
  /// The gadget is constructed such, that the edge-index is big-endian.
  template<typename adapter_t>
  typename adapter_t::dd_t init_special(adapter_t &adapter, const enc_opt &opt)
  {
    std::array<std::tuple<cell, var_t, int>, 4> fixed_bits {{
        { cell::special_0(), var_t::in_bit,  edge(cell::special_0(), cell::special_2()).idx() },
        { cell::special_0(), var_t::out_bit, edge(cell::special_0(), cell::special_1()).idx() },
        { cell::special_1(), var_t::in_bit,  edge(cell::special_1(), cell::special_0()).idx() },
        { cell::special_2(), var_t::out_bit, edge(cell::special_2(), cell::special_0()).idx() },
      }};

    const auto bot = adapter.build_node(false);
    auto root = adapter.build_node(true);

    for (int x = MAX_CELL_VAR(opt); MIN_CELL_VAR(opt) <= x; --x) {
      const cell c_x = cell_of_var(x, opt);
      const var_t t_x = type_of_var(x, opt);

      for (auto& [c,t, val] : fixed_bits) {
        if (c == c_x && t == t_x) {
          const bool bit_val = next_fixed_bit(val, opt);
          root = adapter.build_node(x, bit_val ? bot : root, bit_val ? root : bot);
          goto init_special__loop_end;
        }
      } // for-else:
      root = adapter.build_node(x, root, root);

    init_special__loop_end:
      continue;
    }

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    // adapter.print_dot(out, "fix_corner.dot");

    return out;
  }

  /// \brief Constraint making sure exactly one of the in-going and out-going
  ///        bits are set to true.
  ///
  /// The gadget is constructed such, that the edge-index is big-endian.
  template<typename adapter_t>
  typename adapter_t::dd_t one_hot_edges(adapter_t &adapter, const enc_opt &opt)
  {
    // TODO: Support for variable orderings that do not have in/out bits
    //       interleaved and each cell being independent.

    int x = MAX_CELL_VAR(opt);

    // Don't care chain below last out-bit
    auto root = adapter.build_node(true);
    for (; type_of_var(x, opt) == var_t::gadget_bit; --x) {
      root = adapter.build_node(x, root, root);
    }

    // Test for each cell
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
          _o = adapter.build_node(x, _o, child);
        }
        if (max_o < x) {
          const auto child = t_x == var_t::out_bit ? io : adapter.build_node(false);
          i_ = adapter.build_node(x, i_, child);
        }
        if (max_i < x && max_o < x) {
          io = adapter.build_node(x, io, adapter.build_node(false));
        }
      }

      root = __;
    }

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    // adapter.print_dot(out, "edges_one-hot.dot");

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
  template<typename adapter_t>
  typename adapter_t::dd_t unmatch_in_out(adapter_t &adapter, const enc_opt &opt)
  {
    // TODO: Support for variable orderings that do not have in/out bits
    //       interleaved and each cell being independent.

    int x = MAX_CELL_VAR(opt);

    // Don't care chain below last out-bit
    auto root = adapter.build_node(true);
    for (; type_of_var(x, opt) == var_t::gadget_bit; --x) {
      root = adapter.build_node(x, root, root);
    }

    // Test for each cell
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
        if (edge_out_var(c_x, 0, opt) < x) {
          success = adapter.build_node(x, success, success);
        }
      }

      root = test;
    }

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    // adapter.print_dot(out, "unmatch_in_out.dot");

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
  template<typename adapter_t>
  typename adapter_t::dd_t remove_illegal(adapter_t &adapter,
                                          const int edge_idx,
                                          const enc_opt &opt)
  {
    // TODO: Support for variable orderings that do not have in/out bits
    //       interleaved and each cell being independent.

    int x = MAX_CELL_VAR(opt);

    // Don't care chain below last out-bit
    auto root = adapter.build_node(true);
    for (; type_of_var(x, opt) == var_t::gadget_bit; --x) {
      root = adapter.build_node(x, root, root);
    }

    // Test for each cell
    while (MIN_CELL_VAR(opt) < x) {
      const cell c_x = cell_of_var(x, opt);

      if (edge::has_idx(c_x, edge_idx)) {
        for (; 0 <= x && cell_of_var(x, opt) == c_x; --x) {
          root = adapter.build_node(x, root, root);
        }
      } else { // !edge::has_idx(c_x, edge_idx)
        int  c_val_i = edge_idx;
        int  c_val_o = edge_idx;

        auto success = root;
        auto test_io = adapter.build_node(false);

        const int max_i = edge_out_var(c_x, 0, opt);
        auto test_i = adapter.build_node(false);

        const int max_o = edge_in_var(c_x, 0, opt);
        auto test_o = adapter.build_node(false);

        for (; 0 <= x && cell_of_var(x, opt) == c_x; --x) {
          const var_t t_x = type_of_var(x, opt);
          assert(t_x != var_t::gadget_bit);

          if (t_x == var_t::out_bit) {
            const bool bit_val = next_fixed_bit(c_val_o, opt);

            test_io  = adapter.build_node(x,
                                          bit_val ? test_i : test_io,
                                          bit_val ? test_io : test_i);

            if (max_o < x) {
              test_o = adapter.build_node(x,
                                          bit_val ? success : test_o,
                                          bit_val ? test_o : success);
            }
            if (max_i < x) {
              test_i = adapter.build_node(x, test_i, test_i);
            }
          } else { // t_x == var_t::in_bit
            const bool bit_val = next_fixed_bit(c_val_i, opt);

            test_io  = adapter.build_node(x,
                                          bit_val ? test_o : test_io,
                                          bit_val ? test_io : test_o);

            if (max_o < x) {
              test_o = adapter.build_node(x, test_o, test_o);
            }
            if (max_i < x) {
              test_i = adapter.build_node(x,
                                          bit_val ? success : test_i,
                                          bit_val ? test_i : success);
            }
          }

          if (max_i < x && max_o < x) {
            success = adapter.build_node(x, success, success);
          }
        }

        root = test_io;
      }
    }

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    /*
      std::string filename = "remove_edge_";
      filename += static_cast<char>('0'+edge_idx);
      filename += ".dot";

      adapter.print_dot(out, filename);
    */

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
  template<typename adapter_t>
  typename adapter_t::dd_t match_u_v(adapter_t &adapter,
                                     const edge &e,
                                     const enc_opt &opt)
  {
    const int max_bit = bits_per_edge(opt)-1;
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
    for (; y_max_var < z; --z) {
      root = adapter.build_node(z, root, root);
    }

    // Test chain for cell `y` that fails if not `e.idx()`.
    const var_t y_t = y_c == e.u() ? var_t::out_bit : var_t::in_bit;

    int y_val = (y_t == var_t::out_bit ? e : e.reversed()).idx();

    // chain to check `y != y_val`
    auto y_neq = adapter.build_node(false);

    // chain to check `y == y_val`
    auto y_eq  = root;

    assert(z == y_max_var);
    for (; y_min_var <= z; --z) {
      if (type_of_var(z, opt) == y_t) {
        const bool bit_val = next_fixed_bit(y_val, opt);

        y_neq = adapter.build_node(z,
                                   bit_val ? root : y_neq,
                                   bit_val ? y_neq : root);

        y_eq  = adapter.build_node(z,
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
      y_eq  = adapter.build_node(z, y_eq,  y_eq);
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

        x_chain = adapter.build_node(z,
                                     bit_val ? y_neq : x_chain,
                                     bit_val ? x_chain : y_neq);
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
    for (; MIN_CELL_VAR(opt) <= z; --z) {
      root = adapter.build_node(z, root, root);
    }

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Binary Counter increment relation.
  ///
  /// The gadget is constructed such, that the counter is big-endian.
  ///
  /// \remark This is expected to work well with both BDDs and ZDDs.
  template<typename adapter_t>
  typename adapter_t::dd_t adder_gadget(adapter_t &/*adapter*/,
                                        const edge &/*e*/,
                                        int /*m*/)
  {
    throw std::invalid_argument("Binary Adder not implemented!");
  }

  /// \brief Binary Counter with a fixed value
  ///
  /// The gadget is constructed such, that the counter is big-endian.
  ///
  /// \remark This is expected to work well with both BDDs and ZDDs.
  template<typename adapter_t>
  typename adapter_t::dd_t adder_gadget(adapter_t &/*adapter*/,
                                        const cell &/*c*/,
                                        const int /*m*/,
                                        int /*value*/)
  {
    throw std::invalid_argument("Binary Adder not implemented!");
  }

  /// \brief Linear-Feedback Shift Register (LFSR) increment relation.
  ///
  /// \param p A Mersenne Prime number
  ///
  /// \remark This is expected to work well with both BDDs and ZDDs.
  template<typename adapter_t>
  typename adapter_t::dd_t lfsr_gadget(adapter_t &/*adapter*/,
                                       const edge &/*e*/,
                                       int /*m*/)
  {
    assert(is_mersenne_prime(m));
    throw std::invalid_argument("LFSR not implemented!");
  }

  /// \brief Linear-Feedback Shift Register (LFSR) with a fixed value
  ///
  /// The gadget is constructed such, that the counter is big-endian.
  ///
  /// \remark This is expected to work well with both BDDs and ZDDs.
  template<typename adapter_t>
  typename adapter_t::dd_t lfsr_gadget(adapter_t &/*adapter*/,
                                       const cell &/*c*/,
                                       const int /*m*/,
                                       int /*value*/)
  {
    assert(is_mersenne_prime(m));
    throw std::invalid_argument("LFSR not implemented!");
  }

  /// \brief One-hot encoding with a linear number of variables.
  ///
  /// While we use a linear number of bits, it is technically incorrect to call
  /// this a *unary* encoding; a better word for it might be *one-hot*.
  ///
  /// \remark This is expected to primarily work well with ZDDs.
  template<typename adapter_t>
  typename adapter_t::dd_t unary_gadget(adapter_t &/*adapter*/,
                                        const edge &/*e*/,
                                        int /*m*/)
  {
    throw std::invalid_argument("Unary Gadget not implemented!");
  }

  /// \brief One-hot encoding with a linear number of variables with a fixed value
  ///
  /// While we use a linear number of bits, it is technically incorrect to call
  /// this a *unary* encoding; a better word for it might be *one-hot*.
  ///
  /// \remark This is expected to primarily work well with ZDDs.
  template<typename adapter_t>
  typename adapter_t::dd_t unary_gadget(adapter_t &/*adapter*/,
                                        const cell &/*c*/,
                                        const int /*m*/,
                                        int /*value*/)
  {
    throw std::invalid_argument("Unary Gadget not implemented!");
  }

  /// \brief Wrapper to pick the desired gadget for an increment relation.
  template<typename adapter_t>
  typename adapter_t::dd_t gadget(adapter_t &adapter,
                                  const edge &e,
                                  int p,
                                  const enc_opt &opt)
  {
    return adapter.top();

    switch (opt) {
    case enc_opt::UNARY:
    case enc_opt::CRT__UNARY: {
      return unary_gadget(adapter, e, p);
    }
    case enc_opt::BINARY: {
      return adder_gadget(adapter, e, p);
    }
    case enc_opt::CRT__BINARY: {
      return is_mersenne_prime(p)
        ? lfsr_gadget(adapter, e, p)
        : adder_gadget(adapter, e, p);
    }
    case enc_opt::TIME:
    default:
      { throw std::invalid_argument("No gadgets exist for time-based encoding"); }
    }
  }

  /// \brief Wrapper to pick the desired gadget for a fixed value.
  template<typename adapter_t>
  typename adapter_t::dd_t gadget(adapter_t &adapter,
                                  const cell &c,
                                  const int p,
                                  const int value,
                                  const enc_opt &opt)
  {
    return adapter.top();

    switch (opt) {
    case enc_opt::UNARY:
    case enc_opt::CRT__UNARY: {
      return unary_gadget(adapter, c, p, value);
    }
    case enc_opt::BINARY: {
      return adder_gadget(adapter, c, p, value);
    }
    case enc_opt::CRT__BINARY: {
      return is_mersenne_prime(p)
        ? lfsr_gadget(adapter, c, p, value)
        : adder_gadget(adapter, c, p, value);
    }
    case enc_opt::TIME:
    default:
      { throw std::invalid_argument("No gadgets exist for time-based encoding"); }
    }
  }

  /// \brief Predicate that is true for a given type of bits for cells on a
  /// specific row.
  auto bit_pred(const var_t &t, const enc_opt &opt) {
    return [=](const int x) {
      return type_of_var(x, opt) == t;
    };
  }

  /// \brief Predicate that is true for a given type of bits for cells on a
  /// specific row.
  auto bit_pred(const int row, const var_t &t, const enc_opt &opt) {
    return [=](const int x) {
      return cell_of_var(x, opt).row() == row && type_of_var(x, opt) == t;
    };
  }

  /// \brief Predicate that is true for a cell's specific given type of bits.
  auto bit_pred(const cell &c, const var_t &t, const enc_opt &opt) {
    return [=](const int x) {
      return cell_of_var(x, opt) == c && type_of_var(x, opt) == t;
    };
  }

  /// \brief Encoding of the Knight's Tour problem given a non-zero number of
  ///        modulo values.
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
  template<typename adapter_t>
  typename adapter_t::dd_t create(adapter_t &adapter, const enc_opt &opt)
  {
    if (rows() < cols()) {
      std::cout << "   | Note:\n"
                << "   |   The variable ordering is designed for 'cols <= rows'.\n"
                << "   |   Maybe restart with the dimensions flipped?\n"
                << "   |\n";
    }

    // -------------------------------------------------------------------------
    // Trivial cases
    if (cells() == 1) {
      return adapter.ithvar(cell(0,0).dd_var());
    }

    for (int row = 0; row < rows(); ++row) {
      for (int col = 0; col < cols(); ++col) {
        const cell c_from(row, col);

        if (!c_from.has_neighbour()) { return adapter.bot(); }
      }
    }

    assert(3 <= rows() && 3 <= cols());
    assert(3 <  rows() || 3 <  cols());

    // -------------------------------------------------------------------------
    // Start with all edges (even illegal ones), but '1A -> 2C', '3B -> 1A'
    // already fixed.
    typename adapter_t::dd_t paths = init_special(adapter, opt);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "   | Fix Corner (nodes):       " << adapter.nodecount(paths) << "\n"
              << std::flush;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Make one-hot for binary
    if (opt == enc_opt::UNARY || opt == enc_opt::CRT__UNARY) {
#ifdef BDD_BENCHMARK_STATS
      std::cout << "   |\n"
                << "   | Force one-hot";
#endif // BDD_BENCHMARK_STATS
        paths &= one_hot_edges(adapter, opt);

#ifdef BDD_BENCHMARK_STATS
        const size_t nodecount = adapter.nodecount(paths);
        largest_bdd = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << " (nodes):    " << nodecount << "\n"
                  << std::flush;
#endif // BDD_BENCHMARK_STATS
    }

    // -------------------------------------------------------------------------
    // Force different choice for in-going and out-going edge
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n"
              << "   | In != Out";
#endif // BDD_BENCHMARK_STATS
    // Apply constraint
    paths &= unmatch_in_out(adapter, opt);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(paths);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << " (nodes):        " << nodecount << "\n"
              << std::flush;
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Remove illegal edges
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n"
              << "   | Remove non-existent Edges\n";
#endif // BDD_BENCHMARK_STATS
    for (int edge_idx = cell::max_moves-1; 0 <= edge_idx; --edge_idx) {
      paths &= remove_illegal(adapter, edge_idx, opt);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << "   |  --> [" << edge_idx << "]"
                << " (nodes):         " << nodecount << "\n"
                << std::flush;
#endif // BDD_BENCHMARK_STATS
    }

    // -------------------------------------------------------------------------
    // Force matching choice in in-going and out-going edge
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n"
              << "   | Match Edge-index between cells\n";
#endif // BDD_BENCHMARK_STATS
    for (int row = MAX_ROW(); 0 <= row; --row) {
      for (int col = MAX_COL(); 0 <= col; --col) {
        const cell u(row, col);

        // Skip (0,0) since both its ingoing and outgoing edges are fixed
        if (u != cell::special_0()) {
          for (const cell v : u.neighbours()) {
            const edge e(u,v);

            // Skip (0,0) since both its ingoing and outgoing edges are fixed
            if (v == cell::special_0()) { continue; }

            paths &= match_u_v(adapter, e, opt);

#ifdef BDD_BENCHMARK_STATS
            const size_t nodecount = adapter.nodecount(paths);
            largest_bdd = std::max(largest_bdd, nodecount);
            total_nodes += nodecount;

            std::cout << "   |  " << e.to_string() << " (nodes):          " << nodecount << "\n"
                      << std::flush;
#endif // BDD_BENCHMARK_STATS
          }
        }

        // Quantify a cell two rows below and one to the right of the current;
        // this one will never be relevant for later cells.
        const cell q_cell(row+2, col+1);
        if (!q_cell.out_of_range()) {
          paths = adapter.exists(paths, bit_pred(q_cell, var_t::in_bit, opt));

#ifdef BDD_BENCHMARK_STATS
          const size_t nodecount = adapter.nodecount(paths);
          largest_bdd = std::max(largest_bdd, nodecount);
          total_nodes += nodecount;

          std::cout << "   |  Exists " << q_cell.to_string() << " (nodes):       " << nodecount << "\n"
                    << std::flush;
#endif // BDD_BENCHMARK_STATS
        }
      }

      // Quantify the last cell on row+2, since it will not be relevant beyond
      // this point.
      const cell q_cell(row+2, 0);
      if (!q_cell.out_of_range()) {
        paths = adapter.exists(paths, bit_pred(q_cell, var_t::in_bit, opt));

#ifdef BDD_BENCHMARK_STATS
        const size_t nodecount = adapter.nodecount(paths);
        largest_bdd = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << "   |  Exists " << q_cell.to_string() << " (nodes):       " << nodecount << "\n"
                  << std::flush;
#endif // BDD_BENCHMARK_STATS
      }
    }

    { // Quantify remaining two rows
      paths = adapter.exists(paths, bit_pred(var_t::in_bit, opt));

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << "   |  Exists 1_,2_ (nodes):    " << nodecount << "\n"
                << std::flush;
#endif // BDD_BENCHMARK_STATS
    }

    // -------------------------------------------------------------------------
    // Add cycle length constraint(s) per modulo value
    const std::vector<int> ps = gadget_moduli(opt);
    for (const int p : ps) {
#ifdef BDD_BENCHMARK_STATS
      std::cout << "   |\n"
                << "   | Add path-length constraints ( % " << p << " )\n";
#endif // BDD_BENCHMARK_STATS

      // TODO (ZDD): extend variables for
      //
      //      (N-1,N-1), ..., (N-1,0), (N-2,N-1), ...(N-2,0), (N-3,0).

      for (int row = MAX_ROW(); 0 <= row; --row) {
        for (int col = MAX_COL(); 0 <= col; --col) {
          const cell u(row, col);

          // TODO (ZDD): Extend with gadget bits for (row-2,col-1).

          if (u.is_special()) {
            const int u_val = u == cell::special_0() ? 0
                            : u == cell::special_1() ? 1
                            : cells()-1;

            paths &= gadget(adapter, u, p, u_val, opt);

#ifdef BDD_BENCHMARK_STATS
            const size_t nodecount = adapter.nodecount(paths);
            largest_bdd = std::max(largest_bdd, nodecount);
            total_nodes += nodecount;

            std::cout << "   | |  " << u.to_string() << " (nodes):            " << nodecount << "\n"
                      << std::flush;
#endif // BDD_BENCHMARK_STATS
          } else {
            for (const cell v : u.neighbours()) {
              const edge e(u,v);

              paths &= gadget(adapter, e, p, opt);

#ifdef BDD_BENCHMARK_STATS
              const size_t nodecount = adapter.nodecount(paths);
              largest_bdd = std::max(largest_bdd, nodecount);
              total_nodes += nodecount;

              std::cout << "   | |  " << e.to_string() << " (nodes):        " << nodecount << "\n"
                        << std::flush;
#endif // BDD_BENCHMARK_STATS
            }

            // Quantify a cell two rows below and one to the right of the current;
            // this one will never be relevant for later cells.
            const cell q_cell(row+2, col+1);
            if (!q_cell.out_of_range()) {
              paths = adapter.exists(paths, bit_pred(q_cell, var_t::gadget_bit, opt));

#ifdef BDD_BENCHMARK_STATS
              const size_t nodecount = adapter.nodecount(paths);
              largest_bdd = std::max(largest_bdd, nodecount);
              total_nodes += nodecount;

              std::cout << "   | |  Exists " << q_cell.to_string() << " (nodes):     " << nodecount << "\n"
                        << std::flush;
#endif // BDD_BENCHMARK_STATS
            }
          }
        }

        // Quantify the last cell on row+2, since it will not be relevant beyond
        // this point.
        const cell q_cell(row+2, 0);
        if (!q_cell.out_of_range()) {
          paths = adapter.exists(paths, bit_pred(q_cell, var_t::gadget_bit, opt));

#ifdef BDD_BENCHMARK_STATS
          const size_t nodecount = adapter.nodecount(paths);
          largest_bdd = std::max(largest_bdd, nodecount);
          total_nodes += nodecount;

          std::cout << "   | |  Exists " << q_cell.to_string() << " (nodes):     " << nodecount << "\n"
                    << std::flush;
#endif // BDD_BENCHMARK_STATS
        }
      }

      { // Quantify remaining two rows
        paths = adapter.exists(paths, bit_pred(var_t::gadget_bit, opt));

#ifdef BDD_BENCHMARK_STATS
        const size_t nodecount = adapter.nodecount(paths);
        largest_bdd = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << "   | |  Exists 1_,2_ (nodes):  " << nodecount << "\n"
                  << std::flush;
#endif // BDD_BENCHMARK_STATS
      }
    }

    // -------------------------------------------------------------------------
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n";
#endif // BDD_BENCHMARK_STATS

    // adapter.print_dot(paths, "paths.dot");

    return paths;
  }
}


////////////////////////////////////////////////////////////////////////////////
/// \brief Algorithms for the `enc_opt::TIME` encoding
///
/// A drastically different way to encode the Knight's Tour problem with a
/// quartic (N^4) number of variables rather than a quadratic(ish) number. To
/// this end, we do not encode edges on the board. Instead, each cell of the
/// board `(r,c)` is associated with a time-step `t` which is up to `r*c`. Each
/// of the variable is `true` if one visits `(r,c)` at time `t`.
///
/// Initially, we accumulate all paths of length `t` before adding a hamiltonian
/// constraint on each cell one-by-one.
///
/// Symmetries are broken by encoding the special starting cell separately and
/// forcing it to visit a pre-determined neighbour at time `1` and the other at
/// `t-1`. While we are at it, we may as well also include the hamiltonian
/// constraint instead of adding it later.
///
/// \remark This is expected to primarily with ZDDs.
////////////////////////////////////////////////////////////////////////////////
namespace enc_time
{
  /// \brief Number of different time-steps
  inline int times()
  { return cells(); }

  /// \brief Smallest valid time-step
  constexpr int MIN_TIME()
  { return 0; }

  /// \brief Largest valid time-step
  inline int MAX_TIME()
  { return times() - 1; }

  /// \brief The shift needed for the DD variable for a cell at time-step `t`.
  inline int time_shift(int t)
  { return cells() * t; }

  /// \brief Number of variables used in this encoding.
  inline int vars()
  {
    const int shift = time_shift(MAX_TIME());
    const int max_var = cell(MAX_ROW(), MAX_COL()).dd_var(shift);
    return max_var+1;
  }

  /// \brief Number of variables to use for final model count
  inline int satcount_vars()
  { return vars(); }

  /// \brief Helper function to fix one cell to true and all others to false for
  ///        one time step.
  ///
  /// \see rel_init
  template<typename adapter_t>
  void rel_0__fix(adapter_t &adapter,
                  const cell &fixed_cell,
                  int time,
                  typename adapter_t::build_node_t &root)
  {
    const int shift = time_shift(time);

    for (const cell &c : cells_descending) {
      const int var = c.dd_var(shift);

      root = c == fixed_cell
        ? adapter.build_node(var, adapter.build_node(false), root)
        : adapter.build_node(var, root, adapter.build_node(false));
    }
  }

  /// \brief Constraint to break symmetries and fix it to be a cycle.
  template<typename adapter_t>
  typename adapter_t::dd_t rel_0(adapter_t &adapter)
  {
    auto root = adapter.build_node(true);

    // Fix t = MAX_TIME() to be `cell::special_2()`
    rel_0__fix(adapter, cell::special_2(), MAX_TIME(), root);

    // Set t = MAX_TIME()-1, ..., 3, 2 as don't care (but with hamiltonian
    // constraint for the special cells).
    for (int time = MAX_TIME()-1; 1 < time; --time) {
      const int shift = time_shift(time);

      for (const cell &c : cells_descending) {
        const int var = c.dd_var(shift);

        root = c.is_special()
          ? adapter.build_node(var, root, adapter.build_node(false))
          : adapter.build_node(var, root, root);
      }
    }

    // Fix t = 1, 0 to be `cell::special_1()` and `cell::special_0()`
    rel_0__fix(adapter, cell::special_1(), 1, root);
    rel_0__fix(adapter, cell::special_0(), 0, root);

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    // This will already be accounted for in 'create()' below
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Helper function to fix all time steps in an interval to be "don't
  ///        care" nodes (except for the unreachable ones)
  ///
  /// \see rel_t
  template<typename adapter_t>
  void rel_t__dont_care(adapter_t &adapter,
                        int t_begin, int t_end,
                        typename adapter_t::build_node_t &out)
  {
    assert(t_end <= t_begin);

    for (int time = t_begin; t_end < time; --time) {
      const int shift = time_shift(time);

      for (const cell &c : cells_descending) {
        const int var = c.dd_var(shift);

        // Fix unreachable cells to unvisitable.
        out = c.has_neighbour()
          ? adapter.build_node(var, out, out)
          : adapter.build_node(var, out, adapter.build_node(false));
      }
    }
  }

  /// \brief Diagram for a transitions at time step `t` to `t+1`.
  template<typename adapter_t>
  typename adapter_t::dd_t rel_t(adapter_t &adapter, int t)
  {
    // Time steps: t' > t+1
    //   Chain of "don't cares" for whatever happens after t+1.
    typename adapter_t::build_node_t post_chain = adapter.build_node(true);
    rel_t__dont_care(adapter, MAX_TIME(), t+1, post_chain);

    // Time step: t+1
    //   Chain with decision on where to be at time 't+1' given where one was at time 't'.
    std::vector<typename adapter_t::build_node_t> to_chains(cells(), adapter.build_node(false));
    {
      const int shift = time_shift(t+1);

      for (const cell& to : cells_descending) {
        const int to_var = to.dd_var(shift);

        for (const cell &from : cells_descending)  {
          // Do not build the chain for unreachable nodes. Notice, we skip this
          // entire possibility when building the nodes for time step t.
          if (!from.has_neighbour()) { continue; }

          const int idx = from.dd_var();
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
        for (const cell &o : cells_descending) {
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

      for (const cell &c : cells_descending) {
        const int var = c.dd_var(shift);

        // Create next node in chain of choices for "we are here".
        {
          const int idx = c.dd_var();

          root = c.has_neighbour()
            ? adapter.build_node(var, root, to_chains.at(idx))
            : adapter.build_node(var, root, adapter.build_node(false));
        }

        // Expand all `to_chains` that still are of interest, i.e. that will be
        // used later. Here, we should write that they cannot pick this variable.
        for (const cell &o : cells_descending) {
          // Skip cells that already have been or never will be processed
          if (c < o || c == o || !o.has_neighbour()) { continue; }

          const int idx = o.dd_var();
          to_chains.at(idx) = adapter.build_node(var, to_chains.at(idx), adapter.build_node(false));
        }
      }
    }

    // Time steps: t' < t
    //   Chain of "don't cares" for whatever happens before t.
    rel_t__dont_care(adapter, t-1, -1, root);

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  /// \brief Diagram for the hamiltonian constraint for all time steps.
  ///
  /// \details Essentially, we have two chains, one for "still not visited" (0)
  /// and the other for "has been visited" (1).
  template<typename adapter_t>
  typename adapter_t::dd_t hamiltonian(adapter_t &adapter, const cell& ham_c)
  {
    // TODO: include in the encoding, that one cannot be here at time 0, 1 or MAX

    auto out_0 = adapter.build_node(false);
    auto out_1 = adapter.build_node(true);

    for (int time = MAX_TIME(); MIN_TIME() <= time; --time) {
      const int shift = time_shift(time);

      for (const cell &c : cells_descending) {
        const int var = c.dd_var(shift);

        out_0 = c == ham_c
          ? adapter.build_node(var, out_0, out_1)
          : adapter.build_node(var, out_0, out_0);

        if (MIN_TIME() < time || ham_c < c) {
          out_1 = c == ham_c
            ? adapter.build_node(var, out_1, adapter.build_node(false))
            : adapter.build_node(var, out_1, out_1);
        }
      }
    }

    typename adapter_t::dd_t out = adapter.build();

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(out);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;
#endif // BDD_BENCHMARK_STATS

    return out;
  }

  template<typename adapter_t>
  typename adapter_t::dd_t create(adapter_t &adapter)
  {
    // -------------------------------------------------------------------------
    // Trivial cases
    if (cells() == 1) {
      return adapter.ithvar(cell(0,0).dd_var());
    }

    for (int row = 0; row < rows(); ++row) {
      for (int col = 0; col < cols(); ++col) {
        const cell c_from(row, col);

        if (!c_from.has_neighbour()) { return adapter.bot(); }
      }
    }

    assert(3 <= rows() && 3 <= cols());
    assert(3 <  rows() || 3 <  cols());

    // -------------------------------------------------------------------------
    // Accumulate cell-relation constraints
    typename adapter_t::dd_t paths = rel_0(adapter);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(paths);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;

    std::cout << "   |\n"
              << "   | All Paths\n"
              << "   |   [t = " << MAX_TIME() << (MAX_TIME() < 10 ? " " : "") << ", 0" << "] (nodes):    " << nodecount << "\n"
              << std::flush;
#endif // BDD_BENCHMARK_STATS

    // Aggregate transitions backwards in time.
    for (int t = MAX_TIME()-1; MIN_TIME() < t; --t) {
      paths &= rel_t(adapter, t);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << "   |   [t = " << t << (t < 10 ? " " : "") << "   ] (nodes):    " << nodecount << "\n"
                << std::flush;
#endif // BDD_BENCHMARK_STATS
    }
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n";
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // Accumulate hamiltonian constraints
    //
    // TODO: Follow 'cells_descending' ordering (possibly in reverse)?
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   | Hamiltonian Constraint\n"
              << std::flush;
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
        largest_bdd = std::max(largest_bdd, nodecount);
        total_nodes += nodecount;

        std::cout << "   |   " << c.to_string() << " (nodes):             " << nodecount << "\n"
                  << std::flush;
#endif // BDD_BENCHMARK_STATS
      }
    }

#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n";
#endif // BDD_BENCHMARK_STATS
    return paths;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// \brief Knight's Tour program: pick encoding and time its execution.
////////////////////////////////////////////////////////////////////////////////
template<typename adapter_t>
int run_knights_tour(int argc, char** argv)
{
  enc_opt opt = enc_opt::TIME; // Default strategy
  bool should_exit = parse_input(argc, argv, opt);

  if (input_sizes.size() == 0) { input_sizes.push_back(8); }
  if (input_sizes.size() == 1) { input_sizes.push_back(input_sizes.at(0)); }

  if (should_exit) { return -1; }

  // ---------------------------------------------------------------------------
  std::cout << rows() << " x " << cols() << " - Knight's Tour (" << adapter_t::NAME << " " << M << " MiB):\n"
            << "   | Encoding:               " << option_str(opt) << "\n";

  if (rows() == 0 || cols() == 0) {
    std::cout << "\n"
              << "  The board has no cells. Please provide Ns > 1 (-N)\n";
    return 0;
  }

  // ---------------------------------------------------------------------------
  // Initialise package manager
  int vars = 0;

  switch (opt) {
  case enc_opt::BINARY:
  case enc_opt::UNARY:
  case enc_opt::CRT__BINARY:
  case enc_opt::CRT__UNARY: {
    vars = enc_gadgets::vars(opt);
    break;
  }
  case enc_opt::TIME: {
    vars = enc_time::vars();
    break;
  }
  default:
    { /* ? */ }
  }

  time_point t_init_before = get_timestamp();
  adapter_t adapter(vars);
  time_point t_init_after = get_timestamp();

  std::cout << "\n   " << adapter_t::NAME << " initialisation:\n"
            << "   | variables:                " << vars << "\n"
            << "   | time (ms):                " << duration_of(t_init_before, t_init_after) << "\n"
            << std::flush;

  // -----------------------------------------------------------------------------
  // Initialise cells (i.e. variable ordering)
  init_cells_descending();

  uint64_t solutions;
  {
    // ---------------------------------------------------------------------------
    // Construct paths based on chosen encoding
    std::cout << "\n"
              << "   Paths Construction\n";

    typename adapter_t::dd_t paths;

    const time_point before_paths = get_timestamp();
    switch (opt) {
    case enc_opt::BINARY:
    case enc_opt::UNARY:
    case enc_opt::CRT__BINARY:
    case enc_opt::CRT__UNARY: {
      paths = enc_gadgets::create(adapter, opt);
      break;
    }
    case enc_opt::TIME:
    default: {
      paths = enc_time::create(adapter);
      break;
    }
    }
    const time_point after_paths = get_timestamp();
    const time_duration paths_time = duration_of(before_paths, after_paths);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "   | total no. nodes:          " << total_nodes << "\n"
              << "   | largest size (nodes):     " << largest_bdd << "\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "   | final size (nodes):       " << adapter.nodecount(paths) << "\n"
              << "   | time (ms):                " << paths_time << "\n"
              << std::flush;

    // -------------------------------------------------------------------------
    // Count number of solutions
    const size_t vc = opt == enc_opt::TIME
      ? enc_time::satcount_vars()
      : enc_gadgets::satcount_vars(opt);

    const time_point before_satcount = get_timestamp();
    solutions = adapter.satcount(paths, vc);
    const time_point after_satcount = get_timestamp();

    const time_duration satcount_time = duration_of(before_satcount, after_satcount);

    std::cout << "\n"
              << "   Counting solutions:\n"
              << "   | number of solutions:      " << solutions << "\n"
              << "   | time (ms):                " << satcount_time << "\n"
              << std::flush;

    // -------------------------------------------------------------------------
    // Print out a solution
    std::cout << "\n"
              << "   Solution Example:\n"
              << "   | ";

    const auto path = adapter.pickcube(paths);
    for (const auto& [x,v] : path) {
      std::cout << "x" << x << "=" << v << " ";
    }
    if (path.empty()) { std::cout << "none..."; }

    std::cout << "\n"
              << std::flush;

    // -------------------------------------------------------------------------
    std::cout << "\n"
              << "total time (ms):               " << (paths_time + satcount_time) << "\n"
              << std::flush;
  }

  adapter.print_stats();

  const int N = rows()+cols();
  if (N < size(expected_knights_tour_closed)
      && expected_knights_tour_closed[N] != UNKNOWN && solutions != expected_knights_tour_closed[N]) {
    return -1;
  }
  return 0;
}
