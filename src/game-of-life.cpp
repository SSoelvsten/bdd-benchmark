// Assertions
#include <assert.h>

// Data Structures
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

// Types
#include <cstdlib>

// Other
#include <utility>

#include "common/adapter.h"
#include "common/chrono.h"
#include "common/input.h"

// ============================================================================================== //
//                             PRIMING OF VARIABLES WITH TRANSITIONS                              //

/// \brief   Renaming of Boolean values to something less error-prone.
///
/// \details One quickly forgets whether 'bool prime == true' means before or after the transition.
enum prime : bool { pre = false, post = true };

// ============================================================================================== //

/// \brief Number of rows (depending on primality)
inline int rows(bool p = false)
{ return input_sizes.at(0) + 2*(!p); }

/// \param p Whether the variable is primed.
inline int MIN_ROW(bool p = false)
{ return p; }

/// \param p Whether the variable is primed.
inline int MAX_ROW(bool p = false)
{ return rows(p) - (!p); }

/// \brief Number of columns (depending on primality)
inline int cols(bool p = false)
{ return input_sizes.at(1) + 2*(!p); }

/// \param p Whether the variable is primed.
inline int MIN_COL(bool p = false)
{ return p; }

/// \param p Whether the variable is primed.
inline int MAX_COL(bool p = false)
{ return cols(p) - (!p); }

/// \brief Whether the input size describes a sqaure board.
inline bool is_square()
{ return rows() == cols(); }

// ============================================================================================== //
//                                             OPTION                                             //

/// \brief Enum for choosing the encoding.
enum symmetry {
  /** No symmetry */
  none,
  /** Mirror (vertical) */
  mirror_vertical,
  /** Mirror (diagonal) */
  mirror_diagonal,
  /** Mirror (double-diagonal) */
  mirror_double_diagonal,
  /** Mirror (quadrants) */
  mirror_quadrant,
  /** Rotate (90 degrees) */
  rotate_90,
  /** Rotate (180 degrees) */
  rotate_180,
};

/// \brief Specialization for '--help'
template<>
std::string option_help_str<symmetry>()
{ return "Restriction to solutions with (some) symmetry"; }

/// \brief Specialization for parsing the option value
template<>
symmetry parse_option(const std::string &arg, bool &should_exit)
{
  const std::string lower_arg = ascii_tolower(arg);

  if (lower_arg == "none")
    { return symmetry::none; }
  if (lower_arg == "mirror" || lower_arg == "mirror-vertical")
    { return symmetry::mirror_vertical; }
  if (lower_arg == "mirror-quadrant" || lower_arg == "mirror-quad")
    { return symmetry::mirror_quadrant; }
  if (lower_arg == "mirror-diagonal" || lower_arg == "mirror-diag")
    { return symmetry::mirror_diagonal; }
  if (lower_arg == "mirror-double_diagonal" || lower_arg == "mirror-double_diag")
    { return symmetry::mirror_double_diagonal; }
  if (lower_arg == "rotate" || lower_arg == "rotate-90")
    { return symmetry::rotate_90; }
  if (lower_arg == "rotate-180")
    { return symmetry::rotate_180; }

  std::cerr << "Undefined option: " << arg << "\n";
  should_exit = true;

  return symmetry::none;
}

/// \brief Human-Friendly print
std::string option_str(const symmetry& s)
{
  switch (s) {
  case symmetry::none:
    return "None";
  case symmetry::mirror_vertical:
    return "Mirror (Vertical)";
  case symmetry::mirror_quadrant:
    return "Mirror (Quadrant)";
  case symmetry::mirror_diagonal:
    return "Mirror (Diagonal)";
  case symmetry::mirror_double_diagonal:
    return "Mirror (Double Diagonal)";
  case symmetry::rotate_90:
    return "Rotate 90°";
  case symmetry::rotate_180:
    return "Rotate 180°";
  default:
    return "Unknown";
  }
}

// ============================================================================================== //
//                                              CELLS                                             //

/// \brief A single cell, its coordinate, its DD variable, and its neighbours.
///
/// \details Major parts of this class is copied from `hamiltonian.cpp`.
class cell
{
private:
  /// \brief Row index
  char _row;

  /// \brief Column index
  char _col;

  /// \brief Column index
  bool _prime;

public:
  /// \brief Defalt construction of illegal cell.
  cell()
    : _row(-1), _col(-1), _prime(prime::pre)
  { }

  /// \brief Construction of cell [r,c] with given primality.
  cell(char row, char col, bool prime = prime::pre)
    : _row(row), _col(col), _prime(prime)
  {
    if (this->out_of_range()) {
      throw std::out_of_range("Cell not within valid boundaries");
    }
  }

  /// \brief Copy-construction of another cell, overwriting its primality;
  ///
  /// \remark This does not check whether the resulting cell actually is legal. To do so, please use
  ///         `out_of_range`.
  cell(const cell& c, bool prime)
    : _row(c.row()), _col(c.col()), _prime(prime)
  { }

  /// \brief Obtain the smallest cell (for some primality).
  static cell min(bool p)
  {
    return cell(MIN_ROW(p), MIN_COL(p), p);
  }

  /// \brief Obtain the largest cell (for some primality).
  static cell max(bool p)
  {
    return cell(MAX_ROW(p), MAX_COL(p), p);
  }

public:
  /// \brief Obtain this cell's row.
  char row() const
  { return this->_row; }

