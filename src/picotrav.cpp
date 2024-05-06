// Assertions
#include <cassert>

// Data Structures
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>
#include <vector>

// Randomisation
#include <random>

// Parser library for BLIF files
#include <blifparse.hpp>
#include <filesystem>

#include "common/adapter.h"
#include "common/chrono.h"
#include "common/input.h"

// ========================================================================== //
// Parsing input parameters
std::string file_0 = "";
std::string file_1 = "";

enum variable_order
  {
    INPUT,
    DF,
    LEVEL,
    LEVEL_DF,
    RANDOM
  };

std::string
to_string(const variable_order& o)
{
  switch (o) {
  case INPUT:    return "input";
  case DF:       return "depth-first";
  case LEVEL:    return "level";
  case LEVEL_DF: return "level-df";
  case RANDOM:   return "random";
  }
  return "?";
}

variable_order var_order = variable_order::INPUT;

class parsing_policy
{
public:
  static constexpr std::string_view name = "Picotrav";
  static constexpr std::string_view args = "f:o:";

  static constexpr std::string_view help_text =
    "        -f PATH              Path to '.blif' file(s)\n"
    "        -o ORDER    [input]  Variable order to derive from first circuit";

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
    case 'o': {
      const std::string lower_arg = ascii_tolower(arg);

      if (lower_arg == "input" || lower_arg == "i") {
        var_order = variable_order::INPUT;
      } else if (lower_arg == "depth-first" || lower_arg == "df") {
        var_order = variable_order::DF;
      } else if (lower_arg == "level" || lower_arg == "l") {
        var_order = variable_order::LEVEL;
      } else if (lower_arg == "level_depth-first" || lower_arg == "l_df") {
        var_order = variable_order::LEVEL_DF;
      } else if (lower_arg == "random" || lower_arg == "r") {
        var_order = variable_order::RANDOM;
      } else {
        std::cerr << "Undefined ordering: " << arg << "\n";
        return true;
      }
      return false;
    }
    default:
      return true;
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

struct node_t
{
public:
  // The boolean value set on the output plane
  bool is_onset = true;

  // Input (dependant) nets
  std::vector<std::string> nets;
  std::vector<std::vector<logic_value>> so_cover;
};

struct net_t
{
public:
  std::unordered_map<std::string, int> inputs_w_order;
  std::unordered_set<std::string> outputs;
  std::vector<std::string> outputs_in_order;
  std::unordered_map<std::string, int> level;
  std::unordered_map<std::string, int> ref_count;
  std::unordered_map<std::string, node_t> nodes;

  bool
  is_input(const std::string& n) const
  {
    const auto lookup_inputs = inputs_w_order.find(n);
    return lookup_inputs != inputs_w_order.end();
  }

  bool
  is_output(const std::string& n) const
  {
    const auto lookup_outputs = outputs.find(n);
    return lookup_outputs != outputs.end();
  }
};

class construct_net_callback : public blifparse::Callback
{
private:
  int input_idx = 0;

  net_t& net;

  bool has_error_ = false;
  bool has_names_ = false;

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
    if (has_names_) { parse_error(".inputs", "Defining '.inputs' after a '.names'"); }

    for (const std::string& i : inputs) { net.inputs_w_order.insert({ i, input_idx++ }); }
  }

  // Note down what nets are outputs
  void
  outputs(std::vector<std::string> outputs) override
  {
    if (has_names_) { parse_error(".outputs", "Defining '.outputs' after a '.names'"); }

    for (const std::string& o : outputs) {
      net.outputs.insert(o);
      net.outputs_in_order.push_back(o);
    }
  }

