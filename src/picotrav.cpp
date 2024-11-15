// Assertions
#include <cassert>

// Data Structures
#include <optional>
#include <unordered_map>
#include <string>
#include <vector>

// Randomisation
#include <random>

// Parser library for BLIF files
#include <blifparse.hpp>
#include <filesystem>

// sorting, shuffling
#include <algorithm>

#include "common/adapter.h"
#include "common/chrono.h"
#include "common/input.h"

// ========================================================================== //
// Parsing input parameters
std::string file_0 = "";
std::string file_1 = "";

enum class variable_order
{
  INPUT,
  ZIP,
  DF,
  DF_LEVEL,
  FUJITA,
  LEVEL,
  LEVEL_DF,
  RANDOM
};

std::string
to_string(const variable_order o)
{
  switch (o) {
  case variable_order::INPUT: return "input";
  case variable_order::ZIP: return "zip";
  case variable_order::DF: return "depth-first";
  case variable_order::DF_LEVEL: return "df-level";
  case variable_order::FUJITA: return "fujita";
  case variable_order::LEVEL: return "level";
  case variable_order::LEVEL_DF: return "level-df";
  case variable_order::RANDOM: return "random";
  }
  return "?";
}

variable_order var_order = variable_order::INPUT;
bool match_io_names      = false;

class parsing_policy
{
public:
  static constexpr std::string_view name = "Picotrav";
  static constexpr std::string_view args = "f:o:m:";

  static constexpr std::string_view help_text =
    "        -f PATH               Path to '.blif' file(s)\n"
    "        -m MATCH     [order]  Matching of circuit inputs and outputs\n"
    "        -o ORDER     [input]  Variable order to derive from first circuit";

  static inline bool
  parse_input(const int c, const char* arg)
  {
    switch (c) {
    case 'f': {
      if (!std::filesystem::exists(arg)) {
        std::cerr << "File '" << arg << "' does not exist\n";
        return true;
      }

      if (file_0 == "") {
        file_0 = arg;
      } else {
        file_1 = arg;
      }
      return false;
    }
    case 'm': {
      const std::string lower_arg = ascii_tolower(arg);

      if (is_prefix(lower_arg, "order")) {
        match_io_names = false;
      } else if (is_prefix(lower_arg, "name")) {
        match_io_names = true;
      } else {
        std::cerr << "Unknown input/output matching: " << arg << "\n";
        return true;
      }
      return false;
    }
    case 'o': {
      const std::string lower_arg = ascii_tolower(arg);

      if (is_prefix(lower_arg, "input")) {
        var_order = variable_order::INPUT;
      } else if (is_prefix(lower_arg, "zip")) {
        var_order = variable_order::ZIP;
      } else if (is_prefix(lower_arg, "depth-first") || lower_arg == "df") {
        var_order = variable_order::DF;
      } else if (is_prefix(lower_arg, "depth-first_level") || lower_arg == "df_level"
                 || lower_arg == "df_l") {
        var_order = variable_order::DF_LEVEL;
      } else if (is_prefix(lower_arg, "fujita")) {
        var_order = variable_order::FUJITA;
      } else if (is_prefix(lower_arg, "level")) {
        var_order = variable_order::LEVEL;
      } else if (is_prefix(lower_arg, "level_depth-first") || lower_arg == "level_df"
                 || lower_arg == "l_df") {
        var_order = variable_order::LEVEL_DF;
      } else if (is_prefix(lower_arg, "random")) {
        var_order = variable_order::RANDOM;
      } else {
        std::cerr << "Undefined ordering: " << arg << "\n";
        return true;
      }
      return false;
    }
    default: return true;
    }
  }
};

// ========================================================================== //
// Parsing input .blif file
enum logic_value
{
  FALSE,
  TRUE,
  DONT_CARE
};

using node_id_t = unsigned;

struct node_t
{
public:
  /// The node's name
  std::string name = {};

  /// The boolean value set on the output plane
  bool is_onset = true;

  /// Only relevant for checking if the net is valid
  ///
  /// After parsing the entire input file, all (reachable) nodes should have
  /// this flag set.
  bool is_defined = false;

  /// True iff this is an input node
  bool is_input = false;
  /// True iff this is an output node
  bool is_output = false;

  /// Input (dependant) nets
  std::vector<node_id_t> deps                    = {};
  std::vector<std::vector<logic_value>> so_cover = {};

  /// Maximal distance from an input node (0 for inputs)
  unsigned depth     = 0;
  unsigned ref_count = 0;
};

struct net_t
{
public:
  std::unordered_map<std::string, node_id_t> name_map = {};

  std::unordered_map<node_id_t, unsigned> inputs_w_order = {};
  std::vector<node_id_t> outputs_in_order                = {};
  /// Node store shared across nets
  ///
  /// Enables us to compute variable orders on the "union" net
  std::vector<node_t>& nodes;

