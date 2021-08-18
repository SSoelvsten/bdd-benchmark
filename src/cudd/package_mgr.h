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

class cudd_mgr
{
private:
  Cudd __mgr;
  int varcount;

public:
  inline static const std::string NAME = "CUDD";

  // Variable type
public:
  typedef BDD bdd_t;

  // Init and Deinit
public:
  cudd_mgr(int varcount)
    : __mgr(varcount, 0,
            CUDD_UNIQUE_SLOTS,
            cudd_cachesize(varcount),
            static_cast<size_t>(M)*1024u*1024u),
      varcount(varcount)
  { // Disable dynamic ordering
    __mgr.AutodynDisable();
  }

  ~cudd_mgr()
  { /* Do nothing */ }

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

  inline BDD ite(const BDD &f, const BDD &g, const BDD &h)
  { return f.Ite(g,h); }

  inline BDD negate(const BDD &b)
  { return ~b; }

  inline BDD exists(const BDD &b, int label)
  { return b.ExistAbstract(__mgr.bddVar(label)); }

  inline uint64_t nodecount(const BDD &b)
  { return b.nodeCount() - 1; /* CUDD also counts leafs */ }

  inline uint64_t satcount(const BDD &b)
  { return b.CountMinterm(varcount); }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return __mgr.ReadKeys(); }

  void print_stats()
  {
    INFO("CUDD Statistics:\n");

    INFO(" | Table:\n");
    INFO(" | | peak node count:     %zu\n", __mgr.ReadPeakNodeCount());
    INFO(" | | node count:          %zu\n", __mgr.ReadNodeCount());
    INFO(" | | keys:                %u\n",  __mgr.ReadKeys());
    INFO(" | | dead:                %u\n",  __mgr.ReadDead());

    // Commented lines are only available if 'DD_STATS' flag is set in CUDD compilation

    // INFO(" | Cache:\n");
    // INFO(" | | slots:               %zu\n", __mgr.ReadCacheUsedSlots());
    // INFO(" | | lookups:             %zu\n", __mgr.ReadCacheLookUps());
    // INFO(" | | hits:                %zu\n", __mgr.ReadCacheHits());

    INFO(" | Garbage Collections:\n");
    INFO(" | | runs:                %u\n",  __mgr.ReadGarbageCollections());
    INFO(" | | time (ms):           %zu\n", __mgr.ReadGarbageCollectionTime());
  }
};
