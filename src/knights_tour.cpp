#include "common.cpp"
#include "expected.h"

// ========================================================================== //
//                             Board Indexation                               //

inline int cols()
{ return N / 2; }

inline int MAX_COL()
{ return cols() - 1; }

inline int rows()
{ return N - cols(); }

inline int MAX_ROW()
{ return rows() - 1; }

inline int MAX_TIME()
{ return rows() * cols() - 1; }

inline int int_of_position(int r, int c, int t = 0)
{ return (rows() * cols() * t) + (cols() * r) + c; }

inline int MAX_POSITION()
{ return int_of_position(MAX_ROW(), MAX_COL(), MAX_TIME()); }

inline int row_of_position(int pos)
{ return (pos / cols()) % rows(); }

inline int col_of_position(int pos)
{ return pos % cols(); }

inline std::string pos_to_string(int r, int c)
{
  std::stringstream ss;
  ss << (r+1) << (char) ('A'+c);
  return ss.str();
}

// ========================================================================== //
//                          Closed Tour Constraints                           //
const int closed_squares [3][2] = {{0,0}, {1,2}, {2,1}};

bool is_closed_square(int r, int c)
{
  return (r == closed_squares[0][0] && c == closed_squares[0][1])
    || (r == closed_squares[1][0] && c == closed_squares[1][1])
    || (r == closed_squares[2][0] && c == closed_squares[2][1]);
}

template<typename adapter_t>
typename adapter_t::dd_t knights_tour_closed(adapter_t &adapter);

// ========================================================================== //
//                 Transition Relation + Hamiltonian Constraint               //

constexpr int row_moves[8]    = { -2, -2, -1, -1,  1,  1,  2,  2 };
constexpr int column_moves[8] = { -1,  1, -2,  2, -2,  2, -1,  1 };

bool is_legal_move(int r_from, int c_from, int r_to, int c_to)
{
  for (int idx = 0; idx < 8; idx++) {
    if (r_from + row_moves[idx] == r_to &&
        c_from + column_moves[idx] == c_to) {
      return true;
    }
  }
  return false;
}

bool is_legal_position(int r, int c, int t = 0)
{
  if (r < 0 || MAX_ROW() < r)  { return false; }
  if (c < 0 || MAX_COL() < c)  { return false; }
  if (t < 0 || MAX_TIME() < t) { return false; }

  return true;
}

bool is_reachable(int r, int c)
{
  for (int idx = 0; idx < 8; idx++) {
    if (is_legal_position(r + row_moves[idx], c + column_moves[idx])) {
      return true;
    }
  }
  return false;
}

int next_reachable_position(int r, int c, int t)
{
  int postulate = int_of_position(r,c,t);
  bool reachable = false;

  do {
    postulate++;
    const int row = row_of_position(postulate);
    const int col = col_of_position(postulate);

    reachable = is_reachable(row,col);
  } while (!reachable);

  return postulate;
}

constexpr int no_pos = std::numeric_limits<int>::max();

int first_legal(int r_from, int c_from, int t)
{
  for (int idx = 0; idx < 8; idx++) {
    const int r_to = r_from + row_moves[idx];
    const int c_to = c_from + column_moves[idx];

    if (!is_legal_position(r_to, c_to)) { continue; }

    return int_of_position(r_to, c_to, t);
  }
  return no_pos;
}

int next_legal(int r_from, int c_from, int r_to, int c_to, int t)
{
  bool seen_move = false;

  for (int idx = 0; idx < 8; idx++) {
    const int r = r_from + row_moves[idx];
    const int c = c_from + column_moves[idx];

    if (!is_legal_position(r,c)) { continue; }

    if (seen_move) { return int_of_position(r, c, t); }
    seen_move |= r == r_to && c == c_to;
  }
  return no_pos;
}

// ========================================================================== //
//                                 Options                                    //
enum iter_opt { OPEN, CLOSED };

template<>
std::string option_help_str<iter_opt>()
{ return "Desired Variable ordering"; }

template<>
iter_opt parse_option(const std::string &arg, bool &should_exit)
{
  const std::string lower_arg = ascii_tolower(arg);

  if (lower_arg == "open" || lower_arg == "o")
  { return iter_opt::OPEN; }

  if (lower_arg == "closed" || lower_arg == "c")
  { return iter_opt::CLOSED; }

  std::cerr << "Undefined option: " << arg << "\n";
  should_exit = true;

  return iter_opt::OPEN;
}