  /// Get the node for `name` (if present) or add `node` to the net
  ///
  /// Returns the pair `(id, inserted)`.
  std::pair<node_id_t, bool>
  get_or_add_node(const std::string& name, node_t node)
  {
    const auto [it, inserted] = name_map.try_emplace(name, nodes.size());
    const node_id_t id        = it->second;
    if (inserted) {
      node.name = name;
      nodes.emplace_back(std::move(node));
    }
    assert(name_map.size() <= nodes.size()); // `name_map` is net-local, `nodes` is shared
    return { id, inserted };
  }

  /// Checks if all reachable nodes are defined and the net is acyclic
  ///
  /// Also computes the nodes' depths and reference counts. This method must not
  /// be called more than once.
  ///
  /// Returns true iff the net is valid
  bool
  validate()
  {
    // If a cycle was found, this contains its nodes in reversed order
    std::vector<node_id_t> cycle;

    for (const node_id_t output : outputs_in_order) {
      if (!validate_rec(output, cycle)) { return false; }
    }
    return true;
  }

private:
  /// Returns the node's depth or none if the validation failed
  std::optional<unsigned>
  validate_rec(const node_id_t id, std::vector<node_id_t>& cycle)
  {
    node_t& node = nodes[id];
    if (node.ref_count++ != 0) {
      // The node has already been visited. We use the depth field to detect
      // cycles: We only set the depth once all children have been visited. So
      // if the depth is still 0 and the node has children, the node is part of
      // a cycle.
      if (node.depth == 0 && !node.deps.empty()) {
        std::cerr << "Cycle detected: " << node.name;
        cycle.push_back(id);
        return {};
      }
      return node.depth;
    }

    if (!node.is_defined) {
      std::cerr << "Referenced net '" << node.name << "' is undefined." << std::endl;
      return {};
    }

    unsigned depth = 0;
    for (const node_id_t dep : node.deps) {
      const std::optional<unsigned> dep_depth = validate_rec(dep, cycle);

      if (!dep_depth) { // error
        if (!cycle.empty()) {
          // A cycle was detected, we capture all the nodes until the first node
          // of the cycle we visited while returning. Then we print the cycle.
          if (cycle[0] != id) {
            cycle.push_back(id);
          } else {
            for (auto it = cycle.rbegin(), end = cycle.rend(); it != end; ++it) {
              std::cerr << " -> " << nodes[*it].name;
            }
            std::cerr << "\n";
            cycle.clear(); // only print the cycle
          }
        }
        return {};
      }

      depth = std::max(depth, *dep_depth + 1);
    }

    // Important: only set `node.depth` after all children have been visited for
    // the cycle detection.
    node.depth = depth;
    return depth;
  }
};

class construct_net_callback : public blifparse::Callback
{
private:
  net_t& net;

  bool has_error_ = false;

  int line_num;
  std::string fname;

public:
  construct_net_callback(net_t& n)
    : blifparse::Callback()
    , net(n)
  {}

  bool
  has_error()
  {
    return has_error_;
  }

public:
  // Sets current filename
  void
  filename(std::string fn) override
  {
    fname = fn;
  }

  // Sets current line number
  void
  lineno(int ln) override
  {
    line_num = ln;
  }

  // Create the input set with the ordering as given in the input file.
  void
  inputs(std::vector<std::string> inputs) override
  {
    net.name_map.reserve(inputs.size());
    net.nodes.reserve(inputs.size());
    net.inputs_w_order.reserve(inputs.size());

    for (const std::string& name : inputs) {
      const auto [id, inserted] =
        net.get_or_add_node(name, { .is_defined = true, .is_input = true });
      if (!inserted) {
        if (!net.nodes[id].is_defined) {
          net.nodes[id].is_input   = true;
          net.nodes[id].is_defined = true;
        } else {
          parse_error(".input", "Net '" + name + "' defined multiple times");
          return;
        }
      }
      net.inputs_w_order.insert({ id, net.inputs_w_order.size() });
    }
  }

  // Note down what nets are outputs
  void
  outputs(std::vector<std::string> outputs) override
  {
    net.name_map.reserve(outputs.size());
    net.nodes.reserve(outputs.size());
    net.outputs_in_order.reserve(outputs.size());

    for (const std::string& name : outputs) {
      const auto [id, inserted] = net.get_or_add_node(name, { .is_output = true });
      if (!inserted) {
        if (!net.nodes[id].is_output) {
          net.nodes[id].is_output = true;
        } else {
          parse_error(".output", "Output '" + name + "' given twice");
          return;
        }
      }
      net.outputs_in_order.push_back(id);
    }
  }

