#include <bdd.h>

////////////////////////////////////////////////////////////////////////////////
/// Macro to initialise BuDDy with M megabytes of memory.
///
/// - bdd_init:
///     We initialise BuDDy with a unique table of M*47100 number of nodes and a
///     cache table of 10000. The number 47100 is derived purely by
///     experimentation.
///
/// - bdd_setmaxincrease:
///     The amount the original unique table is allowed to be increased during
///     garbage collection. Since we already initialised it to be M megabytes,
///     then we wish it to not change its size.
///
/// - bdd_setcacheratio:
///     Based on the manual for BuDDy a cache ratio of 1:64 is good for bigger
///     examples, such as our benchmarks.
///
/// - bdd_setvarnum:
///     Declare the number of variables to expect to be used.
////////////////////////////////////////////////////////////////////////////////
#define MAX_INT 2147483647
#define CACHE_RATIO 16

size_t buddy_nodetotal_from_mb(size_t M)
{
  // Magic constant found by experimentation. Works very well in the [4; 8]GB
  // range and is about 1MB below at 16GB and 2MB below 20GB. It's about 0.2MB
  // above for 512MB.
  return M * 38415;
}

int buddy_nodesize_from_mb(size_t M)
{
#ifndef GRENDEL
  size_t nodes = (CACHE_RATIO * buddy_nodetotal_from_mb(M)) / (CACHE_RATIO+1);
  return std::min((int) nodes,MAX_INT);
#else
  return MAX_INT;
#endif
}

int buddy_cachesize_from_mb(size_t M)
{
#ifndef GRENDEL
  if (buddy_nodesize_from_mb(M) == MAX_INT) {
    return MAX_INT / CACHE_RATIO;
  }
  return buddy_nodetotal_from_mb(M) / (CACHE_RATIO+1);
#else
  return MAX_INT / CACHE_RATIO;
#endif
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
    bdd_init(buddy_nodesize_from_mb(M), buddy_cachesize_from_mb(M));
    bdd_setmaxincrease(0);
    bdd_setvarnum(varcount);

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

  inline bdd ithvar(size_t label)
  { return bdd_ithvar(label); }

  inline bdd nithvar(size_t label)
  { return bdd_nithvar(label); }

  inline bdd ite(const bdd &f, const bdd &g, const bdd &h)
  { return bdd_ite(f,g,h); }

  inline bdd exists(const bdd &b, size_t label)
  { return bdd_exist(b, bdd_ithvar(label)); }

  inline uint64_t nodecount(const bdd_t &b)
  { return bdd_nodecount(b); }

  inline uint64_t satcount(const bdd_t &b)
  { return bdd_satcount(b); }
};
