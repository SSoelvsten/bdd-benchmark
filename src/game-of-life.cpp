#include "common.cpp"
#include "expected.h"

#include <unordered_map>
#include <utility>

// ========================================================================== //
//                   PRIMING OF VARIABLES WITH TRANSITIONS                    //

/// \brief   Renaming of Boolean values to something less error-prone.
///
/// \details One quickly forgets whether 'primed == true' means before or after
///          the transition.
enum prime : bool { pre = false, post = true };

// ========================================================================== //
inline int rows(bool p = false)
{ return input_sizes.at(0) + 2*(!p); }

/// \param p Whether the variable is primed.
inline int MIN_ROW(bool p = false)
{ return p; }

/// \param p Whether the variable is primed.
inline int MAX_ROW(bool p = false)
{ return rows(p) - (!p); }

inline int cols(bool p = false)
{ return input_sizes.at(1) + 2*(!p); }

/// \param p Whether the variable is primed.
inline int MIN_COL(bool p = false)
{ return p; }

/// \param p Whether the variable is primed.
inline int MAX_COL(bool p = false)
{ return cols(p) - (!p); }

inline int varcount(bool p)
{ return rows(p) * cols(p); }

inline int varcount()
{ return varcount(false) + varcount(true); }

// ========================================================================== //

// TODO: Symmetry Option

// ========================================================================== //
//                              CELLS + VARIABLES                             //

////////////////////////////////////////////////////////////////////////////////
/// \brief A single cell, its coordinate, its DD variable, and its neighbours.
///
/// \details Major parts of this class is copied from `hamiltonian.cpp`.
////////////////////////////////////////////////////////////////////////////////
class cell
{
  //////////////////////////////////////////////////////////////////////////////
  // Members
private:
  /// \brief Row index
  int  _row;

  /// \brief Column index
  int  _col;

  //////////////////////////////////////////////////////////////////////////////
  // Constructors
public:

  /// \brief Defalt construction of illegal cell.
  cell()
    : _row(-1), _col(-1)
  { }

  /// \brief Construction of cell [r,c].
  ///
  /// \remark This does not check whether the cell actually is legal. To do so,
  ///         please use `out_of_range`.
  cell(int row, int col)
    : _row(row), _col(col)
  { /* TODO: throw std::out_of_range if given bad (row,col)? */ }

  /// \brief Obtain the smallest cell (with respect to variable ordering).
  static cell min(bool p)
  {
    return cell(MIN_ROW(p), MIN_COL(p));
  }

  /// \brief Obtain the largest cell (with respect to variable ordering).
  static cell max(bool p)
  {
    return cell(MAX_ROW(p), MAX_COL(p));
  }

  //////////////////////////////////////////////////////////////////////////////
  // State
public:
  /// \brief Obtain this cell's row.
  int row() const
  { return _row; }

  /// \brief Obtain this cell's column.
  int col() const
  { return _col; }

  //////////////////////////////////////////////////////////////////////////////
  // Decisin Diagram Variable
public:

  /// \brief Row-major DD variable name
  ///
  /// \param p Whether the variable is primed (see `prime`).
  int dd_var(bool p = false) const
  {
    assert(!this->out_of_range(p));
    assert(MIN_ROW(prime::pre) == 0 && MIN_COL(prime::pre) == 0);

    // First row.
    if (this->row() == MIN_ROW(prime::pre)) {
      assert(p == prime::pre && this->out_of_range(prime::post));
      return this->col();
    }

    // Accumulate number of variables
    int vars = cols(prime::pre);

    const int other_rows     = (this->row()-1) - MIN_ROW(prime::pre);
    const int other_row_vars = cols(prime::pre) + cols(prime::post);

    vars += other_rows * other_row_vars;

    // Last row
    if (this->row() == MAX_ROW(prime::pre)) {
      assert(p == prime::pre && this->out_of_range(prime::post));

      return vars + this->col();
    }

    // First cell on row
    if (this->col() == MIN_COL(prime::pre)) {
      assert(p == prime::pre && this->out_of_range(prime::post));

      return vars;
    }

    // Last cell on row
    if (this->col() == MAX_COL(prime::pre)) {
      assert(p == prime::pre && this->out_of_range(prime::post));

      return vars + 2*cols(prime::post) + 1;
    }

    // Add double variables in-between
    vars += 2*(this->col() - MIN_COL(prime::post)) + 1 + p;
    return vars;
  }