  /// \brief Obtain this cell's column.
  char col() const
  { return this->_col; }

  /// \brief Obtain this cell's primality
  bool prime() const
  { return this->_prime; }

public:
  /// \brief   Whether this cell represents an actual valid position depending
  ///          on whether it is primed or not.
  ///
  /// \param p Whether the variable is primed (see `prime`).
  bool out_of_range() const
  {
    return this->row() < MIN_ROW(this->prime()) || MAX_ROW(this->prime()) < this->row()
        || this->col() < MIN_COL(this->prime()) || MAX_COL(this->prime()) < this->col();
  }

  /// \brief Vertical distance between two cells
  size_t vertical_dist_to(const cell &o) const
  { return std::abs(this->row() - o.row()); }

  /// \brief Horizontal distance between two cells
  size_t horizontal_dist_to(const cell &o) const
  { return std::abs(this->col() - o.col()); }

  /// \brief Whether one cell is in the neighbourhood surrounding *this* cell.
  bool in_neighbourhood(const cell &o) const
  {
    return this->vertical_dist_to(o) <= 1 && this->horizontal_dist_to(o) <= 1;
  }

  /// \brief The number of cells in the neighbourhood.
  ///
  /// \see neighbourhood
  int neighbourhood_size() const
  {
    assert(!cell(*this, prime::post).out_of_range());
    return 9;
  }

  /// \brief All unprimed cells that are in the neighbourhood of *this* cell.
  ///
  /// \details The returned list is in ascending row-major order.
  std::vector<cell> neighbourhood() const
  {
    assert(this->prime() == prime::post);

    std::vector<cell> res = {
      cell(this->row()-1, this->col()-1, prime::pre),
      cell(this->row()-1, this->col(),   prime::pre),
      cell(this->row()-1, this->col()+1, prime::pre),
      cell(this->row(),   this->col()-1, prime::pre),
      *this,
      cell(this->row(),   this->col()+1, prime::pre),
      cell(this->row()+1, this->col()-1, prime::pre),
      cell(this->row()+1, this->col(),   prime::pre),
      cell(this->row()+1, this->col()+1, prime::pre)
    };

    for ([[maybe_unused]] const cell& c : res) {
      assert (!c.out_of_range());
    }

    return res;
  }

  /// \brief Whether a cell is a neighbour of *this* cell.
  bool is_neighbour(const cell &o) const
  {
    if (this->row() == o.row() && this->col() == o.col()) {
      return false;
    }
    return this->in_neighbourhood(o);
  }

public:
  /// \brief Human-friendly string
  std::string to_string() const
  {
    const char r = static_cast<char>('0'+this->row());
    const char c = static_cast<char>('A'+this->col()-1);
    const char p = this->prime() == prime::pre ? ' ' : '\'';

    return { r, c, p };
  }

  /// \brief Whether two `cell`s refer to the same coordinate.
  bool operator== (const cell &o) const
  {
    return this->row() == o.row() && this->col() == o.col();
  }

  /// \brief Whether two `cell`s refer to different coordinates.
  bool operator!= (const cell &o) const
  {
    return !(*this == o);
  }

  /// \brief Whether a cell preceedes another in the row-major order
  bool operator< (const cell &o) const
  {
    // Sort first on row
    if (this->row() != o.row()) { return this->row() < o.row(); }

    // Sort secondly on column
    if (this->col() != o.col()) { return this->col() < o.col(); }

    // Finally, sort on primality
    return this->prime() < o.prime();
  }
};

/// \brief Hash function for `cell` class
template<>
struct std::hash<cell>
{
  std::size_t operator()(const cell &c) const
  {
    return std::hash<char>{}(c.row()) ^ std::hash<char>{}(c.col()) ^ c.prime();
  }
};

// ============================================================================================== //
//                                    CELL <-> VARIABLE MAPPING                                   //

/// \brief Container of mapping from `cell` to decision diagram variable.
///
/// \remark If a symmetry is applied, then we group all `prime::pre` variables for that cell
///         together and have it preceede a single variable that is mapped to a single `prime::post`
///         variable (if any).
///
/// \remark All mappings preserve a row-major ordering for the variables. That is, given two cells
///         u and v, if u preceedes v in the row-major order then the same also applies to their
///         decision diagram variables.
class var_map
{
private:
  /// \brief Number of variables (depending on primality)
  size_t _varcount[2] = { 0u, 0u };

  /// \brief Hash-map
  std::unordered_map<cell, int> _map;

  /// \brief Partially inversed map
  std::vector<cell> _inv;

