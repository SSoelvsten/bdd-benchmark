#include "common.cpp"

// Algorithms and Operations
#include <algorithm>

// Data Structures
#include <array>
#include <unordered_map>
#include <vector>

// Files, Streams, and so on...
#include <filesystem>
#include <fstream>
#include <iterator>

// Types
#include <stdint.h>
#include <limits>

// Other
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
//                              Apply Operand                                 //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \brief Enum for choosing the desired Apply operation.
////////////////////////////////////////////////////////////////////////////////
enum oper_opt { AND, OR };

template<>
std::string option_help_str<oper_opt>()
{ return "Desired Boolean Operator for Apply Algorithm"; }

template<>
oper_opt parse_option(const std::string &arg, bool &should_exit)
{
  const std::string lower_arg = ascii_tolower(arg);

  if (lower_arg == "and")
    { return oper_opt::AND; }

  if (lower_arg == "or")
    { return oper_opt::OR; }

  std::cerr << "Undefined operand: " << arg << "\n";
  should_exit = true;

  return oper_opt::AND;
}

std::string option_str(const oper_opt& enc)
{
  switch (enc) {
  case oper_opt::AND:
    return "and";
  case oper_opt::OR:
    return "or";
  default:
    return "?";
  }
}

////////////////////////////////////////////////////////////////////////////////
//                      Deserialization from 'lib-bdd'                        //
////////////////////////////////////////////////////////////////////////////////
namespace lib_bdd
{
  /// \brief Similar to Rust's `u??::from_le_bytes`.
  template<typename UInt>
  UInt from_le_bytes(const std::array<char, sizeof(UInt)> &bytes)
  {
    UInt res = 0;
    for (size_t byte = 0; byte < bytes.size(); ++byte) {
      // HACK: Reinterpret 'char' as an 'unsigned char' without changing any of
      //       the bit values (see also 'fast inverse square root' algorithm).
      unsigned char unsigned_byte = *((unsigned char*) (&bytes.at(byte)));

      res |= static_cast<UInt>(unsigned_byte) << (8*byte);
    }
    return res;
  }

  /// \brief Minimal recreation of the BDD nodes in 'lib-bdd'.
  class node
  {
  public:
    using var_type = uint16_t;

    static constexpr var_type terminal_level = std::numeric_limits<var_type>::max();

    using ptr_type = uint32_t;

    static constexpr ptr_type false_ptr = 0u;
    static constexpr ptr_type true_ptr  = 1u;

  private:
    var_type _level;
    ptr_type _low;
    ptr_type _high;

  public:
    /// \brief Number of bytes
    static constexpr size_t size()
    { return sizeof(var_type) + 2*sizeof(ptr_type); }

  public:
    /// \brief Constructor for an internal BDD node.
    node(const var_type &var, const ptr_type &low, const ptr_type &high)
      : _level(low == high ? terminal_level : var)
      , _low()
    {
      if (var == terminal_level) {
        throw std::overflow_error("BDD variable level too large");
      }
      if (low == high && low > true_ptr /*|| high > true_ptr*/) {
        throw std::invalid_argument("Creation of suppressed BDD node");
      }
    }

    /// \brief Constructor for a Boolean BDD terminal.
    node(bool terminal_value)
      : _level(terminal_level), _low(terminal_value), _high(terminal_value)
    { }

    /// \brief Default constructor, creating the `false` terminal.
    node()
      : node(false)
    { }

    /// \brief Constructor from serialized (little-endian) bytes
    template<typename Iterator>
    node(Iterator begin, [[maybe_unused]] Iterator end)
    {
      assert(std::distance(begin, end) == size());
      static_assert(size() == 10u, "Mismatch in size of C++ class in comparison to 'lib-bdd'");

      this->_level = from_le_bytes<var_type>({ *(begin+0), *(begin+1) });
      this->_low   = from_le_bytes<ptr_type>({ *(begin+2), *(begin+3), *(begin+4), *(begin+5) });
      this->_high  = from_le_bytes<ptr_type>({ *(begin+6), *(begin+7), *(begin+8), *(begin+9) });
    }

