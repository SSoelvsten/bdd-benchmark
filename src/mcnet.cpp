// Algorithms and Operations
#include <algorithm>

// Assertions
#include <cassert>

// Data Structures
#include <array>
#include <map>
#include <queue>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

// Files
#include <filesystem>
#include <fstream>
#include <istream>

// Types
#include <cstdlib>
#include <type_traits>

// Other
#include <functional>
#include <random>
#include <stdexcept>
#include <utility>

// XML Parser
#include <pugixml.hpp>

// Common
#include "common/adapter.h"
#include "common/chrono.h"
#include "common/input.h"

// Boost
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/cuthill_mckee_ordering.hpp>
#include <boost/graph/sloan_ordering.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// PARAMETER PARSING
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Path to input file.
////////////////////////////////////////////////////////////////////////////////////////////////////
std::string path = "";

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Supported analysis algorithms.
////////////////////////////////////////////////////////////////////////////////////////////////////
enum analysis : char
{
  /** Identify (reachable?) deadlock states */
  DEADLOCK = 0,
  /** Identify reachable states */
  REACHABILITY = 1,
  /** Compute the set of Strongly Connected Components (SCCs) */
  SCC = 2
};

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string
to_string(const analysis& a)
{
  switch (a) {
  case analysis::DEADLOCK: return "deadlock";
  case analysis::REACHABILITY: return "reachability";
  case analysis::SCC: return "SCC";
  }
  return "?";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Array with On/Off switches for each analysis.
////////////////////////////////////////////////////////////////////////////////////////////////////
std::array<bool, 3> analysis_flags = { false, false, false };

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Supported variable orderings.
////////////////////////////////////////////////////////////////////////////////////////////////////
enum class variable_order : char
{
  /** The Cuthill-Mckee algorithm to reduce Bandwidth */
  CUTHILL_MCKEE,
  /** Use declaration order in file. */
  INPUT,
  /** Permute order randomly. */
  RANDOM,
  /** Sloan's algorithm to reduce Bandwidth */
  SLOAN
};

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string
to_string(const variable_order& vo)
{
  switch (vo) {
  case variable_order::CUTHILL_MCKEE: return "cuthill-mckee";
  case variable_order::INPUT: return "input";
  case variable_order::RANDOM: return "random";
  case variable_order::SLOAN: return "sloan";
  }
  return "?";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Chosen variable ordering.
////////////////////////////////////////////////////////////////////////////////////////////////////
variable_order var_order = variable_order::INPUT;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Whether to use Synchronous Update Semantics (asynchronous, otherwise).
////////////////////////////////////////////////////////////////////////////////////////////////////
bool synchronous_update = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
class parsing_policy
{
public:
  static constexpr std::string_view name = "McNet";
  static constexpr std::string_view args = "a:f:o:s";

  static constexpr std::string_view help_text =
    "        -f PATH              Path to file containing a model\n"
    "        -a ALGO     [reach]  Analyses to run on the net\n"
    "        -o ORDER    [input]  Variable Order to derive from the model\n"
    "        -s                   If set, interprets the model with synchronous updates";

  static inline bool
  parse_input(const int c, const char* arg)
  {
    switch (c) {
    case 'a': {
      const std::string lower_arg = ascii_tolower(arg);

      if (is_prefix(lower_arg, "deadlock")) {
        analysis_flags[analysis::DEADLOCK] = true;
      } else if (is_prefix(lower_arg, "reachability") || is_prefix(lower_arg, "reachable")) {
        analysis_flags[analysis::REACHABILITY] = true;
      } else if (is_prefix(lower_arg, "scc")) {
        analysis_flags[analysis::REACHABILITY] = true;
        analysis_flags[analysis::SCC]          = true;
      } else {
        std::cerr << "Undefined analysis: " << arg << "\n";
        return true;
      }
      return false;
    }
    case 'f': {
      if (!std::filesystem::exists(arg)) {
        std::cerr << "File '" << arg << "' does not exist\n";
        return true;
      }
      path = arg;
      return false;
    }
    case 'o': {
      const std::string lower_arg = ascii_tolower(arg);

      if (is_prefix(lower_arg, "cuthill-mckee")) {
        var_order = variable_order::CUTHILL_MCKEE;
      } else if (is_prefix(lower_arg, "input")) {
        var_order = variable_order::INPUT;
      } else if (is_prefix(lower_arg, "random")) {
        var_order = variable_order::RANDOM;
      } else if (is_prefix(lower_arg, "sloan")) {
        var_order = variable_order::SLOAN;
      } else {
        std::cerr << "Undefined ordering: " << arg << "\n";
        return true;
      }
      return false;
    }
    case 's': {
      synchronous_update = true;
      return false;
    }
    default: return true;
    }
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// TRANSITION SYSTEM PARSING
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overload : Ts...
{
  using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Generic Transition System.
////////////////////////////////////////////////////////////////////////////////////////////////////
class transition_system
{
public:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Container for a Boolean formula (in Reverse Polish Notation).
  ///
  /// \details The insertion operations below are the independent steps of Dijkstra's "Shunting Yard
  ///          Algorithm". For more details, see:
  ///          https://en.wikipedia.org/wiki/Shunting_yard_algorithm
  //////////////////////////////////////////////////////////////////////////////////////////////////
  class bool_exp
  {
  public:
    /// \brief Unary operator to be applied.
    ///
    /// \details Values reflect order of precedence.
    enum unary_operator : char
    {
      Not = 4
    };

    /// \brief Binary operator to be applied.
    ///
    /// \details Values reflect order of precedence.
    enum binary_operator : char
    {
      Or  = 0,
      And = 1,
      Xor = 2,
      Eq  = 3
    };

    /// \brief Binary operator to be applied.
    ///
    /// \details Values reflect order of precedence.
    enum parenthesis : char
    {
      LParen = 5,
      RParen = 6
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    using value_type = std::variant<bool, int, unary_operator, binary_operator>;

  private:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Formula in Reverse polish notation.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<value_type> _rpn_stack;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    using operand_type = std::variant<unary_operator, binary_operator, parenthesis>;

    static value_type
    operand_to_value(const operand_type x)
    {
      return std::visit(overload{ [](const parenthesis /**/) -> value_type {
                                   throw std::runtime_error("Unable to convert parenthesis");
                                 },
                                  [](const auto x) -> value_type { return x; } },
                        x);
    }

    static char
    precedence(const operand_type oper)
    {
      return std::visit([](const auto o) -> char { return static_cast<char>(o); }, oper);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief To-be done
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<operand_type> _op_stack;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Whether the formula is a constant formula.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool _is_const = true;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Whether the formula is guaranteed to be cubic.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool _is_cubic = true;

  private:
    void inline move_op_top_to_stack()
    {
      const value_type v = this->operand_to_value(this->_op_stack.back());
      this->_op_stack.pop_back();

      const bool double_negation = v == this->_rpn_stack.back() && v == value_type{ Not };
      if (double_negation) {
        this->_rpn_stack.pop_back();
        return;
      }
      this->_rpn_stack.push_back(v);
    }

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Push Boolean constant to stack.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool_exp&
    push(const bool value)
    {
      this->_rpn_stack.push_back(value);

      return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Push input variable to stack.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool_exp&
    push(const int var)
    {
      this->_is_const = false;

      this->_rpn_stack.push_back(var);

      return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Push unary operation of (infix) expression.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool_exp&
    push(const unary_operator op)
    {
      // Cancel previous '!' operator.
      if (!this->_op_stack.empty() && this->_op_stack.back() == operand_type{ Not }) {
        this->_op_stack.pop_back();
      }

      // '!' operation has highest precedence.
      //
      // So unlike below, no checks on operator stack are necessary.
      this->_op_stack.push_back(op);

      return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Push binary operation of (infix) expression.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool_exp&
    push(const binary_operator op)
    {
      this->_is_cubic &= op == And;

      while (!this->_op_stack.empty()) {
        // Stop, if top of operator stack is a parenthesis.
        assert(this->_op_stack.back() != operand_type{ RParen });
        if (this->_op_stack.back() == operand_type{ LParen }) { break; }

        // Stop, if top of operator stack has lower precedence.
        if (precedence(this->_op_stack.back()) < precedence(op)) { break; }

        // Move operator with same or higher precedence to RPN stack.
        this->move_op_top_to_stack();
      }

      // Push operator to stack.
      this->_op_stack.push_back(op);

      return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Push parenthesis of (infix) expression.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool_exp&
    push(const parenthesis paren)
    {
      switch (paren) {
      case LParen: {
        this->_is_cubic &= this->_op_stack.empty() || this->_op_stack.back() != operand_type{ Not };
        this->_op_stack.push_back(paren);
        break;
      }
      case RParen:
      default: {
        assert(!this->_op_stack.empty());
        while (this->_op_stack.back() != operand_type{ LParen }) {
          // Move operator in just-closed scope to RPN stack.
          this->move_op_top_to_stack();
          assert(!this->_op_stack.empty());
        }
        assert(this->_op_stack.back() == operand_type{ LParen });
        this->_op_stack.pop_back();
        break;
      }
      }
      return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Flush operator stack to finalize Reverse-Polish Notation.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void
    flush()
    {
      assert(!this->empty());
      while (!this->_op_stack.empty()) {
        assert(this->_op_stack.back() != operand_type{ LParen });

        // Move operator from outermost scope to RPN stack.
        this->move_op_top_to_stack();
      }

      // Clear all space used by the operator stack.
      this->_op_stack.clear();
      this->_op_stack.shrink_to_fit();
      assert(this->_op_stack.empty());

      // Default constant value for empty expressions
      if (this->_rpn_stack.empty()) { this->_rpn_stack.push_back(false); }
    }

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Whether the formula is a constant Boolean value.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool
    is_const() const
    {
      return this->_is_const;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Whether the formula is cubic, i.e. it is only a conjunction of literals.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool
    is_cubic() const
    {
      return this->_is_cubic;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Size of entire formula in bytes.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    size_t
    bytes() const
    {
      return this->_rpn_stack.size() * sizeof(value_type);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Whether the formula is empty.
    ///
    /// \details This would be an invalid state.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool
    empty() const
    {
      return this->_rpn_stack.empty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Iterator to beginning of Boolean expression (in Reverse-Polish Notation).
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<value_type>::const_iterator
    begin() const
    {
      // if (!this->_op_stack.empty()) { this->flush(); }
      return this->_rpn_stack.cbegin();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Iterator to beginning of Boolean expression (in Reverse-Polish Notation).
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<value_type>::const_iterator
    end() const
    {
      // if (!this->_op_stack.empty()) { this->flush(); }
      return this->_rpn_stack.cend();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Iterator to beginning of Boolean expression (in *Reverse* Reverse-Polish Notation).
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<value_type>::const_reverse_iterator
    rbegin() const
    {
      // if (!this->_op_stack.empty()) { this->flush(); }
      return this->_rpn_stack.crbegin();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Iterator to beginning of Boolean expression (in *Reverse* Reverse-Polish Notation).
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<value_type>::const_reverse_iterator
    rend() const
    {
      // if (!this->_op_stack.empty()) { this->flush(); }
      return this->_rpn_stack.crend();
    }

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Evaluate the constant value (if possible)
    ///
    /// \pre `is_const() == true`
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool
    eval_const() const
    {
      assert(this->is_const());

      std::vector<bool> stack;
      for (const value_type& v : *this) {
        std::visit(
          overload{
            [&stack](const bool x) -> void { stack.push_back(x); },
            [](const int /*x*/) -> void { throw std::runtime_error("Unresolveable variable"); },
            [&stack](const unary_operator o) -> void {
              const bool x = stack.back();
              stack.pop_back();

              switch (o) {
              case Not: {
                stack.push_back(!x);
                return;
              }
              }
              stack.push_back(false);
            },
            [&stack](const binary_operator o) -> void {
              const bool x = stack.back();
              stack.pop_back();
              const bool y = stack.back();
              stack.pop_back();

              switch (o) {
              case Or: {
                stack.push_back(x | y);
                return;
              }
              case And: {
                stack.push_back(x & y);
                return;
              }
              case Xor: {
                stack.push_back(x ^ y);
                return;
              }
              case Eq: {
                stack.push_back(x == y);
                return;
              }
              }
              stack.push_back(false);
            },
          },
          v);
      }
      return stack.back();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Obtain the support, i.e. the set of all variables explicitly mentioned.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::set<int>
    support() const
    {
      std::set<int> res;
      for (const value_type& v : *this) {
        std::visit(overload{
                     [&res](const int x) -> void { res.insert(x); },
                     [](const auto& /*t*/) -> void { /* do nothing */ },
                   },
                   v);
      }
      return res;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Obtain a string representation of the formula (in Reverse-Polish Notation).
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      std::stringstream ss;
      for (const value_type& v : *this) {
        std::visit(overload{
                     [&ss](const bool x) -> void { ss << x << " "; },
                     [&ss](const int x) -> void { ss << "x" << x << " "; },
                     [&ss](const unary_operator o) -> void {
                       switch (o) {
                       case Not: {
                         ss << "! ";
                         return;
                       }
                       }
                       ss << "? ";
                     },
                     [&ss](const binary_operator o) -> void {
                       switch (o) {
                       case Or: {
                         ss << "| ";
                         return;
                       }
                       case And: {
                         ss << "& ";
                         return;
                       }
                       case Xor: {
                         ss << "+ ";
                         return;
                       }
                       case Eq: {
                         ss << "= ";
                         return;
                       }
                       }
                       ss << "? ";
                     },
                   },
                   v);
      }
      return ascii_trim(ss.str());
    }
  };

  class transition
  {
  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Semantics for how the pre and postconditions are related.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    enum semantics : char
    {
      /** The pre and postconditions are an If-Then */
      Imply,
      /** The truthity of the postcondition is assigned to-be the truthity of the precondition */
      Assignment
    };

  private:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Semantics of transition
    ////////////////////////////////////////////////////////////////////////////////////////////////
    semantics _semantics;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Formula for pre-condition, i.e. this formula has to be satisfied for the transition
    ///        to fire.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool_exp _pre;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief   Formula for post-condition, i.e. the state of variables after being fired.
    ///
    /// \details Only the variables in the *suppport* are affected as-per this formula. The
    ///          remaining variables are unchanged (frame-rule).
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool_exp _post;

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Construct transition of given precondition and postcondition.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    transition(const bool_exp& pre, const semantics semantics, const bool_exp& post)
      : _semantics(semantics)
      , _pre(pre)
      , _post(post)
    {
      if (this->_pre.empty()) { throw std::invalid_argument("Invalid empty precondition"); }
      if (this->_post.empty()) { throw std::invalid_argument("Invalid empty postcondition"); }

      if (this->_semantics == Assignment && !this->_post.is_cubic()) {
        throw std::invalid_argument("'Assignment' unusable with non-cubical postcondition");
      }
    }

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Semantics.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const semantics&
    semantics() const
    {
      return this->_semantics;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Precondition.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const bool_exp&
    pre() const
    {
      return this->_pre;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Postcondition.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const bool_exp&
    post() const
    {
      return this->_post;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Construct transition of given precondition and postcondition.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    size_t
    bytes() const
    {
      return sizeof(this->_semantics) + this->_pre.bytes() + this->_post.bytes();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Obtain a string representation of the transition.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      const std::string arrow = this->_semantics == Assignment ? "=:" : "-->";
      return "'" + this->_pre.to_string() + "' " + (arrow) + " '" + this->_post.to_string() + "'";
    }
  };

private:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief List of variable names in declaration order.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  std::vector<std::string> _int_to_var;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Map from variable name to their declaration order index.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  std::unordered_map<std::string, int> _var_to_int;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Formula for the initial state(s).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  bool_exp _initial;

  ////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Formula for constant variables, i.e. variables that always have a certain value.
  ////////////////////////////////////////////////////////////////////////////////////////////////
  bool_exp _invariant;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief List of transitions in declaration order.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  std::vector<transition> _int_to_trans;

public:
  transition_system()
  {
    // Set initial states to `true` formula by default.
    this->_initial.push(true);

    // Set invariant to `true` formula by default.
    this->_invariant.push(true);
  }

public:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain read-only access to variables.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const std::vector<std::string>&
  vars() const
  {
    return this->_int_to_var;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain declaration order index of a given variable.
  ///
  /// \details If the variable is yet unknown, it will be added as a new variable.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  find_var(const std::string& var)
  {
    const auto find_res = this->_var_to_int.find(var);
    if (find_res != this->_var_to_int.end()) { return find_res->second; }
    const int res = this->_int_to_var.size();
    this->_int_to_var.push_back(var);
    this->_var_to_int.insert({ var, res });
    return res;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Whether a variable already has been created.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  contains_var(const std::string& var)
  {
    const auto find_res = this->_var_to_int.find(var);
    return find_res != this->_var_to_int.end();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Insert a new transition.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  void
  insert_transition(const transition& t)
  {
    this->_int_to_trans.push_back(t);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain read-only access to all transitions.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const std::vector<transition>&
  transitions() const
  {
    return this->_int_to_trans;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain read-only access to initial state(s).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const bool_exp&
  initial() const
  {
    return this->_initial;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Overwrite the initial set of states.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  void
  initial(const bool_exp& initial)
  {
    if (initial.empty()) { throw std::invalid_argument("Invalid empty initial state formula"); }
    this->_initial = initial;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain read-only access to initial state(s).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const bool_exp&
  invariant() const
  {
    return this->_invariant;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Overwrite the initial set of states.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  void
  invariant(const bool_exp& invariant)
  {
    if (invariant.empty()) { throw std::invalid_argument("Invalid empty invariant state formula"); }
    this->_invariant = invariant;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Number of bytes used to describe the original transition system (non-symbolic).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  size_t
  bytes() const
  {
    size_t res = this->_initial.bytes();
    for (const auto& t : this->_int_to_trans) { res += t.bytes(); }
    return res;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain string representation of input.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  std::string
  to_string() const
  {
    std::stringstream ss;
    ss << "transition_system {\n"
       << "  initial:     '" << this->_initial.to_string() << "',\n"
       << "  invariant:   '" << this->_invariant.to_string() << "',\n"
       << "  transitions: [\n";
    for (const auto& t : this->transitions()) { ss << "    { " << t.to_string() << " },\n"; }
    ss << "  ]\n"
       << "}";
    return ss.str();
  }
};

transition_system::bool_exp
parse_exp(transition_system& ts, const std::string& exp)
{
  using bool_exp = transition_system::bool_exp;

  bool_exp result;
  std::string var_buffer;

  const auto flush_buffer = [&]() {
    if (!var_buffer.empty()) {
      // Convert long-form constant 'true' and 'false'
      if (var_buffer == "true" || var_buffer == "false") {
        result.push(var_buffer == "true");
      }
      // Otherwise, treat it as a variable
      else {
        result.push(ts.find_var(var_buffer));
      }
      var_buffer.clear();
    }
  };

  for (const char x : exp) {
    // Extend or flush variable buffer
    if (ascii_isalpha(x) || (var_buffer != "" && ascii_isnumeric(x))) {
      var_buffer += x;
      continue;
    }
    flush_buffer();

    // Skip over white space.
    if (std::isspace(x)) { continue; }

    // Operators and constants
    switch (x) {
    case '!': result.push(bool_exp::Not); break;
    case '|': result.push(bool_exp::Or); break;
    case '&': result.push(bool_exp::And); break;
    case '(': result.push(bool_exp::LParen); break;
    case ')': result.push(bool_exp::RParen); break;
    case '0': result.push(false); break;
    case '1': result.push(true); break;
    default: std::cerr << "Parsing error : '" << x << "'" << std::endl; throw std::exception();
    }
  }
  flush_buffer();
  result.flush();
  return result;
}

void
parse_exp__math(transition_system& ts,
                const std::unordered_map<std::string, int>& constants,
                const pugi::xml_node& xml_node,
                transition_system::bool_exp& out)
{
  using bool_exp = transition_system::bool_exp;

  if (!xml_node) { throw std::runtime_error("Unexpected missing subtree!"); }

  const std::string node_name(xml_node.name());

  if (node_name == "math") {
    parse_exp__math(ts, constants, xml_node.first_child(), out);
    return;
  }
  if (node_name == "ci") {
    const std::string text     = ascii_trim(xml_node.child_value());
    const auto& constants_find = constants.find(text);

    if (constants_find != constants.end()) {
      out.push(constants_find->second != 0);
    } else {
      const int var = ts.find_var(text);
      out.push(var);
    }
    return;
  }
  if (node_name == "cn") {
    const int value = std::stoi(ascii_trim(xml_node.child_value()));
    out.push(value != 0);
    return;
  }
  if (node_name == "apply") {
    const pugi::xml_node& first_child = xml_node.first_child();
    if (!first_child) { throw std::runtime_error("Empty <apply /> subtree"); }

    std::string first_child_name(first_child.name());

    if (first_child_name == "not") {
      out.push(bool_exp::Not).push(bool_exp::LParen);

      if (xml_node.last_child() == xml_node.first_child()) {
        throw std::runtime_error("Empty <not /> statement");
      }

      parse_exp__math(ts, constants, xml_node.last_child(), out);
      out.push(bool_exp::RParen);
      return;
    } else if (first_child_name == "and" || first_child_name == "or" || first_child_name == "xor"
               || first_child_name == "eq") {
      const bool_exp::binary_operator op = first_child_name == "and" ? bool_exp::And
        : first_child_name == "or"                                   ? bool_exp::Or
        : first_child_name == "xor"                                  ? bool_exp::Xor
                                    : /*: first_child_name == "eq"*/ bool_exp::Eq;

      bool done_first = false;
      for (const pugi::xml_node& subtree : xml_node) {
        if (subtree == first_child) { continue; }

        if (done_first) { out.push(op); }
        out.push(bool_exp::LParen);
        parse_exp__math(ts, constants, subtree, out);
        out.push(bool_exp::RParen);

        done_first = true;
      }
      return;
    }
    throw std::runtime_error("Unknown operation <" + first_child_name + " /> in <apply />");
  }

  throw std::runtime_error("Unknown XML Math node <" + node_name + "/>");
}

transition_system
parse_file__aeon(const std::filesystem::path& path)
{
  using bool_exp   = transition_system::bool_exp;
  using transition = transition_system::transition;

  assert(ascii_tolower(path.extension()) == ".aeon");

  transition_system ts;
  bool_exp initial;

  std::map<int, std::pair<std::set<int>, std::set<int>>> activators_inhibitors;
  std::set<int> customized;

  std::ifstream infile(path);

  std::string line;
  while (std::getline(infile, line)) {
    // Skip comments and empty lines
    line = ascii_trim(line.substr(0, line.find("#")));
    if (line == "") { continue; }

    // Parse remainder of current line
    const auto dollar_pos = line.find("$");

    // Case: '$y: ...'
    if (dollar_pos != std::string::npos) {
      const auto colon_pos = line.find(":");
      assert(colon_pos != std::string::npos);

      const int var =
        ts.find_var(ascii_trim(line.substr(dollar_pos + 1, colon_pos - dollar_pos - 1)));
      customized.insert(var);

      const bool_exp pre = parse_exp(ts, line.substr(colon_pos + 1, line.size()));

      if (pre.is_const()) {
        if (!initial.empty()) { initial.push(bool_exp::And); }
        if (!pre.eval_const()) { initial.push(bool_exp::Not); }
        initial.push(var);
      } else {
        bool_exp post;
        post.push(var);
        ts.insert_transition({ pre, transition::Assignment, post });
      }

      continue;
    }

    // Case: 'x -? y'
    const auto custom_pos = line.find("-?");
    if (custom_pos != std::string::npos) { continue; }

    // Case: 'x -> y' and 'x -| y'
    const auto activate_pos = line.find("->");
    const auto inhibit_pos  = line.find("-|");
    assert(activate_pos == std::string::npos || inhibit_pos == std::string::npos);

    const auto relation_pos = activate_pos == std::string::npos ? inhibit_pos : activate_pos;

    const int pre_var  = ts.find_var(ascii_trim(line.substr(0, relation_pos)));
    const int post_var = ts.find_var(ascii_trim(line.substr(relation_pos + 2, line.size())));

    if (activators_inhibitors.find(post_var) == activators_inhibitors.end()) {
      activators_inhibitors.insert({ post_var, { {}, {} } });
    }

    auto& dependencies = inhibit_pos == std::string::npos
      ? activators_inhibitors.find(post_var)->second.first
      : activators_inhibitors.find(post_var)->second.second;

    dependencies.insert(pre_var);
  }

  // Convert (pending) activators and inhibitors
  for (auto it = activators_inhibitors.begin(); it != activators_inhibitors.end(); ++it) {
    // Skip anything customized
    if (customized.find(it->first) != customized.end()) { continue; }

    bool_exp pre;
    { // Construct '(a1 | a2 | a3) & !(h1 | h2 | h3)'
      pre.push(bool_exp::LParen);
      for (const int activating_var : it->second.first) {
        if (activating_var != *it->second.first.begin()) { pre.push(bool_exp::Or); }
        pre.push(activating_var);
      }
      pre.push(bool_exp::RParen);

      if (it->second.second.begin() != it->second.second.end()) {
        pre.push(bool_exp::And).push(bool_exp::Not).push(bool_exp::LParen);
        for (const int inhibiting_var : it->second.second) {
          if (inhibiting_var != *it->second.second.begin()) { pre.push(bool_exp::Or); }
          pre.push(inhibiting_var);
        }
        pre.push(bool_exp::RParen);
      }
      pre.flush();
    }

    bool_exp post;
    post.push(it->first);

    ts.insert_transition({ pre, transition::Assignment, post });
  }

  // Initial state(s) and invariant
  if (initial.empty()) { initial.push(true); }
  initial.flush();
  assert(initial.is_cubic());
  ts.initial(initial);
  ts.invariant(initial);

  return ts;
}

transition_system
parse_file__bnet(const std::filesystem::path& path)
{
  assert(ascii_tolower(path.extension()) == ".bnet");

  transition_system ts;
  transition_system::bool_exp initial;

  std::ifstream infile(path);

  std::string line;
  while (std::getline(infile, line)) {
    // Skip comments and empty lines
    line = ascii_trim(line.substr(0, line.find("#")));
    if (line == "") { continue; }

    // Parse remainder of line
    const auto comma_position       = line.find(",");
    transition_system::bool_exp pre = parse_exp(ts, line.substr(comma_position + 1, line.size()));

    const int var = ts.find_var(ascii_trim(line.substr(0, comma_position)));
    if (pre.is_const()) {
      if (!initial.empty()) { initial.push(transition_system::bool_exp::And); }
      if (!pre.eval_const()) { initial.push(transition_system::bool_exp::Not); }
      initial.push(var);
    } else {
      transition_system::bool_exp post;
      post.push(var);
      ts.insert_transition({ pre, transition_system::transition::Assignment, post });
    }
  }

  // Initial state(s) and invariant
  if (initial.empty()) { initial.push(true); }
  initial.flush();
  assert(initial.is_cubic());
  ts.initial(initial);
  ts.invariant(initial);

  return ts;
}

transition_system
parse_file__pnml(const std::filesystem::path& path)
{
  assert(ascii_tolower(path.extension()) == ".pnml");

  if (synchronous_update) {
    std::cerr << "Synchronous semantics are not supported by Petri nets.\n";
    synchronous_update = false;
  }

  pugi::xml_document doc;
  if (!doc.load_file(path.string().data())) {
    throw std::runtime_error("PNML file could not be parsed");
  }

  using pnml_marking    = std::set<int>;
  using pnml_transition = std::pair<pnml_marking, pnml_marking>;

  pnml_marking initial_marking;
  std::unordered_map<std::string, pnml_transition> transitions;

  transition_system ts;

  using bool_exp   = transition_system::bool_exp;
  using transition = transition_system::transition;

  const pugi::xml_node& doc_page = doc.child("pnml").child("net").child("page");
  for (const pugi::xml_node& n : doc_page) {
    const std::string n_name(n.name());

    if (n_name == "place") {
      const std::string name = n.attribute("id").value();

      if (ts.contains_var(name)) {
        throw std::runtime_error("Place '" + name + "' has already been defined");
      }
      const int var = ts.find_var(name);

      if (n.child("initialMarking")) { initial_marking.insert(var); }
    } else if (n_name == "transition") {
      const std::string name = n.attribute("id").value();

      if (transitions.find(name) != transitions.end()) {
        throw std::runtime_error("Transition '" + name + "' has already been defined");
      }

      transitions.insert({ name, { {}, {} } });
    } else if (n_name == "arc") {
      const std::string source_name = n.attribute("source").value();
      const bool source_is_place    = ts.contains_var(source_name);

      const std::string target_name = n.attribute("target").value();
      const bool target_is_place    = ts.contains_var(target_name);

      if (source_is_place == target_is_place) {
        throw std::runtime_error("'" + source_name + "' -> '" + target_name
                                 + "' are of the same type (or undefined)");
      }

      const std::string t_name = source_is_place ? target_name : source_name;
      const auto t_iter        = transitions.find(t_name);

      if (t_iter == transitions.end()) {
        throw std::runtime_error("Transition '" + t_name + "' is unknown");
      }

      pnml_transition& t = t_iter->second;

      if (source_is_place) {
        t.first.insert(ts.find_var(source_name));
      } else { // if (target_is_place)
        t.second.insert(ts.find_var(target_name));
      }
    } else {
      // ignore node...
      continue;
    }
  }

  // Convert initial state into Boolean expressions.
  {
    bool_exp initial;
    for (int x = 0; x < static_cast<int>(ts.vars().size()); ++x) {
      if (x > 0) { initial.push(bool_exp::And); }
      if (initial_marking.find(x) == initial_marking.end()) { initial.push(bool_exp::Not); }
      initial.push(x);
    }
    if (initial.empty()) { initial.push(false); }
    initial.flush();
    assert(initial.is_cubic());
    ts.initial(initial);
  }

  // Set invariant to 'true'
  {
    bool_exp invariant;
    invariant.push(true);
    invariant.flush();
    ts.invariant(invariant);
  }

  // Convert transitions into Boolean expressions.
  for (const auto& t_iter : transitions) {
    const pnml_transition& t = t_iter.second;

    bool_exp pre;
    bool_exp post;

    { // Precondition

      // Source markings (turned on)
      if (t.first.empty()) { pre.push(true); }
      for (const int p : t.first) {
        if (p != *t.first.begin()) { pre.push(bool_exp::And); }
        pre.push(p);
      }
    }
    assert(!pre.empty());

    { // Postcondition

      // Target markings (turned on)
      post.push(bool_exp::LParen);
      if (t.second.empty()) { post.push(true); }
      for (const int p : t.second) {
        if (p != *t.second.begin()) { post.push(bool_exp::And); }
        post.push(p);
      }
      post.push(bool_exp::RParen);

      // Source markings (turned off)
      if (t.first != t.second) {
        if (!post.empty()) { post.push(bool_exp::And); }
        post.push(bool_exp::LParen);

        bool need_op = false;
        for (const int p : t.first) {
          // Skip overlapping places that stay true
          if (t.second.find(p) != t.second.end()) { continue; }
          // Add non-overlapping places are turned off
          if (need_op) { post.push(bool_exp::And); }
          post.push(bool_exp::Not).push(p);
          need_op = true;
        }
        post.push(bool_exp::RParen);
      }
    }
    assert(!post.empty());

    pre.flush();
    assert(pre.is_cubic());

    post.flush();
    assert(post.is_cubic());

    ts.insert_transition({ pre, transition::Imply, post });
  }

  return ts;
}

transition_system
parse_file__sbml(const std::filesystem::path& path)
{
  assert(ascii_tolower(path.extension()) == ".sbml");

  pugi::xml_document doc;
  if (!doc.load_file(path.string().data())) {
    throw std::runtime_error("SBML file could not be parsed");
  }

  transition_system ts;

  using bool_exp   = transition_system::bool_exp;
  using transition = transition_system::transition;

  bool_exp initial;

  const pugi::xml_node& doc_model = doc.child("sbml").child("model");
  for (const pugi::xml_node& n : doc_model) {
    std::string n_name(n.name());
    const std::string qual_prefix = "qual:";
    // const bool has_qual_prefix    = n_name.find(qual_prefix) == 0;

    if (n_name == "listOfCompartments") {
      continue;
    } else if (n_name == qual_prefix + "listOfQualitativeSpecies") {
      for (const pugi::xml_node& c : n) {
        const std::string c_name(c.name());
        assert(c_name == qual_prefix + "qualitativeSpecies");

        const std::string id(c.attribute((qual_prefix + "id").data()).value());
        const int var = ts.find_var(id);

        const auto& initialLevel_attribute = c.attribute((qual_prefix + "initialLevel").data());
        if (initialLevel_attribute) {
          const bool initialLevel = initialLevel_attribute ? initialLevel_attribute.as_int() : 0;

          if (!initial.empty()) { initial.push(bool_exp::And); }
          if (!initialLevel) { initial.push(bool_exp::Not); }
          initial.push(var);
        }
      }
    } else if (n_name == qual_prefix + "listOfTransitions") {
      for (const pugi::xml_node& c : n) {
        const std::string c_name(c.name());
        if (c_name != qual_prefix + "transition") { continue; }

        std::unordered_map<std::string, int> constants; // input::id -> <threshold>
        std::unordered_map<int, bool> inputs;           // var -> is_consumed
        bool input_consumption = false;

        std::unordered_map<int, bool> outputs; // var -> is_production
        bool output_production = false;
        bool output_assignment = false;

        // Process ingoing variables
        for (const pugi::xml_node& i : c.child((qual_prefix + "listOfInputs").data())) {
          if (i.name() != qual_prefix + "input") { continue; }

          const std::string species_id(
            i.attribute((qual_prefix + "qualitativeSpecies").data()).value());
          const int var = ts.find_var(species_id);

          const std::string transitionEffect =
            i.attribute((qual_prefix + "transitionEffect").data()).value();
          const bool consumption = transitionEffect == "consumption";

          inputs.insert({ var, consumption });
          input_consumption |= consumption;

          const auto& id             = i.attribute((qual_prefix + "id").data());
          const auto& thresholdLevel = i.attribute((qual_prefix + "thresholdLevel").data());

          if (id && thresholdLevel) { constants.insert({ id.value(), thresholdLevel.as_int() }); }
        }

        // Process outgoing variables
        for (const pugi::xml_node& o : c.child((qual_prefix + "listOfOutputs").data())) {
          if (o.name() != qual_prefix + "output") { continue; }

          const std::string species_id(
            o.attribute((qual_prefix + "qualitativeSpecies").data()).value());
          const int var = ts.find_var(species_id);

          const std::string transitionEffect =
            o.attribute((qual_prefix + "transitionEffect").data()).value();

          const bool is_assignment = transitionEffect == "assignmentLevel";
          const bool is_production = transitionEffect == "production";

          outputs.insert({ var, is_production });
          output_production |= is_production;
          output_assignment |= is_assignment;
        }
        assert(output_production || output_assignment);

        // Process functionTerms
        bool_exp pre;

        const pugi::xml_node& listOfFunctionTerms =
          c.child((qual_prefix + "listOfFunctionTerms").data());
        const pugi::xml_node& defaultTerm =
          listOfFunctionTerms.child((qual_prefix + "defaultTerm").data());
        const bool defaultResult =
          defaultTerm.attribute((qual_prefix + "resultLevel").data()).as_int();

        if (defaultResult) {
          pre.push(bool_exp::Not);
          pre.push(bool_exp::LParen);
        }
        assert(pre.empty());

        bool defaultResult_terms = false;
        pre.push(bool_exp::LParen);
        for (const pugi::xml_node& f : listOfFunctionTerms) {
          const std::string f_name(f.name());
          if (f_name != qual_prefix + "functionTerm") { continue; }

          const bool resultLevel = f.attribute((qual_prefix + "resultLevel").data()).as_int();

          // The default result terms should take precedence.
          if (resultLevel == defaultResult) {
            defaultResult_terms = true;
            continue;
          }

          if (!pre.empty()) { pre.push(bool_exp::Or); }
          pre.push(bool_exp::LParen);
          parse_exp__math(ts, constants, f.child("math"), pre);
          pre.push(bool_exp::RParen);
        }
        pre.push(bool_exp::RParen);
        if (pre.empty()) {
          pre.push(false);
        } else if (defaultResult_terms) {
          // Retraverse for outputs with 'resultLevel' same as 'defaultTerm'
          for (const pugi::xml_node& f : listOfFunctionTerms) {
            const std::string f_name(f.name());
            if (f_name != qual_prefix + "functionTerm") { continue; }

            const bool resultLevel = f.attribute((qual_prefix + "resultLevel").data()).as_int();
            if (resultLevel != defaultResult) { continue; }

            pre.push(resultLevel ? bool_exp::Or : bool_exp::And);
            if (resultLevel) { pre.push(bool_exp::Not); }
            pre.push(bool_exp::LParen);
            parse_exp__math(ts, constants, f.child("math"), pre);
            pre.push(bool_exp::RParen);
          }
        }
        if (defaultResult) { pre.push(bool_exp::RParen); }

        // TODO: Add 'thresholdLevel' on ingoing variables to precondition?
        pre.flush();

        bool_exp post_positive;
        for (const auto& var_prod : outputs) {
          if (!post_positive.empty()) { post_positive.push(bool_exp::And); }
          post_positive.push(var_prod.first);
        }
        for (const auto& var_prod : inputs) {
          // Skip non-consumed inputs
          if (!var_prod.second) { continue; }

          // Skip consumed inputs that are also outputs (overwritten)
          if (outputs.find(var_prod.first) != outputs.end()) { continue; }

          post_positive.push(bool_exp::And).push(bool_exp::Not).push(var_prod.first);
        }
        post_positive.flush();

        if (output_production + output_assignment == 1
            && (output_assignment && !input_consumption && !defaultResult)) {
          ts.insert_transition({ pre, transition::Assignment, post_positive });
        } else {
          ts.insert_transition({ pre, transition::Imply, post_positive });
        }

        if (output_production + output_assignment > 1
            || (output_assignment && (input_consumption || defaultResult))) {
          // HACK: Since pre is already flushed, pushing an '!' operation and then re-flushing is a
          //       valid statement in Reverse-Polish Notation.
          pre.push(bool_exp::Not);
          pre.flush();

          bool_exp post_negative;
          for (const auto& var_prod : outputs) {
            // Skip all production variables
            if (var_prod.second) { continue; }

            if (!post_negative.empty()) { post_negative.push(bool_exp::And); }
            post_negative.push(bool_exp::Not).push(var_prod.first);
          }
          for (const auto& var_prod : inputs) {
            // Skip non-consumed inputs
            if (!var_prod.second) { continue; }

            // Skip consumed inputs that are also (assigned) outputs
            const auto outputs_iter = outputs.find(var_prod.first);
            if (outputs_iter != outputs.end() && !outputs_iter->second) { continue; }

            post_negative.push(bool_exp::And).push(bool_exp::Not).push(var_prod.first);
          }

          post_negative.flush();

          ts.insert_transition({ pre, transition::Imply, post_negative });
        }
      }
    } else {
      // ignore node...
      continue;
    }
  }

  // Initial state(s) and invariant
  if (initial.empty()) { initial.push(true); }
  initial.flush();
  assert(initial.is_cubic());
  ts.initial(initial);
  ts.invariant(initial);

  return ts;
}

transition_system
parse_file(const std::filesystem::path& path)
{
  const std::string extension = ascii_tolower(path.extension());

  if (extension == ".aeon") { return parse_file__aeon(path); }
  if (extension == ".bnet") { return parse_file__bnet(path); }
  if (extension == ".pnml") { return parse_file__pnml(path); }
  if (extension == ".sbml") { return parse_file__sbml(path); }

  throw std::runtime_error("Unknown file type");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO (variable orders):
// - [x] Identity
// - [x] Random
// - [x] boost::sloan_ordering
// - [x] boost::cuthill_mckee_ordering
// - [ ] noack

// TODO:
// - [ ] Add transition ordering here too!

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Permutations to match a certain variable order.
////////////////////////////////////////////////////////////////////////////////////////////////////
class variable_permutation
{
private:
  /*const*/ std::unordered_map<int, int> _permutation;
  std::unordered_map<int, int> _permutation_inv;

  void
  compute_inv()
  {
    this->_permutation_inv.clear();
    for (const auto& [x, y] : this->_permutation) { this->_permutation_inv.insert({ y, x }); }
  }

  variable_permutation(const std::unordered_map<int, int>& permutation)
    : _permutation(permutation)
  {
    compute_inv();
  }

  variable_permutation&
  operator=(const std::unordered_map<int, int>& permutation)
  {
    this->_permutation = permutation;
    compute_inv();
    return *this;
  }

  variable_permutation(std::unordered_map<int, int>&& permutation)
    : _permutation(std::move(permutation))
  {
    compute_inv();
  }

  variable_permutation&
  operator=(std::unordered_map<int, int>&& permutation)
  {
    this->_permutation = std::move(permutation);
    compute_inv();
    return *this;
  }

public:
  int
  find(int x) const
  {
    const auto iter = this->_permutation.find(x);
    return iter != this->_permutation.end() ? iter->second : -1;
  }

  int
  find_inv(int x) const
  {
    const auto iter = this->_permutation_inv.find(x);
    return iter != this->_permutation_inv.end() ? iter->second : -1;
  }

private:
  using boost__vertex_properties = boost::property<
    boost::vertex_color_t,
    boost::default_color_type,
    boost::property<boost::vertex_degree_t, int, boost::property<boost::vertex_priority_t, int>>>;

  using boost__graph_type =
    boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS, boost__vertex_properties>;

  using boost__vertex_type = boost::graph_traits<boost__graph_type>::vertex_descriptor;
  using boost__size_type   = boost::graph_traits<boost__graph_type>::vertices_size_type;

  /// \brief Creates the incidence graph, i.e. a graph where variables are nodes and are connected
  ///        if they occur in the same transition together.
  static inline boost__graph_type
  boost__incidence_graph(const transition_system& ts)
  {
    boost__graph_type g(ts.vars().size());

    // Add an edge to the graph, for variables that occur in the same transition.
    for (const transition_system::transition& t : ts.transitions()) {
      const std::set<int> pre_support  = t.pre().support();
      const std::set<int> post_support = t.post().support();

      for (const int x : pre_support) {
        for (const int y : pre_support) { boost::add_edge(x, y, g); }
        for (const int y : post_support) { boost::add_edge(x, y, g); }
      }
    }

    return g;
  }

  /// \brief Converts an ordering from a `boost__incidence_graph(ts)` into a variable permutation.
  static inline variable_permutation
  boost__incidence_permutation(const transition_system&/*ts*/,
                               const std::vector<boost__vertex_type>& o)
  {
    std::unordered_map<int, int> out;
    for (auto it = o.begin(); it != o.end(); ++it) {
      out.insert({ *it, out.size() });
    }
    return variable_permutation(std::move(out));
  }


  /// \brief Creates the write graph, i.e. a graph where a transition node has edges to the
  ///        variables that are read from and/or written to.
  ///
  /// \details See "Bandwidth and Wavefront Reduction for Static Variable Ordering in Symbolic Model
  ///          Checking" by Jeroen Meijer and Jaco van de Pol.
  template <bool IncludeRead, bool IncludeWrite>
  static inline boost__graph_type
  boost__rw_graph(const transition_system& ts)
  {
    const boost__vertex_type var_count        = ts.vars().size();
    const boost__vertex_type transition_count = ts.transitions().size();

    boost__graph_type g(var_count + transition_count);

    for (int t_idx = 0; t_idx < transition_count; ++t_idx) {
      const transition_system::transition& t = ts.transitions().at(t_idx);

      if constexpr (IncludeRead) {
        const std::set<int> pre_support = t.pre().support();
        for (const int x : pre_support) {
          boost::add_edge(t_idx, transition_count + x, g);
        }
      }
      if constexpr (IncludeWrite) {
        const std::set<int> post_support = t.post().support();
        for (const int x : post_support) {
          boost::add_edge(t_idx, transition_count + x, g);
        }
      }
    }

    return g;
  }

  /// \brief Converts an ordering from a `boost__write_graph(ts)` into a variable permutation.
  static inline variable_permutation
  boost__rw_permutation(const transition_system& ts, const std::vector<boost__vertex_type>& o)
  {
    const boost__vertex_type transition_count = ts.transitions().size();

    std::unordered_map<int, int> out;
    for (auto it = o.begin(); it != o.end(); ++it) {
      if (*it < transition_count) { continue; }
      out.insert({ *it - transition_count, out.size() });
    }
    return variable_permutation(std::move(out));
  }

public:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Derive a variable ordering using the Cuthill-Mckee algorithm.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  static variable_permutation
  cuthill_mckee(const transition_system& ts)
  {
    auto g              = boost__incidence_graph(ts);
    const auto g_color  = boost::get(boost::vertex_color, g);
    const auto g_degree = boost::make_degree_map(g);

    // From Boost Documentation on "Sloan's Ordering":
    //   "Usually you need the reversed ordering with the Cuthill-McKee algorithm (...)"
    //
    // From our preliminary experiments, the opposite is the case for BDDs.
    std::vector<boost__vertex_type> boost_order(boost::num_vertices(g));
    boost::cuthill_mckee_ordering(g, boost_order.begin(), g_color, g_degree);

    return boost__incidence_permutation(ts, boost_order);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief The *identify* variable permutation, i.e. the original input declaration order.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  static variable_permutation
  identity(const transition_system& ts)
  {
    std::unordered_map<int, int> permutation;
    for (int i = 0; i < static_cast<int>(ts.vars().size()); ++i) { permutation.insert({ i, i }); }
    return variable_permutation(std::move(permutation));
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief A *random* variable permutation.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  static variable_permutation
  random(const transition_system& ts)
  {
    const int varcount = ts.vars().size();

    // Vector of all variables
    std::vector<int> permutation_vector;
    permutation_vector.reserve(varcount);
    for (int i = 0; i < varcount; ++i) { permutation_vector.push_back(i); }

    // Shuffle vector
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(permutation_vector.begin(), permutation_vector.end(), gen);

    // Turn into a map
    std::unordered_map<int, int> permutation;
    for (int i = 0; i < varcount; ++i) { permutation.insert({ i, permutation_vector[i] }); }
    return variable_permutation(std::move(permutation));
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Derive a variable ordering using Sloan's algorithm.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  static variable_permutation
  sloan(const transition_system& ts)
  {
    auto g                = boost__incidence_graph(ts);
    const auto g_color    = boost::get(boost::vertex_color, g);
    const auto g_degree   = boost::make_degree_map(g);
    const auto g_priority = boost::get(boost::vertex_priority, g);

    // From Boost Documentation on "Sloan's Ordering":
    //   "(...) and the direct ordering with the Sloan algorithm."
    //
    // From our preliminary experiments, the opposite is the case for BDDs.
    std::vector<boost__vertex_type> boost_order(boost::num_vertices(g));
    boost::sloan_ordering(g, boost_order.rbegin(), g_color, g_degree, g_priority);

    return boost__incidence_permutation(ts, boost_order);
  }

public:
  variable_permutation(const transition_system& ts, const variable_order& vo)
  {
    switch (vo) {
    case variable_order::CUTHILL_MCKEE: {
      *this = cuthill_mckee(ts);
      return;
    }
    case variable_order::INPUT: {
      *this = identity(ts);
      return;
    }
    case variable_order::RANDOM: {
      *this = random(ts);
      return;
    }
    case variable_order::SLOAN: {
      *this = sloan(ts);
      return;
    }
    }
    throw std::runtime_error("Unknown variable order");
  }

  variable_permutation(const transition_system& ts)
    : variable_permutation(ts, var_order)
  {}

  std::string
  to_string() const
  {
    std::stringstream ss;
    ss << "permutation {\n";
    for (const auto& [x, y] : this->_permutation) { ss << "  x" << x << " -> x" << y << ",\n"; }
    ss << "}";
    return ss.str();
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO :
// - [ ] Merge async transitions if requested

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Symbolic representation of a Transition System.
///
/// \details Converts in the constructor a `transition_system` into its Decision Diagram
///          representation such that it is ready to be used in symbolic algorithms.
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
class symbolic_transition_system
{
public:
  using dd_t = typename Adapter::dd_t;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Symbolic representation of one (or more) transition(s).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  class transition
  {
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief The relational expression between pre- and post-state variables.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    dd_t _relation;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief The support of a transition (as a cube of pre-state variables).
    ////////////////////////////////////////////////////////////////////////////////////////////////
    dd_t _support;

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    ////////////////////////////////////////////////////////////////////////////////////////////////
    transition(const dd_t& r, const dd_t s)
      : _relation(r)
      , _support(s)
    {}

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief The relational expression between pre- and post-state variables.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const dd_t&
    relation() const
    {
      return this->_relation;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief The support of a transition (as a cube of pre-state variables).
    ///
    /// \details These only include the variables that are affected by the transition.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const dd_t&
    support() const
    {
      return this->_support;
    }
  };

private:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Reference to adapter for BDD package.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  Adapter& _adapter;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Transition system (non-symbolic).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const transition_system _ts;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Variable permutation (bridge between `_ts` and anything of type `dd_t`).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const variable_permutation _vp;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Symbolic invariant.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  dd_t _all;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Symbolic initial state(s).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  dd_t _initial;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Symbolic transition(s)
  //////////////////////////////////////////////////////////////////////////////////////////////////
  std::vector<transition> _transitions;

public:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Renaming of Boolean values to something less error-prone.
  ///
  /// \details One quickly forgets whether 'bool prime == true' is before or after the transition.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  enum prime : bool
  {
    pre  = false,
    post = true
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Convert a *Transition System* variable into a *Decision Diagram* variable.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  dd_var(int x, bool is_prime = prime::pre) const
  {
    assert(0 <= x && x < static_cast<int>(this->_ts.vars().size()));
    return 2 * this->_vp.find(x) + is_prime;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Number of Decision Diagram variables for a specific primality.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  varcount(bool /*is_prime*/) const
  {
    return this->_ts.vars().size();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Number of Decision Diagram variables.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  varcount() const
  {
    return 2 * this->varcount(prime::pre);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Minimal Decision Diagram variable.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  min_var(bool is_prime = prime::pre) const
  {
    return is_prime;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Maximal Decision Diagram variable.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  max_var(bool is_prime = prime::pre) const
  {
    return this->varcount() - 1 - is_prime;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Convert a *Decision Diagram* variable back into  *Transition System* variable.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  int
  ts_var(int x) const
  {
    assert(0 <= x && x < 2 * this->varcount());
    return this->_vp.find_inv(x / 2);
  }

private:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Convert a Boolean expression in the transition system into a Decision Diagram.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  dd_t
  convert(const transition_system::bool_exp& e, bool is_prime = prime::pre) const
  {
    // ------------------------------------------------------------------------------------------
    // Case: 'e' is a constant, build the equivalent terminal;
    if (e.is_const()) { return e.eval_const() ? this->_adapter.top() : this->_adapter.bot(); }

    // ------------------------------------------------------------------------------------------
    // Case: 'e' is a cube, build it by hand bottom-up
    if (e.is_cubic()) {
      using build_ptr = typename Adapter::build_node_t;

      std::vector<std::pair<int, bool>> cube;
      {
        bool negate_next = false;
        for (auto it = e.rbegin(); it != e.rend(); ++it) {
          std::visit(overload{
                       [this, &is_prime, &cube, &negate_next](const int x) -> void {
                         const int dd_x = this->dd_var(x, is_prime);
                         cube.push_back({ dd_x, negate_next });
                         negate_next = false;
                       },
                       [&negate_next](const transition_system::bool_exp::unary_operator o) -> void {
                         switch (o) {
                         case transition_system::bool_exp::Not: {
                           negate_next = !negate_next;
                           return;
                         }
                         }
                       },
                       [](auto) -> void { /* Do nothing */ },
                     },
                     *it);
        }
        std::sort(cube.begin(), cube.end(), [](auto a, auto b) { return a.first < b.first; });
      }

      const build_ptr false_ptr = this->_adapter.build_node(false);
      const build_ptr true_ptr  = this->_adapter.build_node(true);

      build_ptr root = true_ptr;

      for (auto it = cube.rbegin(); it != cube.rend(); ++it) {
        root = it->second ? this->_adapter.build_node(it->first, false_ptr, root)
                          : this->_adapter.build_node(it->first, root, false_ptr);
      }

      return this->_adapter.build();
    }

    // ------------------------------------------------------------------------------------------
    // Case: 'e' is complex, build it by executing the Reverse Polish Notation.
    {
      std::vector<dd_t> stack;
      for (const auto& v : e) {
        std::visit(overload{
                     [this, &stack](const bool x) -> void {
                       stack.push_back(x ? this->_adapter.top() : this->_adapter.bot());
                     },
                     [this, &stack, &is_prime](const int x) -> void {
                       const int dd_x = this->dd_var(x, is_prime);
                       stack.push_back(this->_adapter.ithvar(dd_x));
                     },
                     [this, &stack](const transition_system::bool_exp::unary_operator o) -> void {
                       const dd_t x = stack.back();
                       stack.pop_back();

                       switch (o) {
                       case transition_system::bool_exp::Not: {
                         stack.push_back(~x);
                         return;
                       }
                       }
                     },
                     [this, &stack](const transition_system::bool_exp::binary_operator o) -> void {
                       const dd_t x = stack.back();
                       stack.pop_back();
                       const dd_t y = stack.back();
                       stack.pop_back();

                       switch (o) {
                       case transition_system::bool_exp::Or: {
                         stack.push_back(this->_adapter.apply_or(x, y));
                         return;
                       }
                       case transition_system::bool_exp::And: {
                         stack.push_back(this->_adapter.apply_and(x, y));
                         return;
                       }
                       case transition_system::bool_exp::Xor: {
                         stack.push_back(this->_adapter.apply_xor(x, y));
                         return;
                       }
                       case transition_system::bool_exp::Eq: {
                         stack.push_back(this->_adapter.apply_xnor(x, y));
                         return;
                       }
                       }
                     },
                   },
                   v);
      }
      return stack.back();
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Convert a single Transition in the transition system into a Decision Diagram.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  transition
  convert(const transition_system::transition& t) const
  {
    // Convert expressions for Precondition and Postcondition
    const dd_t pre_dd  = this->convert(t.pre(), prime::pre);
    const dd_t post_dd = this->convert(t.post(), prime::post);

    const std::set<int> pre_support  = t.pre().support();
    const std::set<int> post_support = t.post().support();

    // Frame Rule
    dd_t frame_dd = this->_adapter.top();
    if (!synchronous_update) {
      const auto bot = this->_adapter.build_node(false);
      const auto top = this->_adapter.build_node(true);

      auto root = top;

      for (int x = this->varcount() - 2; 0 <= x; x -= 2) {
        assert(x % 2 == 0);

        // Skip variables also mentioned in the Postcondition
        if (post_support.find(this->ts_var(x)) != post_support.end()) { continue; }

        // Skip variable neither in Precondition, if BDD package does not need it.
        if constexpr (!Adapter::needs_frame_rule) {
          if (pre_support.find(this->ts_var(x)) == pre_support.end()) { continue; }
        }

        const auto root0 = this->_adapter.build_node(x + 1, root, bot);
        const auto root1 = this->_adapter.build_node(x + 1, bot, root);
        root             = this->_adapter.build_node(x, root0, root1);
      }

      frame_dd = this->_adapter.build();
    }

    // Support Cube
    std::set<int> support = pre_support;
    support.insert(post_support.begin(), post_support.end());

    dd_t support_dd = this->_adapter.top();
    {
      const auto bot = this->_adapter.build_node(false);
      const auto top = this->_adapter.build_node(true);

      auto root = top;
      for (auto iter = support.rbegin(); iter != support.rend(); ++iter) {
        root = this->_adapter.build_node(this->dd_var(*iter), bot, root);
      }
      support_dd = this->_adapter.build();
    }

    // Combine into Relation
    dd_t rel_dd;
    switch (t.semantics()) {
    case transition_system::transition::Assignment: {
      rel_dd =
        this->_adapter.apply_xnor(std::move(pre_dd), std::move(post_dd)) & std::move(frame_dd);
      break;
    }
    case transition_system::transition::Imply: {
      rel_dd =
        this->_adapter.apply_and(std::move(pre_dd), std::move(post_dd) & std::move(frame_dd));
      break;
    }
    }

    // Return Relation and Support
    return { rel_dd, support_dd };
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Convert given `transition_system` into its Decision Diagrams.
  ///
  /// \brief This should be called after the member initialization list is done in the constructor.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  void
  convert()
  {
    this->_all     = this->convert(this->_ts.invariant(), prime::pre);
    this->_initial = this->convert(this->_ts.initial(), prime::pre);

    this->_transitions.reserve(this->_ts.transitions().size());
    for (const auto& t : this->_ts.transitions()) {
      this->_transitions.push_back(this->convert(t));
    }

    if (synchronous_update) {
      std::queue<typename Adapter::dd_t> work_queue;

      const auto begin = this->_transitions.begin();
      const auto end   = this->_transitions.end();

      for (auto t_iter = begin; t_iter != end; t_iter += 2) {
        if ((t_iter + 1) == end) {
          work_queue.push(t_iter->relation());
        } else {
          work_queue.push(t_iter->relation() & (t_iter + 1)->relation());
        }
      }
      while (work_queue.size() > 1) {
        const auto t1 = work_queue.front();
        work_queue.pop();
        const auto t2 = work_queue.front();
        work_queue.pop();

        work_queue.push(t1 & t2);
      }
      const auto is_prime_pre = [](int x) { return x % 2 == prime::pre; };
      this->_transitions = { transition(work_queue.front(), this->_adapter.cube(is_prime_pre)) };
    }
  }

public:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Copy constructor
  //////////////////////////////////////////////////////////////////////////////////////////////////
  symbolic_transition_system(Adapter& adapter,
                             const transition_system& ts,
                             const variable_permutation& vp)
    : _adapter(adapter)
    , _ts(ts)
    , _vp(vp)
  {
    convert();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Move constructor
  //////////////////////////////////////////////////////////////////////////////////////////////////
  symbolic_transition_system(Adapter& adapter, transition_system&& ts, variable_permutation&& vp)
    : _adapter(adapter)
    , _ts(std::move(ts))
    , _vp(std::move(vp))
  {
    convert();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Decision Diagram of *all* States.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  dd_t
  all() const
  {
    return this->_all;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain the Decision Diagram for the Initial State(s).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const dd_t&
  initial() const
  {
    return this->_initial;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain the list of all transitions.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  const std::vector<transition>&
  transitions() const
  {
    return this->_transitions;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Number of bytes used to describe the original transition system (non-symbolic).
  //////////////////////////////////////////////////////////////////////////////////////////////////
  size_t
  bytes() const
  {
    return this->_ts.bytes();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Accumulated size of all Decision Diagrams.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  size_t
  nodecount() const
  {
    size_t res = this->_adapter.nodecount(this->_initial);
    for (const auto& t : this->_transitions) {
      res += this->_adapter.nodecount(t.relation());
      res += this->_adapter.nodecount(t.support());
    }
    return res;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain string representation of Transition System.
  ///
  /// \details This representation is merely of the (non-symbolic) input. That is, the variables are
  ///          not permuted nor primed.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  std::string
  to_string() const
  {
    return this->_ts.to_string() + "\n" + this->_vp.to_string();
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO :
// - [x] Reachability (forwards)
// - [x] Reachability (backwards)
// - [X] Deadlock States (on given set of states - which could be all or only reachable)
// - [X] Chain Algorithm (SCC)
// - [ ] CTL formula

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Compute all reachable states forwards from the given state.
///
/// \param initial_set The set of states to search from (default: `sts.initial()`).
///
/// \param bound The set of states to search within (default: `sts.all()`).
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
typename Adapter::dd_t
forwards(Adapter& adapter,
         const symbolic_transition_system<Adapter>& sts,
         const typename Adapter::dd_t& initial_set,
         const typename Adapter::dd_t& bound)
{
  auto previous = adapter.bot();
  auto current  = initial_set;

  while (previous != current) {
    previous = current;
    for (const auto& t : sts.transitions()) {
      if (current == bound) { break; }
      current |= bound & adapter.relnext(current, t.relation(), t.support());
    }
  }
  return current;
}

template <typename Adapter>
typename Adapter::dd_t
forwards(Adapter& adapter,
         const symbolic_transition_system<Adapter>& sts,
         const typename Adapter::dd_t& initial_set)
{
  return forwards(adapter, sts, initial_set, sts.all());
}

template <typename Adapter>
typename Adapter::dd_t
forwards(Adapter& adapter, const symbolic_transition_system<Adapter>& sts)
{
  return forwards(adapter, sts, sts.initial());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Compute all reachable states forwards from the given state together with the 'newest'
///        reached states.
///
/// \details This is taken from the subprocedure of the Chain algorithm from Larsen et al. 'A Truly
///          Symbolic Linear Time Algorithm for SCC Decomposition' [TACAS 23].
///
/// \param initial_set The set of states to search from (default: `sts.initial()`).
///
/// \param bound The set of states to search within (default: `sts.all()`).
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
std::pair<typename Adapter::dd_t, typename Adapter::dd_t>
forwards_layer(Adapter& adapter,
               const symbolic_transition_system<Adapter>& sts,
               const typename Adapter::dd_t& initial_set,
               const typename Adapter::dd_t& bound)
{
  auto forward_set    = adapter.bot();
  auto previous_layer = adapter.bot();
  auto current_layer  = initial_set;

  while (current_layer != adapter.bot()) {
    forward_set |= current_layer;
    previous_layer = current_layer;

    current_layer = adapter.bot();
    for (const auto& t : sts.transitions()) {
      current_layer |= adapter.relnext(previous_layer, t.relation(), t.support());
    }
    current_layer = (current_layer & bound) - forward_set;
  }

  return { forward_set, previous_layer };
}

template <typename Adapter>
std::pair<typename Adapter::dd_t, typename Adapter::dd_t>
forwards_layer(Adapter& adapter,
               const symbolic_transition_system<Adapter>& sts,
               const typename Adapter::dd_t& initial_set)
{
  return forwards_layer(adapter, sts, initial_set, sts.all());
}

template <typename Adapter>
std::pair<typename Adapter::dd_t, typename Adapter::dd_t>
forwards_layer(Adapter& adapter, const symbolic_transition_system<Adapter>& sts)
{
  return forwards_layer(adapter, sts, sts.initial());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Compute all reachable states backwards from the given state.
///
/// \param initial_set The set of states to search from (default: `sts.initial()`).
///
/// \param bound The set of states to search within (default: `sts.all()`).
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
typename Adapter::dd_t
backwards(Adapter& adapter,
          const symbolic_transition_system<Adapter>& sts,
          const typename Adapter::dd_t& initial_set,
          const typename Adapter::dd_t& bound)
{
  auto previous = adapter.bot();
  auto current  = initial_set;

  while (previous != current) {
    previous = current;

    for (const auto& t : sts.transitions()) {
      if (current == bound) { break; }
      current |= bound & adapter.relprev(current, t.relation(), t.support());
    }
  }
  return current;
}

template <typename Adapter>
typename Adapter::dd_t
backwards(Adapter& adapter,
          const symbolic_transition_system<Adapter>& sts,
          const typename Adapter::dd_t& states)
{
  return backwards(adapter, sts, states, sts.all());
}

template <typename Adapter>
typename Adapter::dd_t
backwards(Adapter& adapter, const symbolic_transition_system<Adapter>& sts)
{
  return backwards(adapter, sts, sts.initial());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Obtain all Deadlocked states, i.e. all states without a successor.
///
/// \details Note, this operation seems nicely parallelizeable (if the BDD package is threadsafe!)
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
typename Adapter::dd_t
deadlock(Adapter& adapter,
         const symbolic_transition_system<Adapter>& sts,
         const typename Adapter::dd_t& states)
{
  auto result = states;
  for (const auto& t : sts.transitions()) {
    result &= ~adapter.relprev(states, t.relation(), t.support());
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Struct to hold the result of the SCC algorithm
///
/// \see scc
////////////////////////////////////////////////////////////////////////////////////////////////////
struct scc_summary
{
  /// \brief Number of SCCs found.
  size_t count = 0;

  // TODO: Number of Bottom SCCs found.
  // size_t bottom_count = 0;

#ifdef BDD_BENCHMARK_STATS
  /// \brief Number of states in largest SCC.
  size_t max_states = std::numeric_limits<size_t>::min();

  /// \brief Number of states in smallest SCC.
  size_t min_states = std::numeric_limits<size_t>::max();

  /// \brief Size of largest Decision Diagram for an SCC.
  size_t max_dd = std::numeric_limits<size_t>::min();

  /// \brief Size of smallest Decision Diagram for an SCC.
  size_t min_dd = std::numeric_limits<size_t>::max();
#endif // BDD_BENCHMARK_STATS
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Obtain a list of all Strongly Connected Components (SCCs) in the given reachable set of
///        states.
///
/// \details This is the Chain algorithm from Larsen et al. 'A Truly Symbolic Linear Time Algorithm
///          for SCC Decomposition' [TACAS 23].
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
scc_summary
scc(Adapter& adapter,
    const symbolic_transition_system<Adapter>& sts,
    const typename Adapter::dd_t reachable)
{
  // Output struct
  scc_summary out;

  // Stack with {vertices, pivots}
  std::vector<std::pair<typename Adapter::dd_t, typename Adapter::dd_t>> call_stack = {
    { reachable, adapter.bot() }
  };

  // Decision Diagrams that are reused throughout
  constexpr auto prime_pre = symbolic_transition_system<Adapter>::prime::pre;

  const typename Adapter::dd_t bot_dd   = adapter.bot();
  const typename Adapter::dd_t var_cube = adapter.cube([](int x) { return x % 2 == prime_pre; });

  // Execute call stack
  while (!call_stack.empty()) {
    // Pop latest from call stack
    const typename Adapter::dd_t vertices = call_stack.back().first;
    assert(vertices != bot_dd);

    const typename Adapter::dd_t pivots = call_stack.back().second;
    assert(pivots == bot_dd || (pivots & vertices) != bot_dd);

    call_stack.pop_back();

    // Pick a pivot on the chain, if possible.
    const typename Adapter::dd_t pivot =
      adapter.satone(pivots != bot_dd ? pivots : vertices, var_cube);
    assert(pivot != bot_dd);

    // Compute forward(v, V) and backwards(v, forward(v, V)), i.e. SCC(v).
    const auto [forward_set, latest_layer] = forwards_layer(adapter, sts, pivot, vertices);
    const auto pivot_scc                   = backwards(adapter, sts, pivot, forward_set);
    assert(pivot_scc != bot_dd);

    // Output SCC(v)
    out.count += 1;
    // out.bottom_count += TODO;

#ifdef BDD_BENCHMARK_STATS
    {
      const size_t scc_size = adapter.satcount(pivot_scc, sts.varcount(prime_pre));
      out.min_states        = std::min(out.min_states, scc_size);
      out.max_states        = std::max(out.max_states, scc_size);
    }
    {
      const size_t dd_size = adapter.nodecount(pivot_scc);
      out.min_dd           = std::min(out.min_dd, dd_size);
      out.max_dd           = std::max(out.max_dd, dd_size);
    }
#endif // BDD_BENCHMARK_STATS

    { // "Recursive" call on the forward set
      const typename Adapter::dd_t forward_vertices = forward_set - pivot_scc;
      const typename Adapter::dd_t forward_pivots   = latest_layer - pivot_scc;

      if (forward_vertices != bot_dd) {
        call_stack.push_back({ forward_vertices, forward_pivots });
      }
    }
    { // "Recursive" call on the rest
      const typename Adapter::dd_t rest_vertices = vertices - forward_set;

      typename Adapter::dd_t rest_pivots = bot_dd;
      for (const auto& t : sts.transitions()) {
        rest_pivots |= adapter.relprev(pivot_scc, t.relation(), t.support());
      }
      rest_pivots = (rest_pivots - forward_set) & rest_vertices;

      if (rest_vertices != bot_dd) { call_stack.push_back({ rest_vertices, rest_pivots }); }
    }
  }
  return out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Adapter>
int
run_mcnet(int argc, char** argv)
{
  const bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  if (path == "") {
    std::cerr << "Input file not specified\n";
    return -1;
  }

  // Parse input file and derive variable order.
  const transition_system ts = parse_file(path);
  const variable_permutation vp(ts);

  return run<Adapter>("mcnet", 2 * ts.vars().size(), [&](Adapter& adapter) {
    constexpr auto prime_pre = symbolic_transition_system<Adapter>::prime::pre;

    const time_point sts_before = now();
    const symbolic_transition_system sts(adapter, std::move(ts), std::move(vp));
    const time_point sts_after = now();

    std::cout << json::field("variable order") << json::value(to_string(var_order)) << json::comma
              << json::endl;

    std::cout << json::field("net") << json::brace_open << json::endl;
    std::cout << json::field("path") << json::value(path) << json::comma << json::endl;
    std::cout << json::field("places") << json::value(sts.varcount() / 2) << json::comma
              << json::endl;
    std::cout << json::field("transitions") << json::value(sts.transitions().size()) << json::comma
              << json::endl;
    std::cout << json::field("input size (bytes)") << json::value(sts.bytes()) << json::comma
              << json::endl;
    std::cout << json::field("symbolic size (nodes)") << json::value(sts.nodecount()) << json::comma
              << json::endl;
    std::cout << json::field("time (ms)") << json::value(duration_ms(sts_before, sts_after))
              << json::endl;
    std::cout << json::brace_close << json::comma << json::endl;

    std::cout << json::endl;

    time_duration total_time = 0;

    typename Adapter::dd_t reachable_states = sts.all();

    std::cout << json::field("initial") << json::brace_open << json::endl;
    std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(sts.initial()))
              << json::comma << json::endl;
    std::cout << json::field("satcount (states)")
              << json::value(adapter.satcount(sts.initial(), sts.varcount(prime_pre)))
              << json::endl;
    std::cout << json::brace_close << json::comma << json::endl;

    std::cout << json::field("invariant") << json::brace_open << json::endl;
    std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(sts.all()))
              << json::comma << json::endl;
    std::cout << json::field("satcount (states)")
              << json::value(adapter.satcount(sts.initial(), sts.varcount(prime_pre)))
              << json::endl;
    std::cout << json::brace_close << json::comma << json::endl;

    if (analysis_flags[analysis::REACHABILITY]) {
      std::cout << json::field(to_string(analysis::REACHABILITY)) << json::brace_open << json::endl;

      const time_point t1 = now();
      reachable_states    = forwards(adapter, sts);
      const time_point t2 = now();

      const time_duration time = duration_ms(t1, t2);
      total_time += time;

      std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(reachable_states))
                << json::comma << json::endl;
      std::cout << json::field("satcount (states)")
                << json::value(adapter.satcount(reachable_states, sts.varcount(prime_pre)))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)") << json::value(time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }
    if (analysis_flags[analysis::DEADLOCK]) {
      std::cout << json::field(to_string(analysis::DEADLOCK)) << json::brace_open << json::endl;

      const time_point t1                          = now();
      const typename Adapter::dd_t deadlock_states = deadlock(adapter, sts, reachable_states);
      const time_point t2                          = now();

      const time_duration time = duration_ms(t1, t2);
      total_time += time;

      std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(deadlock_states))
                << json::comma << json::endl;
      std::cout << json::field("satcount (states)")
                << json::value(adapter.satcount(deadlock_states, sts.varcount(prime_pre)))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)") << json::value(time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }
    if (analysis_flags[analysis::SCC]) {
      std::cout << json::field(to_string(analysis::SCC)) << json::brace_open << json::endl;

      const time_point t1           = now();
      const scc_summary scc_summary = scc(adapter, sts, reachable_states);
      const time_point t2           = now();

      const time_duration time = duration_ms(t1, t2);
      total_time += time;

      std::cout << json::field("components") << json::value(scc_summary.count) << json::comma
                << json::endl;
      // std::cout << json::field("components (attractors)") <<
      // json::value(scc_summary.bottom_count)
      //           << json::comma << json::endl;

#ifdef BDD_BENCHMARK_STATS
      std::cout << json::field("min SCC (states)") << json::value(scc_summary.min_states)
                << json::comma << json::endl;
      std::cout << json::field("max SCC (states)") << json::value(scc_summary.max_states)
                << json::comma << json::endl;

      std::cout << json::field("min SCC (nodes)") << json::value(scc_summary.min_dd) << json::comma
                << json::endl;
      std::cout << json::field("max SCC (nodes)") << json::value(scc_summary.max_dd) << json::comma
                << json::endl;
#endif // BDD_BENCHMARK_STATS

      std::cout << json::field("time (ms)") << json::value(time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }

    std::cout << json::field("total time (ms)") << json::value(init_time + total_time)
              << json::endl;
    return 0;
  });
}
