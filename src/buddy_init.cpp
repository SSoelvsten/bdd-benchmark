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
