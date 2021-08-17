#include "common.cpp"

#include <unordered_map>
#include <unordered_set>
#include <random>

#include <blifparse.hpp>

// ========================================================================== //
// Parsing the input
enum logic_value { FALSE, TRUE, DONT_CARE };

struct node_t {
public:
  // The boolean value set on the output plane
  bool is_onset = true;

  // Input (dependant) nets
  std::vector<std::string> nets;
  std::vector<std::vector<logic_value>> so_cover;
};

struct net_t {
public:
  std::unordered_map<std::string, int> inputs_w_order;
  std::unordered_set<std::string> outputs;
  std::vector<std::string> outputs_in_order;
  std::unordered_map<std::string, int> level;
  std::unordered_map<std::string, int> ref_count;
  std::unordered_map<std::string, node_t> nodes;


  bool is_input(const std::string &n) const
  {
    const auto lookup_inputs = inputs_w_order.find(n);
    return lookup_inputs != inputs_w_order.end();
  }

  bool is_output(const std::string &n) const
  {
    const auto lookup_outputs = outputs.find(n);
    return lookup_outputs != outputs.end();
  }
};

class construct_net_callback : public blifparse::Callback
{
private:
  int input_idx = 0;

  net_t &net;

  bool has_error_ = false;
  bool has_names = false;

public:
  construct_net_callback(net_t &n) : blifparse::Callback(), net(n)
  { }

  bool has_error()
  {
    return has_error_;
  }

public:
  // Create the input set with the ordering as given in the input file.
  void inputs(std::vector<std::string> inputs) override
  {
    if (has_names) {
      std::cerr << "Defining '.inputs' after a '.names'" << std::endl;
      has_error_ = true;
    }

    for (const std::string &i : inputs) {
      net.inputs_w_order.insert({ i, input_idx++ });
    }
  }

  // Note down what nets are outputs
  void outputs(std::vector<std::string> outputs) override
  {
    if (has_names) {
      std::cerr << "Defining '.outputs' after a '.names'" << std::endl;
      has_error_ = true;
    }

    for (const std::string &o : outputs) {
      net.outputs.insert(o);
      net.outputs_in_order.push_back(o);
    }
  }

  // Construct a node in the net
  void names(std::vector<std::string> nets,
             std::vector<std::vector<blifparse::LogicValue>> so_cover) override
  {
    has_names = true;

    if (nets.size() == 0) {
      std::cerr << ".names given without any net names" << std::endl;
      has_error_ = true;
      return;
    }

    { // Create a new_node for this Net
      bool has_is_onset = false;
      bool new_is_onset = false;

      const std::string new_name = nets[nets.size() - 1];

      if (net.nodes.find(new_name) != net.nodes.end()) {
        std::cerr << "Net '" << new_name << "' defined multiple times" << std::endl;
        has_error_ = true;
        return;
      }

      std::vector<std::string> new_nets(nets.begin(), nets.end() - 1);
      std::vector<std::vector<logic_value>> new_so_cover;

      for (const std::vector<blifparse::LogicValue> &row : so_cover) {
        if (row.size() != nets.size()) {
          std::cerr << "Incorrect number of logic values defined on a row for net '" << new_name << "'" << std::endl;
          has_error_ = true;
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
              std::cerr << "Cannot have 'dont care' in output plane of net '" << new_name << "'" << std::endl;
              has_error_ = true;
            } else {
              new_row.push_back(logic_value::DONT_CARE);
            }
            break;

          case blifparse::LogicValue::UNKNOWN :
            std::cerr << "Cannot deal with 'unknown' value of net '" << new_name << "'" << std::endl;
            has_error_ = true;
          }
        } while (it != row.end());

        if (has_is_onset && row_is_onset != new_is_onset) {
          std::cerr << "Cannot handle both on-set and off-set in output plane of '" << new_name << "'" << std::endl;
          has_error_ = true;
          return;
        }

        has_is_onset = true;
        new_is_onset = row_is_onset;

        new_so_cover.push_back(new_row);
      }