  // Construct a node in the net
  void
  names(std::vector<std::string> nets,
        std::vector<std::vector<blifparse::LogicValue>> so_cover) override
  {
    if (nets.size() == 0) {
      parse_error(".names", "at least one net name should be given");
      return;
    }

    { // Create a new_node for this Net
      const std::string new_name = std::move(nets.back());
      nets.pop_back(); // from now on, `nets` only contains the dependencies

      const node_id_t id = net.get_or_add_node(new_name, {}).first;
      if (net.nodes[id].is_defined) {
        parse_error(".names - " + new_name, "Net '" + new_name + "' defined multiple times");
        return;
      }
      net.nodes[id].is_defined = true;

      net.nodes[id].deps.reserve(nets.size());
      for (const std::string& name : nets) {
        const node_id_t dep_id = net.get_or_add_node(name, {}).first;
        net.nodes[id].deps.push_back(dep_id);
      }

      // Important: only create the reference here, because
      // `net.get_or_add_node(..)` may resize the `net.nodes` and thereby
      // invalidate the reference.
      node_t& node = net.nodes[id];

      node.so_cover.reserve(so_cover.size());
      bool has_is_onset = false;
      for (const std::vector<blifparse::LogicValue>& row : so_cover) {
        if (row.size() != nets.size() + 1) {
          parse_error(".names - " + new_name,
                      "Incompatible number of logic values defined on a row");
          return;
        }

        std::vector<logic_value> new_row;
        new_row.reserve(row.size() - 1);
        bool row_is_onset = true;

        auto it = row.begin();
        do {
          blifparse::LogicValue v = *it;

          const bool is_out_plane = ++it == row.end();

          switch (v) {
          case blifparse::LogicValue::FALSE:
            if (is_out_plane) {
              row_is_onset = false;
            } else {
              new_row.push_back(logic_value::FALSE);
            }
            break;

          case blifparse::LogicValue::TRUE:
            if (is_out_plane) {
              row_is_onset = true;
            } else {
              new_row.push_back(logic_value::TRUE);
            }
            break;

          case blifparse::LogicValue::DONT_CARE:
            if (is_out_plane) {
              parse_error(".names - " + new_name, "Cannot have 'dont care' in output plane");
            } else {
              new_row.push_back(logic_value::DONT_CARE);
            }
            break;

          case blifparse::LogicValue::UNKNOWN:
            parse_error(".names - " + new_name, "Cannot deal with 'unknown' logic value");
          }
        } while (it != row.end());

        if (has_is_onset && row_is_onset != node.is_onset) {
          parse_error(line_num,
                      ".names - " + new_name,
                      "Cannot handle both on-set and off-set in output plane");
          return;
        }

        has_is_onset  = true;
        node.is_onset = row_is_onset;

        node.so_cover.push_back(new_row);
      }
    }
  }

  void
  latch(std::string /* input */,
        std::string /* output */,
        blifparse::LatchType /* type */,
        std::string /* control */,
        blifparse::LogicValue /* init */) override
  {
    // TODO: When state transitions are used, then add <x> and <x'> variables
    parse_error(".latch", "State transitions not (yet) supported");
  }

  void
  parse_error(const std::string& near_text, const std::string& msg)
  {
    parse_error(line_num, near_text, msg);
  }

  void
  parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) override
  {
    std::cerr << "Parsing error at line " << curr_lineno << " near '" << near_text << "': " << msg
              << "\n";
    has_error_ = true;
  }
};

/// Parse the net at `filename` and validate it
///
/// Returns true on success
bool
construct_net(std::string& filename, net_t& net)
{
  construct_net_callback callback(net);
  blifparse::blif_parse_filename(filename, callback);
  if (callback.has_error()) {
    std::cerr << "Parsing error for '" << filename << "'\n";
    return false;
  }
  if (!net.validate()) {
    std::cerr << "Validation error for '" << filename << "'\n";
    return false;
  }
  return true;
}

// ========================================================================== //
// Variable Ordering
template<typename RecursionOrder>
void
df_variable_order_rec(const node_id_t id,
                      std::vector<unsigned>& new_ordering,
                      unsigned& ordered_count,
                      const net_t& net,
                      std::vector<bool>& visited)
{
  if (ordered_count == net.inputs_w_order.size()) { return; }

  if (visited[id]) { return; }
  visited[id] = true;

  const node_t& n = net.nodes[id];

  // Case: input gate
  if (n.is_input) {
    const unsigned old_idx = net.inputs_w_order.at(id);
    new_ordering[old_idx]  = ordered_count++;
    return;
  }

  // Case: non-input gates
  for (const node_id_t dep_id : RecursionOrder::sort(n.deps, net)) {
    df_variable_order_rec<RecursionOrder>(dep_id, new_ordering, ordered_count, net, visited);
  }
}

template<typename RecursionOrder>
std::vector<unsigned>
df_variable_order(const net_t& net_0, const net_t& net_1)
{
  assert(&net_0.nodes == &net_1.nodes); // pointer equality

  // TODO: Break ties based on previous ordering.
  // TODO: Derive variable ordering from the structure of 'net_1' too?

  // Output value
  assert(net_0.inputs_w_order.size() == net_1.inputs_w_order.size());
  std::vector<unsigned> new_ordering(net_0.inputs_w_order.size());

  // Data structures for a depth-first traversal (of net_0 only).
  std::vector<bool> visited(net_0.nodes.size());
  unsigned int ordered_count = 0;

  // Derive a variable order from net_0, i.e.\ the specification.
  for (const node_id_t output : RecursionOrder::sort_outputs(net_0)) {
    df_variable_order_rec<RecursionOrder>(output, new_ordering, ordered_count, net_0, visited);
  }

  return new_ordering;
}

