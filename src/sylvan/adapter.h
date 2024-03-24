#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

#include "../common/adapter.h"

#include <sylvan.h>
#include <sylvan_table.h>
#include <sylvan_obj.hpp>

////////////////////////////////////////////////////////////////////////////////
/// Running code in a LACE context
///
/// If one just uses Sylvan's C++ API as-is without further thought, then every
/// call to a BDD operation has to start out with initialising the queue of a
/// LACE worker.
///
/// To circumvent the introduction of this overhead, one should run all Sylvan
/// operations within a single 'main' task.
////////////////////////////////////////////////////////////////////////////////
TASK_1(int, lace_lambda, const std::function<int()>*, f)
{
  return (*f)();
}

////////////////////////////////////////////////////////////////////////////////
/// Initialisation of Sylvan.
///
/// From 'sylvan_commons.h' we know that every node takes up 24 bytes of memory
/// and every operation cache entry takes up 36 bytes.
///
/// Lace initialisation
/// - lace_start:             Initializes LACE given the number of threads and
///                           the size of the task queue.
///
/// - sylvan_set_limit:       Set the memory limit, the (exponent of the) ratio
///                           between node table and cache, and lastly make the
///                           table sizes be as big as possible.
///
/// - sylvan_set_granularity: 1 for "use cache for every operation".
///
/// From `sylvan_set_limits` we are encouraged to use a table-to-cache ratio
/// between 8:1 and 1:8.
////////////////////////////////////////////////////////////////////////////////

/// Number of table entries per cache entry (as recommended by Sylvan).
constexpr size_t cache_ratio = 2;

/// Computation of initial size for Sylvan
size_t table_doublings(const size_t memory_bytes)
{
  // Table entry size(s); see implementation of `sylvan::sylvan_set_limits(...)`
  constexpr size_t table_entry = 24;
  constexpr size_t cache_entry = 36;
  constexpr int entry_log = ilog2(cache_ratio * table_entry + cache_entry) + 1;

  // Starting table size (normalised for cache ratio)
  constexpr size_t start_bytes = 64 * 1024 * 1024;
  constexpr int start_log = ilog2(start_bytes) - entry_log;

  // Final table size (normalised for cache ratio)
  const int final_log   = ilog2(memory_bytes) - entry_log;

  // Return difference (since we are doing logarithms, this is the number of
  // doublings). Make sure to truncate negative numbers for small memory sizes.
  return std::max(final_log - start_log, 0);
}

class sylvan_bdd_adapter
{
public:
  static constexpr std::string_view NAME = "Sylvan";

  static constexpr bool needs_extend = false;

public:
  typedef sylvan::Bdd dd_t;
  typedef sylvan::Bdd build_node_t;

private:
  const int _varcount;
  sylvan::Bdd _latest_build;

  // Init and Deinit
public:
  sylvan_bdd_adapter(int varcount)
    : _varcount(varcount)
  {
    // Init LACE
    lace_start(1, 1000000);

    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;

    // Init Sylvan
    sylvan::sylvan_set_limits(memory_bytes, ilog2(cache_ratio), table_doublings(memory_bytes)
    );
    sylvan::sylvan_set_granularity(1);
    sylvan::sylvan_init_package();
    sylvan::sylvan_init_bdd();

    _latest_build = bot();
  }

  ~sylvan_bdd_adapter()
  {
    sylvan::sylvan_quit();
    lace_stop();
  }

public:
  // LACE context
  inline int
  run(const std::function<int()>& f)
  {
    return RUN(lace_lambda, &f);
  }

private:
  template <typename IT>
  inline sylvan::Bdd
  make_cube(IT rbegin, IT rend)
  {
    sylvan::Bdd res = top();
    while (rbegin != rend) { res = sylvan::Bdd::bddVar(*(rbegin++)).Ite(res, bot()); }
    return res;
  }

  inline sylvan::Bdd
  make_cube(const std::function<bool(int)>& pred)
  {
    sylvan::Bdd res = top();
    for (int i = _varcount - 1; 0 <= i; --i) {
      if (pred(i)) { res = sylvan::Bdd::bddVar(i).Ite(res, bot()); }
    }
    return res;
  }

  // BDD Operations
public:
  inline sylvan::Bdd
  top()
  {
    return sylvan::Bdd::bddOne();
  }