  /// \brief Applied symmetry
  const symmetry _sym;

public:
  /// \brief Initialize decision diagram variables, given some symmetry.
  var_map(const symmetry &s = symmetry::none)
    : _sym(s)
  {
    // TODO: Add private 'insert()' function to decrease code complexity below.
    //
    // TODO: Skip if-guard on '.insert(...)' and resolve it inside the map instead.

    const bool odd_cols = cols(prime::pre) % 2;
    const int  mid_col  = MIN_COL(prime::pre) + cols(prime::pre)/2 - !odd_cols;

    const bool odd_rows = rows(prime::pre) % 2;
    const int  mid_row  = MIN_ROW(prime::pre) + rows(prime::pre)/2 - !odd_rows;

    int x = 0;

    switch (this->_sym) {
    // ---------------------------------------------------------------------------------------------
    // " Every cell has separate value. "
    //                  - Randal E. Bryant
    case symmetry::none: {
      for (int row = MIN_ROW(prime::pre); row <= MAX_ROW(prime::pre); ++row) {
        for (int col = MIN_COL(prime::pre); col <= MAX_COL(prime::pre); ++col) {
          const cell cell_pre(row, col, prime::pre);
          assert(!cell_pre.out_of_range());

          this->_map.insert({ cell_pre, x++ });
          this->_varcount[prime::pre] += 1;

          const cell cell_post(cell_pre, prime::post);
          if (!cell_post.out_of_range()) {
            this->_map.insert({ cell_post, x++ });
            this->_varcount[prime::post] += 1;
          }
        }
      }
      break;
    }
    // ---------------------------------------------------------------------------------------------
    // Reflect left half.
    case symmetry::mirror_vertical: {
      for (int row = MIN_ROW(prime::pre); row <= MAX_ROW(prime::pre); ++row) {
        for (int left_col = MIN_COL(prime::pre); left_col <= mid_col; ++left_col) {
          const int  right_col  = MAX_COL(prime::pre) - left_col;
          const bool add_mirror = mid_col < right_col;

          // pre variable(s)
          const cell pre_left(row, left_col, prime::pre);
          assert(!pre_left.out_of_range());

          this->_map.insert({ pre_left, x++ });
          this->_varcount[prime::pre] += 1;

          const cell pre_right(row, right_col, prime::pre);
          assert(!pre_right.out_of_range());

          if (add_mirror) {
            this->_map.insert({ pre_right, x++ });
            this->_varcount[prime::pre] += 1;
          }

          // post variable
          const cell post_left(pre_left, prime::post);
          if (!post_left.out_of_range()) {
            const int post_var = x++;
            this->_varcount[prime::post] += 1;

            this->_map.insert({ post_left, post_var });

            if (add_mirror) {
              this->_map.insert({ cell(pre_right, prime::post), post_var });
            }
          }
        }
      }
      break;
    }
    // ---------------------------------------------------------------------------------------------
    // Loosely based on source code for a CNF encoding by Marijn Heule.
    //
    // Reflect across a diagonal from top-left to bottom-right.
    case symmetry::mirror_diagonal: {
      if (!is_square()) {
        throw std::invalid_argument("Diagonal symmetry is only available for square grids.");
      }

      for (int row = MIN_ROW(prime::pre); row <= MAX_ROW(prime::pre); ++row) {
        const int max_col = MAX_COL(prime::pre) - (MAX_ROW(prime::pre) - row);

        for (int col = MIN_COL(prime::pre); col <= max_col; ++col) {
          const bool add_mirror = col < row;

          // pre variable(s)
          const cell pre_mirror(col, row, prime::pre);
          assert(!pre_mirror.out_of_range());

          if (add_mirror) {
            this->_map.insert({ pre_mirror, x++ });
            this->_varcount[prime::pre] += 1;
          }

          const cell pre(row, col, prime::pre);
          assert(!pre.out_of_range());

          this->_map.insert({ pre, x++ });
          this->_varcount[prime::pre] += 1;

          // post variable
          const cell post(pre, prime::post);
          if (!post.out_of_range()) {
            const int post_var = x++;
            this->_varcount[prime::post] += 1;

            if (add_mirror) {
              this->_map.insert({ cell(pre_mirror, prime::post), post_var });
            }
            this->_map.insert({ post, post_var });
          }
        }
      }
      break;
    }
    // ---------------------------------------------------------------------------------------------
    // Based on source code for a CNF encoding by Marijn Heule.
    case symmetry::mirror_double_diagonal: {
      if (!is_square()) {
        throw std::invalid_argument("Diagonal symmetry is only available for square grids.");
      }

      for (int row = MIN_ROW(prime::pre); row <= MAX_ROW(prime::pre); ++row) {
        const int max_col = std::min(row, MAX_COL(prime::pre) - row);

        for (int col = 0; col <= max_col; ++col) {
          // The mirrors of the pre cell changes order. So, we use a data structure to sort them.
          std::set<cell> pre_cells;

          // Pre variable(s)
          const int a_row = row;
          const int a_col = col;

          const cell pre_a(a_row, a_col, prime::pre);
          pre_cells.insert(pre_a);

          // mirror 'a' along top-right / bottom-left diagonal
          const int b_row = MAX_ROW(prime::pre) - row;
          const int b_col = MAX_COL(prime::pre) - col;

          pre_cells.insert(cell(b_row, b_col, prime::pre));

          // mirror 'b' along top-left / bottom-right diagonal
          const int c_row = b_col;
          const int c_col = b_row;

          pre_cells.insert(cell(c_row, c_col, prime::pre));

          // flip along top-left -> bottom-right diagonal
          const int d_row = col;
          const int d_col = row;

          pre_cells.insert(cell(d_row, d_col, prime::pre));

          assert(pre_cells.size() > 0);
          for (const cell &c : pre_cells) {
            this->_map.insert({ c, x++ });
            this->_varcount[prime::pre] += 1;
          }

          // Post variable
          if (!cell(pre_a, prime::post).out_of_range()) {
            const int post_var = x++;
            this->_varcount[prime::post] += 1;

            for (const cell &c : pre_cells) {
              this->_map.insert({ cell(c, prime::post), post_var });
            }
          }
        }
      }
      break;
    }
    // ---------------------------------------------------------------------------------------------
    // Based on source code for a CNF encoding by Marijn Heule.
    //
    // " Reflect single quadrant to mirror in X and Y. "
    //                  - Randal E. Bryant
    case symmetry::mirror_quadrant: {
      for (int top_row = mid_row; MIN_ROW(prime::pre) <= top_row; --top_row) {
        for (int left_col = mid_col; MIN_COL(prime::pre) <= left_col; --left_col) {
          const int right_col = MAX_COL(prime::pre) - left_col;
          const int bot_row   = MAX_ROW(prime::pre) - top_row;

          const bool mirror_horizontal = mid_row < bot_row;
          const bool mirror_vertical   = mid_col < right_col;

          // pre variable(s)
          const cell pre(top_row, left_col, prime::pre);
          assert(!pre.out_of_range());

          this->_map.insert({ pre, x++ });
          this->_varcount[prime::pre] += 1;

          if (mirror_vertical) {
            const cell c(top_row, right_col, prime::pre);
            assert(!c.out_of_range());

            this->_map.insert({ c, x++ });
            this->_varcount[prime::pre] += 1;
          }
          if (mirror_horizontal) {
            const cell c(bot_row, left_col, prime::pre);
            assert(!c.out_of_range());

            this->_map.insert({ c, x++ });
            this->_varcount[prime::pre] += 1;
          }
          if (mirror_horizontal && mirror_vertical) {
            const cell c(bot_row, right_col, prime::pre);
            assert(!c.out_of_range());

            this->_map.insert({ c, x++ });
            this->_varcount[prime::pre] += 1;
          }

          // post variable
          const cell post(pre, prime::post);
          if (!post.out_of_range()) {
            const int post_var = x++;
            this->_varcount[prime::post] += 1;

            this->_map.insert({ post, post_var });

            if (mirror_vertical) {
              const cell c(top_row, right_col, prime::post);
              this->_map.insert({ c, post_var });
            }
            if (mirror_horizontal) {
              const cell c(bot_row, left_col, prime::post);
              this->_map.insert({ c, post_var });
            }
            if (mirror_horizontal && mirror_vertical) {
              const cell c(bot_row, right_col, prime::post);
              this->_map.insert({ c, post_var });
            }
          }
        }
      }
      break;
    }
    // ---------------------------------------------------------------------------------------------
    // Based on source code for a CNF encoding by Marijn Heule.
    //
    // " Reflect single quadrant to all 4 rotations "
    //                  - Randal E. Bryant
    case symmetry::rotate_90: {
      if (!is_square()) {
        throw std::invalid_argument("Rotational symmetry (90 degrees) is only available for square grids.");
      }

      for (int tl_row = mid_row; MIN_ROW(prime::pre) <= tl_row; --tl_row) {
        for (int tl_col = mid_col; MIN_COL(prime::pre) <= tl_col; --tl_col) {
          // pre variable(s)
          const cell pre_tl(tl_row, tl_col, prime::pre);
          assert(!pre_tl.out_of_range());

          this->_map.insert({ pre_tl, x++ });
          this->_varcount[prime::pre] += 1;

          const int tr_row = tl_col;
          const int tr_col = MAX_COL(prime::pre) - tl_row;

          const cell pre_tr(tr_row, tr_col, prime::pre);
          assert(!pre_tr.out_of_range());

          const bool add_tr = tr_row <= mid_row && mid_col < tr_col;
          if (add_tr) {
            this->_map.insert({ pre_tr, x++ });
            this->_varcount[prime::pre] += 1;
          }

          const int bl_row = MAX_ROW(prime::pre) - tl_col;
          const int bl_col = tl_row;

          const cell pre_bl(bl_row, bl_col, prime::pre);
          assert(!pre_bl.out_of_range());

          const bool add_bl = mid_row < bl_row && bl_col <= mid_col;
          if (add_bl) {
            this->_map.insert({ pre_bl, x++ });
            this->_varcount[prime::pre] += 1;
          }

          const int br_row = MAX_ROW(prime::pre) - tl_row;
          const int br_col = MAX_COL(prime::pre) - tl_col;

          const cell pre_br(br_row, br_col, prime::pre);
          assert(!pre_br.out_of_range());

          const bool add_br = mid_row < br_row && mid_col < br_col;
          if (add_br) {
            this->_map.insert({ pre_br, x++ });
            this->_varcount[prime::pre] += 1;
          }

          // post variable
          if (!cell(pre_tl, prime::post).out_of_range()) {
            const int post_var = x++;
            this->_varcount[prime::post] += 1;

            this->_map.insert({ cell(pre_tl, prime::post), post_var });

            if (add_tr) {
              this->_map.insert({ cell(pre_tr, prime::post), post_var });
            }
            if (add_bl) {
              this->_map.insert({ cell(pre_bl, prime::post), post_var });
            }
            if (add_br) {
              this->_map.insert({ cell(pre_br, prime::post), post_var });
            }
          }
        }
      }
      break;
    }
    // ---------------------------------------------------------------------------------------------
    // Loosely based on source code for a CNF encoding by Marijn Heule.
    //
    // Reflect top half by rotating by 180 degrees.
    case symmetry::rotate_180: {
      for (int top_row = mid_row; MIN_ROW(prime::pre) <= top_row; --top_row) {
        for (int top_col = MIN_COL(prime::pre); top_col <= MAX_COL(prime::pre); ++top_col) {
          const int bot_row = MAX_ROW(prime::pre) - top_row;
          const int bot_col = MAX_COL(prime::pre) - top_col;

          const bool add_bot = top_row < bot_row;

          // pre variable(s)
          const cell pre_top(top_row, top_col, prime::pre);
          assert(!pre_top.out_of_range());

          this->_map.insert({ pre_top, x++ });
          this->_varcount[prime::pre] += 1;

          const cell pre_bot(bot_row, bot_col, prime::pre);
          assert(!pre_top.out_of_range());

          if (add_bot) {
            this->_map.insert({ pre_bot, x++ });
            this->_varcount[prime::pre] += 1;
          }

          const cell post_top(pre_top, prime::post);
          if (!post_top.out_of_range()) {
            const int post_var = x++;
            this->_varcount[prime::post] += 1;

            this->_map.insert({ post_top, post_var });

            if (add_bot) {
              this->_map.insert({ cell(pre_bot, prime::post), post_var });
            }
          }
        }
      }
      break;
    }
    }

    // Check there was no sparsity introduced
    assert(this->varcount() == x);

    // Check all mappings truly are in the map
    assert(this->size() == this->varcount());

    // Check 'prime::pre' variables have not been merged.
    assert(this->varcount(prime::pre) == rows(prime::pre) * cols(prime::pre));

    // Inverse mapping
    this->_inv.resize(this->varcount());
    for (const auto &kv : this->_map) {
      this->_inv.at(kv.second) = kv.first;
    }
  }