/// \brief Simple depth-first policy for `df_variable_order`.
struct df_policy
{
  static std::vector<node_id_t>
  sort(const std::vector<node_id_t>& deps, const net_t&/*net*/)
  {
    return deps;
  }

  static std::vector<node_id_t>
  sort_outputs(const net_t& net)
  {
    return net.outputs_in_order;
  }
};

/// \brief Depth-first policy for `df_variable_order` that sorts dependencies on their 'depth'.
struct df_level_policy
{
  static std::vector<node_id_t>
  sort(const std::vector<node_id_t>& deps, const net_t& net)
  {
    std::vector<node_id_t> deps_copy(deps);
    std::sort(deps_copy.begin(), deps_copy.end(), [&net](const node_id_t a, const node_id_t b) {
      return net.nodes[a].depth > net.nodes[b].depth;
    });
    return deps_copy;
  }

  static std::vector<node_id_t>
  sort_outputs(const net_t& net)
  {
    std::vector<node_id_t> outputs(net.outputs_in_order);
    std::sort(outputs.begin(), outputs.end(), [&net](const node_id_t a, const node_id_t b) {
      return net.nodes[a].depth > net.nodes[b].depth;
    });

    return outputs;
  }
};

/// \brief Depth-first policy for `df_variable_order` that obtains dependencies based on their
///        'fanin' as in the paper "Evaluation and Improvements of Boolean Comparison Methods Based
///        on Binary Decision Diagarms" by Fujita et al.
struct df_fujita_policy
{
  static std::vector<node_id_t>
  sort(const std::vector<node_id_t>& deps, const net_t& net)
  {
    std::vector<node_id_t> deps_copy(deps);
    std::sort(deps_copy.begin(), deps_copy.end(), [&net](const node_id_t a, const node_id_t b) {
      // If both are non-inputs, then recurse on them based on depth.
      if (!net.nodes[a].is_input && !net.nodes[b].is_input) {
        return net.nodes[a].depth > net.nodes[b].depth;
      }

      // If both are inputs, then sort them based on their ref count (secondly their depth)
      if (net.nodes[a].is_input && net.nodes[b].is_input) {
        const auto a_cmp = std::tie(net.nodes[a].ref_count, net.nodes[a].depth);
        const auto b_cmp = std::tie(net.nodes[b].ref_count, net.nodes[b].depth);
        return a_cmp > b_cmp;
      }

      // Input gates with a single reference should be processed after all other gates. On the other
      // hand, the paper follows the (arbitrary) order of the input. Yet, that is for all purposes
      // non-deterministic.
      //
      // Uncommenting the code below makes input gates with 2+ references be processed prior to any
      // recursions. In practice, recursing first seems to create the best inputs.
      /*
      if (net.nodes[a].ref_count > 1 && !net.nodes[b].is_input) {
        return true;
      }
      if (net.nodes[b].ref_count > 1 && !net.nodes[a].is_input) {
        return false;
      }
      */

      // Inputs with only 1 input should be processed after recursions. Due to the above
      // conditionals, this boils down to whether 'a' is the non-input gate.
      return !net.nodes[a].is_input;
    });
    return deps_copy;
  }

  static std::vector<node_id_t>
  sort_outputs(const net_t& net)
  {
    std::vector<node_id_t> outputs(net.outputs_in_order);
    std::sort(outputs.begin(), outputs.end(), [&net](const node_id_t a, const node_id_t b) {
      if (net.nodes[a].is_input != net.nodes[b].is_input) {
        return net.nodes[a].is_input > net.nodes[b].is_input;
      }

      const auto a_cmp = std::tie(net.nodes[a].ref_count, net.nodes[a].depth);
      const auto b_cmp = std::tie(net.nodes[b].ref_count, net.nodes[b].depth);
      return a_cmp > b_cmp;
    });

    return outputs;
  }
};

// For each input, compute the smallest level/depth of a node referencing it
void
compute_input_depth(const node_id_t id,
                    std::unordered_map<node_id_t, unsigned>& deepest_reference,
                    const std::vector<node_t>& nodes,
                    std::vector<bool>& visited)
{
  if (visited[id]) { return; }
  visited[id] = true;

  const node_t& n = nodes[id];

  for (const node_id_t dep_id : n.deps) {
    const node_t& dep = nodes[dep_id];
    if (dep.is_input) {
      unsigned& lookup_depth = deepest_reference[dep_id];
      // If dep_id was not in the map, the value is default-constructed, i.e. 0.
      // All inner nodes have at least depth 1, so we can distinguish this case.
      if (lookup_depth == 0 || n.depth < lookup_depth) { lookup_depth = n.depth; }
    } else {
      compute_input_depth(dep_id, deepest_reference, nodes, visited);
    }
  }
}

