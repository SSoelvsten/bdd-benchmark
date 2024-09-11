// Assertions
#include <cassert>

// Data Structures
#include <string>
#include <vector>

// Other
#include <stdexcept>

#include "common/adapter.h"
#include "common/chrono.h"
#include "common/input.h"
#include "common/libbdd_parser.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        INPUT PARSING                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> inputs_path;

enum operand
{
  AND,
  OR
};

std::string
to_string(const operand& oper)
{
  switch (oper) {
  case operand::AND: return "and";
  case operand::OR: return "or";
  default: return "?";
  }
}

operand oper = operand::AND;

class parsing_policy
{
public:
  static constexpr std::string_view name = "Apply";
  static constexpr std::string_view args = "f:o:";

  static constexpr std::string_view help_text =
    "        -f PATH               Path to '._dd' files (2+ required)\n"
    "        -o OPER      [and]    Boolean operator to use (and/or)";

  static inline bool
  parse_input(const int c, const char* arg)
  {
    switch (c) {
    case 'f': {
      if (!std::filesystem::exists(arg)) {
        std::cerr << "File '" << arg << "' does not exist\n";
        return true;
      }
      inputs_path.push_back(arg);
      return false;
    }
    case 'o': {
      const std::string lower_arg = ascii_tolower(arg);

      if (lower_arg == "and" || lower_arg == "a") {
        oper = operand::AND;
      } else if (lower_arg == "or" || lower_arg == "o") {
        oper = operand::OR;
      } else {
        std::cerr << "Undefined operand: " << arg << "\n";
        return true;
      }
      return false;
    }
    default: return true;
    }
  }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
//                         Benchmark as per Pastva and Henzinger (2023)                           //
////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Adapter>
int
run_apply(int argc, char** argv)
{
  bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  if (inputs_path.size() < 2) {
    std::cerr << "Not enough files provided for binary operation (2+ required)\n";
    return -1;
  }

  // =============================================================================================
  // Load 'lib-bdd' files
  std::vector<lib_bdd::bdd> inputs_binary;
  inputs_binary.reserve(inputs_path.size());

  for (const std::string& path : inputs_path) {
    inputs_binary.push_back(lib_bdd::deserialize(path));
  }

  lib_bdd::var_map vm = lib_bdd::remap_vars(inputs_binary);

  // =============================================================================================
  // Initialize BDD package
  return run<Adapter>("apply", vm.size(), [&](Adapter& adapter) {
    std::cout << json::field("inputs") << json::array_open << json::endl;

    for (size_t i = 0; i < inputs_path.size(); ++i) {
      assert(inputs_binary.size() == i);

      std::cout << json::indent << json::brace_open << json::endl;
      const lib_bdd::stats_t stats = lib_bdd::stats(inputs_binary.at(i));

      std::cout << json::field("path") << json::value(inputs_path.at(i)) << json::comma
                << json::endl;

      std::cout << json::field("size") << json::value(stats.size) << json::comma << json::endl;
      std::cout << json::field("levels") << json::value(stats.levels) << json::comma << json::endl;
      std::cout << json::field("width") << json::value(stats.width) << json::comma << json::endl;

      std::cout << json::field("terminal_edges") << json::brace_open << json::endl;
      std::cout << json::field("false") << json::value(stats.terminals[false]) << json::comma
                << json::endl;
      std::cout << json::field("true") << json::value(stats.terminals[true]) << json::endl;
      std::cout << json::brace_close << json::endl;

      std::cout << json::field("parent_counts") << json::brace_open << json::endl;
      std::cout << json::field("0")
                << json::value(stats.parent_counts[lib_bdd::stats_t::parent_count_idx::None])
                << json::comma << json::endl;
      std::cout << json::field("1")
                << json::value(stats.parent_counts[lib_bdd::stats_t::parent_count_idx::One])
                << json::comma << json::endl;
      std::cout << json::field("2")
                << json::value(stats.parent_counts[lib_bdd::stats_t::parent_count_idx::Two])
                << json::comma << json::endl;
      std::cout << json::field("3")
                << json::value(stats.parent_counts[lib_bdd::stats_t::parent_count_idx::More])
                << json::endl;
      std::cout << json::brace_close << json::endl;

      std::cout << json::brace_close;
      if (i < inputs_path.size() - 1) { std::cout << json::comma; }
      std::cout << json::endl;
    }
    std::cout << json::array_close << json::comma << json::endl << json::endl;

    // =============================================================================================
    // Reconstruct DDs
    std::vector<typename Adapter::dd_t> inputs_dd;
    inputs_dd.reserve(inputs_binary.size());

    size_t total_time = 0;

    std::cout << json::field("rebuild") << json::array_open << json::endl << json::flush;

    for (size_t i = 0; i < inputs_binary.size(); ++i) {
      assert(inputs_dd.size() == i);

      const time_point t_rebuild_before = now();
      inputs_dd.push_back(lib_bdd::reconstruct(adapter, inputs_binary.at(i), vm));
      const time_point t_rebuild_after = now();

      const size_t load_time = duration_ms(t_rebuild_before, t_rebuild_after);
      total_time += load_time;

      std::cout << json::indent << json::brace_open << json::endl;
      std::cout << json::field("path") << json::value(inputs_path.at(i)) << json::comma
                << json::endl;
      std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(inputs_dd.at(i)))
                << json::comma << json::endl;
      std::cout << json::field("satcount") << json::value(adapter.satcount(inputs_dd.at(i)))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)")
                << json::value(duration_ms(t_rebuild_before, t_rebuild_after)) << json::endl;

      std::cout << json::brace_close;
      if (i < inputs_binary.size() - 1) { std::cout << json::comma; }
      std::cout << json::endl;
    }

    std::cout << json::array_close << json::comma << json::endl;

    // =============================================================================================
    // Apply both DDs together
    typename Adapter::dd_t result = inputs_dd.at(0);

    std::cout << json::field("apply") << json::brace_open << json::endl << json::flush;

    const time_point t_apply_before = now();
    for (size_t i = 0; i < inputs_dd.size(); ++i) {
      switch (oper) {
      case operand::AND: result &= inputs_dd.at(i); break;
      case operand::OR: result |= inputs_dd.at(i); break;
      }
    }
    const time_point t_apply_after = now();

    const size_t apply_time = duration_ms(t_apply_before, t_apply_after);
    total_time += apply_time;

    std::cout << json::field("operand") << json::value(to_string(oper)) << json::comma
              << json::endl;
    std::cout << json::field("operations") << json::value(inputs_dd.size() - 1) << json::comma
              << json::endl;
    std::cout << json::field("size (nodes)") << adapter.nodecount(result) << json::comma
              << json::endl;
    std::cout << json::field("satcount") << adapter.satcount(result) << json::comma << json::endl;
    std::cout << json::field("time (ms)") << apply_time << json::endl;

    std::cout << json::brace_close << json::comma << json::endl;

    // =============================================================================================

    std::cout << json::field("total time (ms)") << json::value(init_time + total_time)
              << json::endl;

    return 0;
  });
}