      const node_t new_node = { new_is_onset, new_nets, new_so_cover };
      net.nodes.insert({new_name, new_node});
    }

    // Update the reference counter on all input nets
    for (size_t net_idx = 0; net_idx + 1 < nets.size(); net_idx++) {
      const std::string &dep_name = nets[net_idx];

      if (net.is_input(dep_name)) { continue; }
      if (net.is_output(dep_name)) { continue; }

      auto lookup_ref_count = net.ref_count.find(dep_name);
      if (lookup_ref_count == net.ref_count.end()) { // First time referenced
        net.ref_count.insert({dep_name, 1});
      } else {
        const int new_ref_count = lookup_ref_count -> second + 1;
        net.ref_count.erase(dep_name);
        net.ref_count.insert({dep_name, new_ref_count});
      }

      net.ref_count.find(dep_name);
    }
  }

  void latch(std::string input,
             std::string output,
             blifparse::LatchType /* type */,
             std::string control,
             blifparse::LogicValue /* init */)
  {
    // TODO: When state transitions are used, then add <x> and <x'> variables
    std::cerr << "State transitions with '.latch " << input << " " << output << control
              << "' not (yet) supported" << std::endl;
    has_error_ = true;
  }
};

bool construct_net(std::string filename, net_t &net)
{
  construct_net_callback callback(net);
  blifparse::blif_parse_filename(filename, callback);
  return callback.has_error();
}

// ========================================================================== //
// Variable Ordering
void dfs_variable_order_rec(const std::string &node_name,
                            std::unordered_map<int, int> &new_ordering,
                            const net_t &net,
                            std::unordered_set<std::string> &visited)
{
  if (new_ordering.size() == net.inputs_w_order.size()) { return; }

  const auto lookup_visited = visited.find(node_name);
  if (lookup_visited != visited.end()) { return; }
  visited.insert(node_name);

  const auto lookup_node = net.nodes.find(node_name);
  if (lookup_node == net.nodes.end()) {
    std::cerr << "Referenced net '" << node_name << "' is undefined." << std::endl;
    exit(-1);
  }

  const node_t n = lookup_node -> second;

  // Iterate through for non-input nets (i.e. looking at deeper inputs)
  for (const std::string &dep_name : n.nets) {
    if (net.is_input(dep_name)) { continue; }
    dfs_variable_order_rec(dep_name, new_ordering, net, visited);
  }

  // Add yet unseen inputs (i.e. looking at shallow inputs)
  for (const std::string &dep_name : n.nets) {
    if (!net.is_input(dep_name)) { continue; }

    const int old_idx = net.inputs_w_order.find(dep_name) -> second;

    const auto lookup_order = new_ordering.find(old_idx);
    if (lookup_order != new_ordering.end()) { continue; }

    const int new_idx = new_ordering.size();
    new_ordering.insert({ old_idx, new_idx});
  }
}

std::unordered_map<int, int> dfs_variable_order(const net_t &net)
{
  std::unordered_set<std::string> visited_nodes;
  std::unordered_map<int, int> new_ordering;
  for (const std::string output : net.outputs_in_order) {
    dfs_variable_order_rec(output, new_ordering, net, visited_nodes);
  }
  return new_ordering;
}

// Compute (lazily) the level of a node
int level_of(const std::string &node_name, net_t &net)
{
  if (net.is_input(node_name)) { return 0; }

  const auto lookup_level = net.level.find(node_name);
  if (lookup_level != net.level.end()) { return lookup_level -> second; }

  const auto lookup_node = net.nodes.find(node_name);
  if (lookup_node == net.nodes.end()) {
    std::cerr << "Referenced net '" << node_name << "' is undefined." << std::endl;
    exit(-1);
  }

  const node_t n = lookup_node -> second;

  int level = -1;
  for (const std::string dep_name : n.nets) {
    level = std::max(level, level_of(dep_name, net) + 1);
  }

  net.level.insert({ node_name, level });
  return level;
}

// For each input, compute the smallest level of a net referencing it
void compute_input_depth(const std::string &node_name,
                         std::unordered_map<std::string, int> &deepest_reference,
                         net_t &net,
                         std::unordered_set<std::string> &visited)
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

  const node_t n = lookup_node -> second;

  for (const std::string &dep_name : n.nets) {
    if (net.is_input(dep_name)) {
      const auto lookup_depth = deepest_reference.find(dep_name);

      if (lookup_depth != deepest_reference.end()) {
        if (lookup_depth -> second > node_level) {
          deepest_reference.erase(dep_name);
          deepest_reference.insert({dep_name, node_level});
        }
      } else {
        deepest_reference.insert({dep_name, node_level});
      }
    } else {
      compute_input_depth(dep_name, deepest_reference, net, visited);
    }
  }
}