std::vector<unsigned>
level_variable_order(const net_t& net_0, const net_t& net_1)
{
  assert(&net_0.nodes == &net_1.nodes); // pointer equality
  const std::vector<node_t>& nodes = net_0.nodes;

  // Create a `std::vector` we can sort
  std::vector<node_id_t> inputs;
  inputs.reserve(net_0.inputs_w_order.size());
  for (auto kv : net_0.inputs_w_order) { inputs.push_back(kv.first); }

  std::unordered_map<node_id_t, unsigned> deepest_reference;
  std::vector<bool> visited_nodes(nodes.size());
  deepest_reference.reserve(net_0.inputs_w_order.size());
  for (const node_id_t output : net_0.outputs_in_order) {
    compute_input_depth(output, deepest_reference, nodes, visited_nodes);
  }
  for (const node_id_t output : net_1.outputs_in_order) {
    compute_input_depth(output, deepest_reference, nodes, visited_nodes);
  }

  // Sort based on deepest referenced level (break ties by prior ordering)
  const auto comparator = [&net_0, &deepest_reference](const node_id_t i, const node_id_t j) {
    const int old_i   = net_0.inputs_w_order.at(i);
    const int i_level = deepest_reference.at(i);
    const int old_j   = net_0.inputs_w_order.at(j);
    const int j_level = deepest_reference.at(j);

    return std::tie(i_level, old_i) < std::tie(j_level, old_j);
  };

  std::sort(inputs.begin(), inputs.end(), comparator);

  // Map into new ordering
  std::vector<unsigned> new_ordering(inputs.size());
  for (size_t idx = 0; idx < inputs.size(); idx++) {
    new_ordering[net_0.inputs_w_order.at(inputs[idx])] = idx;
  }

  return new_ordering;
}

/// Assuming the input is given as
/// `a[0], a[1], ..., a[n-1], b[0], b[1], ..., b[n-1]`, this derives the
/// ordering ``a[0], b[0], a[1], b[1], ..., a[n-1], b[n-1]``
std::vector<unsigned>
zip_variable_order(const net_t& net)
{
  unsigned n = net.inputs_w_order.size();
  std::vector<unsigned> permutation;
  permutation.reserve(n);
  // The permutation needs to be `0 2 4 ... 1 3 5 ...`
  // In case there is an odd number of inputs, the first partition half must be
  // larger, e.g. we have `0 2 1` for n = 3.
  unsigned first_half  = (n + 1) / 2;
  unsigned second_half = n - first_half;
  for (unsigned i = 0; i < first_half; ++i) { permutation.push_back(2 * i); }
  for (unsigned i = 0; i < second_half; ++i) { permutation.push_back(2 * i + 1); }
  return permutation;
}

std::vector<unsigned>
random_variable_order(const net_t& net)
{
  // Create a vector we can shuffle
  std::vector<unsigned> permutation;
  permutation.reserve(net.inputs_w_order.size());
  for (unsigned i = 0; i < net.inputs_w_order.size(); ++i) { permutation.push_back(i); }

  std::random_device rd;
  std::mt19937 gen(rd());

  std::shuffle(permutation.begin(), permutation.end(), gen);

  return permutation;
}

/// `new_ordering[i]` is the new position of the variable currently at position
/// `i`
void
update_order(net_t& net, const std::vector<unsigned>& new_ordering)
{
  for (auto& [key, value] : net.inputs_w_order) { value = new_ordering[value]; }
}

void
apply_variable_order(const variable_order vo, net_t& net_0, net_t& net_1)
{
  std::vector<unsigned> new_ordering;

  switch (vo) {
  case variable_order::INPUT:
    // Keep as is
    return;

  case variable_order::ZIP: {
    new_ordering = zip_variable_order(net_0);
    break;
  }

  case variable_order::DF: {
    new_ordering = df_variable_order<df_policy>(net_0, net_1);
    break;
  }

  case variable_order::DF_LEVEL: {
    new_ordering = df_variable_order<df_level_policy>(net_0, net_1);
    break;
  }

  case variable_order::FUJITA: {
    new_ordering = df_variable_order<df_fujita_policy>(net_0, net_1);
    break;
  }

  case variable_order::LEVEL: {
    new_ordering = level_variable_order(net_0, net_1);
    break;
  }

  case variable_order::LEVEL_DF: {
    apply_variable_order(variable_order::DF, net_0, net_1);
    new_ordering = level_variable_order(net_0, net_1);
    break;
  }

  case variable_order::RANDOM: {
    new_ordering = random_variable_order(net_0);
    break;
  }
  }

  update_order(net_0, new_ordering);
  if (net_1.inputs_w_order.size() == net_0.inputs_w_order.size()) {
    update_order(net_1, new_ordering);
  }
}

// ========================================================================== //
// Depth-first BDD construction of net gate

struct bdd_statistics
{
#ifdef BDD_BENCHMARK_STATS
  size_t total_processed = 0;
  size_t total_negations = 0;
  size_t total_applys    = 0;
  size_t max_bdd_size    = 0;
  size_t curr_bdd_sizes  = 0;
  size_t max_bdd_sizes   = 0;
  size_t sum_bdd_sizes   = 0;
  size_t max_roots       = 0;
  size_t max_allocated   = 0;
  size_t sum_allocated   = 0;
#endif // BDD_BENCHMARK_STATS
};

template <typename Adapter>
using bdd_cache = std::unordered_map<node_id_t, typename Adapter::dd_t>;

