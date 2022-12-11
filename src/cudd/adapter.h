#include <cstddef>
#include <cstdio>

#include "cudd.h"
#include "cuddObj.hh"

unsigned int
cudd_cachesize(int varcount)
{
  const size_t number_of_buckets = CUDD_UNIQUE_SLOTS * varcount;

  constexpr size_t sizeof_DdSubtable = 8 + 9 * 4 + 8;
  const size_t buckets_bytes = number_of_buckets * sizeof_DdSubtable;

  const size_t bytes_remaining = static_cast<size_t>(M)*1024u*1024u - buckets_bytes;

  // CUDD may increase the number of buckets, thereby decreasing the space left
  // for nodes. This will skew the ratio in favour of the cache, but this is the
  // best we can do.
  constexpr size_t sizeof_DdNode = 2*4 + 3*8;
  constexpr size_t sizeof_DdCache = 4 * 8;

  // We need to maximise x and y in the following system of inequalities:
  //              24x + 32y <= M , x = y * CACHE_RATIO
  const size_t x = bytes_remaining / ((sizeof_DdNode * CACHE_RATIO + sizeof_DdCache) / CACHE_RATIO);
  const size_t y = x / CACHE_RATIO;

  return y;
}

unsigned long
cudd_memorysize()
{
  constexpr size_t CUDD_MAX = std::numeric_limits<unsigned long>::max();
  return std::min(static_cast<size_t>(M), CUDD_MAX / (1024 * 1024)) * 1024 * 1024;
}

class cudd_adapter
{
protected:
  Cudd __mgr;
  const int varcount;

protected:
  cudd_adapter(const int bdd_varcount, const int zdd_varcount)
    : __mgr(bdd_varcount, zdd_varcount,
            CUDD_UNIQUE_SLOTS,
            cudd_cachesize(bdd_varcount + zdd_varcount),
            cudd_memorysize()),
      varcount(bdd_varcount + zdd_varcount)
  { }

  ~cudd_adapter()
  { /* Do nothing */ }


  // Statistics
public:
  inline size_t allocated_nodes()
  { return __mgr.ReadKeys(); }

  void print_stats()
  {
    INFO("\nCUDD Statistics:\n");

    INFO("   Table:\n");
    INFO("   | peak node count:     %zu\n", __mgr.ReadPeakNodeCount());
    INFO("   | node count (bdd):    %zu\n", __mgr.ReadNodeCount());
    INFO("   | node count (zdd):    %zu\n", __mgr.zddReadNodeCount());
    INFO("   | keys:                %u\n",  __mgr.ReadKeys());
    INFO("   | dead:                %u\n",  __mgr.ReadDead());

    // Commented lines are only available if 'DD_STATS' flag is set in CUDD compilation

    // INFO(" | Cache:\n");
    // INFO(" | | slots:               %zu\n", __mgr.ReadCacheUsedSlots());
    // INFO(" | | lookups:             %zu\n", __mgr.ReadCacheLookUps());
    // INFO(" | | hits:                %zu\n", __mgr.ReadCacheHits());

    INFO("   Garbage Collections:\n");
    INFO("   | runs:                %u\n",  __mgr.ReadGarbageCollections());
    INFO("   | time (ms):           %zu\n", __mgr.ReadGarbageCollectionTime());
  }
};

class cudd_bdd_adapter : public cudd_adapter
{
public:
  inline static const std::string NAME = "CUDD [BDD]";
  typedef BDD dd_t;
  typedef BDD build_node_t;

private:
  BDD _latest_build;

  // Init and Deinit
public:
  cudd_bdd_adapter(int varcount) : cudd_adapter(varcount, 0)
  { // Disable dynamic ordering
    __mgr.AutodynDisable();

    _latest_build = leaf_false();
  }

  // BDD Operations
public:
  inline BDD leaf_true()
  { return __mgr.bddOne(); }

  inline BDD leaf_false()
  { return __mgr.bddZero(); }

  inline BDD ithvar(int label)
  { return __mgr.bddVar(label); }

  inline BDD nithvar(int label)
  { return ~__mgr.bddVar(label); }

  inline BDD negate(const BDD &b)
  { return ~b; }

  inline BDD exists(const BDD &b, int label)
  { return b.ExistAbstract(__mgr.bddVar(label)); }

  inline uint64_t nodecount(const BDD &b)
  { return b.nodeCount(); }

  inline uint64_t satcount(const BDD &b)
  { return b.CountMinterm(varcount); }

  // BDD Build operations
public:
  inline BDD build_node(const bool value)
  {
    const BDD res = value ? leaf_true() : leaf_false();
    if (_latest_build == leaf_false()) { _latest_build = res; }
    return res;
  }

  inline BDD build_node(const int label, const BDD &low, const BDD &high)
  {
    _latest_build = __mgr.makeBddNode(label, high, low);
    return _latest_build;
  }

  inline BDD build()
  {
    const BDD res = _latest_build;
    _latest_build = leaf_false(); // <-- Reset and free builder reference
    return res;
  }
};

class cudd_zdd_adapter : public cudd_adapter
{
public:
  inline static const std::string NAME = "CUDD [ZDD]";
  typedef ZDD dd_t;
  typedef ZDD build_node_t;

private:
  ZDD _latest_build;

  // Init and Deinit
public:
  cudd_zdd_adapter(int varcount) : cudd_adapter(0, varcount)
  { // Disable dynamic ordering
    __mgr.AutodynDisableZdd();

    _latest_build = leaf_false();
  }

  // ZDD Operations
public:
  inline ZDD leaf_true()
  { return __mgr.zddOne(std::numeric_limits<int>::max()); }

  inline ZDD leaf_false()
  { return __mgr.zddZero(); }

  inline ZDD ithvar(int label)
  { return __mgr.makeZddNode(label, leaf_false(), leaf_true()); }

  inline uint64_t nodecount(const ZDD &b)
  { return b.nodeCount(); }

  inline uint64_t satcount(const ZDD &b)
  { return b.CountMinterm(varcount); }

  // ZDD Build operations
public:
  inline ZDD build_node(const bool value)
  {
    const ZDD res = value ? leaf_true() : leaf_false();
    if (_latest_build == leaf_false()) { _latest_build = res; }
    return res;
  }

  inline ZDD build_node(const int label, const ZDD &low, const ZDD &high)
  {
    _latest_build = __mgr.makeZddNode(label, high, low);
    return _latest_build;
  }

  inline ZDD build()
  {
    const ZDD res = _latest_build;
    _latest_build = leaf_false(); // <-- Reset and free builder reference
    return res;
  }
};
