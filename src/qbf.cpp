#include "common.cpp"

// Algorithms and Operations
#include <algorithm>
#include <cmath>
#include <regex>

// Data Structures
#include <array>
#include <queue>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

// Files
#include <stdio.h>
#include <fstream>
#include <istream>
#include <sstream>

// Types
#include <cctype>
#include <type_traits>

// Other
#include <functional>
#include <stdexcept>
#include <utility>

// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

// ========================================================================== //
// QCir Class and Construction

////////////////////////////////////////////////////////////////////////////////
/// \brief Circuit for a Quantified Boolean Formula (QBF) in the QCIR format.
////////////////////////////////////////////////////////////////////////////////
class qcir
{
  // ======================================================================== //
  // Circuit Gates
public:
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Boolean Constant gate.
  ///
  /// \details The standard specifies that an empty 'and' gate is equal to the
  ///          base case of its accumulation, i.e. `true`. Similarly, an empty
  ///          'or' gate is equivalent to the constant `false`. We immediately
  ///          convert this, as needed.
  //////////////////////////////////////////////////////////////////////////////
  struct const_gate
  {
    ////////////////////////////////////////////////////////////////////////////
    /// \brief Binary Constant to use.
    ////////////////////////////////////////////////////////////////////////////
    bool val;

    ////////////////////////////////////////////////////////////////////////////
    const_gate() = default;
    const_gate(const const_gate&) = default;
    const_gate(const_gate&&) = default;

    const_gate(const bool v) : val(v)
    { }

    ////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      std::stringstream ss;
      ss << "const( " << val << " )";
      return ss.str();
    }
  };

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Input Variable gate.
  //////////////////////////////////////////////////////////////////////////////
  struct var_gate
  {
    ////////////////////////////////////////////////////////////////////////////
    /// \brief Input Literal.
    ////////////////////////////////////////////////////////////////////////////
    int var;

    ////////////////////////////////////////////////////////////////////////////
    var_gate() = default;
    var_gate(const var_gate&) = default;
    var_gate(var_gate&&) = default;

    var_gate(const int x) : var(x)
    { }

    ////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      std::stringstream ss;
      ss << "var( " << var << " )";
      return ss.str();
    }
  };

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Binary Boolean Operator gate.
  ///
  /// \details While the standard specifies the XOR gate only is applicable to a
  ///          pair of literals, we allow the more general case of more.
  //////////////////////////////////////////////////////////////////////////////
  struct ngate
  {
    ////////////////////////////////////////////////////////////////////////////
    /// \brief Binary Boolean Operator type.
    ////////////////////////////////////////////////////////////////////////////
    enum type_t { AND, OR, XOR };

    ////////////////////////////////////////////////////////////////////////////
    /// \brief Derives the `type_t` from a string
    ////////////////////////////////////////////////////////////////////////////
    static enum type_t
    parse_type(const std::string &s)
    {
      if (ascii_tolower(s) == "and") { return AND; }
      if (ascii_tolower(s) == "or")  { return OR; }
      if (ascii_tolower(s) == "xor") { return XOR; }

      throw std::domain_error("Unknown Boolean Operator" + s);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// \brief Operator to be applied in this gate.
    ////////////////////////////////////////////////////////////////////////////
    type_t nGateype;

    ////////////////////////////////////////////////////////////////////////////
    /// \brief List of literals to accumulate with the operator.
    ///
    /// \todo This could be turned into an unordered_set to improve performance.
    ////////////////////////////////////////////////////////////////////////////
    std::vector<int> lit_list;

    ////////////////////////////////////////////////////////////////////////////
    ngate() = default;
    ngate(const ngate&) = default;
    ngate(ngate&&) = default;

    ngate(const type_t& ng_t,
               const std::vector<int>& lits)
      : nGateype(ng_t), lit_list(lits)
    { }

    ngate(const std::string& ng_t,
               const std::vector<int>& lits)
      : nGateype(parse_type(ng_t)), lit_list(lits)
    { }

    ////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      std::stringstream ss;
      ss << (  nGateype == AND ? "and"
            :  nGateype == OR  ? "or"
            :/*nGateype == XOR*/ "xor")
         << "( ";
      for (const int i : lit_list) { ss << i << " "; }
      ss << ")";
      return ss.str();
    }
  };

  //////////////////////////////////////////////////////////////////////////////
  /// \brief If-Then-Else gate.
  //////////////////////////////////////////////////////////////////////////////
  struct ite_gate
  {
    ////////////////////////////////////////////////////////////////////////////
    /// \brief List of literals 'if', 'then', and 'else'.
    ////////////////////////////////////////////////////////////////////////////
    int lits[3];

    ////////////////////////////////////////////////////////////////////////////
    ite_gate() = default;
    ite_gate(const ite_gate&) = default;
    ite_gate(ite_gate&&) = default;

    ite_gate(const int g_if, const int g_then, const int g_else)
      : lits{ g_if, g_then, g_else }
    { }

    ////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      std::stringstream ss;
      ss << "ite( " << lits[0] << ", " << lits[1] << ", " << lits[2] << " )";
      return ss.str();
    }
  };

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Quantification Gate
  //////////////////////////////////////////////////////////////////////////////
  struct quant_gate
  {
    ////////////////////////////////////////////////////////////////////////////
    /// \brief Possible Quantification operators.
    ////////////////////////////////////////////////////////////////////////////
    enum type_t { EXISTS, FORALL };

    ////////////////////////////////////////////////////////////////////////////
    /// \brief Derives the `type_t` from a string
    ////////////////////////////////////////////////////////////////////////////
    static enum type_t
    parse_type(const std::string &s)
    {
      if (ascii_tolower(s) == "exists") { return EXISTS; }
      if (ascii_tolower(s) == "forall") { return FORALL; }

      throw std::domain_error("Unknown Quantifier: " + s);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// \brief Quantification operation to be applied.
    ////////////////////////////////////////////////////////////////////////////
    type_t quant;

    ////////////////////////////////////////////////////////////////////////////
    /// \brief List of variables to be quantified.
    ///
    /// \todo Change into unordered_set for O(1) lookup.
    ////////////////////////////////////////////////////////////////////////////
    std::set<int> vars;

    ////////////////////////////////////////////////////////////////////////////
    /// \brief Gate with subformula that quantified over.
    ////////////////////////////////////////////////////////////////////////////
    int lit;

    ////////////////////////////////////////////////////////////////////////////
    quant_gate() = default;
    quant_gate(const quant_gate&) = default;
    quant_gate(quant_gate&&) = default;

    quant_gate(const type_t Q, const std::set<int> &vs, int l)
      : quant(Q), vars(vs), lit(l)
    {
#ifndef NDEBUG
      for (const int x : vars) {
        assert(0 <= x);
      }
#endif
    }

    ////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      std::stringstream ss;
      ss << (  quant == EXISTS ? "exists" : /*quant == FORALL*/ "forall")
         << "( " ;
      for (const int i : vars) { ss << i << " "; }
      ss << "; " << lit << " )";
      return ss.str();
    }
  };

  struct output_gate
  {
    ////////////////////////////////////////////////////////////////////////////
    /// \brief Root of formula.
    ////////////////////////////////////////////////////////////////////////////
    int lit;

    ////////////////////////////////////////////////////////////////////////////
    output_gate() = default;
    output_gate(const output_gate&) = default;
    output_gate(output_gate&&) = default;

    output_gate(int l)
      : lit(l)
    { }

    ////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    {
      std::stringstream ss;
      ss << "output( " << lit << " )";
      return ss.str();
    }
  };

  class gate
    : private std::variant<const_gate, var_gate, ngate, ite_gate, quant_gate, output_gate>
  {
  public:
    using variant_t =
      std::variant<const_gate, var_gate, ngate, ite_gate, quant_gate, output_gate>;

  public:
    ////////////////////////////////////////////////////////////////////////////
    template<typename Gate>
    bool
    is() const
    { return std::holds_alternative<Gate>(*this); }

    ////////////////////////////////////////////////////////////////////////////
    template<typename Gate>
    const Gate&
    as() const
    { return std::get<Gate>(*this); }

    template<typename Gate>
    Gate&
    as()
    { return std::get<Gate>(*this); }

    ////////////////////////////////////////////////////////////////////////////
    template<class T>
    auto
    match(T&& vis) const
    {
      return std::visit<T, variant_t>(std::forward<T>(vis), gate(*this));
    }

    template<class... Ts>
    auto
    match(Ts... args) const
    {
      return match<overload<Ts...>>(overload { args... });
    }

    // TODO: match(Ts...) with 'overloaded' trick.

  public:
    ////////////////////////////////////////////////////////////////////////////
    /// \brief Minimum length of a path from a gate to a variable gate.
    ////////////////////////////////////////////////////////////////////////////
    const size_t depth;

    ////////////////////////////////////////////////////////////////////////////
    /// \brief Number of gates referencing this one.
    ////////////////////////////////////////////////////////////////////////////
    size_t refcount = 0;

  public:
    ////////////////////////////////////////////////////////////////////////////
    gate() = delete;
    gate(const gate& g) = default;
    gate(gate&& g) = default;

    template<typename Gate>
    gate(const size_t d, const Gate &g)
      : variant_t(g), depth(d)
    { __check_self(); }

    template<typename Gate>
    gate(const size_t d, Gate &&g)
      : variant_t(std::forward<Gate>(g)), depth(d)
    { __check_self(); }

  public:
    ////////////////////////////////////////////////////////////////////////////
    std::string
    to_string() const
    { return match([](const auto &g) -> std::string { return g.to_string(); }); }

  private:
    ////////////////////////////////////////////////////////////////////////////
    void __check_self()
    {
      if (std::holds_alternative<const_gate>(*this) && depth > 0) {
        std::cerr << "Warning: Constant gate with non-zero depth created!";
      } else if (std::holds_alternative<var_gate>(*this) && depth > 0) {
        std::cerr << "Warning: Variable gate with non-zero depth created!";
      }
    }
  };