  public:
    /// \brief The variable level. Assuming the identity order this is
    /// equivalent to its 'level'.
    var_type level() const
    { return _level; }

    /// \brief Index of the 'low' child, i.e. the variable being set to `false`.
    ptr_type low() const
    { return _low; }

    /// \brief Index of the 'high' child, i.e. the variable being set to `true`.
    ptr_type high() const
    { return _high; }

  public:
    /// \brief Whether the BDD node is a terminal.
    bool is_terminal() const
    { return _level == terminal_level; }

    /// \brief Whether the BDD node is the `false` terminal.
    bool is_false() const
    { return is_terminal() && _low == false_ptr; }

    /// \brief Whether the BDD node is the `true` terminal.
    bool is_true() const
    { return is_terminal() && _low == true_ptr; }

    /// \brief Whether the BDD node is an internal BDD node, i.e.
    ///        `!is_terminal()`.
    bool is_internal() const
    { return _level < terminal_level; }
  };

  /// \brief lib-bdd representation of a BDD.
  using bdd = std::vector<node>;

  /// \brief Parse a binary file from lib-bdd.
  inline bdd
  deserialize(std::ifstream &in)
  {
    bdd out;

    // 10 byte buffer (matching a single lib-bdd::node)
    constexpr size_t node_size = node::size();
    std::array<char, node_size> buffer{};

    // Read and Push `false` terminal
    in.read(buffer.data(), buffer.size());

    if (!in.good()) {
      throw std::runtime_error("Error while parsing `false` terminal.");
    }

    out.push_back(node(buffer.begin(), buffer.end()));

    if (in.eof()) {
      return out;
    }

    // Read and Push `true` terminal
    in.read(buffer.data(), buffer.size());

    if (in.eof()) {
      return out;
    }
    if (!in.good()) {
      throw std::runtime_error("Error while parsing `true` terminal");
    }

    out.push_back(node(buffer.begin(), buffer.end()));

    // Read and Push remaining nodes
    while (true) {
      in.read(buffer.data(), buffer.size());

      // Hit the end?
      if (in.eof()) {
        return out;
      }

      // Any other errors?
      if (!in.good()) {
        throw std::runtime_error("Bad state of std::ifstream while scanning 10-byte chunk(s).");
      }

      // Create node from buffer
      const node n(buffer.begin(), buffer.end());

      // Sanity checks on node
      if (out.size() <= n.low()) {
        std::stringstream ss;
        ss << "Low index ( " << n.low() << " ) is out-of-bounds";

        throw std::out_of_range(ss.str());
      }
      if (out.size() <= n.high()) {
        std::stringstream ss;
        ss << "High index ( " << n.high() << " ) is out-of-bounds";

        throw std::out_of_range(ss.str());
      }

      // Push node to output
      out.push_back(n);
    }
  }

  /// \brief Parse a binary file from lib-bdd.
  inline bdd
  deserialize(const std::string &path)
  {
    std::ifstream is(path, std::ios::binary);
    return deserialize(is);
  }

  /// \brief Parse a binary file from lib-bdd.
  inline bdd
  deserialize(const std::filesystem::path &path)
  {
    std::ifstream is(path, std::ios::binary);
    return deserialize(is);
  }

  /// \brief Struct with various statistics about a deserialized BDD.
  struct stats_t
  {
  public:
    enum parent_count_idx : int
    {
      None = 0,
      One  = 1,
      Two  = 2,
      More = 3
    };

  public:
    node::ptr_type size             = 0u;
    node::var_type levels           = 0u;
    node::ptr_type width            = 0u;
    node::ptr_type terminals[2]     = {0u, 0u};
    node::ptr_type parent_counts[4] = {0u, 0u, 0u, 0u};
  };