  /// \brief   Converts back from a diagram variable to the cell.
  ///
  /// \details This ignores whether the cell was primed or not.
  static cell from(int dd_var)
  {
    assert(0 <= dd_var);

    //const bool prime = dd_var % 2;

    // First row
    if (dd_var < cols(prime::pre)) {
      return cell(MIN_ROW(prime::pre), dd_var);
    }

    // Last row
    const int last_other_row = varcount() - cols(prime::pre);
    if (dd_var > last_other_row) {
      return cell(MAX_ROW(prime::pre), dd_var - last_other_row);
    }

    // Other rows
    const int other_row_vars = cols(prime::pre) + cols(prime::post);
    dd_var -= cols(prime::pre);

    const int row = 1 + (dd_var / other_row_vars);

    const int col_var = dd_var % other_row_vars;
    const int col = col_var == 0    ? MIN_COL(prime::pre)
      : col_var == other_row_vars-1 ? MAX_COL(prime::pre)
      : (col_var+1) / 2;

    return cell(row, col);
  }

  /// \brief Obtain prime flag.
  static bool is_prime(int x)
  {
    const cell c = cell::from(x);
    return !c.out_of_range(true) && c.dd_var(true) == x;
  }

  /// \brief   Converts back from a diagram variable to the cell.
  ///
  /// \details This ignores whether the cell was primed or not.
  cell(int dd_var)
    : cell(cell::from(dd_var))
  { }

  /// \brief Obtain the minimal DD variable that is within range.
  static int min_var()
  {
    return cell::min(prime::pre).dd_var(prime::pre);
  }

