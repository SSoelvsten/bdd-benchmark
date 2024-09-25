#include "../relprod.cpp"

#include "adapter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//                         Benchmark as per Pastva and Henzinger (2023)                           //
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
int
run_relprod<libbdd_bdd_adapter>(int argc, char** argv)
{
  const bool should_exit = parse_input<parsing_policy>(argc, argv);
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
  int varcount = 0;
  {
    const lib_bdd::bdd libbdd_relation = lib_bdd::deserialize(relation_path);
    varcount = lib_bdd::stats(libbdd_relation).levels;
  }

  // =============================================================================================
  // Initialize BDD package
  return run<libbdd_bdd_adapter>("relprod", varcount, [&](libbdd_bdd_adapter& adapter) {
    size_t total_time = 0;

    // =============================================================================================
    // Reconstruct DDs
    libbdd_bdd_adapter::dd_t relation = adapter.bot();
    {
      std::cout << json::field("relation") << json::brace_open << json::endl;

      std::cout << json::field("path") << json::value(relation_path) << json::comma
                << json::endl;

      const time_point t_rebuild_before = now();
      relation = adapter.load(relation_path);
      const time_point t_rebuild_after = now();

      const size_t rebuild_time = duration_ms(t_rebuild_before, t_rebuild_after);
      total_time += rebuild_time;

      std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(relation))
                << json::comma << json::endl;
      std::cout << json::field("satcount") << json::value(adapter.satcount(relation))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)")
                << json::value(rebuild_time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }

    libbdd_bdd_adapter::dd_t states = adapter.bot();
    {
      std::cout << json::field("states") << json::brace_open << json::endl;

      std::cout << json::field("path") << json::value(states_path) << json::comma
                << json::endl;

      const time_point t_rebuild_before = now();
      states = adapter.load(states_path);
      const time_point t_rebuild_after = now();

      const size_t rebuild_time = duration_ms(t_rebuild_before, t_rebuild_after);
      total_time += rebuild_time;

      std::cout << json::field("size (nodes)") << json::value(adapter.nodecount(states))
                << json::comma << json::endl;
      std::cout << json::field("satcount") << json::value(adapter.satcount(states, varcount / 2))
                << json::comma << json::endl;
      std::cout << json::field("time (ms)")
                << json::value(rebuild_time) << json::endl;

      std::cout << json::brace_close << json::comma << json::endl;
    }

    // =============================================================================================
    // Relational Support
    libbdd_bdd_adapter::dd_t support = adapter.bot();
    {
      std::cout << json::field("support") << json::brace_open << json::endl;

      const time_point t_build_before = now();
      support = build_support(adapter, varcount/2);
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
    libbdd_bdd_adapter::dd_t result = adapter.bot();

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
    std::cout << json::field("satcount") << adapter.satcount(result, varcount) << json::comma << json::endl;
    std::cout << json::field("time (ms)") << relprod_time << json::endl;

    std::cout << json::brace_close << json::comma << json::endl;

    // =============================================================================================
    std::cout << json::endl;

    std::cout << json::field("total time (ms)") << json::value(init_time + total_time)
              << json::endl;

    return 0;
  });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
  return run_relprod<libbdd_bdd_adapter>(argc, argv);
}