std::unordered_map<int, int> level_variable_order(net_t &net)
{
  // Create a std::vector we can sort
  std::vector<std::string> inputs;
  for (auto kv : net.inputs_w_order) {
    inputs.push_back(kv.first);
  }

  std::unordered_map<std::string, int> deepest_reference;
  std::unordered_set<std::string> visited_nodes;
  for (const std::string output : net.outputs_in_order) {
    compute_input_depth(output, deepest_reference, net, visited_nodes);
  }

  // Sort based on deepest referenced level (break ties by prior ordering)
  const auto comparator = [&net, &deepest_reference](const std::string &i, const std::string &j) {
      const int old_i = net.inputs_w_order.find(i) -> second;
      const int i_level = deepest_reference.find(i) -> second;
      const int old_j = net.inputs_w_order.find(j) -> second;
      const int j_level = deepest_reference.find(j) -> second;

      return (i_level < j_level) || (i_level == j_level && old_i < old_j);
    };

  std::sort(inputs.begin(), inputs.end(), comparator);

  // Map into new ordering
  std::unordered_map<int, int> new_ordering;

  for (size_t idx = 0; idx < inputs.size(); idx++) {
    new_ordering.insert({ net.inputs_w_order.find(inputs[idx]) -> second, idx });
  }

  return new_ordering;
}

std::unordered_map<int, int> random_variable_order(net_t &net)
{
  // Create a std::vector we can shuffle
  std::vector<std::string> inputs;
  for (auto kv : net.inputs_w_order) {
    inputs.push_back(kv.first);
  }

  std::random_device rd;
  std::mt19937 gen(rd());

  std::shuffle(inputs.begin(), inputs.end(), gen);

  // Map into new ordering
  std::unordered_map<int, int> new_ordering;

  for (size_t idx = 0; idx < inputs.size(); idx++) {
    new_ordering.insert({ net.inputs_w_order.find(inputs[idx]) -> second, idx });
  }

  return new_ordering;
}

void update_order(net_t &net, const std::unordered_map<int, int> &new_ordering)
{
  std::unordered_map<std::string, int> new_inputs_w_order;

  for (auto kv : net.inputs_w_order) {
    new_inputs_w_order.insert({ kv.first, new_ordering.find(kv.second) -> second });
  }

  net.inputs_w_order = new_inputs_w_order;
}

enum variable_order { INPUT, DFS, LEVEL, LEVEL_DFS, RANDOM };

