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
#define BUDDY_INIT(N, M)\
  bdd_init(M*47100,10000);\
  bdd_setmaxincrease(0);\
  bdd_setcacheratio(64);\
  bdd_setvarnum(N);

#define BUDDY_DEINIT\
  bdd_done();

////////////////////////////////////////////////////////////////////////////////
class buddy_sat_policy
{
private:
  bdd sat_acc = bddtrue;

public:
  void and_clause(clause_t &clause)
  {
    bdd c = bddfalse;

    for (auto it = clause.rbegin(); it != clause.rend(); it++)
      {
        bdd v = (*it).second ? bdd_nithvar((*it).first) : bdd_ithvar((*it).first);
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
    return bdd_satcount(sat_acc);
  }

  uint64_t size()
  {
    return bdd_nodecount(sat_acc);
  }
};

typedef sat_solver<buddy_sat_policy> buddy_sat_solver;