template <typename Adapter>
typename Adapter::dd_t
construct_node_bdd(net_t& net,
                   const node_id_t node_id,
                   bdd_cache<Adapter>& cache,
                   Adapter& adapter,
                   bdd_statistics& stats)
{
  const auto lookup_cache = cache.find(node_id);
  if (lookup_cache != cache.end()) { return lookup_cache->second; }

  const node_t& node_data = net.nodes[node_id];
  if (node_data.is_input) { return adapter.ithvar(net.inputs_w_order.at(node_id)); }

  typename Adapter::dd_t so_cover_bdd = adapter.bot();
#ifdef BDD_BENCHMARK_STATS
  size_t so_nodecount = adapter.nodecount(so_cover_bdd);
  assert(so_nodecount <= 1);
  stats.curr_bdd_sizes += so_nodecount;
#endif // BDD_BENCHMARK_STATS

  for (size_t row_idx = 0; row_idx < node_data.so_cover.size(); row_idx++) {
    typename Adapter::dd_t tmp = adapter.top();

    for (size_t column_idx = 0; column_idx < node_data.deps.size(); column_idx++) {
      const node_id_t dep_id         = node_data.deps.at(column_idx);
      typename Adapter::dd_t dep_bdd = construct_node_bdd(net, dep_id, cache, adapter, stats);

      // Add to row accumulation in 'tmp'
      const logic_value lval = node_data.so_cover.at(row_idx).at(column_idx);
      switch (lval) {
      case logic_value::FALSE: tmp &= ~dep_bdd;
#ifdef BDD_BENCHMARK_STATS
        // TODO (ZDD): intermediate size of negation
        {
          const size_t tmp_nodecount = adapter.nodecount(tmp);
          stats.total_processed += tmp_nodecount;

          stats.total_negations++;
          stats.total_applys++;

          stats.max_bdd_size = std::max(stats.max_bdd_size, tmp_nodecount);
        }
#endif // BDD_BENCHMARK_STATS
        break;

      case logic_value::TRUE: tmp &= dep_bdd;
#ifdef BDD_BENCHMARK_STATS
        {
          const size_t tmp_nodecount = adapter.nodecount(tmp);
          stats.total_processed += tmp_nodecount;

          stats.total_applys++;

          stats.max_bdd_size = std::max(stats.max_bdd_size, tmp_nodecount);
        }
#endif // BDD_BENCHMARK_STATS
        break;

      case logic_value::DONT_CARE:
        // Just do nothing
        break;
      }

      // Decrease reference count on dependency if we are on the last row.
      if (row_idx == node_data.so_cover.size() - 1) {
        node_t& dep_node = net.nodes[dep_id];
        if (!dep_node.is_output && !dep_node.is_input && --dep_node.ref_count == 0) {
          cache.erase(dep_id);
#ifdef BDD_BENCHMARK_STATS
          const size_t dep_nodecount = adapter.nodecount(dep_bdd);
          assert(dep_nodecount <= stats.curr_bdd_sizes);
          stats.curr_bdd_sizes -= dep_nodecount;
#endif // BDD_BENCHMARK_STATS
        }
      }
    }

    so_cover_bdd |= tmp;

#ifdef BDD_BENCHMARK_STATS
    assert(so_nodecount <= stats.curr_bdd_sizes);
    stats.curr_bdd_sizes -= so_nodecount;
    so_nodecount = adapter.nodecount(so_cover_bdd);
    stats.curr_bdd_sizes += so_nodecount;

    stats.total_processed += so_nodecount;
    stats.total_applys++;

    stats.max_bdd_size = std::max(stats.max_bdd_size, so_nodecount);

    stats.max_bdd_sizes = std::max(stats.max_bdd_sizes, stats.curr_bdd_sizes);
    stats.sum_bdd_sizes += stats.curr_bdd_sizes;

    stats.max_allocated = std::max(stats.max_allocated, adapter.allocated_nodes());
    stats.sum_allocated += adapter.allocated_nodes();
#endif // BDD_BENCHMARK_STATS
  }

  if (!node_data.is_onset) {
    so_cover_bdd = ~so_cover_bdd;
#ifdef BDD_BENCHMARK_STATS
    stats.total_negations++;
#endif // BDD_BENCHMARK_STATS
    // TODO (ZDD): remaining statistics
  }

  cache.insert({ node_id, so_cover_bdd });
#ifdef BDD_BENCHMARK_STATS
  stats.max_roots = std::max(stats.max_roots, cache.size());
#endif // BDD_BENCHMARK_STATS
  return so_cover_bdd;
}

