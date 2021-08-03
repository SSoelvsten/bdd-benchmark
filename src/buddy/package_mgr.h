#include <bdd.h>

////////////////////////////////////////////////////////////////////////////////
/// Initialisation of BuDDy. The size of each node in the unique table is 6*4 =
/// 24 bytes (BddNode in kernel.h) while each cache entry takes up 4*4 = 16
/// bytes (BddCacheData in cache.h).
///
/// So, the memory in bytes occupied when given NODE_SLOTS and CACHE_SLOTS is
///
///                       24 * NODE_SLOTS + 16 * CACHE_SLOTS
///
/// - bdd_init:
///     We initialise BuDDy with a unique table of M*MAGIC_NUMBER number of
///     nodes and a cache table of 10000. The magic number is derived purely by
///     experimentation.
///
/// - bdd_setmaxincrease:
///     The amount the original unique table is allowed to be increased during
///     garbage collection. Put it to 0 to fix the current size.
///
/// - bdd_setmaxnodesize
///     Sets the maximum number of nodes in the nodetable based on the above
///     observation.
///
/// - bdd_setcacheratio:
///     Based on the manual for BuDDy a cache ratio of 1:64 is good for bigger
///     examples, such as our benchmarks. But, for comparison we have fixed it
///     to 1:64.
///
/// - bdd_setvarnum:
///     Declare the number of variables to expect to be used.
////////////////////////////////////////////////////////////////////////////////

constexpr size_t MAX_INT = 2147483647;

struct buddy_init_size
{
  int node_size;
  int cache_size;
  bool broke_cache_ratio = false;
};

buddy_init_size compute_init_size()
{
  // We need to maximise x and y in the following system of inequalities:
  //              24x + 16y <= M , x = y * CACHE_RATIO
  const size_t memory_bytes = static_cast<size_t>(M) * 1024 * 1024;
  const size_t x = memory_bytes / (24u + 16u / CACHE_RATIO);

  // memory ceiling for BuDDy is MAX_INT.

  // Let y take the remaining space left.
  const bool broke_cache_ratio = x > MAX_INT;
  const size_t y = broke_cache_ratio
    ? (memory_bytes - (x * 24u)) / 16u
    : x / CACHE_RATIO;

  return { std::min(x, MAX_INT), std::min(y, MAX_INT), broke_cache_ratio };
}

class buddy_mgr
{
public:
  inline static const std::string NAME = "BuDDy";

  // Variable type
public:
  typedef bdd bdd_t;

  // Init and Deinit
public:
  buddy_mgr(int varcount)
  {
#ifndef GRENDEL
    const buddy_init_size init_size = compute_init_size();
    bdd_init(init_size.node_size, init_size.cache_size);

    if (!init_size.broke_cache_ratio) {
      bdd_setcacheratio(CACHE_RATIO);
    }
    bdd_setmaxnodenum(init_size.node_size);
#else
    bdd_init(MAX_INT, MAX_INT);
#endif

    bdd_setvarnum(varcount);

    // Disable default gbc_handler
    bdd_gbc_hook(NULL);

    // Disable dynamic variable reordering
    bdd_disable_reorder();
  }

  ~buddy_mgr()
  {
    bdd_done();
  }

  // BDD Operations
public:
  inline bdd leaf_true()
  { return bddtrue; }

  inline bdd leaf_false()
  { return bddfalse; }

  inline bdd ithvar(int label)
  { return bdd_ithvar(label); }

  inline bdd nithvar(int label)
  { return bdd_nithvar(label); }

  inline bdd ite(const bdd &f, const bdd &g, const bdd &h)
  { return bdd_ite(f,g,h); }

  inline bdd exists(const bdd &b, int label)
  { return bdd_exist(b, bdd_ithvar(label)); }

  inline uint64_t nodecount(const bdd_t &b)
  { return bdd_nodecount(b); }

  inline uint64_t satcount(const bdd_t &b)
  { return bdd_satcount(b); }
};