  inline sylvan::Bdd
  bot()
  {
    return sylvan::Bdd::bddZero();
  }

  inline sylvan::Bdd
  ithvar(int i)
  {
    return sylvan::Bdd::bddVar(i);
  }

  inline sylvan::Bdd
  nithvar(int i)
  {
    return ~sylvan::Bdd::bddVar(i);
  }

  inline sylvan::Bdd
  apply_diff(const sylvan::Bdd& f, const sylvan::Bdd& g)
  {
    return f - g;
  }

  inline sylvan::Bdd
  apply_imp(const sylvan::Bdd& f, const sylvan::Bdd& g)
  {
    return f.Ite(g, sylvan::Bdd::bddOne());
  }

  inline sylvan::Bdd
  apply_xnor(const sylvan::Bdd& f, const sylvan::Bdd& g)
  {
    return f.Xnor(g);
  }

  inline sylvan::Bdd
  ite(const sylvan::Bdd& f, const sylvan::Bdd& g, const sylvan::Bdd& h)
  {
    return f.Ite(g, h);
  }

  template <typename IT>
  inline sylvan::Bdd
  extend(const sylvan::Bdd& f, IT /*begin*/, IT /*end*/)
  {
    return f;
  }

  inline sylvan::Bdd
  exists(const sylvan::Bdd& f, int i)
  {
    return f.ExistAbstract(sylvan::Bdd::bddVar(i));
  }

  inline sylvan::Bdd
  exists(const sylvan::Bdd& f, const std::function<bool(int)>& pred)
  {
    return f.ExistAbstract(make_cube(pred));
  }

  template <typename IT>
  inline sylvan::Bdd
  exists(const sylvan::Bdd& f, IT rbegin, IT rend)
  {
    return f.ExistAbstract(make_cube(rbegin, rend));
  }

  inline sylvan::Bdd
  forall(const sylvan::Bdd& f, int i)
  {
    return f.UnivAbstract(sylvan::Bdd::bddVar(i));
  }

  inline sylvan::Bdd
  forall(const sylvan::Bdd& f, const std::function<bool(int)>& pred)
  {
    return f.UnivAbstract(make_cube(pred));
  }

  template <typename IT>
  inline sylvan::Bdd
  forall(const sylvan::Bdd& f, IT rbegin, IT rend)
  {
    return f.UnivAbstract(make_cube(rbegin, rend));
  }

  inline uint64_t
  nodecount(const sylvan::Bdd& f)
  {
    return f.NodeCount() - 1;
  }

  inline uint64_t
  satcount(const sylvan::Bdd& f)
  {
    return this->satcount(f, this->_varcount);
  }

  inline uint64_t
  satcount(const sylvan::Bdd& f, const size_t vc)
  {
    return f.SatCount(vc);
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const sylvan::Bdd& f)
  {
    std::vector<std::pair<int, char>> res;

    sylvan::Bdd sat = f.PickOneCube();
    while (!sat.isOne() && !sat.isZero()) {
      const int var              = sat.TopVar();
      const sylvan::Bdd sat_low  = sat.Else();
      const sylvan::Bdd sat_high = sat.Then();

      const bool go_high = !sat_high.isZero();
      res.push_back({ var, '0' + go_high });

      sat = go_high ? sat_high : sat_low;
    }
    return res;
  }

  void
  print_dot(const sylvan::Bdd& f, const std::string& filename)
  {
    FILE* p = fopen(filename.data(), "w");
    f.PrintDot(p);
    fclose(p);
  }

  // BDD Build operations
public:
  inline sylvan::Bdd
  build_node(const bool value)
  {
    const sylvan::Bdd res = value ? top() : bot();
    if (_latest_build == bot()) { _latest_build = res; }
    return res;
  }

  inline sylvan::Bdd
  build_node(const int label, const sylvan::Bdd& low, const sylvan::Bdd& high)
  {
    _latest_build = sylvan::Bdd::bddVar(label).Ite(high, low);
    return _latest_build;
  }

  inline sylvan::Bdd
  build()
  {
    const sylvan::Bdd res = _latest_build;
    _latest_build         = bot(); // <-- Reset and free builder reference
    return res;
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return 0;
  }

  void
  print_stats()
  {
    // Requires the "SYLVAN_STATS" property to be set in CMake
    std::cout << "\n";
    sylvan::sylvan_stats_report(stdout);
  }
};
