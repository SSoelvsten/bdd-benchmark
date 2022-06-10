#include "adapter.h"

class cudd_zdd_adapter
{
private:
  Cudd __mgr;
  int varcount;

public:
  inline static const std::string NAME = "CUDD [ZDD]";

  // Variable type
public:
  typedef ZDD dd_t;

  // Init and Deinit
public:
  cudd_zdd_adapter(int varcount)
    : __mgr(0, varcount,
            CUDD_UNIQUE_SLOTS,
            cudd_cachesize(varcount),
            cudd_memorysize()),
      varcount(varcount)
  { // Disable dynamic ordering
    __mgr.AutodynDisableZdd();
  }

  ~cudd_zdd_adapter()
  { /* Do nothing */ }

  // ZDD Operations
public:
  inline ZDD leaf_true()
  { return __mgr.zddOne(std::numeric_limits<int>::max()); }

  inline ZDD leaf_false()
  { return __mgr.zddZero(); }

  inline ZDD make_node(int label, const ZDD &low, const ZDD &high)
  {
    return __mgr.makeZddNode(label, high, low);
  }

  inline uint64_t nodecount(const ZDD &b)
  {
    // CUDD also counts leaves for ZDDs?
    return b.nodeCount();
  }

  inline uint64_t satcount(const ZDD &b)
  { return b.CountMinterm(varcount); }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return __mgr.ReadKeys(); }

  void print_stats()
  {
    INFO("\nCUDD Statistics:\n");

    INFO("   Table:\n");
    INFO("   | peak node count:     %zu\n", __mgr.ReadPeakNodeCount());
    INFO("   | node count:          %zu\n", __mgr.zddReadNodeCount());
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