  // Construct a node in the net
  void
  names(std::vector<std::string> nets,
        std::vector<std::vector<blifparse::LogicValue>> so_cover) override
  {
    has_names_ = true;

    if (nets.size() == 0) {
      parse_error(".names", "at least one net name should be given");
      return;
    }

    { // Create a new_node for this Net
      bool has_is_onset = false;
      bool new_is_onset = false;

      const std::string new_name = nets[nets.size() - 1];

      if (net.nodes.find(new_name) != net.nodes.end()) {
        parse_error(".names - " + new_name, "Net '" + new_name + "' defined multiple times");
        return;
      }

      std::vector<std::string> new_nets(nets.begin(), nets.end() - 1);
      std::vector<std::vector<logic_value>> new_so_cover;

      for (const std::vector<blifparse::LogicValue>& row : so_cover) {
        if (row.size() != nets.size()) {
          parse_error(".names - " + new_name,
                      "Incompatible number of logic values defined on a row");
          return;
        }

        std::vector<logic_value> new_row;
        bool row_is_onset = false;

        auto it = row.begin();
        do {
          blifparse::LogicValue v = *it;

          const bool is_out_plane = ++it == row.end();

          switch (v) {
          case blifparse::LogicValue::FALSE:
            if (is_out_plane) {
              row_is_onset = true;
            } else {
              new_row.push_back(logic_value::TRUE);
            }
            break;

          case blifparse::LogicValue::TRUE:
            if (is_out_plane) {
              row_is_onset = false;
            } else {
              new_row.push_back(logic_value::FALSE);
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

        if (has_is_onset && row_is_onset != new_is_onset) {
          parse_error(line_num,
                      ".names - " + new_name,
                      "Cannot handle both on-set and off-set in output plane");
          return;
        }

        has_is_onset = true;
        new_is_onset = row_is_onset;

        new_so_cover.push_back(new_row);
      }

      const node_t new_node = { new_is_onset, new_nets, new_so_cover };
      net.nodes.insert({ new_name, new_node });
    }

    // Update the reference counter on all input nets
    for (size_t net_idx = 0; net_idx + 1 < nets.size(); net_idx++) {
      const std::string& dep_name = nets[net_idx];

      if (net.is_input(dep_name)) { continue; }
      if (net.is_output(dep_name)) { continue; }

      auto lookup_ref_count = net.ref_count.find(dep_name);
      if (lookup_ref_count == net.ref_count.end()) { // First time referenced
        net.ref_count.insert({ dep_name, 1 });
      } else {
        const int new_ref_count = lookup_ref_count->second + 1;
        net.ref_count.erase(dep_name);
        net.ref_count.insert({ dep_name, new_ref_count });
      }

      net.ref_count.find(dep_name);
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

bool
construct_net(std::string& filename, net_t& net)
{
  construct_net_callback callback(net);
  blifparse::blif_parse_filename(filename, callback);
  return callback.has_error();
}

// ========================================================================== //
// Cycle detection
bool
is_acyclic_rec(const std::string& node_name,
               const net_t& net,
               std::unordered_set<std::string>& net_visited,
               std::vector<std::string>& pth,
               std::unordered_set<std::string>& pth_visited)
{
  if (net.is_input(node_name)) { return true; }

  const auto lookup_pth_visited = pth_visited.find(node_name);
  if (lookup_pth_visited != pth_visited.end()) {
    std::cerr << "Cycle detected: ";
    for (const std::string& n : pth) { std::cerr << n << " -> "; }
    std::cerr << node_name << "\n";

    return false;
  }

  const auto lookup_prior_visited = net_visited.find(node_name);
  if (lookup_prior_visited != pth_visited.end()) { return true; }
  net_visited.insert(node_name);

  bool result = true;

  pth_visited.insert(node_name);
  pth.push_back(node_name);

  const auto lookup_node = net.nodes.find(node_name);

  if (lookup_node == net.nodes.end()) {
    std::cerr << "Referenced net '" << node_name << "' is undefined." << std::endl;
    exit(-1);
  }

  const node_t n = lookup_node->second;

  for (const std::string& dep_name : n.nets) {
    if (!result) { break; }

    result &= is_acyclic_rec(dep_name, net, net_visited, pth, pth_visited);
  }

  pth_visited.erase(node_name);
  pth.pop_back();
  return result;
}

bool
is_acyclic(const net_t& net)
{
  std::unordered_set<std::string> net_visited;

  std::vector<std::string> pth;
  std::unordered_set<std::string> pth_visited;

  bool result = true;
  for (const std::string& output : net.outputs_in_order) {
    if (!result) { break; }
    result &= is_acyclic_rec(output, net, net_visited, pth, pth_visited);
  }
  return result;
}

// ========================================================================== //
// Variable Ordering
void
dfs_variable_order_rec(const std::string& node_name,
                       std::unordered_map<int, int>& new_ordering,
                       const net_t& net,
                       std::unordered_set<std::string>& visited)
{
  if (new_ordering.size() == net.inputs_w_order.size()) { return; }

  const auto lookup_visited = visited.find(node_name);
  if (lookup_visited != visited.end()) { return; }
  visited.insert(node_name);

  const node_t n = net.nodes.find(node_name)->second;

  // Iterate through for non-input nets (i.e. looking at deeper inputs)
  for (const std::string& dep_name : n.nets) {
    if (net.is_input(dep_name)) { continue; }
    dfs_variable_order_rec(dep_name, new_ordering, net, visited);
  }

  // Add yet unseen inputs (i.e. looking at shallow inputs)
  for (const std::string& dep_name : n.nets) {
    if (!net.is_input(dep_name)) { continue; }

    const int old_idx = net.inputs_w_order.find(dep_name)->second;

    const auto lookup_order = new_ordering.find(old_idx);
    if (lookup_order != new_ordering.end()) { continue; }

    const int new_idx = new_ordering.size();
    new_ordering.insert({ old_idx, new_idx });
  }
}

std::unordered_map<int, int>
dfs_variable_order(const net_t& net)
{
  std::unordered_set<std::string> visited_nodes;
  std::unordered_map<int, int> new_ordering;
  for (const std::string& output : net.outputs_in_order) {
    dfs_variable_order_rec(output, new_ordering, net, visited_nodes);
  }
  return new_ordering;
}

// Compute (lazily) the level of a node
int
level_of(const std::string& node_name, net_t& net)
{
  if (net.is_input(node_name)) { return 0; }

  const auto lookup_level = net.level.find(node_name);
  if (lookup_level != net.level.end()) { return lookup_level->second; }

  const node_t n = net.nodes.find(node_name)->second;

  int level = -1;
  for (const std::string& dep_name : n.nets) {
    level = std::max(level, level_of(dep_name, net) + 1);
  }

  net.level.insert({ node_name, level });
  return level;
}

// For each input, compute the smallest level of a net referencing it
void
compute_input_depth(const std::string& node_name,
                    std::unordered_map<std::string, int>& deepest_reference,
                    net_t& net,
                    std::unordered_set<std::string>& visited)
{
  const auto lookup_visited = visited.find(node_name);
  if (lookup_visited != visited.end()) { return; }
  visited.insert(node_name);

  const int node_level = level_of(node_name, net);

  const auto lookup_node = net.nodes.find(node_name);
  if (lookup_node == net.nodes.end()) {
    std::cerr << "Referenced net '" << node_name << "' is undefined." << std::endl;
    exit(-1);
  }

  const node_t n = lookup_node->second;

  for (const std::string& dep_name : n.nets) {
    if (net.is_input(dep_name)) {
      const auto lookup_depth = deepest_reference.find(dep_name);

      if (lookup_depth != deepest_reference.end()) {
        if (lookup_depth->second > node_level) {
          deepest_reference.erase(dep_name);
          deepest_reference.insert({ dep_name, node_level });
        }
      } else {
        deepest_reference.insert({ dep_name, node_level });
      }
    } else {
      compute_input_depth(dep_name, deepest_reference, net, visited);
    }
  }
}

std::unordered_map<int, int>
level_variable_order(net_t& net)
{
  // Create a std::vector we can sort
  std::vector<std::string> inputs;
  for (auto kv : net.inputs_w_order) { inputs.push_back(kv.first); }

  std::unordered_map<std::string, int> deepest_reference;
  std::unordered_set<std::string> visited_nodes;
  for (const std::string& output : net.outputs_in_order) {
    compute_input_depth(output, deepest_reference, net, visited_nodes);
  }

  // Sort based on deepest referenced level (break ties by prior ordering)
  const auto comparator = [&net, &deepest_reference](const std::string& i, const std::string& j) {
    const int old_i   = net.inputs_w_order.find(i)->second;
    const int i_level = deepest_reference.find(i)->second;
    const int old_j   = net.inputs_w_order.find(j)->second;
    const int j_level = deepest_reference.find(j)->second;

    return (i_level < j_level) || (i_level == j_level && old_i < old_j);
  };

  std::sort(inputs.begin(), inputs.end(), comparator);

  // Map into new ordering
  std::unordered_map<int, int> new_ordering;

  for (size_t idx = 0; idx < inputs.size(); idx++) {
    new_ordering.insert({ net.inputs_w_order.find(inputs[idx])->second, idx });
  }

  return new_ordering;
}

std::unordered_map<int, int>
random_variable_order(net_t& net)
{
  // Create a std::vector we can shuffle
  std::vector<std::string> inputs;
  for (auto kv : net.inputs_w_order) { inputs.push_back(kv.first); }

  std::random_device rd;
  std::mt19937 gen(rd());

  std::shuffle(inputs.begin(), inputs.end(), gen);

  // Map into new ordering
  std::unordered_map<int, int> new_ordering;

  for (size_t idx = 0; idx < inputs.size(); idx++) {
    new_ordering.insert({ net.inputs_w_order.find(inputs[idx])->second, idx });
  }

  return new_ordering;
}

void
update_order(net_t& net, const std::unordered_map<int, int>& new_ordering)
{
  std::unordered_map<std::string, int> new_inputs_w_order;

  for (auto kv : net.inputs_w_order) {
    new_inputs_w_order.insert({ kv.first, new_ordering.find(kv.second)->second });
  }

  net.inputs_w_order = new_inputs_w_order;
}

void
apply_variable_order(const variable_order& vo, net_t& net_0, net_t& net_1)
{
  std::unordered_map<int, int> new_ordering;

  switch (vo) {
  case INPUT:
    // Keep as is
    return;

  case DF: {
    new_ordering = dfs_variable_order(net_0);
    break;
  }

  case LEVEL: {
    new_ordering = level_variable_order(net_0);
    break;
  }

  case LEVEL_DF: {
    apply_variable_order(variable_order::DF, net_0, net_1);
    new_ordering = level_variable_order(net_0);
    break;
  }

  case RANDOM: {
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
using bdd_cache = std::unordered_map<std::string, typename Adapter::dd_t>;

bool
decrease_ref_count(net_t& net, const std::string& node_name)
{
  if (net.is_input(node_name)) { return false; }
  if (net.is_output(node_name)) { return false; }

  const auto lookup_ref_count = net.ref_count.find(node_name);

  if (lookup_ref_count == net.ref_count.end()) {
    std::cerr << "Decreasing reference count on '" << node_name << "' not in reference table"
              << std::endl;
    exit(-1);
  }

  const int ref_count = lookup_ref_count->second;
  assert(ref_count > 0);

  net.ref_count.erase(node_name);

  if (ref_count == 1) { return true; }

  net.ref_count.insert({ node_name, ref_count - 1 });
  return false;
}

template <typename Adapter>
typename Adapter::dd_t
construct_node_bdd(net_t& net,
                   const std::string& node_name,
                   bdd_cache<Adapter>& cache,
                   Adapter& adapter,
                   bdd_statistics& stats)
{
  const auto lookup_cache = cache.find(node_name);
  if (lookup_cache != cache.end()) { return lookup_cache->second; }

  const auto lookup_input = net.inputs_w_order.find(node_name);
  if (lookup_input != net.inputs_w_order.end()) { return adapter.ithvar(lookup_input->second); }

  assert(net.nodes.find(node_name) != net.nodes.end());
  const node_t& node_data = net.nodes.find(node_name)->second;

  typename Adapter::dd_t so_cover_bdd = adapter.bot();
#ifdef BDD_BENCHMARK_STATS
  size_t so_nodecount = adapter.nodecount(so_cover_bdd);
  assert(so_nodecount == 0);
#endif // BDD_BENCHMARK_STATS

  for (size_t row_idx = 0; row_idx < node_data.so_cover.size(); row_idx++) {
    typename Adapter::dd_t tmp = adapter.top();

    for (size_t column_idx = 0; column_idx < node_data.nets.size(); column_idx++) {
      const std::string& dep_name    = node_data.nets.at(column_idx);
      typename Adapter::dd_t dep_bdd = construct_node_bdd(net, dep_name, cache, adapter, stats);

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
        if (decrease_ref_count(net, dep_name)) {
          cache.erase(dep_name);
#ifdef BDD_BENCHMARK_STATS
          stats.curr_bdd_sizes -= adapter.nodecount(dep_bdd);
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

  cache.insert({ node_name, so_cover_bdd });
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
  for (const std::string& output : net.outputs_in_order) {
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
  std::cout << json::field("total processed (nodes)") << stats.total_processed << json::comma << json::endl;
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
  std::cout << json::field("not") << json::value(stats.total_negations) << json::comma
            << json::endl;
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
verify_outputs(const net_t& net_0,
               const bdd_cache<Adapter>& cache_0,
               const net_t& net_1,
               const bdd_cache<Adapter>& cache_1)
{
  assert(net_0.outputs_in_order.size() == cache_0.size());
  assert(net_1.outputs_in_order.size() == cache_1.size());
  assert(net_0.outputs_in_order.size() == net_1.outputs_in_order.size());

  std::cout << json::field("equal") << json::brace_open << json::endl;

  const time_point t_compare_before = now();
  bool ret_value                    = true;

  for (size_t out_idx = 0; out_idx < net_0.outputs_in_order.size(); out_idx++) {
    const std::string& output_0 = net_0.outputs_in_order.at(out_idx);
    const std::string& output_1 = net_1.outputs_in_order.at(out_idx);

    const typename Adapter::dd_t bdd_0 = cache_0.find(output_0)->second;
    const typename Adapter::dd_t bdd_1 = cache_1.find(output_1)->second;

    if (bdd_0 != bdd_1) {
      std::cerr << "Output gates ['" << output_0 << "' / '" << output_1 << "'] differ!\n";
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
  // Read file(s) and construct Nets
  net_t net_0;

  const bool parsing_error_0 = construct_net(file_0, net_0);
  if (parsing_error_0) {
    std::cerr << "Parsing error for '" << file_0 << "'\n";
    return -1;
  }

  const bool is_acyclic_0 = is_acyclic(net_0);
  if (!is_acyclic_0) {
    std::cerr << "Input '" << file_0 << "' is not acyclic!\n";
    return -1;
  }

  net_t net_1;
  if (verify_networks) {
    const bool parsing_error_1 = construct_net(file_1, net_1);
    if (parsing_error_1) {
      std::cerr << "Parsing error for '" << file_1 << "'\n";
      return -1;
    }

    const bool is_acyclic_1 = is_acyclic(net_1);
    if (!is_acyclic_1) {
      std::cerr << "Input '" << file_1 << "' is not acyclic!\n";
      return -1;
    }

    const bool inputs_match = net_0.inputs_w_order.size() == net_1.inputs_w_order.size();
    if (!inputs_match) {
      std::cerr << "Number of inputs in '" << file_0 << "' and '" << file_1
                << "' do not match!\n";
      return -1;
    }

    const bool outputs_match = net_0.outputs_in_order.size() == net_1.outputs_in_order.size();
    if (!outputs_match) {
      std::cerr << "Number of outputs in '" << file_0 << "' and '" << file_1
                << "' do not match!\n";
      return -1;
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
    std::cout << json::field("variable order") << json::value(to_string(var_order))
              << json::comma << json::endl;
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

      const auto [errcode_1, time_1] =
        construct_net_bdd(file_1, net_1, cache_1, adapter);

      if (errcode_1) { return errcode_1; }
      total_time += time_1;

      std::cout << json::endl;
      std::cout << json::array_close << json::comma << json::endl;

      const auto [verified, time_eq] = verify_outputs<Adapter>(net_0, cache_0, net_1, cache_1);

      networks_equal = verified;
      total_time += time_eq;
    } else {
      std::cout << json::endl;
      std::cout << json::array_close << json::comma << json::endl;
    }

    // TODO: Fix 'total time' below also measures multiple 'std::flush'.
    std::cout << json::field("total time (ms)") << json::value(total_time) << json::endl;

    if (verify_networks && !networks_equal) { return -1; }
    return 0;
  });
}