  /// \brief Obtain variable for a cell.
  int var_from_cell(const cell &c) const
  {
    if (c.out_of_range()) {
      throw std::out_of_range("Cell not within valid boundaries");
    }

    const auto res = this->_map.find(c);

    if (res == this->_map.end()) {
      throw std::out_of_range("Cell not found in 'cell -> var' map");
    }
    return res->second;
  }

  /// \brief Obtain variable for a cell.
  int operator[] (const cell& c) const
  {
    return this->var_from_cell(c);
  }

  /// \brief Obtain a cell corresponding to a variable.
  ///
  /// \remark If symmetries are in use and `x` is a `prime::post` variable, then this does **not**
  ///         account for any variable mapping collisions.
  cell cell_from_var(const int x) const
  {
    assert(0 <= x && x < this->size());
    return this->_inv.at(x);
  }

  /// \brief Obtain variable for a cell.
  ///
  /// \remark If symmetries are in use and `x` is a `prime::post` variable, then this does **not**
  ///         account for any variable mapping collisions.
  cell operator[] (const int& x) const
  {
    return this->cell_from_var(x);
  }

  /// \brief Obtain a cell corresponding to a variable with `c` as a candidate.
  ///
  /// \remark If `c` maps to `x` with the given symmetry, then `c` will be returned instead of the
  ///         result from `cell_from_var(x)`.
  cell cell_from_var(const int x, const cell &c) const
  {
    const int c_x = this->var_from_cell(c);
    if (c_x == x) { return c; }

    return this->cell_from_var(x);
  }

