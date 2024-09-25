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

std::string relation_path = "";
std::string states_path = "";

enum operand
{
  NEXT,
  PREV
};

std::string
to_string(const operand& oper)
{
  switch (oper) {
  case operand::NEXT: return "next";
  case operand::PREV: return "prev";
  default: return "?";
  }
}

operand oper = operand::NEXT;

class parsing_policy
{
public:
  static constexpr std::string_view name = "RelProd";
  static constexpr std::string_view args = "o:r:s:";

  static constexpr std::string_view help_text =
    "        -o OPER     [next]    Relational Product to use (next/prev)"
    "        -r PATH               Path to '._dd' file for relation\n"
    "        -s PATH               Path to '._dd' file for states\n";

  static inline bool
  parse_input(const int c, const char* arg)
  {
    switch (c) {
    case 'o': {
      const std::string lower_arg = ascii_tolower(arg);

      if (is_prefix(lower_arg, "next") || is_prefix(lower_arg, "relnext")) {
        oper = operand::NEXT;
      } else if (is_prefix(lower_arg, "prev") || is_prefix(lower_arg, "relprev")) {
        oper = operand::PREV;
      } else {
        std::cerr << "Undefined operation " << arg << "\n";
        return true;
      }
      return false;
    }
    case 'r': {
      if (!std::filesystem::exists(arg)) {
        std::cerr << "File '" << arg << "' does not exist\n";
        return true;
      }
      relation_path = arg;
      return false;
    }
    case 's': {
      if (!std::filesystem::exists(arg)) {
        std::cerr << "File '" << arg << "' does not exist\n";
        return true;
      }
      states_path = arg;
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
typename Adapter::dd_t
build_support(Adapter& adapter, const lib_bdd::var_map& vm)
{
  // TODO: We currently assume the relation includes the frame rule and/or touches all variables.
  const typename Adapter::build_node_t false_ptr = adapter.build_node(false);
  const typename Adapter::build_node_t true_ptr  = adapter.build_node(true);

  typename Adapter::build_node_t root_ptr  = true_ptr;

  for (int x = vm.size() - 1; 0 <= x; --x) {
    root_ptr = adapter.build_node(x, false_ptr, root_ptr);
  }

  return adapter.build();
}

template <typename Adapter>
int
run_relprod(int argc, char** argv)
{
  bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  if (relation_path == "") {
    std::cerr << "Path for relation missing\n";
    return -1;
  }
  if (states_path == "") {
    std::cerr << "Path for states missing\n";
    return -1;
  }

  // =============================================================================================
  // Load 'lib-bdd' files
  lib_bdd::bdd libbdd_relation = lib_bdd::deserialize(relation_path);
  lib_bdd::bdd libbdd_states = lib_bdd::deserialize(states_path);

  lib_bdd::var_map vm = lib_bdd::remap_vars({libbdd_relation, libbdd_states});

  // =============================================================================================
  // Initialize BDD package
  return run<Adapter>("relprod", vm.size(), [&](Adapter& adapter) {
    size_t total_time = 0;

    // =============================================================================================
    // Reconstruct DDs
    typename Adapter::dd_t relation = adapter.bot();
    {
      std::cout << json::field("relation") << json::brace_open << json::endl;

      std::cout << json::field("path") << json::value(relation_path) << json::comma
                << json::endl;
      lib_bdd::print_json(lib_bdd::stats(libbdd_relation), std::cout);

      const time_point t_rebuild_before = now();
      relation = reconstruct(adapter, std::move(libbdd_relation), vm);
      const time_point t_rebuild_after = now();

      const size_t rebuild_time = duration_ms(t_rebuild_before, t_rebuild_after);
      total_time += rebuild_time;

      // Free up memory
      libbdd_relation.clear();
      libbdd_relation.shrink_to_fit();

      std::cout << json::field("satcount") << json::value(adapter.satcount(relation))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)")
                << json::value(rebuild_time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }

    typename Adapter::dd_t states = adapter.bot();
    {
      std::cout << json::field("states") << json::brace_open << json::endl;

      std::cout << json::field("path") << json::value(states_path) << json::comma
                << json::endl;
      lib_bdd::print_json(lib_bdd::stats(libbdd_states), std::cout);

      const time_point t_rebuild_before = now();
      states = reconstruct(adapter, std::move(libbdd_states), vm);
      const time_point t_rebuild_after = now();

      const size_t rebuild_time = duration_ms(t_rebuild_before, t_rebuild_after);
      total_time += rebuild_time;

      // Free up memory
      libbdd_states.clear();
      libbdd_states.shrink_to_fit();

      std::cout << json::field("satcount") << json::value(adapter.satcount(states, vm.size() / 2))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)")
                << json::value(rebuild_time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }

    // =============================================================================================
    // Relational Support
    typename Adapter::dd_t support = adapter.bot();
    {
      std::cout << json::field("support") << json::brace_open << json::endl;

      const time_point t_build_before = now();
      support = build_support(adapter, vm);
      const time_point t_build_after = now();

      const size_t build_time = duration_ms(t_build_before, t_build_after);
      total_time += build_time;

      std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(support))
                << json::comma << json::endl;
      std::cout << json::field("satcount") << json::value(adapter.satcount(support))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)")
                << json::value(build_time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }

    std::cout << json::endl;

    // =============================================================================================
    // Relational Product
    typename Adapter::dd_t result = adapter.bot();

    std::cout << json::field("relprod") << json::brace_open << json::endl << json::flush;

    const time_point t_relprod_before = now();
    switch (oper) {
    case operand::NEXT: result = adapter.relnext(states, relation, support); break;
    case operand::PREV: result = adapter.relprev(states, relation, support); break;
    }
    const time_point t_relprod_after = now();

    const size_t relprod_time = duration_ms(t_relprod_before, t_relprod_after);
    total_time += relprod_time;

    std::cout << json::field("operand") << json::value(to_string(oper)) << json::comma
              << json::endl;
    std::cout << json::field("size (nodes)") << adapter.nodecount(result) << json::comma
              << json::endl;
    std::cout << json::field("satcount") << adapter.satcount(result, vm.size()/2) << json::comma << json::endl;
    std::cout << json::field("time (ms)") << relprod_time << json::endl;

    std::cout << json::brace_close << json::comma << json::endl;

    // =============================================================================================
    std::cout << json::endl;

    std::cout << json::field("total time (ms)") << json::value(init_time + total_time)
              << json::endl;

    return 0;
  });
}