  /// \brief Extract statistics from a BDD.
  stats_t stats(const bdd &f)
  {
    stats_t out;

    out.size = f.size();

    // Sweep through BDD
    node::var_type curr_level = node::terminal_level;
    node::ptr_type curr_width = 0u;

    std::vector<int> parent_counts(f.size(), 0);

    for (const auto &n : f) {
      if (n.level() != curr_level) {
        out.levels += 1;
        curr_level = n.level();
        curr_width = 0u;
      }

      curr_width += 1u;

      if (n.is_internal()) {
        out.terminals[false] += (n.low() == node::false_ptr) + (n.high() == node::false_ptr);
        out.terminals[true]  += (n.low() == node::true_ptr)  + (n.high() == node::true_ptr);

        out.width = std::max<size_t>(out.width, curr_width);

        parent_counts.at(n.low())  += 1;
        parent_counts.at(n.high()) += 1;
      }
    }

    // Accumulate data from 'parent_counts'
    for (const auto &pc : parent_counts) {
      if (pc == 0) {
        out.parent_counts[stats_t::parent_count_idx::None] += 1;
      }
      if (pc == 1) {
        out.parent_counts[stats_t::parent_count_idx::One] += 1;
      }
      if (pc == 2) {
        out.parent_counts[stats_t::parent_count_idx::Two] += 1;
      }
      if (pc > 2) {
        out.parent_counts[stats_t::parent_count_idx::More] += 1;
      }
    }

    return out;
  }
}

////////////////////////////////////////////////////////////////////////////////
//               Benchmark as per Pastva and Henzinger (2023)                 //
////////////////////////////////////////////////////////////////////////////////

/// \brief Compacted remapping of lib-bdd variables.
using var_map = std::unordered_map<lib_bdd::node::var_type, int>;

/// \brief Derive a compacted remapping of the variable ordering.
var_map remap_vars(const lib_bdd::bdd &f, const lib_bdd::bdd &g)
{
  auto f_rit = f.rbegin();
  auto g_rit = g.rbegin();

  std::unordered_map<lib_bdd::node::var_type, int> out;
  int next_var = 0;

  while (f_rit != f.rend() || g_rit != g.rend()) {
    const lib_bdd::node::var_type next_level = std::min(f_rit->level(),
                                                        g_rit->level());

    if (next_level == lib_bdd::node::terminal_level) {
      break;
    }

    out.insert({ next_level, next_var++ });

    while (f_rit->level() <= next_level && f_rit != f.rend()) { ++f_rit; }
    while (g_rit->level() <= next_level && g_rit != g.rend()) { ++g_rit; }
  }

  return out;
}

var_map remap_vars(const std::array<lib_bdd::bdd, 2> &fs)
{
  return remap_vars(fs.at(0), fs.at(1));
}

/// \brief Reconstruct DD from 'lib-bdd' inside of BDD package.
template<typename adapter_t>
typename adapter_t::dd_t
reconstruct(adapter_t &adapter, const lib_bdd::bdd &in, const var_map &vm)
{
  // Vector of converted DD nodes
  std::vector<typename adapter_t::build_node_t> out;

  // Iterator through input
  auto it = in.begin();
  assert(it != in.end());

  // False Terminal
  assert(it->is_false());

  out.push_back(adapter.build_node(false));
  ++it;

  if (it == in.end()) {
    return adapter.build();
  }

  // True Terminal
  assert(it->is_true());

  out.push_back(adapter.build_node(true));
  ++it;

  // Remaining Nodes
  for (; it != in.end(); ++it) {
    assert(it->is_internal());

    const auto var = vm.find(it->level());

    if (var == vm.end()) {
      std::stringstream ss;
      ss << "Unmapped variable level: " << it->level();

      throw std::out_of_range(ss.str());
    }

    const auto low  = out.at(it->low());
    const auto high = out.at(it->high());

    out.push_back(adapter.build_node(var->second, low, high));
  }

  return adapter.build();
}

