#include <sylvan.h>
#include <sylvan_table.h>

#include <sylvan_obj.hpp>

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
/// Sylvan initialisation:
///   Nodes table size: 24 bytes * nodes
///   Cache table size: 36 bytes * cache entries
///
/// - sylvan_set_limit:       Set the memory limit, the (exponent of the) ratio
///                           between node table and cache, and lastly make the
///                           table sizes be as big as possible.
///
/// - sylvan_set_granularity: 1 for "use cache for every operation".
////////////////////////////////////////////////////////////////////////////////
size_t log2(size_t n)
{
  size_t exp = 1u;
  size_t val = 2u; // 2^1
  while (val < n) {
    val <<= 1u;
    exp++;
  }
  return exp;
}


class sylvan_bdd_adapter
{
public:
  inline static const std::string NAME = "Sylvan";
  typedef sylvan::Bdd dd_t;
  typedef sylvan::Bdd build_node_t;

private:
  int varcount;
  sylvan::Bdd _latest_build;

  // Init and Deinit
public:
  sylvan_bdd_adapter(int varcount) : varcount(varcount)
  {
    // Init LACE
    lace_start(1, 1000000);

    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;

    // Init Sylvan
    sylvan::sylvan_set_limits(memory_bytes,      // Set memory limit
                              log2(CACHE_RATIO), // Set (exponent) of cache ratio
                              0);                // Initialise unique node table to full size
    sylvan::sylvan_set_granularity(1);
    sylvan::sylvan_init_package();
    sylvan::sylvan_init_bdd();

    _latest_build = leaf_false();
  }

  ~sylvan_bdd_adapter()
  {
    sylvan::sylvan_quit();
    lace_stop();
  }

  // BDD Operations
public:
  inline sylvan::Bdd leaf_true()
  { return sylvan::Bdd::bddOne(); }

  inline sylvan::Bdd leaf_false()
  { return sylvan::Bdd::bddZero(); }

  inline sylvan::Bdd ithvar(int label)
  { return sylvan::Bdd::bddVar(label); }

  inline sylvan::Bdd nithvar(int label)
  { return ~sylvan::Bdd::bddVar(label); }

  inline sylvan::Bdd negate(const sylvan::Bdd &b)
  { return ~b; }

  inline sylvan::Bdd exists(const sylvan::Bdd &b, int label)
  { return b.ExistAbstract(sylvan::Bdd::bddVar(label)); }

  inline uint64_t nodecount(const dd_t &b)
  { return b.NodeCount() - 1; }

  inline uint64_t satcount(const dd_t &b)
  { return b.SatCount(varcount); }

  // BDD Build Operations
  // BDD Build operations
public:
  inline sylvan::Bdd build_node(const bool value)
  {
    const sylvan::Bdd res = value ? leaf_true() : leaf_false();
    if (_latest_build == leaf_false()) { _latest_build = res; }
    return res;
  }

  inline sylvan::Bdd build_node(const int label,
                                const sylvan::Bdd &low,
                                const sylvan::Bdd &high)
  {
    _latest_build = sylvan::Bdd::bddVar(label).Ite(high, low);
    return _latest_build;
  }

  inline sylvan::Bdd build()
  {
    const sylvan::Bdd res = _latest_build;
    _latest_build = leaf_false(); // <-- Reset and free builder reference
    return res;
  }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return 0; }

  void print_stats()
  {
    // Requires the "SYLVAN_STATS" property to be set in CMake
    INFO("\n");
    sylvan::sylvan_stats_report(stdout);
  }
};
