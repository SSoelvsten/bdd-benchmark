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

int compute_maxnodenum()
{
  // We need to maximise x and y in the following system of inequalities:
  //              24x + 16y <= M , x = CACHE_RATIO * y
  const size_t memory_bytes = static_cast<size_t>(M) * 1024 * 1024;
  const size_t x = memory_bytes / (24u * CACHE_RATIO + 16u);

  // memory ceiling for BuDDy is MAX_INT, though that is the same as 'unlimited'
  constexpr size_t MAX_INT = 2147483647;
  return x > MAX_INT ? 0 : x;
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
    bdd_init(INIT_UNIQUE_SLOTS_PER_VAR * varcount,
             (INIT_UNIQUE_SLOTS_PER_VAR * varcount) / CACHE_RATIO);
    bdd_setvarnum(varcount);
    bdd_setcacheratio(CACHE_RATIO);
    bdd_setmaxnodenum(compute_maxnodenum());

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
