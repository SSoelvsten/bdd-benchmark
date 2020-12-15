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

#define BUDDY_INIT(N, M)                                                       \
  bdd_init(buddy_nodesize_from_mb(M),  buddy_cachesize_from_mb(M));            \
  bdd_setmaxincrease(0);                                                       \
  bdd_setvarnum(N)

#define BUDDY_DEINIT                                                           \
  bdd_done()

////////////////////////////////////////////////////////////////////////////////
class buddy_sat_policy
{
private:
  bdd sat_acc = bddtrue;

public:
  void reset()
  {
    sat_acc = bddtrue;
  }

  void and_clause(clause_t &clause)
  {
    bdd c = bddfalse;

    uint64_t label = UINT64_MAX;

    for (auto it = clause.rbegin(); it != clause.rend(); it++)
      {
        assert((*it).first < label);
        label = (*it).first;
        bool negated = (*it).second;

        bdd v = negated ? bdd_nithvar(label) : bdd_ithvar(label);
        c = bdd_ite(v, bddtrue, c);
      }
    sat_acc &= c;
  }

  void quantify_variable(uint64_t var)
  {
    sat_acc = bdd_exist(sat_acc, bdd_ithvar(var));
  }

  bool is_false()
  {
    return sat_acc == bddfalse;
  }

  uint64_t satcount(uint64_t)
  {
    return static_cast<uint64_t>(bdd_satcount(sat_acc));
  }

  uint64_t size()
  {
    return bdd_nodecount(sat_acc);
  }
};

typedef sat_solver<buddy_sat_policy> buddy_sat_solver;
