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
  BDD __latest_build;

  // Init and Deinit
public:
  cudd_bdd_adapter(int varcount) : cudd_adapter(varcount, 0)
  { // Disable dynamic ordering
    __mgr.AutodynDisable();

    __latest_build = bot();
  }

private:
  template<typename IT>
  inline BDD make_cube(IT rbegin, IT rend)
  {
    BDD res = top();
    while (rbegin != rend) {
      res = __mgr.bddVar(*(rbegin++)).Ite(res, bot());
    }
    return res;
  }

  // BDD Operations
public:
  inline BDD top()
  { return __mgr.bddOne(); }

  inline BDD bot()
  { return __mgr.bddZero(); }

  inline BDD ithvar(int label)
  { return __mgr.bddVar(label); }

  inline BDD nithvar(int label)
  { return ~__mgr.bddVar(label); }

  inline BDD negate(const BDD &b)
  { return ~b; }

  inline BDD ite(const BDD &i, const BDD &t, const BDD &e)
  { return i.Ite(t,e); }

  inline BDD exists(const BDD &b, int label)
  { return b.ExistAbstract(__mgr.bddVar(label)); }

  template<typename IT>
  inline BDD exists(const BDD &b, IT rbegin, IT rend)
  { return b.ExistAbstract(make_cube(rbegin, rend)); }

  inline BDD forall(const BDD &b, int label)
  { return b.UnivAbstract(__mgr.bddVar(label)); }

  template<typename IT>
  inline BDD forall(const BDD &b, IT rbegin, IT rend)
  { return b.UnivAbstract(make_cube(rbegin, rend)); }

  inline uint64_t nodecount(const BDD &b)
  { return b.nodeCount(); }

  inline uint64_t satcount(const BDD &b)
  { return b.CountMinterm(varcount); }

  inline std::vector<std::pair<int, char>>
  pickcube(const BDD &b)
  {
    std::string cudd_res(varcount, '_');
    b.PickOneCube(cudd_res.data());

    std::vector<std::pair<int, char>> res;
    for (int x = 0; x < varcount; ++x) {
      const char cudd_val = cudd_res.at(x);
      if (x == '_') { continue; }

      res.push_back({ x, '0'+cudd_val });
    }

    return res;
  }

  // BDD Build operations
public:
  inline BDD build_node(const bool value)
  {
    const BDD res = value ? top() : bot();
    if (__latest_build == bot()) { __latest_build = res; }
    return res;
  }

  inline BDD build_node(const int label, const BDD &low, const BDD &high)
  {
    __latest_build = __mgr.makeBddNode(label, high, low);
    return __latest_build;
  }

  inline BDD build()
  {
    const BDD res = __latest_build;
    __latest_build = bot(); // <-- Reset and free builder reference
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
  ZDD __leaf0;
  ZDD __leaf1;

  ZDD __latest_build;

  // Init and Deinit
public:
  cudd_zdd_adapter(int varcount) : cudd_adapter(0, varcount)
  { // Disable dynamic ordering
    __mgr.AutodynDisableZdd();

    __leaf0 = __mgr.zddZero();
    __leaf1 = __mgr.zddOne(std::numeric_limits<int>::max());
    __latest_build = __mgr.zddZero();
  }

  // ZDD Operations
public:
  inline ZDD top()
  { return __mgr.zddOne(0); }

  inline ZDD bot()
  { return __leaf0; }

  inline ZDD ithvar(const int i)
  { return __mgr.zddVar(i); }

  inline ZDD negate(const ZDD &z)
  { return top() - z; }

  inline uint64_t nodecount(const ZDD &b)
  { return b.nodeCount(); }

  inline uint64_t satcount(const ZDD &b)
  { return b.CountMinterm(varcount); }

  // ZDD Build operations
public:
  inline ZDD build_node(const bool value)
  {
    const ZDD res = value ? __leaf1 : __leaf0;
    if (__latest_build == __leaf0) { __latest_build = res; }
    return res;
  }

  inline ZDD build_node(const int label, const ZDD &low, const ZDD &high)
  {
    return __latest_build = __mgr.makeZddNode(label, high, low);
  }

  inline ZDD build()
  {
    const ZDD res = __latest_build;
    __latest_build = __leaf0; // <-- Reset and free builder reference
    return res;
  }
};