private:
  // ======================================================================== //
  // Members

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Number of Input Variables.
  //////////////////////////////////////////////////////////////////////////////
  size_t m_vars  = 0u;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Number of Gates
  //////////////////////////////////////////////////////////////////////////////
  size_t m_size  = 0u;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Depth of Circuit
  //////////////////////////////////////////////////////////////////////////////
  size_t m_depth = 0u;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Map from Gate Variables to Index of Gate (if any).
  //////////////////////////////////////////////////////////////////////////////
  std::unordered_map<std::string, size_t> m_gvar_map;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Inversed map from Cleansed Gate Variable to Var Name.
  //////////////////////////////////////////////////////////////////////////////
  std::unordered_map<size_t, std::string> m_gvar_invmap;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Inversed map from Cleansed Input Variables to Var Name.
  ///
  /// \details If you want to find the Cleansed Input Variable from its name,
  ///          just use `m_gvar_map` to find the gate and its `var` member.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<std::string> m_var_invmap;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief List of gates.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate> m_circuit;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Whether the output gate has already been created.
  //////////////////////////////////////////////////////////////////////////////
  bool m_has_output_gate = false;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Number of gates with a refcount of 0.
  //////////////////////////////////////////////////////////////////////////////
  size_t m_roots = 0u;

private:
  // ======================================================================== //
  static constexpr int const_idx[2] = { 1+false, 1+true };