  /// \brief Obtain the maximal DD variable that is within range.
  static int max_var()
  {
    return cell::max(prime::pre).dd_var(prime::pre);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Distance and Neighbours
public:
  /// \brief   Whether this cell represents an actual valid position depending
  ///          on whether it is primed or not.
  ///
  /// \param p Whether the variable is primed (see `prime`).
  bool out_of_range(bool p = false) const
  {
    return row() < MIN_ROW(p) || MAX_ROW(p) < row()
        || col() < MIN_COL(p) || MAX_COL(p) < col();
  }

  /// \brief Vertical distance between two cells
  int vertical_dist_to(const cell &o) const
  { return std::abs(this->row() - o.row()); }

  /// \brief Horizontal distance between two cells
  int horizontal_dist_to(const cell &o) const
  { return std::abs(this->col() - o.col()); }

  //////////////////////////////////////////////////////////////////////////////
  // Quality of life
public:
  /// \brief Human-friendly string
  std::string to_string() const
  {
    std::string res;
    res += 'A'+this->row();
    res += 'a'+this->col();
    return res;
  }

  /// \brief Whether two `cell` objects refer to the same coordinate.
  bool operator== (const cell& o) const
  {
    return this->row() == o.row() && this->col() == o.col();
  }
};

// ========================================================================== //
//                             TRANSITION RELATION                            //
//
// From Wikipedia 'https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life':
//
// To avoid decisions and branches in the counting loop, the rules can be
// rearranged from an egocentric approach of the inner field regarding its
// neighbours to a scientific observer's viewpoint: if the sum of all nine
// fields in a given neighbourhood is three, the inner field state for the next
// generation will be life; if the all-field sum is four, the inner field
// retains its current state; and every other sum sets the inner field to death.

/// \brief Decision Diagram that is `true` if exactly `alive` neighbour cells
///        around `c` (including itself) are alive at the 'unprimed' time.
///
/// \details Major parts of this class is copied from `tic-tac-toe.cpp`.
template<typename adapter_t>
typename adapter_t::dd_t
construct_count(adapter_t &adapter, const cell &c, const int alive)
{
  std::vector<typename adapter_t::build_node_t> init_parts(alive+2, adapter.build_node(false));
  init_parts.at(alive) = adapter.build_node(true);

  int cells_remaining = 9;

  int alive_max = alive;
  int alive_min = alive_max;

  for (int x = cell::max_var(); cell::min_var() <= x; --x) {
    const cell curr_cell(x);
    const bool curr_primed = cell::is_prime(x);

    assert(!curr_cell.out_of_range(curr_primed));

    const bool in_neighbourhood =
      c.horizontal_dist_to(curr_cell) <= 1 && c.vertical_dist_to(curr_cell) <= 1;

    if (!curr_primed && in_neighbourhood) {
      cells_remaining -= 1;

      // Open up for one fewer cell is alive (if not already all prior already could be dead).
      alive_min = std::max(alive_min-1, 0);

      // Decrease 'alive_max' if too few variables above could have same number
      // of true variables.
      if (cells_remaining == alive_max) {
        assert(cells_remaining == alive_max && alive_max > 0);

        alive_max -= 1;
      }

      // Update all chains with a possible increment.
      for (int curr_idx = alive_min; curr_idx <= alive_max; ++curr_idx) {
        assert(0 <= curr_idx && curr_idx <= alive);

        const auto low  = init_parts.at(curr_idx);
        const auto high = init_parts.at(curr_idx + 1);

        init_parts.at(curr_idx) = adapter.build_node(x, low, high);
      }
    } else {
      // Update all current chains with "don't-care" nodes
      for (int curr_idx = alive_min; curr_idx <= alive_max; ++curr_idx) {
        assert(0 <= curr_idx && curr_idx <= alive);

        const auto child  = init_parts.at(curr_idx);

        init_parts.at(curr_idx) = adapter.build_node(x, child, child);
      }
    }
  }

  return adapter.build();
}

/// \brief Decision Diagram that is `true` if cell's state is preserved.
///
/// \details Major parts of this class is copied from `tic-tac-toe.cpp`.
template<typename adapter_t>
typename adapter_t::dd_t
construct_eq(adapter_t &adapter, const cell &c)
{
  // TODO: Support non-interleaved variable reordering.

  /// Main root variable
  auto root0 = adapter.build_node(true);
  /// Extra variable on equality split
  auto root1 = root0;

  for (int x = cell::max_var(); cell::min_var() <= x; --x) {
    if (c == cell(x)) {
      if (cell::is_prime(x)) {
        root1 = adapter.build_node(x, adapter.build_node(false), root0);
        root0 = adapter.build_node(x, root0, adapter.build_node(false));
      } else {
        root0 = adapter.build_node(x, root0, root1);
      }
    } else {
      root0 = adapter.build_node(x, root0, root0);
    }
  }

  return adapter.build();
}

template<typename adapter_t>
typename adapter_t::dd_t construct_rel(adapter_t &adapter, const cell &c)
{
  const typename adapter_t::dd_t alive_3 = construct_count(adapter, c, 3);
  const typename adapter_t::dd_t alive_4 = construct_count(adapter, c, 4);

  typename adapter_t::dd_t out;

  { // ---------------------------------------------------------------------------
    // - If the sum is 3, the inner cell will become alife.
    const typename adapter_t::dd_t alive_post = adapter.ithvar(c.dd_var(prime::post));

    out = adapter.apply_imp(alive_3, std::move(alive_post));
  }
  { // ---------------------------------------------------------------------------
    // - If the sum is 4, the inner field retains its state.
    const typename adapter_t::dd_t eq = construct_eq(adapter, c);

    out &= adapter.apply_imp(alive_4, std::move(eq));
  }
  { // ---------------------------------------------------------------------------
    // - Otherwise, the inner field is dead.
    const typename adapter_t::dd_t alive_3or4 = std::move(alive_3) | std::move(alive_4);
    const typename adapter_t::dd_t alive_other = ~std::move(alive_3or4);
    const typename adapter_t::dd_t dead_post = adapter.nithvar(c.dd_var(prime::post));

    out &= adapter.apply_imp(std::move(alive_other), std::move(dead_post));
  }

  return out;
}

// ========================================================================== //
//                               GARDEN OF EDEN                               //

time_duration acc_rel__apply_time  = 0;
time_duration acc_rel__exists_time = 0;

template<typename adapter_t>
typename adapter_t::dd_t acc_rel(adapter_t &adapter)
{
  if (rows() < cols()) {
    std::cout << "   | Note:\n"
              << "   |   The variable ordering is designed for 'cols <= rows'.\n"
              << "   |   Maybe restart with the dimensions flipped?\n"
              << "   |\n";
  }

  const time_point t_apply__before = get_timestamp();
  auto res = adapter.top();

#ifdef BDD_BENCHMARK_STATS
  std::cout << "   | Top        : "
            << adapter.nodecount(res) << " DD nodes\n"
            << std::flush;
#endif // BDD_BENCHMARK_STATS

  // Accumulate all relations. Some of the previous state variables are quantified.
  for (int row = MAX_ROW(prime::post); MIN_ROW(prime::post) <= row; --row) {
    for (int col = MAX_COL(prime::post); MIN_COL(prime::post) <= col; --col) {
      const cell c(row, col);

      // Constrict with relation for cell 'c'.
      res &= construct_rel(adapter, c);

#ifdef BDD_BENCHMARK_STATS
      std::cout << "   | Rel ( " << c.to_string() << " ) : "
                << adapter.nodecount(res) << " DD nodes\n"
                << std::flush;
#endif // BDD_BENCHMARK_STATS
    }

    // Quantify variables on row+1, if it is the bottom-most two rows.
    //
    // - The bottom-most row of `prime::pre` is only used by the bottom-most row for `prime::post`.
    //   Hence, we can make the decision diagram it smaller by skipping any checks on the last rows
    //   values (and merely store them inside the bottom-most `prime::pre` row instead).
    //
    // - The second bottom-most row with `prime::pre` is only used by the two bottom-most rows for
    //   `prime::post`. If we quantify this row, we decrease the size, as we replace the two
    //   bottom-most rows of `prime::post` checking with said row to just comparing their values.
    const int quant_row = row+1;
    if (MAX_ROW(prime::post) <= quant_row) {
      const time_point t_exists__before = get_timestamp();
      res = adapter.exists(res, [&row](int x) -> bool {
        return (cell::is_prime(x) == prime::pre) && cell(x).row() == row+1;
      });
      const time_point t_exists__after = get_timestamp();

      acc_rel__exists_time += duration_of(t_exists__before, t_exists__after);

#ifdef BDD_BENCHMARK_STATS
      std::cout << "   | Exi ( " << static_cast<char>('A'+row+1) << "_ ) : "
                << adapter.nodecount(res) << " DD nodes\n"
                << std::flush;
#endif // BDD_BENCHMARK_STATS
    }
  }
  const time_point t_apply__after = get_timestamp();

  acc_rel__apply_time = duration_of(t_apply__before, t_apply__after) - acc_rel__exists_time;

  // Quantify all 'prime::pre' variables on all other rows. This will explode before it collapses to
  // `true`.
  const time_point t_exists__before = get_timestamp();
  res = adapter.exists(res, [](int x) -> bool {
    return cell::is_prime(x) == prime::pre;
  });
  const time_point t_exists__after = get_timestamp();

  acc_rel__exists_time += duration_of(t_exists__before, t_exists__after);

#ifdef BDD_BENCHMARK_STATS
  std::cout << "   | Exi ( __ ) : "
            << adapter.nodecount(res) << " DD nodes\n"
            << std::flush;
#endif // BDD_BENCHMARK_STATS

  return res;
}

// ========================================================================== //
template<typename adapter_t>
int run_gameoflife(int argc, char** argv)
{
  no_options option = no_options::NONE;
  bool should_exit = parse_input(argc, argv, option);

  if (input_sizes.size() == 0) { input_sizes.push_back(4); }
  if (input_sizes.size() == 1) { input_sizes.push_back(input_sizes.at(0)); }

  if (should_exit) { return -1; }

  // ======================================================================== //
  std::cout << "Game of Life : [" << rows(prime::post) << " x " << cols(prime::post) << "] "
            << "(" << adapter_t::NAME << " " << M << " MiB):\n";

  const time_point t_init_before = get_timestamp();
  adapter_t adapter(varcount());
  const time_point t_init_after = get_timestamp();

  std::cout << "\n"
            << "   " << adapter_t::NAME << " initialisation:\n"
            << "   | variables             : " << varcount() << "\n"
            << "   | | 'prev'              : " << varcount(prime::pre) << "\n"
            << "   | | 'next'              : " << varcount(prime::post) << "\n"
            << "   |\n"
            << "   | time             (ms) : " << duration_of(t_init_before, t_init_after) << "\n"
            << "\n";

  size_t solutions = 0;
  {
    // ========================================================================
    std::cout << "   Construct reachable states:\n"
              << std::flush;

    auto res = acc_rel(adapter);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "   |\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "   | apply  time      (ms) : " << acc_rel__apply_time << "\n"
              << "   | exists time      (ms) : " << acc_rel__exists_time << "\n"
              << "\n"
              << std::flush;

    // ========================================================================
    std::cout << "   Complement into unreachable states:\n"
              << std::flush;

    const time_point t3 = get_timestamp();
    res = ~res;
    const time_point t4 = get_timestamp();

    const time_duration negation_time = duration_of(t3,t4);

    std::cout << "   | time             (ms) : " << negation_time << "\n"
              << std::flush;

    // ========================================================================
    std::cout << "\n"
              << "   Counting unreachable states:\n"
              << std::flush;

    const time_point t5 = get_timestamp();
    solutions = adapter.satcount(res);
    const time_point t6 = get_timestamp();

    const time_duration counting_time = duration_of(t5,t6);

    std::cout << "   | number of states      : " << solutions << "\n"
              << "   | time             (ms) : " << counting_time << "\n"
              << std::flush;

    // ========================================================================
    const time_duration total_time =
      acc_rel__apply_time + acc_rel__exists_time + negation_time + counting_time;

    std::cout << "\n"
              << "   total time         (ms) : " << total_time << "\n"
              << std::flush;
  }

  adapter.print_stats();

  // For all solvable sizes, the number of solutions should be 0.
  return solutions != 0;
}