void apply_variable_order(const variable_order &o, net_t &net_0, net_t &net_1, bool print = true)
{
  std::unordered_map<int, int> new_ordering;

  switch (o) {
  case INPUT:
    if (print) { std::cout << " | Variable order: INPUT" << std::endl; }
    // Keep as is
    return;

  case DFS: {
    if (print) { std::cout << " | Variable order: DFS" << std::endl; }
    new_ordering = dfs_variable_order(net_0);
    break;
  }

  case LEVEL: {
    if (print) { std::cout << " | Variable order: LEVEL" << std::endl; }
    new_ordering = level_variable_order(net_0);
    break;
  }

  case LEVEL_DFS: {
    if (print) { std::cout << " | Variable order: LEVEL / DFS" << std::endl; }
    apply_variable_order(variable_order::DFS, net_0, net_1, false);
    new_ordering = level_variable_order(net_0);
    break;
  }

  case RANDOM: {
    if (print) { std::cout << " | Variable order: RANDOM" << std::endl; }
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
  size_t largest_bdd = 0;
  size_t total_processed = 0;
  size_t curr_nodes = 0;
  size_t sum_bdd_sizes = 0;
  size_t sum_allocated = 0;
};

template<typename mgr_t>
using bdd_cache = std::unordered_map<std::string, typename mgr_t::bdd_t>;

template<typename mgr_t>
void decrease_ref_count(net_t &net, const std::string &node_name, bdd_cache<mgr_t> &cache)
{
  if (net.is_input(node_name)) { return; }
  if (net.is_output(node_name)) { return; }

  const auto lookup_ref_count = net.ref_count.find(node_name);
  if (lookup_ref_count == net.ref_count.end()) {
    std::cerr << "Decreasing reference count on '" << node_name << "' not in reference table";
    exit(-1);
  }

  const int ref_count = lookup_ref_count -> second;
  assert(ref_count > 0);

  net.ref_count.erase(node_name);
  if (ref_count > 1) {
    net.ref_count.insert({ node_name, ref_count - 1});
  } else {
    cache.erase(node_name);
  }
}

template<typename mgr_t>
typename mgr_t::bdd_t construct_node_bdd(net_t &net,
                                         const std::string &node_name,
                                         bdd_cache<mgr_t> &cache,
                                         mgr_t &mgr,
                                         bdd_statistics &stats)
{
  const auto lookup_cache = cache.find(node_name);
  if (lookup_cache != cache.end()) {
    return lookup_cache -> second;
  }

  const auto lookup_input = net.inputs_w_order.find(node_name);
  if (lookup_input != net.inputs_w_order.end()) {
    return mgr.ithvar(lookup_input -> second);
  }

  assert (net.nodes.find(node_name) != net.nodes.end());
  const node_t &node_data = net.nodes.find(node_name) -> second;

  typename mgr_t::bdd_t so_cover_bdd = mgr.leaf_false();

  for (size_t row_idx = 0; row_idx < node_data.so_cover.size(); row_idx++) {
    typename mgr_t::bdd_t tmp = mgr.leaf_true();

    for (size_t column_idx = 0; column_idx < node_data.nets.size(); column_idx++) {
      const std::string &dep_name = node_data.nets.at(column_idx);
      typename mgr_t::bdd_t dep_bdd = construct_node_bdd(net, dep_name, cache, mgr, stats);

      // Add to row accumulation in 'tmp'
      switch (node_data.so_cover.at(row_idx).at(column_idx)) {
      case logic_value::FALSE:
        tmp &= mgr.negate(dep_bdd);
        break;

      case logic_value::TRUE:
        tmp &= dep_bdd;
        break;

      case logic_value::DONT_CARE:
        // Just do nothing
        break;
      }

      // Decrease reference count on dependency if we are on the last row.
      if (row_idx == node_data.so_cover.size() - 1) {
        decrease_ref_count<mgr_t>(net, dep_name, cache);
      }

      const size_t tmp_nodecount =  mgr.nodecount(tmp);
      stats.total_processed += tmp_nodecount;
      stats.largest_bdd = std::max(stats.largest_bdd, tmp_nodecount);
    }

    so_cover_bdd |= tmp;

    const size_t so_nodecount = mgr.nodecount(so_cover_bdd);
    stats.total_processed += so_nodecount;
    stats.largest_bdd = std::max(stats.largest_bdd, so_nodecount);

    stats.curr_nodes += so_nodecount;
    stats.sum_bdd_sizes += stats.curr_nodes;
    stats.sum_allocated += mgr.allocated_nodes();
  }

  so_cover_bdd = node_data.is_onset ? so_cover_bdd : mgr.negate(so_cover_bdd);

  cache.insert({ node_name, so_cover_bdd });
  return so_cover_bdd;
}

// ========================================================================== //
// Construct the BDD for each output gate
template<typename mgr_t>
void construct_net_bdd(const std::string &filename,
                       net_t &net,
                       bdd_cache<mgr_t> &cache,
                       mgr_t &mgr)
{
  if (cache.size() > 0) {
    std::cerr << "Given BDD cache is non-empty" << std::endl;
    exit(-1);
  }

  INFO(" | constructing '%s'\n", filename.c_str());
  INFO(" | | Net info:\n");
  INFO(" | | | inputs:                 %zu\n", net.inputs_w_order.size());
  INFO(" | | | outputs:                %zu\n", net.outputs_in_order.size());
  INFO(" | | | internal nodes:         %zu\n", net.nodes.size());

  const time_point t_construct_before = get_timestamp();
  bdd_statistics stats;
  for (const std::string output : net.outputs_in_order) {
    construct_node_bdd(net, output, cache, mgr, stats);
  }
  const time_point t_construct_after = get_timestamp();

  INFO(" | | BDD construction:\n");
  INFO(" | | | time (ms):              %zu\n", duration_of(t_construct_before, t_construct_after));
  INFO(" | | | total no. nodes:        %zu\n", stats.total_processed);
  INFO(" | | | largest BDD:            %zu\n", stats.largest_bdd);
  INFO(" | | | final BDDs:\n");
  INFO(" | | | | no. roots:            %zu\n", cache.size());
  INFO(" | | | | w/ duplicates:        %zu\n", stats.largest_bdd);
  INFO(" | | | | allocated:            %zu\n", mgr.allocated_nodes());
  INFO(" | | | life-time BDDs:\n");
  INFO(" | | | | w/ duplicates:        %zu\n", stats.sum_bdd_sizes);
  INFO(" | | | | allocated:            %zu\n", stats.sum_allocated);
}

// ========================================================================== //
// Test equivalence of every output gate (in-order they were given)
template<typename mgr_t>
bool verify_outputs(const net_t& net_0, const bdd_cache<mgr_t>& cache_0,
                    const net_t& net_1, const bdd_cache<mgr_t>& cache_1)
{
  assert(net_0.outputs_in_order.size() == cache_0.size());
  assert(net_1.outputs_in_order.size() == cache_1.size());
  assert(net_0.outputs_in_order.size() == net_1.outputs_in_order.size());
  INFO(" | verifying equality:\n");
  INFO(" | | result:\n");

  const time_point t_compare_before = get_timestamp();
  bool ret_value = true;

  for (size_t out_idx = 0; out_idx < net_0.outputs_in_order.size(); out_idx++) {
    const std::string &output_0 = net_0.outputs_in_order.at(out_idx);
    const std::string &output_1 = net_1.outputs_in_order.at(out_idx);

    const typename mgr_t::bdd_t bdd_0 = cache_0.find(output_0) -> second;
    const typename mgr_t::bdd_t bdd_1 = cache_1.find(output_1) -> second;

    if (bdd_0 != bdd_1) {
      std::cout << " | | | output differ in ['" << output_0 << "' / '" << output_1 << "']"  << std::endl;
      ret_value = false;
    }
  }
  const time_point t_compare_after = get_timestamp();
  if (ret_value) { INFO(" | | | all outputs match!\n"); }

  INFO(" | | time (ms):            %zu\n", duration_of(t_compare_before, t_compare_after));
  return ret_value;
}

// ========================================================================== //
template<>
variable_order parse_variable_ordering(const std::string &arg, bool &should_exit)
{
  if (arg == "INPUT") { return variable_order::INPUT; }
  if (arg == "DFS") { return variable_order::DFS; }
  if (arg == "LEVEL") { return variable_order::LEVEL; }
  if (arg == "LEVEL_DFS") { return variable_order::LEVEL_DFS; }
  if (arg == "RANDOM") { return variable_order::RANDOM; }

  std::cerr << "Undefined variable ordering: " << arg << std::endl;
  should_exit = true;

  return variable_order::INPUT;
}

template<typename mgr_t>
void run_picotrav(int argc, char** argv)
{
  variable_order variable_order = variable_order::INPUT;
  bool should_exit = parse_input(argc, argv, variable_order);

  if (input_files.size() == 0) {
    std::cerr << "Input file(s) not specified" << std::endl;
    should_exit = true;
  }

  if (should_exit) { exit(-1); }

  bool verify_networks = input_files.size() > 1;

  // =========================================================================
  std::cout << "Picotrav "
            << " (" << mgr_t::NAME << " " << M << " MiB):" << std::endl;

  // =========================================================================
  // Read file(s) and construct Nets
  net_t net_0;
  if(construct_net(input_files.at(0), net_0)) { exit(-1); }

  net_t net_1;
  if (verify_networks) {
    if(construct_net(input_files.at(1), net_1)) { verify_networks = false; }

    if (net_0.inputs_w_order.size() != net_1.inputs_w_order.size()) {
      std::cerr << "| Number of inputs do not match: skipping verification..." << std::endl;
      verify_networks = false;
    }
    if (net_0.inputs_w_order.size() != net_1.inputs_w_order.size()) {
      std::cerr << "| Number of outputs do not match: skipping verification..." << std::endl;
      verify_networks = false;
    }
  }

  // Nanotrav sorts the output in ascending order by their level. The same is
  // possible here, but experiments show this at times decreases and other
  // times increases the running time.

  // Derive variable order
  apply_variable_order(variable_order, net_0, net_1);

  // ========================================================================
  // Initialise BDD package manager
  const size_t varcount = net_0.inputs_w_order.size();

  const time_point t_init_before = get_timestamp();
  mgr_t mgr(varcount);
  const time_point t_init_after = get_timestamp();
  INFO(" | BDD package init (ms):      %zu\n", duration_of(t_init_before, t_init_after));

  // ========================================================================
  // Construct BDD for net(s)
  bdd_cache<mgr_t> cache_0;
  construct_net_bdd(input_files.at(0), net_0, cache_0, mgr);

  if (verify_networks) {
    bdd_cache<mgr_t> cache_1;
    construct_net_bdd(input_files.at(1), net_1, cache_1, mgr);

    verify_outputs<mgr_t>(net_0, cache_0, net_1, cache_1);
  }
}