public:
  // ======================================================================== //
  // Constructors

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Creates a QCircuit without any gates.
  //////////////////////////////////////////////////////////////////////////////
  qcir()
  {
    // Dummy at index [0]
    const gate dummy_gate(0u, output_gate(0));
    m_circuit.push_back(dummy_gate);

    // Boolean Constant gates
    // - False at const_idx[false] = 1+false = 1
    __push_gate(0u, const_gate(false));
    // - True  at const_idx[true]  = 1+true  = 2
    __push_gate(0u, const_gate(true));

    // Sanity Checks
    assert(m_circuit.size() == 3u + m_size);
    assert(m_roots == 0u);
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Copy-constructor for QCircuit
  //////////////////////////////////////////////////////////////////////////////
  qcir(const qcir&) = default;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Move-constructor for QCircuit
  //////////////////////////////////////////////////////////////////////////////
  qcir(qcir&&) = default;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Constructs a QCircuit as defined in the QCIR-G14 file at the given
  ///        path.
  //////////////////////////////////////////////////////////////////////////////
  qcir(const std::string &path) : qcir()
  {
    std::ifstream is(path);

    // Go to first non-empty line.
    std::string line;
    while (std::getline(is, line)) {
      if (line.size() > 0) break;
    }

    // Derive input format from first line
    if (line.find("QCIR-G14") != std::string::npos) {
      __parse_qcir(is);
    } else {
      throw std::runtime_error("File '" + path + "' is of an unknown format.");
    }
  }

private:
  // ======================================================================== //
  // Constructor Helper Functions Parsing Input
  std::vector<std::string>
  __parse_lit_list(std::string gvar_list, const std::string &gvar_pattern)
  {
    std::regex gvar_regex(gvar_pattern);
    std::vector<std::string> vars;

    for (auto i = std::sregex_iterator(gvar_list.begin(), gvar_list.end(), gvar_regex);
         i != std::sregex_iterator();
         ++i) {
      vars.push_back((*i)[0].str());
    }
    return vars;
  }

  void
  __parse_qcir(std::istream &is)
  {
    const std::string s = "\\s";
    const std::string ss = s + "*";
    const std::string lparen = "\\(";
    const std::string rparen = "\\)";

    const std::string kw_output = "[Oo][Uu][Tt][Pp][Uu][Tt]";
    const std::string kw_and    = "[Aa][Nn][Dd]";
    const std::string kw_or     = "[Oo][Rr]";
    const std::string kw_xor    = "[Xx][Oo][Rr]";
    const std::string kw_ite    = "[Ii][Tt][Ee]";
    const std::string kw_exists = "[Ee][Xx][Ii][Ss][Tt][Ss]";
    const std::string kw_forall = "[Ff][Oo][Rr][Aa][Ll][Ll]";

    const std::string var = "-?\\w+";
    const std::string gvar_list = "[\\-\\w,;\\s]+";

    // Buffers for Prenex and Output Gate
    std::vector<std::pair<const quant_gate::type_t, std::vector<std::string>>> prenex_buffer;
    std::string output_gate;

    { // Parse Prenex and Output
      const std::regex quant_regex("(" + kw_forall + "|" + kw_exists + ")" + ss +
                                   lparen + "(" + gvar_list + ")" + rparen);

      const std::regex output_regex(kw_output + ss + lparen + ss + "(" + var + ")" + ss + rparen);

      for (std::string line; std::getline(is, line); ) {
        // Skip empty lines
        if (line.size() == 0) continue;

        { // Parse Quantifier
          auto i = std::sregex_iterator(line.begin(), line.end(), quant_regex);
          if (i != std::sregex_iterator()) {
            const auto Q    = quant_gate::parse_type((*i)[1].str());
            const auto args = __parse_lit_list((*i)[2].str(), var);
            prenex_buffer.push_back(std::make_pair(Q, args));

            continue;
          }
        }

        { // Output Gate
          auto i = std::sregex_iterator(line.begin(), line.end(), output_regex);
          if (i != std::sregex_iterator()) {
            output_gate = (*i)[1].str();
            break;
          }
        }

        throw std::runtime_error("Unable to parse line '" + line + "'");
      }
    }

    { // Parse Circuit Gates
      const std::regex assignment_regex("(" + var + ")" + ss + "=");

      const std::regex ngate_kw_regex("=" + ss + "(" + kw_and + "|" + kw_or + "|" + kw_xor + ")");
      const std::regex ite_kw_regex("=" + ss + "(" + kw_ite + ")");
      const std::regex quant_kw_regex("=" + ss + "(" + kw_exists + "|" + kw_forall + ")");

      const std::regex litlist_regex(".*" + lparen + "(" + gvar_list + ")" + rparen);

      for (std::string line; std::getline(is, line); ) {
        // Skip empty lines
        if (line.size() == 0) continue;

        // Obtain 'gvar', i.e. the name of the gate.
        std::string gvar;
        { auto i = std::sregex_iterator(line.begin(), line.end(), assignment_regex);
          if (i == std::sregex_iterator()) {
            throw std::runtime_error("Unable to match gvar on line '" + line + "'");
          }
          gvar = output_gate = (*i)[1].str();
          assert(++i == std::sregex_iterator());
        }

        // Obtain 'lit-list', i.e. input wires/variable names to gate.
        std::vector<std::string> args;
        { auto i = std::sregex_iterator(line.begin(), line.end(), litlist_regex);
          if (i != std::sregex_iterator()) {
            args = __parse_lit_list((*i)[1].str(), var);
            assert(++i == std::sregex_iterator());
          }
          // TODO: check the line is not malformed?
        }

        // Obtain 'stmt' type, i.e. the keyword marking the type of gate. Based
        // on this, the specific gates are created.
        { // ------------------------
          // Case: Quant-Gate
          { auto i = std::sregex_iterator(line.begin(), line.end(), quant_kw_regex);
            if (i != std::sregex_iterator()) {
              const auto Q = quant_gate::parse_type((*i)[1].str());
              assert(++i == std::sregex_iterator());

              const auto input_gvar = args.back();
              args.pop_back();
              add_quant_gate(gvar, Q, args, input_gvar);
              continue;
            }
          }

          // ------------------------
          // Case: NGate
          { auto i = std::sregex_iterator(line.begin(), line.end(), ngate_kw_regex);
            if (i != std::sregex_iterator()) {
              const auto ng_t = ngate::parse_type((*i)[1].str());
              assert(++i == std::sregex_iterator());

              add_ngate(gvar, ng_t, args);
              continue;
            }
          }

          // ------------------------
          // Case: ITE-Gate
          { auto i = std::sregex_iterator(line.begin(), line.end(), ite_kw_regex);
            if (i != std::sregex_iterator()) {
              assert(++i == std::sregex_iterator());

              add_ite_gate(gvar, args);
              continue;
            }
          }
        }
        throw std::runtime_error("Unable to match type of statement on line '" + line + "'");
      }
    }

    // Create the buffered Output Gate and Prenex Quantification
    int root = add_output_gate(output_gate);

    for (auto it = prenex_buffer.rbegin(); it != prenex_buffer.rend(); ++it) {
      root = add_quant_gate((*it).first, (*it).second, root);
    }
  }

public:
  // ======================================================================== //
  // Access

private:
  //////////////////////////////////////////////////////////////////////////////
  inline
  gate&
  __at(const int i)
  {
    const size_t idx = std::abs(i);
    if (idx == 0 || m_circuit.size() <= idx) {
      throw std::out_of_range("Given Index '" + std::to_string(i) + "' is out-of-bounds");
    }
    return m_circuit.at(idx);
  }

public:
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain access to the gate at some Unique Index.
  //////////////////////////////////////////////////////////////////////////////
  const gate&
  at(const int i) const
  { return const_cast<qcir*>(this)->__at(i); }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief An iterator to the beginning.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::iterator
  begin()
  {
    return ++(m_circuit.begin());
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief A `const` iterator to the beginning.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::const_iterator
  cbegin() const
  {
    return ++(m_circuit.cbegin());
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief An index to the beginning.
  //////////////////////////////////////////////////////////////////////////////
  int
  begin_idx() const
  { return 1; }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief An iterator to the end.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::iterator
  end()
  {
    return m_circuit.end();
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief A `const` iterator to the end.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::const_iterator
  cend() const
  {
    return m_circuit.cend();
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief An index to the end.
  //////////////////////////////////////////////////////////////////////////////
  int
  end_idx() const
  { return m_circuit.size(); }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Index of a root. Specifically the root at `end_idx() - 1`.
  //////////////////////////////////////////////////////////////////////////////
  int
  root_idx() const
  { return end_idx() - 1; }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief A reversed iterator to the beginning.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::reverse_iterator
  rbegin()
  {
    return m_circuit.rbegin();
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief A `const` reversed iterator to the beginning.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::const_reverse_iterator
  crbegin() const
  {
    return m_circuit.crbegin();
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief A reversed iterator to the end.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::reverse_iterator
  rend()
  {
    return --(m_circuit.rend());
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief A `const` reversed iterator to the end.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<gate>::const_reverse_iterator
  crend() const
  {
    return --(m_circuit.crend());
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Index to mark 'out-of-bounds'.
  ///
  /// \internal The index to the dummy gate placed at [0].
  //////////////////////////////////////////////////////////////////////////////
  static constexpr int npos = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain the index for a gate variable of a specific name. If it does
  ///        not exist, then it returns `npos`.
  //////////////////////////////////////////////////////////////////////////////
  int
  find(const std::string &gvar) const noexcept
  {
    if (gvar.size() == 0) return npos;

    const bool negated = gvar.at(0) == '-';
    const std::string map_key = negated ?  gvar.substr(1) : gvar;
    if (gvar.size() == 0) return npos;

    const auto gvar_map_res = m_gvar_map.find(map_key);
    if (gvar_map_res == m_gvar_map.end()) return npos;
    return (negated ? -1 : 1) * (*gvar_map_res).second;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief The `find(const std::string &)` lifted to an iterated list.
  //////////////////////////////////////////////////////////////////////////////
  template<typename IT, typename ret_t = std::vector<int>>
  ret_t
  find(IT begin, IT end) const noexcept
  {
    ret_t res;
    while (begin != end) {
      res.push_back(find(*(begin++)));
    }
    return res;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain the human-readable name of a gate variable. If it has
  ///        no-name, then the empty string is returned.
  //////////////////////////////////////////////////////////////////////////////
  std::string
  gvar(int i) const noexcept
  {
    const auto gvar_invmap_res = m_gvar_invmap.find(std::abs(i));
    if (gvar_invmap_res == m_gvar_invmap.end()) {
      return "";
    }
    return (i < 0 ? "-" : "") + (*gvar_invmap_res).second;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain the human-readable name of an input variable.
  ///
  /// \pre `0 <= i < vars()`
  //////////////////////////////////////////////////////////////////////////////
  std::string
  var(int i) const
  {
    if (vars() <= static_cast<unsigned int>(std::abs(i))) {
      throw std::out_of_range("Given 'i' is an unknown variable");
    }
    return (i < 0 ? "-" : "") + m_var_invmap.at(std::abs(i));
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Number of unique variables within the circuit.
  //////////////////////////////////////////////////////////////////////////////
  size_t
  vars() const noexcept
  {
    return m_vars;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Number of gates within the circuit.
  //////////////////////////////////////////////////////////////////////////////
  size_t
  size() const noexcept
  {
    return m_size
      - (at(const_idx[false]).refcount == 0u)
      - (at(const_idx[true]).refcount == 0u);
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Depth of the circuit.
  //////////////////////////////////////////////////////////////////////////////
  size_t
  depth() const noexcept
  {
    return m_depth;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Number of roots in the circuit.
  //////////////////////////////////////////////////////////////////////////////
  size_t
  roots() const noexcept
  {
    return m_roots;
  }

public:
  // ======================================================================== //
  // Builder Functions

  // TODO: Move 'gvar' variable to be the final parameter. This makes it more
  //       natural to be an optional argument.

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Add a Variable gate to the circuit. If it already exists, then the
  ///        prior made one is returned.
  ///
  /// \param var Human-readable variable name.
  ///
  /// \returns Unique Index of the constructed gate.
  //////////////////////////////////////////////////////////////////////////////
  int
  add_var_gate(const std::string var)
  {
    if (var.size() == 0) {
      throw std::invalid_argument("Cannot create var gate: ''");
    }

    const int cached = find(var);
    if (cached != npos) {
      return cached;
    }

    const bool negated = var.at(0) == '-';
    const std::string unnegated_var = negated ? var.substr(1) : var;

    if (unnegated_var.size() == 0) {
      throw std::invalid_argument("Cannot create var gate: '-'");
    }

    const int cvar = m_vars++;
    m_var_invmap.push_back(unnegated_var);
    return (negated ? -1 : 1) * __push_gate(unnegated_var, 0u, var_gate(cvar));
  }

  // TODO: var_gate without 'var' parameter

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Adds a Binary Operator gate.
  ///
  /// \returns Unique identifier of the constructed gate.
  //////////////////////////////////////////////////////////////////////////////
  int
  add_ngate(const std::string gvar,
            const ngate::type_t& ng_t,
            const std::vector<int>& lits)
  {
    if (m_has_output_gate) {
      throw std::invalid_argument("Cannot create an NGATE gate after having created the OUTPUT gate");
    }

    // --------------------------
    // Case: Empty lit-list
    if (lits.size() == 0) {
      if (ng_t == ngate::XOR) { // TODO: remove and just allow empty XOR?
        throw std::invalid_argument("Cannot create an XOR gate with 0 inputs.");
      }
      const int ret_idx = const_idx[ng_t == ngate::AND];
      __assoc_idx(gvar, ret_idx);
      return ret_idx;
    }

    // --------------------------
    // Case: Singleton lit-list
    if (lits.size() == 1) {
      // For all three operations, we can just skip creating a gate and provide
      // the single child instead.
      const int ret_idx = lits.at(0);
      __assoc_idx(gvar, ret_idx);
      return ret_idx;
    }

    // --------------------------
    // Case: 2+ lit-list
    size_t g_depth = 0;
    for (const int i : lits) {
      gate &g = __at(i);
      __inc_refcount(g);
      g_depth = std::max(g_depth, g.depth + 1);
    }
    return __push_gate(gvar, g_depth, ngate(ng_t, lits));
  }

  int
  add_ngate(const std::string gvar,
            const std::string& ng_t,
            const std::vector<int>& lits)
  { return add_ngate(gvar, ngate::parse_type(ng_t), lits); }

  int
  add_ngate(const std::string gvar,
            const ngate::type_t& ng_t,
            const std::vector<std::string>& lits)
  { return add_ngate(gvar, ng_t, __find_or_add(lits.begin(), lits.end())); }

  // TODO: 'add_ngate' without 'gvar' parameter

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Adds an If-Then-Else gate.
  ///
  /// \returns Unique identifier of the constructed gate.
  //////////////////////////////////////////////////////////////////////////////
  template<typename Lits>
  int
  add_ite_gate(const std::string gvar,
               const Lits& lits)
  {
    if (m_has_output_gate) {
      throw std::invalid_argument("Cannot create an ITE gate after having created the OUTPUT gate");
    }
    if (lits.size() != 3) {
      throw std::invalid_argument("An ITE gate ought to have three arguments");
    }

    size_t g_depth = 0u;
    for (auto it = lits.begin(); it != lits.end(); ++it) {
      gate &g = __at(*it);
      __inc_refcount(g);
      g_depth = std::max(g_depth, g.depth + 1);
    }
    return __push_gate(gvar, g_depth, ite_gate(lits[0], lits[1], lits[2]));
  }

  int
  add_ite_gate(const std::string gvar,
               const std::vector<std::string> lits)
  {
    return add_ite_gate(gvar, find(lits.begin(), lits.end()));
  }

  int
  add_ite_gate(const std::string gvar,
               const int g_if,
               const int g_then,
               const int g_else)
  { return add_ite_gate(gvar, std::array<int, 3>{ g_if, g_then, g_else }); }


  int
  add_ite_gate(const std::string gvar,
               const std::string g_if,
               const std::string g_then,
               const std::string g_else)
  {
    return add_ite_gate(gvar, std::array<int, 3>{ __find_or_add(g_if),
                                                 __find_or_add(g_then),
                                                 __find_or_add(g_else) });
  }

  // TODO: ite_gate without 'gvar' parameter

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Adds Variable Quantification gate.
  ///
  /// \returns Unique identifier of the constructed gate.
  //////////////////////////////////////////////////////////////////////////////
  template<typename IT>
  int
  add_quant_gate(const quant_gate::type_t Q,
                 IT vars_begin, IT vars_end,
                 int i)
  {
    if (m_has_output_gate && i != root_idx()) {
      throw std::invalid_argument("'i' cannot point to anything but the root in the Prenex");
    }

    // --------------------------
    // Case: Empty var-list
    if (vars_begin == vars_end) {
      return i;
    }

    // --------------------------
    // Case: Non-empty var-list
    gate &i_gate = __at(i);
    const size_t g_depth = i_gate.depth + 1;

    std::set<int> int_vars;
    while (vars_begin != vars_end) {
      const auto quant_var = *(vars_begin++);
      const auto quant_var_idx = find(quant_var);

      if (quant_var_idx == npos) {
        std::cerr << "Skipping variable '" << quant_var << "' that never has been mentioned" << std::endl;
        continue;
      }
      if (quant_var_idx < 0) {
        throw std::invalid_argument("Quantified variable '" + quant_var + "' cannot be negated");
      }

      gate& g = __at(quant_var_idx);
      if (!g.is<var_gate>()) {
        throw std::invalid_argument("Quantified variable '" + quant_var + "' refers to a non-var gate");
      }

      // TODO: check 'g' actually is a transitive child of 'i_gate'.
      int_vars.insert(g.as<var_gate>().var);
    }

    // Pruned var-list ended up being empty?
    if (int_vars.empty()) {
      return i;
    }

    // ---------------------------------------------------
    // Case: Consecutively the same quantifier in Prenex
    if (m_has_output_gate && i_gate.is<quant_gate>() && i_gate.as<quant_gate>().quant == Q) {
      // TODO: add 'int_vars' to 'i_gate'
    }

    // ---------------------------------------------------
    // Indeed, create a new gate
    __inc_refcount(i_gate);
    return __push_gate(g_depth, quant_gate(Q, int_vars, i));
  }

  template<typename IT>
  int
  add_quant_gate(const quant_gate::type_t Q,
                 IT vars_begin, IT vars_end,
                 std::string i)
  {
    return add_quant_gate(Q, vars_begin, vars_end, find(i));
  }

  int
  add_quant_gate(const quant_gate::type_t Q,
                 std::vector<std::string> vars,
                 int i)
  { return add_quant_gate(Q, vars.begin(), vars.end(), i); }

  int
  add_quant_gate(const quant_gate::type_t Q,
                 std::vector<std::string> vars,
                 std::string i)
  { return add_quant_gate(Q, vars.begin(), vars.end(), find(i)); }

  template<typename IT>
  int
  add_quant_gate(const std::string gvar,
                 const quant_gate::type_t Q,
                 IT vars_begin, IT vars_end,
                 int i)
  {
    const int idx = add_quant_gate(Q, vars_begin, vars_end, i);
    __assoc_idx(gvar, idx);
    return idx;
  }

  int
  add_quant_gate(const std::string gvar,
                 const quant_gate::type_t Q,
                 std::vector<std::string> vars,
                 int i)
  { return add_quant_gate(gvar, Q, vars.begin(), vars.end(), i); }

  int
  add_quant_gate(const std::string gvar,
                 const quant_gate::type_t Q,
                 std::vector<std::string> vars,
                 std::string i)
  { return add_quant_gate(gvar, Q, vars.begin(), vars.end(), find(i)); }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Adds an Output gate.
  ///
  /// \returns Unique identifier of the constructed gate.
  //////////////////////////////////////////////////////////////////////////////
  int
  add_output_gate(const int i)
  {
    if (m_has_output_gate) {
      throw std::invalid_argument("Cannot create two OUTPUT gates");
    }
    const bool deref_unreachable = m_roots > 1 || i != root_idx();

    m_has_output_gate = true;

    gate &i_gate = __at(i);
    __inc_refcount(i_gate);
    const size_t g_depth = i_gate.depth + 1;

    const int res_idx = __push_gate(g_depth, output_gate(i));

    if (deref_unreachable) {
      std::cerr << "Unreferenced gates after creation of output gate!" << std::endl;

      // Find reachable set of gates
      std::vector<int> reachable_indices = reachable(i);
      std::sort(reachable_indices.begin(), reachable_indices.end(), std::less<int>());

      // Dereference children of unreachable gates
      int g_idx = const_idx[false];
      auto it = reachable_indices.cbegin();

      while (g_idx < res_idx) {
        int next_reachable = it != reachable_indices.cend()
          ? *(it++)
          : res_idx;

        // Forward 'g_idx' and deref everyone in the gap between the prior and
        // the next reachable gate.
        while (g_idx < next_reachable) {
#ifdef BDD_BENCHMARK_STATS
          std::cerr << "  " << g_idx << " : " << at(g_idx).to_string() << std::endl;
#endif

          __at(g_idx++).match
            ([this](const ngate &g) -> void {
              for (auto g_it = g.lit_list.cbegin(); g_it != g.lit_list.cend(); ++g_it) {
                __dec_refcount(__at(*g_it));
              }
            },
             [this](const ite_gate &g) -> void {
               __dec_refcount(__at(g.lits[0]));
               __dec_refcount(__at(g.lits[1]));
               __dec_refcount(__at(g.lits[2]));
             },
             [this](const quant_gate &g) -> void {
               __dec_refcount(__at(g.lit));
             },
             [](const auto &/*g*/) -> void { /* do nothing */ });
        }

        // Skip over the one that matches and should not be dereferenced
        ++g_idx;
      }
    }
    return res_idx;
  }

  int
  add_output_gate(const std::string i)
  { return add_output_gate(find(i)); }

  // TODO: with a 'gvar'

public:
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Traverses the circuit in a depth-first order.
  ///
  /// \tparam rtl Whether nodes should be visited in right-to-left order
  ///             (default: `false`).
  ///
  /// \param callback Function to call for each node visited.
  ///
  /// \param root_idx Index of the root node to start the depth-first traversal
  ///                 from (default: `root_idx()`).
  //////////////////////////////////////////////////////////////////////////////
  template<bool rtl = false>
  void
  dfs_trav(const std::function<bool(const int i, const gate &g)> &callback,
           const int root_idx) const
  {
    std::vector<bool> visited(m_circuit.size(), false);
    std::vector<int> dfs = { root_idx };

    const auto push_if_unvisited = [&visited, &dfs](const int i) -> void {
      if (visited.at(std::abs(i))) { return; }
      dfs.push_back(i);
    };

    while (!dfs.empty()) {
      const int i = dfs.back();
      dfs.pop_back();

      if (visited.at(std::abs(i))) { continue; }
      visited.at(std::abs(i)) = true;

      const gate &g = at(i);
      if (!callback(std::abs(i), g)) { return; }

      g.match
        ([&push_if_unvisited]
         (const ngate &g) -> void {
          if constexpr (rtl) {
            for (auto g_it = g.lit_list.cbegin(); g_it != g.lit_list.cend(); ++g_it) {
              push_if_unvisited(*g_it);
            }
          } else { // if (!rtl)
            for (auto g_it = g.lit_list.crbegin(); g_it != g.lit_list.crend(); ++g_it) {
              push_if_unvisited(*g_it);
            }
          }
         },
         [&push_if_unvisited]
         (const ite_gate &g) -> void {
           push_if_unvisited(g.lits[rtl ? 0 : 2]);
           push_if_unvisited(g.lits[rtl ? 1 : 1]);
           push_if_unvisited(g.lits[rtl ? 2 : 0]);
         },
         [&push_if_unvisited]
         (const quant_gate &g) -> void {
           push_if_unvisited(g.lit);
         },
         [&push_if_unvisited]
         (const output_gate &g) -> void {
           push_if_unvisited(g.lit);
         },
         [](const auto &/*g*/) -> void { /* do nothing */ });
    }
  }

  template<bool rtl = false>
  void
  dfs_trav(const std::function<bool(const int i, const gate &g)> &callback) const
  {
    dfs_trav<rtl>(callback, root_idx());
  }

public:
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Obtain a list of reachable gates indices in depth-first order.
  //////////////////////////////////////////////////////////////////////////////
  std::vector<int>
  reachable(const int root_idx) const
  {
    std::vector<int> res;
    dfs_trav([&res](const int i, const gate &/*g*/) {
      res.push_back(i);
        return true;
    }, root_idx);
    return res;
  }

  std::vector<int>
  reachable() const
  {
    return reachable(root_idx());
  }

public:
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Print an ASCII representation of the circuit to the given output
  ///        stream.
  //////////////////////////////////////////////////////////////////////////////
  template<class out_stream_t>
  void
  to_string(out_stream_t &out)
  {
    for (int i = begin_idx(); i < end_idx(); ++i) {
      out << i << " = " << at(i).to_string() << std::endl;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Creates an ASCII representation of the circuit.
  //////////////////////////////////////////////////////////////////////////////
  std::string
  to_string()
  {
    std::stringstream ss;
    to_string(ss);
    return ss.str();
  }

private:
  // ======================================================================== //
  // Other Helper Functions

  //////////////////////////////////////////////////////////////////////////////
  void
  __inc_refcount(gate &g)
  {
    if (g.refcount == 0u && !g.is<const_gate>()) { m_roots--; }
    g.refcount++;
  }

  void
  __dec_refcount(gate &g)
  {
    if (g.refcount == 0u) {
      throw std::underflow_error("Trying to decrement refcount below 0.");
    }
    g.refcount--;
  }

  //////////////////////////////////////////////////////////////////////////////
  void
  __assoc_idx(const std::string var, const int idx)
  {
    if (m_gvar_map.find(var) != m_gvar_map.end()) {
      throw std::invalid_argument("Gate '" + var + "' already exists.");
    }
    m_gvar_map.insert({ var, idx });
    m_gvar_invmap.insert({ idx, var });
  }

  //////////////////////////////////////////////////////////////////////////////
  template<typename Gate>
  size_t
  __push_gate(const size_t d, const Gate& g)
  {
    const size_t idx = m_circuit.size();
    m_circuit.push_back(gate(d, g));

    m_size++;
    m_depth = std::max(m_depth, d);

    if constexpr (!std::is_same<Gate, const_gate>::value) {
      m_roots++;
    }

    return idx;
  }

  template<typename Gate>
  size_t
  __push_gate(const std::string var, const size_t d, const Gate& g)
  {
    const size_t idx = __push_gate(d, g);
    __assoc_idx(var, idx);
    return idx;
  }

  //////////////////////////////////////////////////////////////////////////////
  int
  __find_or_add(const std::string &gvar) noexcept
  {
    const int find_res = find(gvar);
    return find_res == npos ? add_var_gate(gvar) : find_res;
  }

  template<typename IT, typename ret_t = std::vector<int>>
  ret_t
  __find_or_add(IT begin, IT end) noexcept
  {
    ret_t res;
    while (begin != end) {
      res.push_back(__find_or_add(*(begin++)));
    }
    return res;
  }
};

// ========================================================================== //
// Variable Orders
enum variable_order { INPUT, DF_LTR, DF_RTL, LEVEL };

struct var_order_map
{
private:
  using map_t = std::unordered_map<int,int>;

  map_t map;
  map_t invmap;

public:
  void add(const int qcir_variable, const int dd_variable)
  {
    map.insert({ qcir_variable, dd_variable });
    invmap.insert({ dd_variable, qcir_variable });
  }

  int
  dd_var(const int qcir_variable) const
  { return map.find(qcir_variable)->second; }

  int
  qcir_var(const int dd_variable) const
  { return invmap.find(dd_variable)->second; }

  size_t size() const
  { return map.size(); }
};

var_order_map
obtain_var_order__input(const qcir &q)
{
  var_order_map vom;
  for (size_t x = 0; x < q.vars(); ++x) {
    vom.add( x,x );
  }
  return vom;
}

template<bool rtl>
var_order_map
obtain_var_order__df(const qcir &q)
{
  var_order_map res;

  q.dfs_trav<rtl>([&q, &res](const int /*i*/, const qcir::gate &g) {
    bool keep_running = true;
    g.match
      ([&q, &res, &keep_running](const qcir::var_gate &g)  -> void {
         res.add(g.var, res.size());
         keep_running = res.size() < q.vars();
       },
       [](const auto &/*g*/) -> void { /* do nothing */ });
    return keep_running;
  });
  return res;
}

var_order_map
obtain_var_order__level(const qcir &q)
{
  // Obtain lowest level something is used.
  std::unordered_map<int, size_t> var_depth;

  for (size_t x = 0; x < q.vars(); ++x) {
    var_depth.insert({ x, q.depth() });
  }

  const auto dec_if_var = [&q, &var_depth](const int lit, size_t depth) {
    q.at(lit).match
      ([&var_depth, &depth]
       (const qcir::var_gate &g)  -> void {
          size_t& curr_depth = var_depth.find(g.var)->second;
          curr_depth = std::min(curr_depth, depth);
       },
       []
       (const auto &/*g*/) -> void { /* do nothing */ });
  };

  for (auto g_it = q.cbegin(); g_it != q.cend(); ++g_it) {
    const qcir::gate& g = *g_it;
    const size_t depth = g.depth;

    g.match
      ([&dec_if_var, &depth]
       (const qcir::ngate &g) -> void {
         for (auto g_it = g.lit_list.cbegin(); g_it != g.lit_list.cend(); ++g_it) {
           dec_if_var(*g_it, depth);
         }
       },
       [&dec_if_var, &depth]
       (const qcir::ite_gate &g) -> void {
         dec_if_var(g.lits[0], depth);
         dec_if_var(g.lits[1], depth);
         dec_if_var(g.lits[2], depth);
       },
       [&var_depth, &depth]
       (const qcir::quant_gate &g) -> void {
         for (const int x : g.vars) {
           // simple copy of 'dec_if_var'.
           size_t& curr_depth = var_depth.find(x)->second;
           curr_depth = std::min(curr_depth, depth);
         }
       },
       [&dec_if_var, &depth]
       (const qcir::output_gate &g) -> void {
         dec_if_var(g.lit, depth);
       },
       [](const auto &/*g*/) -> void { /* do nothing */ });
  }

  // Order vector of variables
  const var_order_map tie_breaker = obtain_var_order__df<false>(q);

  std::vector<int> vars;
  for (size_t x = 0; x < q.vars(); ++x) {
    vars.push_back(x);
  }
  std::sort(vars.begin(),
            vars.end(),
            [&var_depth, &tie_breaker](const int a, const int b) -> bool {
    const size_t a_depth = var_depth.find(a)->second;
    const size_t b_depth = var_depth.find(b)->second;
    if (a_depth != b_depth) {
      return a_depth < b_depth;
    } else  {
      return tie_breaker.dd_var(a) < tie_breaker.dd_var(b);
    }
  });

  // Copy over sorted vector to output;
  var_order_map res;
  for (size_t x = 0; x < q.vars(); ++x) {
    res.add( vars[x], x );
  }
  return res;
}

var_order_map
obtain_var_order(const qcir &q, variable_order vo)
{
  switch (vo) {
  case INPUT:
    return obtain_var_order__input(q);
  case LEVEL:
    return obtain_var_order__level(q);
  case DF_RTL:
    return obtain_var_order__df<true>(q);
  default:
  case DF_LTR:
    return obtain_var_order__df<false>(q);
  }
}

// ========================================================================== //
// Execution Order

using exe_order = std::vector<int>;

template<variable_order vo>
exe_order
obtain_exe_order(const qcir &q);

template<>
exe_order
obtain_exe_order<INPUT>(const qcir &q)
{
  exe_order res = q.reachable();
  std::sort(res.begin(), res.end(), std::less<int>());
  return res;
}

template<>
exe_order
obtain_exe_order<LEVEL>(const qcir &q)
{
  exe_order res = q.reachable();
  std::sort(res.begin(),
            res.end(),
            [&q](const int a, const int b) -> bool {
              if (q.at(a).depth != q.at(b).depth) {
                return q.at(a).depth < q.at(b).depth;
              }
              return a < b;
            });
  return res;
}

exe_order
obtain_exe_order(const qcir &q, variable_order exe_order)
{
  switch (exe_order) {
  case INPUT:
    return obtain_exe_order<INPUT>(q);
  default:
  case DF_LTR:
  case DF_RTL:
  case LEVEL:
    return obtain_exe_order<LEVEL>(q);
  }
}

// ========================================================================== //
// Max Index
int max_solve_idx(const qcir &q)
{
  bool run = true;
  qcir::quant_gate::type_t root_quant = qcir::quant_gate::EXISTS;

  int res = q.root_idx();
  q.at(res--).match
    ([&run]
     (const qcir::output_gate &/*g*/) { run = false; },
     [&root_quant]
     (const qcir::quant_gate &g) { root_quant = g.quant; },
     []
     (const auto &/*g*/) -> void { /* do nothing */ });

  if (!run) { return q.root_idx(); }

  while (run && res > 2) {
    const qcir::gate g = q.at(res);

    // TODO: check one should not actually jump around? This assumes the indices
    //       are properly done in a bottom-up way and there are no dead nodes.
    g.match
      ([&run]
       (const qcir::output_gate &/*g*/) {
         run = false;
       },
       [&run, &root_quant]
       (const qcir::quant_gate &g) {
         if (g.quant != root_quant) { run = false; }
       },
       []
       (const auto &/*g*/) -> void { /* do nothing */ });

    if (run) { res--; }
  }
  return res;
}

// ========================================================================== //
// Decision Diagram Construction

class solve_res
{
public:
  using sat_res_t = bool;
  using witness_t = std::vector<std::pair<int,char>>;

public:
  sat_res_t sat_res;
  witness_t witness;

  struct stats_t {
    size_t prenex_time;
    size_t solve_time;
    struct cache_t {
      size_t max_size;
    } cache;
    struct dd_t {
      size_t max_size;
      size_t matrix_max_size;
      size_t prenex_max_size;
    } dd;
  } stats;
};

template<typename Adapter>
solve_res
solve(Adapter& adapter, qcir& q,
      const variable_order vo = variable_order::INPUT)
{
  const time_point t_prep_before = now();


  // TODO: check there are no free variables; if this is the case, then they
  //       ought to be existentially quantified at the root of the circuit.
  const int max_q_idx = max_solve_idx(q);

  const var_order_map vom = obtain_var_order(q, vo);

  // TODO: Derive an execution order that minimises the cuts in the graph during
  //       execution.
  const exe_order exo = obtain_exe_order(q, variable_order::INPUT);

  const time_point t_prep_after = now();

  constexpr size_t max_print = 10;

  std::cout << "  | variable order:      [ ";
  for (size_t x = 0; x < q.vars() && x < max_print; ++x) {
    std::cout << vom.qcir_var(x) << " ";
  }
  if (q.vars() > max_print) { std::cout << "..."; }
  std::cout << "]" << std::endl;

  std::cout << "  | execution order:     [ ";
  for (size_t x = 0; x < exo.size() && x < max_print; ++x) {
    std::cout << exo.at(x) << " ";
  }
  if (q.size() > max_print) { std::cout << "..."; }
  std::cout << "]" << std::endl;

  std::cout << "  | max solve idx:       " << max_q_idx << "\n"
            << "  | setup time (ms):     " << duration_ms(t_prep_before, t_prep_after) << "\n\n"
            << std::flush;

  // Set-up BDD computation cache
  std::unordered_map<int, std::pair<typename Adapter::dd_t, size_t>> cache;

  const auto cache_get = [&adapter, &cache](int i) {
    // Get idx from identifier
    const int idx = std::abs(i);

    // Obtain decision diagram from cache
    auto& cache_res = cache[idx];
    const auto res = cache_res.first;

    if (2 < idx) { // Decrement Reference Count
      size_t& refcount = cache_res.second;
      assert(refcount > 0);
      if (refcount > 1) {
        refcount--;
      } else {
        cache.erase(idx);
      }
    }

    // Negate, if needed
    return i < 0 ? ~res : res;
  };

  size_t cache_max_size = 0u;
  size_t dd_max_size = 0u;
  size_t dd_matrix_max_size = 0u;
  size_t dd_prenex_max_size = 0u;

  const time_point t_solve_before = now();
  time_point t_prenex_before = t_solve_before;

#ifdef BDD_BENCHMARK_STATS
  std::cout << "  | Matrix\n";
#endif

  // TODO: bail out, if collapsing to a terminal (and the output gate has been
  //       processed).
  for (auto exo_it = exo.cbegin(); exo_it != exo.cend(); ++exo_it) {
    const int q_idx = *exo_it;
    if (q_idx > max_q_idx) { continue; }

    const qcir::gate& g = q.at(q_idx);

#ifdef BDD_BENCHMARK_STATS
    std::cout << "  | | " << q_idx << " : " << g.to_string() << "\n";
    const time_point t_start = now();
#endif

    const typename Adapter::dd_t g_dd = g.match
      ([&adapter]
       (const qcir::const_gate &g) -> typename Adapter::dd_t {
        return g.val ? adapter.top() : adapter.bot();
       },
       [&adapter, &vom]
       (const qcir::var_gate &g)  -> typename Adapter::dd_t {
#ifdef BDD_BENCHMARK_STATS
         std::cout << "  | | | DD var:          " << vom.dd_var(g.var) << "\n";
#endif
         return adapter.ithvar(vom.dd_var(g.var));
       },
       [&adapter, &cache_get]
       (const qcir::ngate &g) -> typename Adapter::dd_t {
         const auto apply = [&g]
           (const typename Adapter::dd_t &dd_1,
            const typename Adapter::dd_t &dd_2)
         { // TODO: move switch outside of lambda?
           switch (g.nGateype) {
           case qcir::ngate::AND:
             return dd_1 & dd_2;
           case qcir::ngate::OR:
             return dd_1 | dd_2;
           case qcir::ngate::XOR:
             return dd_1 ^ dd_2;
           }
           throw std::invalid_argument("Unknown Operator");
         };

         std::queue<typename Adapter::dd_t> tmp;

         { // Populate FIFO queue with pairs of BDDs
           auto g_it = g.lit_list.cbegin();
           while (g_it != g.lit_list.cend()) {
             const auto dd_1 = cache_get(*(g_it++));
             if (g_it == g.lit_list.cend()) {
               if (tmp.empty()) { return dd_1; }
               tmp.push(dd_1);
               break;
             }

             const auto dd_2 = cache_get(*(g_it++));
             if (g_it == g.lit_list.cend() && tmp.empty()) {
               return apply(dd_1, dd_2);
             }

             tmp.push(apply(dd_1, dd_2));
           }
         }

         // Merge pairs in the FIFO queue
         while (true) {
           const auto dd_1 = tmp.front(); tmp.pop();
           const auto dd_2 = tmp.front(); tmp.pop();

           if (tmp.empty()) {
             return apply(dd_1, dd_2);
           }
           tmp.push(apply(dd_1, dd_2));
         }
       },
       [&adapter, &cache_get]
       (const qcir::ite_gate &g) -> typename Adapter::dd_t {
         const auto dd_if   = cache_get(g.lits[0]);
         const auto dd_then = cache_get(g.lits[1]);
         const auto dd_else = cache_get(g.lits[2]);

         return adapter.ite(dd_if, dd_then, dd_else);
       },
       [&adapter, &vom, &cache_get]
       (const qcir::quant_gate &g) -> typename Adapter::dd_t {
         std::set<int> vars;
         for (const int x : g.vars) { vars.insert(vom.dd_var(x)); }

#ifdef BDD_BENCHMARK_STATS
         std::cout << "  | | | DD vars:         [ ";
         size_t out_counter = 0;
         for (auto it = vars.begin(); it != vars.end(); ++it) {
           if (++out_counter > max_print) {
             std::cout << "... ";
             break;
           }
           std::cout << *it << " ";
         }
         std::cout << "]\n";
#endif

         // iterator quantification
         /*
         return g.quant == qcir::quant_gate::EXISTS
           ? adapter.exists(cache_get(g.lit), vars.crbegin(), vars.crend())
           : adapter.forall(cache_get(g.lit), vars.crbegin(), vars.crend());
         */

         // predicated quantification
         const auto pred = [&vars](int i) { return vars.find(i) != vars.end(); };

         return g.quant == qcir::quant_gate::EXISTS
           ? adapter.exists(cache_get(g.lit), pred)
           : adapter.forall(cache_get(g.lit), pred);
       },
       [&cache_get, &t_prenex_before]
       (const qcir::output_gate &g) -> typename Adapter::dd_t {
         t_prenex_before = now();
         return cache_get(g.lit);
       });

#ifdef BDD_BENCHMARK_STATS
    const time_point t_end = now();

    const size_t g_dd_size = adapter.nodecount(g_dd);
    dd_max_size = std::max(dd_max_size, g_dd_size);
    if (t_prenex_before != t_solve_before) { // Matrix
      dd_matrix_max_size = std::max(dd_matrix_max_size, g_dd_size);
    } else { // Prenex
      dd_prenex_max_size = std::max(dd_prenex_max_size, g_dd_size);
    }
    std::cout << "  | | | DD size:         " << g_dd_size << "\n"
              << "  | | | time (ms):       " << duration_ms(t_start,t_end) << "\n";

    if (g.is<qcir::output_gate>()) {
      std::cout << "  | Prefix\n";
    }
#endif

    cache.insert({ q_idx, std::make_pair(g_dd, g.refcount) });
    cache_max_size = std::max(cache_max_size, cache.size());
  }
#ifdef BDD_BENCHMARK_STATS
  std::cout << "\n";
#endif

  const auto res = cache_get(max_q_idx);
  const time_point t_solve_after = now();

  const qcir::quant_gate::type_t root_quant =
    q.root_idx() <= max_q_idx
    // All gates, including the top-most quantifier block, have been processed. In
    // this case, we are existentially quantifying free variables.
    ? qcir::quant_gate::EXISTS
    // The top-most quantifier block has NOT been resolved. In this case, there
    // are no free variables (TODO: check for free variables?).
    : q.at(q.root_idx()).as<qcir::quant_gate>().quant;

  solve_res::sat_res_t sat_res;
  solve_res::witness_t witness;

  if (res == adapter.bot()) {
    sat_res = false;
  } else if (res == adapter.top()) {
    sat_res = true;
  } else { // res == non-leaf
    sat_res = root_quant == qcir::quant_gate::EXISTS;

    const auto cube = adapter.pickcube(sat_res ? res : ~res);
    for (const auto &xv : cube) {
      const int qcir_var = vom.qcir_var(xv.first);
      const char val = xv.second;
      witness.push_back({ qcir_var, val });
    }
  }

  return
    {
      sat_res,
      witness,
      {
        duration_ms(t_prenex_before, t_solve_after),
        duration_ms(t_solve_before, t_solve_after),
        {
          cache_max_size
        },
        {
          dd_max_size,
          dd_matrix_max_size,
          dd_prenex_max_size
        }
      }
    };
}

// ========================================================================== //
template<>
std::string option_help_str<variable_order>()
{ return "Desired Variable ordering"; }

template<>
variable_order parse_option(const std::string &ARG, bool &should_exit)
{
  const std::string arg = ascii_tolower(ARG);

  if (arg == "input" || arg == "matrix") {
    return variable_order::INPUT;
  }

  if (arg == "df" || arg == "df_ltr" || arg == "depth-first" || arg == "depth-first_ltr") {
    return variable_order::DF_LTR;
  }

  if (arg == "df_rtl" || arg == "depth-first_rtl") {
    return variable_order::DF_RTL;
  }

  if (arg == "level" || "level-df") {
    return variable_order::LEVEL;
  }

  std::cerr << "Undefined variable/execution ordering: " << arg.c_str() << "\n";
  should_exit = true;

  return variable_order::INPUT;
}

template<typename Adapter>
int run_qbf(int argc, char** argv)
{
  variable_order variable_order = variable_order::INPUT;
  bool should_exit = parse_input(argc, argv, variable_order);

  if (input_files.size() == 0) {
    std::cerr << "Input file(s) not specified\n";
    should_exit = true;
  }

  if (should_exit) { return -1; }

  // =========================================================================
  std::cout << "QBF Solver (" << Adapter::NAME << " " << M << " MiB):\n";

  // Parse QCir Input
  const std::string input_file = input_files.at(0);
  std::cout << "\n  Circuit: " << input_file << "\n";

  qcir q(input_file);

  std::cout << "  | depth: " << q.depth() << "\n"
            << "  | size:  " << q.size() << "\n"
            << "  | vars:  " << q.vars() << "\n"
            << std::flush;

  const time_point t_init_before = now();
  Adapter adapter(q.vars());
  const time_point t_init_after = now();

  // Initialise BDD package
  std::cout << "\n  BDD init (ms):         " << duration_ms(t_init_before, t_init_after) << "\n"
            << "\n"
            << "  Solving Circuit\n"
            << std::flush;

  return adapter.run([&]() {
    const auto [ sat_res, witness, stats ] = solve(adapter, q, variable_order);

    std::cout << "  | solving time (ms):   " << stats.solve_time << "\n"
              << "  | | matrix:            " << (stats.solve_time - stats.prenex_time) << "\n"
              << "  | | prenex:            " << stats.prenex_time << "\n"
              << "  | cache (max):         " << stats.cache.max_size << "\n"
#ifdef BDD_BENCHMARK_STATS
              << "  | DD size (max):       " << stats.dd.max_size << "\n"
              << "  | | matrix:            " << stats.dd.matrix_max_size << "\n"
              << "  | | prenex:            " << stats.dd.prenex_max_size << "\n"
#endif
      ;

    std::cout << "  | result:              " << (sat_res ? "SAT" : "UNSAT");

    if (witness.size() > 0) {
      std::cout << " [ ";
      for (const auto &xv : witness) {
        const std::string var_name = q.var(xv.first);
        const char var_value = xv.second;
        std::cout << var_name << "=" << var_value << " ";
      }
      std::cout << "]";
    }
    std::cout << "\n"
              << std::flush;

    adapter.print_stats();

    return 0;
  });
}