template<typename adapter_t>
int run_apply(int argc, char** argv)
{
  oper_opt oper_opt = oper_opt::AND;
  bool should_exit = parse_input(argc, argv, oper_opt);

  constexpr size_t inputs = 2u;

  if (input_files.size() != inputs) {
    std::cerr << "Incorrect number of files given (2 required)\n";
    should_exit = true;
  }

  if (should_exit) { return -1; }

  // =========================================================================
  std::cout << "Apply (" << adapter_t::NAME << " " << M << " MiB):\n";

  // =========================================================================
  // Load 'lib-bdd' files
  std::array<lib_bdd::bdd, inputs> inputs_binary;

  for (size_t i = 0; i < inputs; ++i) {
    std::cout << "  Parsing '" << input_files.at(i) << "':\n";
    inputs_binary.at(i) = lib_bdd::deserialize(input_files.at(i));

    const lib_bdd::stats_t stats = lib_bdd::stats(inputs_binary.at(i));

    std::cout << "  | size:                   " << stats.size << "\n"
              << "  | levels:                 " << stats.levels << "\n"
              << "  | width:                  " << stats.width << "\n"
              << "  | terminal edges:\n"
              << "  | | false                 " << stats.terminals[false] << "\n"
              << "  | | true                  " << stats.terminals[true] << "\n"
              << "  | parent counts:\n"
              << "  | | 0                     " << stats.parent_counts[lib_bdd::stats_t::parent_count_idx::None] << "\n"
              << "  | | 1                     " << stats.parent_counts[lib_bdd::stats_t::parent_count_idx::One] << "\n"
              << "  | | 2                     " << stats.parent_counts[lib_bdd::stats_t::parent_count_idx::Two] << "\n"
              << "  | | 3+                    " << stats.parent_counts[lib_bdd::stats_t::parent_count_idx::More] << "\n"
              << "\n"
              << std::flush;
  }

  var_map vm = remap_vars(inputs_binary);

  // =========================================================================
  // Initialize BDD package
  const size_t varcount = vm.size();

  const time_point t_init_before = get_timestamp();
  adapter_t adapter(varcount);
  const time_point t_init_after = get_timestamp();

  std::cout << "  Initialisation:\n"
            << "  | variables:              " << varcount << "\n"
            << "  | time (ms):              " << duration_of(t_init_before, t_init_after) << "\n"
            << std::flush;

  // =========================================================================
  // Reconstruct DDs
  std::array<typename adapter_t::dd_t, inputs> inputs_dd;

  for (size_t i = 0; i < inputs; ++i) {
    const time_point t_rebuild_before = get_timestamp();
    inputs_dd.at(i) = reconstruct(adapter, inputs_binary.at(i), vm);
    const time_point t_rebuild_after = get_timestamp();

    std::cout << "\n  DD '" << input_files.at(i) << "':\n"
              << "  | size (nodes):           " << adapter.nodecount(inputs_dd.at(i)) << "\n"
              << "  | satcount:               " << adapter.satcount(inputs_dd.at(i)) << "\n"
              << "  | time (ms):              " << duration_of(t_rebuild_before, t_rebuild_after) << "\n"
              << std::flush;
  }

  // =========================================================================
  // Apply both DDs together
  typename adapter_t::dd_t result;

  const time_point t_apply_before = get_timestamp();
  switch (oper_opt) {
  case oper_opt::AND:
    result = inputs_dd.at(0) & inputs_dd.at(1);
    break;
  case oper_opt::OR:
    result = inputs_dd.at(0) | inputs_dd.at(1);
    break;
  }
  const time_point t_apply_after = get_timestamp();

  std::cout << "\n  Apply ( " << option_str(oper_opt) << " ):\n"
            << "  | size (nodes):           " << adapter.nodecount(result) << "\n"
            << "  | satcount:               " << adapter.satcount(result) << "\n"
            << "  | time (ms):              " << duration_of(t_apply_before, t_apply_after) << "\n"
            << std::flush;

  // =========================================================================
  adapter.print_stats();

  return 0;
}
