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
/// - lace_init:              Single-threaded and use a 1,000,000 size task
///                           queue.
///
/// - lace_startup:           Auto-detect program stack, do not use a callback
///                           for startup.
///
/// - LACE_ME:                Lace is initialized, now set local variables.
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


class sylvan_mgr
{
private:
  int varcount;

public:
  inline static const std::string NAME = "Sylvan";

  // Variable type
public:
  typedef sylvan::Bdd bdd_t;

  // Init and Deinit
public:
  sylvan_mgr(int varcount) : varcount(varcount)
  {
    // Init LACE
    lace_init(1, 1000000);
    lace_startup(0, NULL, NULL);
    LACE_ME;

    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;

    // Init Sylvan
    sylvan::sylvan_set_limits(memory_bytes,      // Set memory limit
                              log2(CACHE_RATIO), // Set (exponent) of cache ratio
                              0);                // Initialise unique node table to full size
    sylvan::sylvan_set_granularity(1);
    sylvan::sylvan_init_package();
    sylvan::sylvan_init_bdd();
  }

  ~sylvan_mgr()
  {
    sylvan::sylvan_quit();
    lace_exit();
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

  inline sylvan::Bdd ite(const sylvan::Bdd &f, const sylvan::Bdd &g, const sylvan::Bdd &h)
  { return f.Ite(g,h); }

  inline sylvan::Bdd exists(const sylvan::Bdd &b, int label)
  { return b.ExistAbstract(sylvan::Bdd::bddVar(label)); }

  inline uint64_t nodecount(const bdd_t &b)
  { return b.NodeCount(); }

  inline uint64_t satcount(const bdd_t &b)
  { return b.SatCount(varcount); }
};