// ========================================================================== //
// Construct the BDD for each output gate
template <typename Adapter>
std::pair<int, time_duration>
construct_net_bdd(const std::string& filename,
                  net_t& net,
                  bdd_cache<Adapter>& cache,
                  Adapter& adapter)
{
  if (cache.size() > 0) {
    std::cerr << "Given BDD cache is non-empty";
    return { -1, 0 };
  }

  std::cout << json::indent << json::brace_open << json::endl;
  std::cout << json::field("path") << json::value(filename) << json::comma << json::endl;
  std::cout << json::field("input gates") << json::value(net.inputs_w_order.size()) << json::comma
            << json::endl;
  std::cout << json::field("output gates") << json::value(net.outputs_in_order.size())
            << json::comma << json::endl;
  std::cout << json::field("total gates") << json::value(net.nodes.size()) << json::comma
            << json::endl;
  std::cout << json::endl;

  const time_point t_construct_before = now();
  bdd_statistics stats;
  for (const node_id_t output : net.outputs_in_order) {
    construct_node_bdd(net, output, cache, adapter, stats);
  }
  const time_point t_construct_after = now();

  size_t sum_final_sizes = 0;
  size_t max_final_size  = 0;
  for (auto kv : cache) {
    size_t nodecount = adapter.nodecount(kv.second);
    sum_final_sizes += nodecount;
    max_final_size = std::max(max_final_size, nodecount);
  }

  std::cout << json::field("final_diagrams") << json::brace_open << json::endl;
  std::cout << json::field("size[max] (nodes)") << json::value(max_final_size) << json::comma
            << json::endl;
  std::cout << json::field("size[sum] (nodes)") << json::value(sum_final_sizes) << json::comma
            << json::endl;
  std::cout << json::field("allocated (nodes)") << json::value(adapter.allocated_nodes())
            << json::endl;
  std::cout << json::brace_close << json::comma << json::endl;

#ifdef BDD_BENCHMARK_STATS
  std::cout << json::field("life_time") << json::brace_open << json::endl;
  std::cout << json::field("total processed (nodes)") << stats.total_processed << json::comma
            << json::endl;
  std::cout << json::field("max roots") << json::value(stats.max_roots) << json::comma
            << json::endl;
  std::cout << json::field("size[max] (nodes)") << json::value(stats.max_bdd_size) << json::comma
            << json::endl;
  std::cout << json::field("sizes[sum] (nodes)") << json::value(stats.sum_bdd_sizes) << json::comma
            << json::endl;
  std::cout << json::field("sizes[max] (nodes)") << json::value(stats.max_bdd_sizes) << json::comma
            << json::endl;
  std::cout << json::field("allocated[sum]") << json::value(stats.sum_allocated) << json::comma
            << json::endl;
  std::cout << json::field("allocated[max]") << json::value(stats.max_allocated) << json::endl;
  std::cout << json::brace_close << json::comma << json::endl;

  std::cout << json::field("operation count") << json::brace_open << json::endl;
  std::cout << json::field("apply") << json::value(stats.total_applys) << json::comma << json::endl;
  std::cout << json::field("not") << json::value(stats.total_negations) << json::endl;
  std::cout << json::brace_close << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS

  const time_duration total_time = duration_ms(t_construct_before, t_construct_after);
  std::cout << json::field("time (ms)") << total_time << json::endl;
  std::cout << json::brace_close; // << json::endl

  return { 0, total_time };
}

// ========================================================================== //
// Test equivalence of every output gate (in-order they were given)
template <typename Adapter>
std::pair<bool, time_duration>
verify_outputs(Adapter& adapter,
               const net_t& net_0,
               const bdd_cache<Adapter>& cache_0,
               const net_t& net_1,
               const bdd_cache<Adapter>& cache_1)
{
  // The cache should have exactly as many entries as there are outputs that are
  // not inputs as well.
  assert(cache_0.size()
         == size_t(std::count_if(net_0.outputs_in_order.begin(),
                                 net_0.outputs_in_order.end(),
                                 [&net_0](node_id_t id) { return !net_0.nodes[id].is_input; })));
  assert(cache_1.size()
         == size_t(std::count_if(net_1.outputs_in_order.begin(),
                                 net_1.outputs_in_order.end(),
                                 [&net_1](node_id_t id) { return !net_1.nodes[id].is_input; })));
  assert(net_0.outputs_in_order.size() == net_1.outputs_in_order.size());

  std::cout << json::field("equal") << json::brace_open << json::endl;

  const time_point t_compare_before = now();
  bool ret_value                    = true;

  for (size_t out_idx = 0; out_idx < net_0.outputs_in_order.size(); out_idx++) {
    const node_id_t output_0 = net_0.outputs_in_order.at(out_idx);
    const node_id_t output_1 = net_1.outputs_in_order.at(out_idx);

    const typename Adapter::dd_t bdd_0 = net_0.nodes[output_0].is_input
      ? adapter.ithvar(net_0.inputs_w_order.at(output_0))
      : cache_0.at(output_0);
    const typename Adapter::dd_t bdd_1 = net_1.nodes[output_1].is_input
      ? adapter.ithvar(net_1.inputs_w_order.at(output_1))
      : cache_1.at(output_1);

    if (bdd_0 != bdd_1) {
      if (match_io_names) {
        assert(net_0.nodes[output_0].name == net_1.nodes[output_1].name);
        std::cerr << "Output gate '" << net_0.nodes[output_0].name << "' differs!\n";
      } else {
        std::cerr << "Output gate ['" << net_0.nodes[output_0].name << "'/'"
                  << net_1.nodes[output_1].name << "'] differs!\n";
      }
      ret_value = false;
    }
  }
  const time_point t_compare_after = now();
  std::cout << json::field("result") << json::value(ret_value) << json::comma << json::endl;

  const time_duration time = duration_ms(t_compare_before, t_compare_after);
  std::cout << json::field("time (ms)") << json::value(time) << json::endl;

  std::cout << json::brace_close << json::comma << json::endl;
  return { ret_value, time };
}

