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

#include "common/adapter.h"
#include "common/chrono.h"
#include "common/input.h"

// ========================================================================== //
// Parsing the input
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
  std::cout << "  Parsing '" << filename << "'\n";
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
    std::cerr << "Net is cyclic: ";
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

enum variable_order
{
  INPUT,
  DF,
  LEVEL,
  LEVEL_DF,
  RANDOM
};

void
apply_variable_order(const variable_order& o, net_t& net_0, net_t& net_1, bool print = true)
{
  std::unordered_map<int, int> new_ordering;
  if (print) { std::cout << "  Variable Order              "; }

  switch (o) {
  case INPUT:
    if (print) { std::cout << "INPUT\n"; }
    // Keep as is
    return;

  case DF: {
    if (print) { std::cout << "DF\n"; }
    new_ordering = dfs_variable_order(net_0);
    break;
  }

  case LEVEL: {
    if (print) { std::cout << "LEVEL\n"; }
    new_ordering = level_variable_order(net_0);
    break;
  }

  case LEVEL_DF: {
    if (print) { std::cout << "LEVEL / DF\n"; }
    apply_variable_order(variable_order::DF, net_0, net_1, false);
    new_ordering = level_variable_order(net_0);
    break;
  }

  case RANDOM: {
    if (print) { std::cout << "RANDOM\n"; }
    new_ordering = random_variable_order(net_0);
    break;
  }
  }

  update_order(net_0, new_ordering);
  if (net_1.inputs_w_order.size() == net_0.inputs_w_order.size()) {
    update_order(net_1, new_ordering);
  }
  if (print) { std::cout << "  | derived\n"; }
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
int
construct_net_bdd(const std::string& filename,
                  net_t& net,
                  bdd_cache<Adapter>& cache,
                  Adapter& adapter)
{
  if (cache.size() > 0) {
    std::cerr << "Given BDD cache is non-empty";
    return 42;
  }

  std::cout << "\n"
            << "  Constructing BDD" << (net.outputs.size() > 1 ? "s" : "") << " for '" << filename
            << "'\n";

  const time_point t_construct_before = now();
  bdd_statistics stats;
  for (const std::string& output : net.outputs_in_order) {
    construct_node_bdd(net, output, cache, adapter, stats);
  }
  const time_point t_construct_after = now();

  std::cout << "  | time (ms)                 "
            << duration_ms(t_construct_before, t_construct_after) << "\n";
#ifdef BDD_BENCHMARK_STATS
  std::cout << "  | total no. nodes           " << stats.total_processed << "\n";
#endif // BDD_BENCHMARK_STATS

  size_t sum_final_sizes = 0;
  size_t max_final_size  = 0;
  for (auto kv : cache) {
    size_t nodecount = adapter.nodecount(kv.second);
    sum_final_sizes += nodecount;
    max_final_size = std::max(max_final_size, nodecount);
  }
  std::cout << "  | final BDDs\n"
            << "  | | max BDD size            " << max_final_size << "\n"
            << "  | | w/ duplicates           " << sum_final_sizes << "\n"
            << "  | | allocated               " << adapter.allocated_nodes() << "\n";

#ifdef BDD_BENCHMARK_STATS
  std::cout << "  | life-time BDDs\n"
            << "  | | max no. roots           " << stats.max_roots << "\n"
            << "  | | max BDD size            " << stats.max_bdd_size << "\n"
            << "  | | sum w/ duplicates       " << stats.sum_bdd_sizes << "\n"
            << "  | | max w/ duplicates       " << stats.max_bdd_size << "\n"
            << "  | | sum allocated           " << stats.sum_allocated << "\n"
            << "  | | max allocated           " << stats.max_allocated << "\n";

  std::cout << "  | BDD operations\n"
            << "  | | Apply                   " << stats.total_applys << "\n"
            << "  | | Negations               " << stats.total_negations << "\n";
#endif // BDD_BENCHMARK_STATS
  std::cout << std::flush;

  return 0;
}

// ========================================================================== //
// Test equivalence of every output gate (in-order they were given)
template <typename Adapter>
bool
verify_outputs(const net_t& net_0,
               const bdd_cache<Adapter>& cache_0,
               const net_t& net_1,
               const bdd_cache<Adapter>& cache_1)
{
  assert(net_0.outputs_in_order.size() == cache_0.size());
  assert(net_1.outputs_in_order.size() == cache_1.size());
  assert(net_0.outputs_in_order.size() == net_1.outputs_in_order.size());

  std::cout << "\n"
            << "  Verifying equality\n"
            << "  | result\n"
            << std::flush;

  const time_point t_compare_before = now();
  bool ret_value                    = true;

  for (size_t out_idx = 0; out_idx < net_0.outputs_in_order.size(); out_idx++) {
    const std::string& output_0 = net_0.outputs_in_order.at(out_idx);
    const std::string& output_1 = net_1.outputs_in_order.at(out_idx);

    const typename Adapter::dd_t bdd_0 = cache_0.find(output_0)->second;
    const typename Adapter::dd_t bdd_1 = cache_1.find(output_1)->second;

    if (bdd_0 != bdd_1) {
      std::cout << "  | | output differ in ['" << output_0 << "' / '" << output_1 << "']\n";
      ret_value = false;
    }
  }
  const time_point t_compare_after = now();
  if (ret_value) { std::cout << "  | | all outputs match!\n"; }

  std::cout << "  | time (ms)                 " << duration_ms(t_compare_before, t_compare_after)
            << "\n"
            << std::flush;

  return ret_value;
}

// ========================================================================== //
template <>
std::string
option_help_str<variable_order>()
{
  return "Desired Variable ordering";
}

template <>
variable_order
parse_option(const std::string& arg, bool& should_exit)
{
  const std::string lower_arg = ascii_tolower(arg);

  if (lower_arg == "input") { return variable_order::INPUT; }

  if (lower_arg == "df" || lower_arg == "depth-first") { return variable_order::DF; }

  if (lower_arg == "level") { return variable_order::LEVEL; }

  if (lower_arg == "level_df") { return variable_order::LEVEL_DF; }

  if (lower_arg == "random") { return variable_order::RANDOM; }

  std::cerr << "Undefined variable ordering: " << arg << "\n";
  should_exit = true;

  return variable_order::INPUT;
}

template <typename Adapter>
int
run_picotrav(int argc, char** argv)
{
  variable_order variable_order = variable_order::INPUT;
  bool should_exit              = parse_input(argc, argv, variable_order);

  if (input_files.size() == 0) {
    std::cerr << "Input file(s) not specified\n";
    should_exit = true;
  }

  if (should_exit) { return -1; }

  const bool verify_networks = input_files.size() > 1;

  // =========================================================================
  std::cout << "Picotrav\n";

  // =========================================================================
  // Read file(s) and construct Nets
  net_t net_0;

  const bool parsing_error_0 = construct_net(input_files.at(0), net_0);
  std::cout << "  | Input Validation\n";
  std::cout << "  | | [" << (parsing_error_0 ? " " : "x") << "] parsing\n";
  if (parsing_error_0) { return -1; }

  const bool is_not_cyclic_0 = is_acyclic(net_0);
  std::cout << "  | | [" << (is_not_cyclic_0 ? "x" : " ") << "] acyclic\n";
  if (!is_not_cyclic_0) { return -1; }

  std::cout << "  | net info\n"
            << "  | | inputs                  " << net_0.inputs_w_order.size() << "\n"
            << "  | | outputs                 " << net_0.outputs_in_order.size() << "\n"
            << "  | | internal nodes          " << net_0.nodes.size() << "\n"
            << std::flush;

  net_t net_1;
  if (verify_networks) {
    const bool parsing_error_1 = construct_net(input_files.at(1), net_1);
    std::cout << "  | input validation\n"
              << "  | | [" << (parsing_error_1 ? " " : "x") << "] parsing\n";

    const bool is_not_cyclic_1 = is_acyclic(net_1);
    std::cout << "  | | [" << (is_not_cyclic_1 ? "x" : " ") << "] acyclic\n";

    const bool inputs_match = net_0.inputs_w_order.size() == net_1.inputs_w_order.size();
    std::cout << "  | | [" << (inputs_match ? "x" : " ") << "] number of inputs match\n";

    const bool outputs_match = net_0.outputs_in_order.size() == net_1.outputs_in_order.size();
    std::cout << "  | | [" << (outputs_match ? "x" : " ") << "] number of outputs match\n";

    if (parsing_error_1 || !is_not_cyclic_1 || !inputs_match || !outputs_match) { return -1; }
  }

  std::cout << "  | net info\n"
            << "  | | inputs                  " << net_1.inputs_w_order.size() << "\n"
            << "  | | outputs                 " << net_1.outputs_in_order.size() << "\n"
            << "  | | internal nodes          " << net_1.nodes.size() << "\n"
            << "\n"
            << std::flush;

  // Nanotrav sorts the output in ascending order by their level. The same is
  // possible here, but experiments show this at times decreases and other
  // times increases the running time.

  // Derive variable order
  apply_variable_order(variable_order, net_0, net_1);

  // ========================================================================
  // Initialise BDD package manager
  const size_t varcount = net_0.inputs_w_order.size();
  std::cout << "\n";

  return run<Adapter>(varcount, [&](Adapter& adapter) {
    // ========================================================================
    // Construct BDD for first net
    bdd_cache<Adapter> cache_0;

    const time_point t_before = now();

    int errcode_0       = 0;
    int errcode_1       = 0;
    bool networks_equal = true;

    errcode_0 = construct_net_bdd(input_files.at(0), net_0, cache_0, adapter);

    if (errcode_0) { return errcode_0; }

    // ========================================================================
    // Construct BDD for second net (if any) and compare them
    if (verify_networks) {
      bdd_cache<Adapter> cache_1;
      errcode_1 = construct_net_bdd(input_files.at(1), net_1, cache_1, adapter);

      if (errcode_1) { return errcode_1; }

      networks_equal = verify_outputs<Adapter>(net_0, cache_0, net_1, cache_1);
    }
    const time_point t_after = now();

    // TODO: Fix 'total time' below also measures multiple 'std::flush'.
    std::cout << "\n"
              << "  total time (ms)             " << duration_ms(t_before, t_after) << "\n"
              << std::flush;

    if (verify_networks && !networks_equal) { return -1; }
    return 0;
  });
}
