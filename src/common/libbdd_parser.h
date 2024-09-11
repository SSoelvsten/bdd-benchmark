// Algorithms and Operations
#include <algorithm>

// Assertions
#include <cassert>

// Data Structures
#include <array>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <vector>

// Files, Streams, and so on...
#include <filesystem>
#include <fstream>

// Types
#include <cstdint>
#include <cstdlib>
#include <limits>

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                Deserialization from 'Lib-BDD'                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace lib_bdd
{
  /// \brief Similar to Rust's `u??::from_le_bytes`.
  template <typename UInt>
  UInt
  from_le_bytes(const std::array<char, sizeof(UInt)>& bytes)
  {
    UInt res = 0;
    for (size_t byte = 0; byte < bytes.size(); ++byte) {
      // HACK: Reinterpret 'char' as an 'unsigned char' without changing any of
      //       the bit values (see also 'fast inverse square root' algorithm).
      unsigned char unsigned_byte = *((unsigned char*)(&bytes.at(byte)));

      res |= static_cast<UInt>(unsigned_byte) << (8 * byte);
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
    static constexpr size_t
    size()
    {
      return sizeof(var_type) + 2 * sizeof(ptr_type);
    }

  public:
    /// \brief Constructor for an internal BDD node.
    node(const var_type& var, const ptr_type& low, const ptr_type& high)
      : _level(((low == false_ptr || low == true_ptr) && low == high) ? terminal_level : var)
      , _low(low)
      , _high(high)
    {
      if (var == terminal_level) { throw std::overflow_error("BDD variable level too large"); }
      if (low == high && low > true_ptr /*|| high > true_ptr*/) {
        throw std::invalid_argument("Creation of suppressed BDD node");
      }
    }

    /// \brief Constructor for a Boolean BDD terminal.
    node(bool terminal_value)
      : _level(terminal_level)
      , _low(terminal_value)
      , _high(terminal_value)
    {}

    /// \brief Default constructor, creating the `false` terminal.
    node()
      : node(false)
    {}

    /// \brief Constructor from serialized (little-endian) bytes
    template <typename Iterator>
    node(Iterator begin, [[maybe_unused]] Iterator end)
    {
      assert(std::distance(begin, end) == size());
      static_assert(size() == 10u, "Mismatch in size of C++ class in comparison to 'lib-bdd'");

      this->_level = from_le_bytes<var_type>({ *(begin + 0), *(begin + 1) });
      this->_low =
        from_le_bytes<ptr_type>({ *(begin + 2), *(begin + 3), *(begin + 4), *(begin + 5) });
      this->_high =
        from_le_bytes<ptr_type>({ *(begin + 6), *(begin + 7), *(begin + 8), *(begin + 9) });
    }

  public:
    /// \brief The variable level. Assuming the identity order this is
    /// equivalent to its 'level'.
    var_type
    level() const
    {
      return _level;
    }

    /// \brief Index of the 'low' child, i.e. the variable being set to `false`.
    ptr_type
    low() const
    {
      return _low;
    }

    /// \brief Index of the 'high' child, i.e. the variable being set to `true`.
    ptr_type
    high() const
    {
      return _high;
    }

  public:
    /// \brief Whether the BDD node is a terminal.
    bool
    is_terminal() const
    {
      return _level == terminal_level;
    }

    /// \brief Whether the BDD node is the `false` terminal.
    bool
    is_false() const
    {
      return is_terminal() && _low == false_ptr;
    }

    /// \brief Whether the BDD node is the `true` terminal.
    bool
    is_true() const
    {
      return is_terminal() && _low == true_ptr;
    }

    /// \brief Whether the BDD node is an internal BDD node, i.e.
    ///        `!is_terminal()`.
    bool
    is_internal() const
    {
      return _level < terminal_level;
    }
  };

  /// \brief lib-bdd representation of a BDD.
  using bdd = std::vector<node>;

  /// \brief Parse a binary file from lib-bdd.
  inline bdd
  deserialize(std::ifstream& in)
  {
    bdd out;

    // 10 byte buffer (matching a single lib-bdd::node)
    constexpr size_t node_size = node::size();
    std::array<char, node_size> buffer{};

    // Read and Push `false` terminal
    in.read(buffer.data(), buffer.size());

    if (!in.good()) { throw std::runtime_error("Error while parsing `false` terminal."); }

    out.push_back(node(buffer.begin(), buffer.end()));

    if (in.eof()) { return out; }

    // Read and Push `true` terminal
    in.read(buffer.data(), buffer.size());

    if (in.eof()) { return out; }
    if (!in.good()) { throw std::runtime_error("Error while parsing `true` terminal"); }

    out.push_back(node(buffer.begin(), buffer.end()));

    // Read and Push remaining nodes
    while (true) {
      in.read(buffer.data(), buffer.size());

      // Hit the end?
      if (in.eof()) { return out; }

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
  deserialize(const std::string& path)
  {
    std::ifstream is(path, std::ios::binary);
    return deserialize(is);
  }

  /// \brief Parse a binary file from lib-bdd.
  inline bdd
  deserialize(const std::filesystem::path& path)
  {
    std::ifstream is(path, std::ios::binary);
    return deserialize(is);
  }

  /// \brief Comparator for a level-by-level bottom-up traversal.
  inline auto
  levelized_order(const lib_bdd::bdd& f)
  {
    return [&f](const int a, const int b) -> bool {
      assert(f.at(a).is_internal());
      assert(f.at(b).is_internal());

      const lib_bdd::node& a_node = f.at(a);
      const lib_bdd::node& b_node = f.at(b);

      // Deepest first (but it is a maximum priority queue)
      if (a_node.level() != b_node.level()) { return a_node.level() < b_node.level(); }
      // Break ties on the same level by its index
      return a > b;
    };
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
    node::ptr_type terminals[2]     = { 0u, 0u };
    node::ptr_type parent_counts[4] = { 0u, 0u, 0u, 0u };
  };

  /// \brief Extract statistics from a BDD.
  stats_t
  stats(const bdd& f)
  {
    stats_t out;

    out.size = f.size();

    // Sweep through BDD
    node::var_type curr_level = node::terminal_level;
    node::ptr_type curr_width = 0u;

    std::vector<int> parent_counts(f.size(), 0);

    const auto pq_comp = levelized_order(f);

    std::priority_queue<int, std::vector<int>, decltype(pq_comp)> pq(pq_comp);
    for (size_t i = 2; i < f.size(); ++i) {
      assert(i < std::numeric_limits<int>::max());
      pq.push(static_cast<int>(i));
    }

    while (!pq.empty()) {
      const int i = pq.top();
      pq.pop();

      const auto& n = f.at(i);

      if (n.level() != curr_level) {
        out.levels += 1;
        curr_level = n.level();
        curr_width = 0u;
      }

      curr_width += 1u;

      if (n.is_internal()) {
        out.terminals[false] += (n.low() == node::false_ptr) + (n.high() == node::false_ptr);
        out.terminals[true] += (n.low() == node::true_ptr) + (n.high() == node::true_ptr);

        out.width = std::max<size_t>(out.width, curr_width);

        parent_counts.at(n.low()) += 1;
        parent_counts.at(n.high()) += 1;
      }
    }

    // Accumulate data from 'parent_counts'
    for (const auto& pc : parent_counts) {
      if (pc == 0) { out.parent_counts[stats_t::parent_count_idx::None] += 1; }
      if (pc == 1) { out.parent_counts[stats_t::parent_count_idx::One] += 1; }
      if (pc == 2) { out.parent_counts[stats_t::parent_count_idx::Two] += 1; }
      if (pc > 2) { out.parent_counts[stats_t::parent_count_idx::More] += 1; }
    }

    return out;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  //                  Reconstruction from 'Lib_BDD' in any BDD package (2023)                     //
  //////////////////////////////////////////////////////////////////////////////////////////////////

  /// \brief Compacted remapping of lib-bdd variables.
  using var_map = std::unordered_map<lib_bdd::node::var_type, int>;

  /// \brief Derive a compacted remapping of the variable ordering.
  var_map
  remap_vars(const std::vector<lib_bdd::bdd>& fs)
  {
    // Minimum Priority Queue
    std::priority_queue<int, std::vector<int>, std::greater<>> pq;

    for (const lib_bdd::bdd& f : fs) {
      for (const auto& n : f) {
        if (!n.is_terminal()) { pq.push(n.level()); }
      }
    }

    std::unordered_map<lib_bdd::node::var_type, int> out;
    int var = 0;

    while (!pq.empty()) {
      // Get next level (in ascending order)
      const int level = pq.top();

      // Add mapping for all non-terminals
      if (level != lib_bdd::node::terminal_level) { out.insert({ level, var++ }); }

      // Pop all duplicates
      while (!pq.empty() && pq.top() == level) { pq.pop(); }
    }

    return out;
  }

  /// \brief Reconstruct DD from 'lib-bdd' inside of BDD package.
  template <typename Adapter>
  typename Adapter::dd_t
  reconstruct(Adapter& adapter, const lib_bdd::bdd& in, const var_map& vm)
  {
    if (in.size() <= 2) {
      adapter.build_node(in.size() == 2);
      return adapter.build();
    }

    // Vector of converted DD nodes
    std::vector<typename Adapter::build_node_t> out(in.size(), adapter.build_node(false));

    // Terminal Nodes
    out.at(0) = adapter.build_node(false);
    out.at(1) = adapter.build_node(true);

    // Internal Nodes
    const auto pq_comp = levelized_order(in);

    std::priority_queue<int, std::vector<int>, decltype(pq_comp)> pq(pq_comp);
    for (size_t i = 2; i < in.size(); ++i) {
      assert(i < std::numeric_limits<int>::max());
      pq.push(static_cast<int>(i));
    }

    while (!pq.empty()) {
      const int i = pq.top();
      pq.pop();

      const lib_bdd::node& n = in.at(i);

      const auto var = vm.find(n.level());

      if (var == vm.end()) {
        std::stringstream ss;
        ss << "Unmapped variable level: " << n.level();
        throw std::out_of_range(ss.str());
      }

      const auto low  = out.at(n.low());
      const auto high = out.at(n.high());

      out.at(i) = adapter.build_node(var->second, low, high);
    }

    return adapter.build();
  }
}