  /// \brief Number of variables (with a given primality)
  int varcount(bool p) const
  {
    return this->_varcount[p];
  }

  /// \brief Number of variables (with either primality)
  int varcount() const
  {
    return this->varcount(false) + this->varcount(true);
  }

  /// \brief Number of mappings (with either primality)
  int size() const
  {
    return _map.size();
  }

  /// \brief Get symmetry for variable ordering.
  symmetry sym() const
  {
    return _sym;
  }

  /// \brief Whether a cell is symmetric to a cell that is on the given row.
  ///
  /// \remark For some orderings, this function returns `false` even if it actually is symmetric.
  ///         That is, this is (at this point) an under approximation of this fact.
  bool row_symmetric(const cell& c, const int row) const
  {
    if (c.row() == row) {
      return true;
    }

    const int row_flipped = MAX_ROW(prime::post) - c.row();

    switch (this->_sym) {
    case symmetry::none:
    case symmetry::mirror_vertical: {
      // Already covered by initial check on row.
      return false;
    }
    case symmetry::mirror_diagonal: {
      // If cell is above diagonal, also check its column.
      return c.row() < c.col() && c.col() == row;
    }
    case symmetry::mirror_double_diagonal:
    case symmetry::rotate_90: {
      // Since everything is mirrored twice, the entire outer border is symmetric and so on...
      return c.col() == row || c.row() == row_flipped || c.col() == row_flipped;
    }
    case symmetry::mirror_quadrant:
    case symmetry::rotate_180: {
      // Check row inverted vertically
      return row_flipped == row;
    }
    default: {
      return false; // <-- squelches the compiler
    }
    }
  }

public:
  std::string to_string()
  {
    std::stringstream o;

    for (int row = MIN_ROW(prime::pre); row <= MAX_ROW(prime::pre); ++row) {
      for (int col = MIN_COL(prime::pre); col <= MAX_COL(prime::pre); ++col) {
        const cell cell_pre(row, col, prime::pre);

        o << cell_pre.to_string() << " -> " << this->var_from_cell(cell_pre) << '\n';

        const cell cell_post(cell_pre, prime::post);
        if (!cell_post.out_of_range()) {
          o << cell_post.to_string() << " -> " << this->var_from_cell(cell_post) << '\n';
        }
      }
    }

    return o.str();
  }
};

