#include "../apply.cpp"

#include "adapter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//                         Benchmark as per Pastva and Henzinger (2023)                           //
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
int
run_apply<libbdd_bdd_adapter>(int argc, char** argv)
{
  const bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  if (inputs_path.size() < 2) {
    std::cerr << "Not enough files provided for binary operation (2+ required)\n";
    return -1;
  }

  // =============================================================================================
  // Initialize BDD package
  return run<libbdd_bdd_adapter>("apply", 0, [&](libbdd_bdd_adapter& adapter) {
    std::cout << json::field("inputs") << json::array_open << json::endl;

    // =============================================================================================
    // Load DDs
    std::vector<libbdd_bdd_adapter::dd_t> inputs_dd;
    inputs_dd.reserve(inputs_path.size());

    size_t total_time = 0;

    std::cout << json::field("load") << json::array_open << json::endl << json::flush;

    for (size_t i = 0; i < inputs_path.size(); ++i) {
      assert(inputs_dd.size() == i);

      const time_point t_rebuild_before = now();
      inputs_dd.push_back(adapter.load(inputs_path.at(i)));
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
      if (i < inputs_path.size() - 1) { std::cout << json::comma; }
      std::cout << json::endl;
    }

    std::cout << json::array_close << json::comma << json::endl;

    // =============================================================================================
    // Apply DDs together
    libbdd_bdd_adapter::dd_t result = inputs_dd.at(0);

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

////////////////////////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
  return run_apply<libbdd_bdd_adapter>(argc, argv);
}