// ========================================================================== //

/// Perform name-based matching of inputs and outputs
///
/// This permutes the inputs and outputs of `net_1` accordingly.
///
/// Returns true on success
bool
do_match_io_names(net_t& net_0, net_t& net_1)
{
  // inputs
  for (auto& [id_0, pos_0] : net_0.inputs_w_order) {
    const std::string& name = net_0.nodes[id_0].name;
    const auto name_map_it  = net_1.name_map.find(name);
    if (name_map_it != net_1.name_map.end()) {
      const auto order_it = net_1.inputs_w_order.find(name_map_it->second);
      if (order_it != net_1.inputs_w_order.end()) {
        order_it->second = pos_0;
        continue; // success, continue with the next input
      }
    }

    std::cerr << "Input '" << name << "' is present in '" << file_0 << "' but not in '" << file_1
              << "'\n";
    return false;
  }

  // outputs
  for (unsigned i = 0; i < net_0.outputs_in_order.size(); ++i) {
    const std::string& name = net_0.nodes[net_0.outputs_in_order[i]].name;
    const auto name_map_it  = net_1.name_map.find(name);
    if (name_map_it != net_1.name_map.end()) {
      const node_id_t id_1 = name_map_it->second;
      if (net_1.nodes[id_1].is_output) {
        net_1.outputs_in_order[i] = id_1;
        continue; // success, continue with the next input
      }
    }

    std::cerr << "Output '" << name << "' is present in '" << file_0 << "' but not in '" << file_1
              << "'\n";
    return false;
  }

  return true;
}

// ========================================================================== //
template <typename Adapter>
int
run_picotrav(int argc, char** argv)
{
  const bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  if (file_0 == "") {
    std::cerr << "Input file(s) not specified\n";
    return -1;
  }
  const bool verify_networks = file_1 != "";

  // =========================================================================
  // Read file(s) and construct nets
  std::vector<node_t> nodes;

  net_t net_0 = { .nodes = nodes };
  if (!construct_net(file_0, net_0)) return -1; // error has been printed

  net_t net_1 = { .nodes = nodes };
  if (verify_networks) {
    if (!construct_net(file_1, net_1)) return -1; // error has been printed

    const bool inputs_match = net_0.inputs_w_order.size() == net_1.inputs_w_order.size();
    if (!inputs_match) {
      std::cerr << "Number of inputs in '" << file_0 << "' and '" << file_1 << "' do not match!\n";
      return -1;
    }

    const bool outputs_match = net_0.outputs_in_order.size() == net_1.outputs_in_order.size();
    if (!outputs_match) {
      std::cerr << "Number of outputs in '" << file_0 << "' and '" << file_1 << "' do not match!\n";
      return -1;
    }

    if (match_io_names) {
      if (!do_match_io_names(net_0, net_1)) return -1;
    }
  }

  // Nanotrav sorts the output in ascending order by their level. The same is
  // possible here, but experiments show this at times decreases and other
  // times increases the running time.
  //
  // So, well keep it simple by not doing so.

  // Derive variable order
  apply_variable_order(var_order, net_0, net_1);

  // ========================================================================
  // Initialise BDD package manager
  const size_t varcount = net_0.inputs_w_order.size();

  return run<Adapter>("Picotrav", varcount, [&](Adapter& adapter) {
    std::cout << json::field("variable order") << json::value(to_string(var_order)) << json::comma
              << json::endl;
    std::cout << json::endl;

    // ========================================================================
    // Construct BDD for first net
    time_duration total_time = 0;

    std::cout << json::field("apply+not") << json::array_open << json::endl;

    bdd_cache<Adapter> cache_0;

    bool networks_equal = true;

    const auto [errcode_0, time_0] = construct_net_bdd(file_0, net_0, cache_0, adapter);

    if (errcode_0) { return errcode_0; }
    total_time += time_0;

    // ========================================================================
    // Construct BDD for second net (if any) and compare them
    if (verify_networks) {
      std::cout << json::comma << json::endl;

      bdd_cache<Adapter> cache_1;

      const auto [errcode_1, time_1] = construct_net_bdd(file_1, net_1, cache_1, adapter);

      if (errcode_1) { return errcode_1; }
      total_time += time_1;

      std::cout << json::endl;
      std::cout << json::array_close << json::comma << json::endl;

      const auto [verified, time_eq] =
        verify_outputs<Adapter>(adapter, net_0, cache_0, net_1, cache_1);

      networks_equal = verified;
      total_time += time_eq;
    } else {
      std::cout << json::endl;
      std::cout << json::array_close << json::comma << json::endl;
    }

    std::cout << json::field("total time (ms)") << json::value(init_time + total_time)
              << json::endl;

    if (verify_networks && !networks_equal) { return -1; }
    return 0;
  });
}