// ============================================================================================== //
//                             TRANSITION RELATION + GARDEN OF EDEN                               //
//
// " To avoid decisions and branches in the counting loop, the rules can be rearranged from an
//   egocentric approach of the inner field regarding its neighbours to a scientific observer's
//   viewpoint: if the sum of all nine fields in a given neighbourhood is three, the inner field
//   state for the next generation will be life; if the all-field sum is four, the inner field
//   retains its current state; and every other sum sets the inner field to death. "
//
//                         - [Wikipedia 'https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life']

/// \brief Accumulated time for all apply (and negation) operations
time_duration goe__apply_time  = 0;

/// \brief Accumulated time for all existential quantification operations
time_duration goe__exists_time = 0;

/// \brief Decision Diagram that is `true` if exactly `alive` neighbour cells around `c` (including
///        itself) are alive at the 'unprimed' time.
///
/// \details Major parts of this is copied from `tic-tac-toe.cpp`.
template<typename Adapter>
typename Adapter::dd_t
construct_count(Adapter &adapter, const var_map &vm, const cell &c, const int alive)
{
  assert(0 <= alive);

  std::vector<typename Adapter::build_node_t> init_parts(alive+2, adapter.build_node(false));
  init_parts.at(alive) = adapter.build_node(true);

  int remaining_cells = c.neighbourhood_size() + 1;

  if (alive > remaining_cells) {
    return adapter.bot();
  }

  int alive_max = alive;
  int alive_min = alive_max;

  for (int x = vm.varcount()-1; 0 <= x; --x) {
    const cell curr_cell = vm[x];

    if (curr_cell.prime() == prime::pre && c.in_neighbourhood(curr_cell)) {
      remaining_cells -= 1;

      // Open up for one fewer cell is alive (if not already all prior already could be dead).
      alive_min = std::max(alive_min-1, 0);
      assert(0 <= alive_min);

      // Decrease 'alive_max' if too few variables above could have same number
      // of true variables.
      alive_max = 0 < remaining_cells && remaining_cells == alive_max
        ? alive_max - 1
        : alive_max;

      assert(0 <= alive_max);

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
template<typename Adapter>
typename Adapter::dd_t
construct_eq(Adapter &adapter, const var_map &vm, const cell &c)
{
  const int x_pre  = vm[cell(c, prime::pre)];
  const int x_post = vm[cell(c, prime::post)];
  assert(x_pre < x_post);

  auto root0 = adapter.build_node(true);

  int x = vm.varcount()-1;

  // below 'x_post'
  for (; x_post < x; --x) {
    root0 = adapter.build_node(x, root0, root0);
  }

  // 'x_post'
  assert(x_post == x);

  auto root1 = adapter.build_node(x, adapter.build_node(false), root0);
  root0 = adapter.build_node(x, root0, adapter.build_node(false));

  // between 'x_pre' and 'x_post'
  x -= 1;
  for (; x_pre < x; --x) {
    root1 = adapter.build_node(x, root1, root1);
    root0 = adapter.build_node(x, root0, root0);
  }

  // 'x_pre'
  assert(x_pre == x);

  root0 = adapter.build_node(x, root0, std::move(root1));

  // above 'x_pre'
  x -= 1;

  for (; 0 <= x; --x) {
    root0 = adapter.build_node(x, root0, root0);
  }

  return adapter.build();
}

/// \brief Combine decision diagrams together into transition relation for a single cell.
template<typename Adapter>
typename Adapter::dd_t acc_rel(Adapter &adapter, const var_map &vm, const cell &c)
{
  const int c_post_var = vm[cell(c, prime::post)];

  const typename Adapter::dd_t alive_3 = construct_count(adapter, vm, c, 3);
  const typename Adapter::dd_t alive_4 = construct_count(adapter, vm, c, 4);

  typename Adapter::dd_t out;

  { // -----------------------------------------------------------------------------------------------
    // - If the sum is 3, the inner cell will become alive.
    const typename Adapter::dd_t alive_post = adapter.ithvar(c_post_var);

    out = adapter.apply_imp(alive_3, std::move(alive_post));
  }
  { // -----------------------------------------------------------------------------------------------
    // - If the sum is 4, the inner field retains its state.
    const typename Adapter::dd_t eq = construct_eq(adapter, vm, c);

    out &= adapter.apply_imp(alive_4, std::move(eq));
  }
  { // -----------------------------------------------------------------------------------------------
    // - Otherwise, the inner field is dead.
    const typename Adapter::dd_t alive_other = ~(std::move(alive_3) | std::move(alive_4));
    const typename Adapter::dd_t dead_post   = adapter.nithvar(c_post_var);

    out &= adapter.apply_imp(std::move(alive_other), std::move(dead_post));
  }

  return out;
}

/// \brief Combine decision diagrams together into transition relation for an entire row.
template<typename Adapter>
typename Adapter::dd_t acc_rel(Adapter &adapter, const var_map &vm, const int row)
{
  auto res = adapter.top();

#ifdef BDD_BENCHMARK_STATS
  std::cout << "  | | Rel " << row << "\n"
            << "  | | | --                    "
            << adapter.nodecount(res) << "\n"
            << std::flush;
#endif // BDD_BENCHMARK_STATS

  const time_point t_apply__before = now();

  for (int col = MAX_COL(prime::post); MIN_COL(prime::post) <= col; --col) {
    const cell c(row, col);

    // Constrict with relation for cell 'c'.
    res &= acc_rel(adapter, vm, c);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "  | | | " << c.to_string() << "                   "
              << adapter.nodecount(res) << "\n"
              << std::flush;
#endif // BDD_BENCHMARK_STATS
  }

  const time_point t_apply__after  = now();
  goe__apply_time += duration_ms(t_apply__before, t_apply__after);

  return res;
}

/// \brief Combine decision diagrams together into transition relation for top/bottom half of grid.
template<typename Adapter>
typename Adapter::dd_t acc_rel(Adapter &adapter, const var_map &vm, const bool bottom)
{
  // TODO (symmetry::none): Use Manual Variable Reordering to only compute a row once.

  const int half_rows = rows(prime::post) / 2;

  const int top_begin = MIN_ROW(prime::post);
  const int top_end   = top_begin + half_rows - 1;

  const int bot_begin = MAX_ROW(prime::post);
  const int bot_end   = bot_begin - half_rows + 1;

  const int begin     = bottom ? bot_begin : top_begin;
  const int end       = bottom ? bot_end   : top_end;

  auto res = adapter.top();

  for (int row = begin; bottom ? end <= row : row <= end; bottom ? --row : ++row) {
    // ---------------------------------------------------------------------------------------------
    const auto row_rel = acc_rel(adapter, vm, row);

    const time_point t_apply__before = now();
    res &= std::move(row_rel);
    const time_point t_apply__after  = now();
    goe__apply_time += duration_ms(t_apply__before, t_apply__after);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "  | |\n"
              << "  | | Acc [" << begin << "-" << row << "]               " << adapter.nodecount(res) << "\n" << std::flush;
#endif // BDD_BENCHMARK_STATS

    // ---------------------------------------------------------------------------------------------
    // NOTE: Since all transition relations are very local, the complexity of the problem is hidden
    //       within the quantification. Hence, the decision diagram explodes during this operation.
    //       The exception is, that we can quantify the top-most and two bottom-most rows early.
    //
    //       - The top-most, resp. bottom-most, row of `prime::pre` is only used by the top-most,
    //         resp. bottom-most, row for `prime::post`. Hence, we can make the decision diagram it
    //         smaller by skipping any checks on the last rows values (and merely store them inside
    //         the bottom-most `prime::pre` row instead).
    //
    //       For the bottom, the following also applies:
    //
    //       - The second bottom-most row with `prime::pre` is only used by the two bottom-most rows
    //         for `prime::post`. If we quantify this row, we decrease the size, as we replace the
    //         two bottom-most `prime::post` rows check with said `prime::pre` with them just
    //         comparing their values.
    const int quant_row = row + (bottom ? +1 : -1);

    if (bottom ? begin <= quant_row : quant_row < begin) {
      const time_point t_exists__before = now();
      res = adapter.exists(res, [&quant_row, &vm](int x) -> bool {
        return vm[x].prime() == prime::pre && vm[x].row() == quant_row;
      });
      const time_point t_exists__after = now();

      goe__exists_time += duration_ms(t_exists__before, t_exists__after);

#ifdef BDD_BENCHMARK_STATS
      std::cout << "  | | Exi [" << quant_row << "]                 "
                << adapter.nodecount(res) << "\n"
                << std::flush;
#endif // BDD_BENCHMARK_STATS
    }

    if (row != end) {
#ifdef BDD_BENCHMARK_STATS
      std::cout << "  | |\n";
#endif // BDD_BENCHMARK_STATS
    }
  }

  return res;
}

/// \brief Combine decision diagrams together into transition relation for entire grid and then
///        quantify out all previous-state variables.
template<typename Adapter>
typename Adapter::dd_t garden_of_eden(Adapter &adapter, const var_map &vm)
{
  if (rows() < cols()) {
    std::cout << "  | Note:\n"
              << "  |   The variable ordering is designed for 'cols <= rows'.\n"
              << "  |   Maybe restart with the dimensions flipped?\n"
              << "  |\n";
  }

  // -----------------------------------------------------------------------------------------------
  // Top half
#ifdef BDD_BENCHMARK_STATS
  std::cout << "  | Top Half\n";
#endif // BDD_BENCHMARK_STATS
  auto res = acc_rel(adapter, vm, false);

  // -----------------------------------------------------------------------------------------------
  // Bottom half
#ifdef BDD_BENCHMARK_STATS
  std::cout << "  |\n"
            << "  | Bottom Half\n";
#endif // BDD_BENCHMARK_STATS
  res &= acc_rel(adapter, vm, true);

  // -----------------------------------------------------------------------------------------------
  // Middle row between halfs (if any)
  if (rows(prime::post) % 2 == 1) {
#ifdef BDD_BENCHMARK_STATS
    std::cout << "  |\n"
              << "  | Middle Row\n";
#endif // BDD_BENCHMARK_STATS
    res &= acc_rel(adapter, vm, rows(prime::post) / 2 + 1);
  }

  // -----------------------------------------------------------------------------------------------
  // Top half
  //
  // TODO (symmetry::none): Use Manual Variable Reordering to obtain Top Half from Bottom Half
#ifdef BDD_BENCHMARK_STATS
  std::cout << "  |\n"
            << "  | Acc [" << MIN_ROW(prime::pre) << "-" << MAX_ROW(prime::pre) << "]                 "
            << adapter.nodecount(res) << "\n"
            << std::flush;
#endif // BDD_BENCHMARK_STATS

  // -----------------------------------------------------------------------------------------------
  // Quantify all pre-variables that are symmetric to an 'easy' row.
  {
    const time_point t_exists__before = now();
    res = adapter.exists(res, [&vm](int x) -> bool {
      return vm[x].prime() == prime::pre
        && (vm.row_symmetric(vm[x], MIN_ROW(prime::pre))
            || vm.row_symmetric(vm[x], MAX_ROW(prime::post))
            || vm.row_symmetric(vm[x], MAX_ROW(prime::pre)));
    });
    const time_point t_exists__after = now();

    goe__exists_time += duration_ms(t_exists__before, t_exists__after);
  }

#ifdef BDD_BENCHMARK_STATS
  std::cout << "  |\n"
            << "  | Exi [EASY]                "
            << adapter.nodecount(res) << "\n"
            << std::flush;
#endif // BDD_BENCHMARK_STATS

  // -----------------------------------------------------------------------------------------------
  // Quantify all remaining 'prime::pre' variables. This will explode and then collapses to `top`.
  {
    const time_point t_exists__before = now();
    res = adapter.exists(res, [&vm](int x) -> bool {
      return vm[x].prime() == prime::pre;
    });
    const time_point t_exists__after = now();

    goe__exists_time += duration_ms(t_exists__before, t_exists__after);
  }

#ifdef BDD_BENCHMARK_STATS
  std::cout << "  |\n"
            << "  | Exi [HARD]                "
            << adapter.nodecount(res) << "\n"
            << std::flush;
#endif // BDD_BENCHMARK_STATS

  // -----------------------------------------------------------------------------------------------
  return res;
}


// ============================================================================================== //
//                                             EXPECTED                                           //
//
// For all solvable sizes, we expect to find 'no solution exists'. That is, we expect ALL initial
// states to have at least one predecessor. That is, the above `garden_of_eden(...)` function should
// return a decision diagram that is true for all assignments to the `prime::post` variables.

/// \brief Decision Diagram that is `true` for any assignment to `prime::post` variables.
template<typename Adapter>
typename Adapter::dd_t
construct_post(Adapter &adapter, const var_map &vm)
{
  auto root = adapter.build_node(true);

  for (int x = vm.varcount()-1; 0 <= x; --x) {
    if (vm[x].prime() != prime::post) { continue; }

    root = adapter.build_node(x, root, root);
  }

  return adapter.build();
}

// ============================================================================================== //
template<typename Adapter>
int run_gameoflife(int argc, char** argv)
{
  symmetry option = symmetry::none;
  bool should_exit = parse_input(argc, argv, option);

  if (input_sizes.size() == 0) { input_sizes.push_back(4); }
  if (input_sizes.size() == 1) { input_sizes.push_back(input_sizes.at(0)); }

  if (should_exit) { return -1; }

  // -----------------------------------------------------------------------------------------------
  std::cout << "Game of Life : [" << rows(prime::post) << " x " << cols(prime::post) << "]\n"
            << "  | Symmetry                  " << option_str(option) << "\n"
            << "\n";

  var_map vm(option);

  return run<Adapter>(vm.varcount(), [&](Adapter &adapter) {
    std::cout << "  | | 'prev'                  " << vm.varcount(prime::pre) << "\n"
              << "  | | 'next'                  " << vm.varcount(prime::post) << "\n"
              << "\n";

    size_t solutions = 0;

    // ---------------------------------------------------------------------------------------------
    std::cout << "  Construct reachable initial states\n"
              << std::flush;

    const time_point t1 = now();
    auto res = garden_of_eden(adapter, vm);
    const time_point t2 = now();

#ifdef BDD_BENCHMARK_STATS
    std::cout << "  |\n";
#endif // BDD_BENCHMARK_STATS
    std::cout << "  | time (ms)                 " << duration_ms(t1,t2) << "\n"
              << "  | | apply                   " << goe__apply_time << "\n"
              << "  | | exists                  " << goe__exists_time << "\n"
              << "\n"
              << std::flush;

    // ---------------------------------------------------------------------------------------------
    std::cout << "  Obtaining unreachable states\n"
              << std::flush;

    const time_point t3 = now();
    const auto post_top = construct_post(adapter, vm);
    res = adapter.apply_diff(post_top, res);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "  | Top                       " << adapter.nodecount(post_top) << "\n"
              << "  | Top - States              " << adapter.nodecount(res) << "\n"
              << "  |\n"
              << std::flush;
#endif // BDD_BENCHMARK_STATS
    const time_point t4 = now();

    const time_duration flip_time = duration_ms(t3,t4);

    std::cout << "  | time (ms)                 " << flip_time << "\n"
              << "\n"
              << std::flush;

    // ---------------------------------------------------------------------------------------------
    std::cout << "  Counting unreachable states\n"
              << std::flush;

    const time_point t5 = now();
    solutions = adapter.satcount(res, vm.varcount(prime::post));
    const time_point t6 = now();

    const time_duration counting_time = duration_ms(t5,t6);

    std::cout << "  | number of states          " << solutions << "\n"
              << "  | time (ms)                 " << counting_time << "\n"
              << "\n"
              << std::flush;

    // ---------------------------------------------------------------------------------------------
    const time_duration total_time =
      goe__apply_time + goe__exists_time + flip_time + counting_time;

    std::cout << "  total time (ms)             " << total_time << "\n"
              << std::flush;

    // For all solvable sizes, the number of solutions should be 0.
    return solutions != 0;
  });
}
