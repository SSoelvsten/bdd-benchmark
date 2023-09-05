#include "common.cpp"
#include "expected.h"

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
  //////////////////////////////////////////////////////////////////////////////
  // Members
private:
  int _r;
  int _c;

  //////////////////////////////////////////////////////////////////////////////
  // Constructors
public:
  cell()
    : _r(0), _c(0)
  { }

  cell(int r, int c)
    : _r(r), _c(c)
  { /* TODO: throw std::out_of_range if given bad (r,c)? */ }

  cell(const cell &o) = default;
  cell(cell &&o) = default;

  //////////////////////////////////////////////////////////////////////////////
  // Accessor and DD conversion
public:
  int row() const
  { return _r; }

  int col() const
  { return _c; }

  /// \brief Row-major DD variable name
  int dd_var(const int shift = 0) const
  { return shift + (cols() * _r) + _c; }

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
private:
  /// \brief Number of possible neighbours for a knight
  static constexpr int max_moves = 8;

  /// \brief Hard coded moves relative to the current cell (following the
  ///       variable ordering as per `dd_var`).
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

  /// \brief Whether there is a single knight-move from `this` to `o`
  ///
  /// \details One can move from `this` to `o` if one moves at least one in each
  ///          dimension and by exactly three cells.
  bool has_move_to(const cell& o) const
  {
    const int r_moved = std::abs(this->row() - o.row());
    const int c_moved = std::abs(this->col() - o.col());
    return (0 < r_moved) && (0 < c_moved) && (r_moved + c_moved == 3);
  }

  /// \brief All cells on the board that can be reached from this cell
  std::vector<cell> neighbours() const
  {
    std::vector<cell> res;
    for (int i = 0; i < max_moves; ++i) {
      const cell neighbour(cell(this->row() + moves[i][0], this->col() + moves[i][1]));
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

/// \brief Class to encapsulate logic related to a cell and the Knight's move.
class edge
{
private:
  cell _u;
  cell _v;

public:
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
  const cell& u() const
  { return _u; }

  const cell& v() const
  { return _v; }

  // TODO: int dd_var() const

  //////////////////////////////////////////////////////////////////////////////
  // Quality of life
public:
  bool operator==(const edge& o) const
  { return this->_u == o._u && this->_v == o._v; }

  /// \brief Obtain the reversed directed edge, i.e. from `v` to `u`.
  edge reversed() const
  { return edge{this->_v, this->_u}; }

  /// \brief Human-friendly string
  std::string to_string() const
  { return _u.to_string()+"->"+_v.to_string(); }
};


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
  using graph_t = std::vector<edge>;

  /// \brief Construct the Knight's Graph
  graph_t graph()
  {
    graph_t out;

    assert(rows() > 1 && cols() > 1);
    const int edges = 4 * cells() - 6 * (rows() + cells()) + 8;
    out.reserve(edges);

    for (int row = 0; row < rows(); ++row) {
      for (int col = 0; col < cols(); ++col) {
        const cell c_from(row, col);

        for (const cell &c_to : c_from.neighbours()) {
          out.push_back(edge(c_from, c_to));
        }
      }
    }

    assert(out.size() == edges);
    return out;
  }

  /// \brief Convert a graph into a DOT file for debugging purposes.

  // TODO:
  //   std::string dot_graph(const graph_t &g)

  /// \brief Convert a graph into a diagram.

  // TODO:
  //   template<typename adapter_t>
  //   typename adapter_t::dd_t dd_graph(adapter_t &adapter, const graph_t &g)

  /// \brief Binary Counter with a Modulus.
  ///
  /// \remark This is expected to work well with both BDDs and ZDDs.

  // TODO: Binary Adder with modulo

  /// \brief A binary counter (with 2^n overflow).
  ///
  /// \remark This is expected to work well with both BDDs and ZDDs.

  // TODO: Binary Adder with no modulo (above with `mod = (1<<bits)`).

  /// \brief Linear-Feedback Shift Register (LFSR).
  ///
  /// \param p A Mersenne Prime number
  ///
  /// \remark This is expected to work well with both BDDs and ZDDs.

  // TODO: LFSR Circuit given a Mersenne Prime

  /// \brief One-hot encoding with a linear number of variables.
  ///
  /// While we use a linear number of bits, it is technically incorrect to call
  /// this a *unary* encoding; a better word for it might be *one-hot*.
  ///
  /// \remark This is expected to primarily work well with ZDDs.

  // TODO: One-hot encoding

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

  // TODO: Combine above gadgeets
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
    const int shift = time_shift(enc_time::MAX_TIME());
    const int max_var = cell(MAX_ROW(), MAX_COL()).dd_var(shift);
    return max_var+1;
  }

  /// \brief Cells in descending order (relative to variable ordering).
  std::vector<cell> cells_descending;

  /// \brief Initialise the list of all cells on the board (descendingly)
  ///        following the variable ordering.
  void init_cells_descending()
  {
    cells_descending.clear();
    cells_descending.reserve(cells());

    for (int row = MAX_ROW(); 0 <= row; --row)
      for (int col = MAX_COL(); 0 <= col; --col)
        cells_descending.push_back(cell(row, col));

    // TODO (variable orderings):
    // std::sort<std::greater<cell>>(cells_descending.begin(), cells_descending.end());
  }

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
    assert(3 <= rows() && 3 <= cols());

    // -------------------------------------------------------------------------
    init_cells_descending();

    // -------------------------------------------------------------------------
    // Accumulate cell-relation constraints
    typename adapter_t::dd_t paths = rel_0(adapter);

#ifdef BDD_BENCHMARK_STATS
    const size_t nodecount = adapter.nodecount(paths);
    largest_bdd = std::max(largest_bdd, nodecount);
    total_nodes += nodecount;


    std::cout << "   |\n"
              << "   | Path Construction\n"
              << "   | [t = " << MAX_TIME() << ", 0" << "] : " << nodecount << " DD nodes\n";
#endif // BDD_BENCHMARK_STATS

    // Aggregate transitions backwards in time.
    for (int t = MAX_TIME()-1; MIN_TIME() < t; --t) {
      paths &= rel_t(adapter, t);

#ifdef BDD_BENCHMARK_STATS
      const size_t nodecount = adapter.nodecount(paths);
      largest_bdd = std::max(largest_bdd, nodecount);
      total_nodes += nodecount;

      std::cout << "   | [t = " << t << "] : " << nodecount << " DD nodes\n";
#endif // BDD_BENCHMARK_STATS
    }
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n";
#endif // BDD_BENCHMARK_STATS

    // -------------------------------------------------------------------------
    // accumulate hamiltonian constraints
    //
    // TODO: Follow 'cells_descending' ordering (possibly in reverse)?
#ifdef BDD_BENCHMARK_STATS
    std::cout << "   Hamiltonian Constraint\n";
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

        std::cout << "   | " << c.to_string() << " : " << nodecount << " DD nodes\n";
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

  switch (opt) {
  case enc_opt::BINARY:
  case enc_opt::UNARY:
  case enc_opt::CRT__BINARY:
  case enc_opt::CRT__UNARY:
    std::cout << "   | Encoding not yet supported...\n";
    return 0;
  case enc_opt::TIME: break;
  default:
    { /* ? */ }
  }

  if (rows() == 0 || cols() == 0) {
    std::cout << "\n"
              << "  The board has no cells. Please provide an N > 1 (-N)\n";
    return 0;
  }

  if ((rows() < 3 || cols() < 3) && (rows() != 1 || cols() != 1)) {
    std::cout << "\n"
              << "  There cannot exist closed tours on boards smaller than 3 x 3\n"
              << "  Aborting computation...\n";
    return 0;
  }

  // ---------------------------------------------------------------------------
  // Initialise package manager
  int vars = 0;

  switch (opt) {
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
            << "   | variables:              " << vars << "\n"
            << "   | time (ms):              " << duration_of(t_init_before, t_init_after) << "\n"
            << std::flush;

  uint64_t solutions;
  {
    // ---------------------------------------------------------------------------
    // Construct paths based on chosen encoding
    typename adapter_t::dd_t paths;

    const time_point before_paths = get_timestamp();
    switch (opt) {
    // case enc_opt::BINARY:
    // case enc_opt::UNARY:
    // case enc_opt::CRT__BINARY:
    // case enc_opt::CRT__UNARY:
    case enc_opt::TIME:
    default: {
      paths = enc_time::create(adapter);
      break;
    }
    }
    const time_point after_paths = get_timestamp();
    const time_duration paths_time = duration_of(before_paths, after_paths);

    std::cout << "   | final size (nodes):     " << adapter.nodecount(paths) << "\n"
              << "   | time (ms):              " << paths_time << "\n"
              << std::flush;

    // -------------------------------------------------------------------------
    // Count number of solutions
    const time_point before_satcount = get_timestamp();
    solutions = adapter.satcount(paths);
    const time_point after_satcount = get_timestamp();

    const time_duration satcount_time = duration_of(before_satcount, after_satcount);

    std::cout << "\n"
              << "   Counting solutions:\n"
              << "   | number of solutions:    " << solutions << "\n"
              << "   | time (ms):              " << satcount_time << "\n"
              << std::flush;

    // -------------------------------------------------------------------------
    std::cout << "\n"
              << "total time (ms):             " << (paths_time + satcount_time) << "\n"
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
