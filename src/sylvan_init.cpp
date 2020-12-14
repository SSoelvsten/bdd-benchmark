#include <sylvan.h>
#include <sylvan_table.h>

#include <sylvan_obj.hpp>

using namespace sylvan;

////////////////////////////////////////////////////////////////////////////////
/// Macro to initialise Sylvan with M megabytes of memory.
///
/// Lace initialisation
/// - lace_init:              Single-threaded and use a 1,000,000 size task
///                           queue.
///
/// - lace_startup:           Auto-detect program stack, do not use a callback
///                           for startup.
///
/// - LACE_ME:                Lace is initialized, now set local variables.
///
/// Sylvan initialisation:
///   Nodes table size: 24 bytes * nodes
///   Cache table size: 36 bytes * cache entries
///
/// - sylvan_set_limit:       Set the memory limit, the ratio between node table
///                           and cache to be 4:1, and lastly make the table
///                           sizes be as big as possible.
///
/// - sylvan_set_granularity: 1 means "use cache for every operation".
////////////////////////////////////////////////////////////////////////////////
#define SYLVAN_INIT(M)\
  lace_init(1, 1000000);\
  lace_startup(0, NULL, NULL);\
  LACE_ME;\
  sylvan_set_limits(M * 1024 * 1024, 6, 0);\
  sylvan_init_package();\
  sylvan_set_granularity(1);\
  sylvan_init_bdd()

#define SYLVAN_DEINIT\
  sylvan_quit();\
  lace_exit()

#define SYLVAN_SOLVER_INIT(solver_name, varcount)                 \
  BDD sat_acc = sylvan_true;                                      \
  sylvan_protect(&sat_acc);                                       \
                                                                  \
  const auto on_reset = [&]() -> void                             \
  {                                                               \
    sat_acc = sylvan_true;                                        \
  };                                                              \
                                                                  \
  const auto on_and_clause = [&](clause_t &clause) -> void        \
  {                                                               \
    BDD c = sylvan_false;                                         \
    sylvan_protect(&c);                                           \
                                                                  \
    uint64_t label = UINT64_MAX;                                  \
                                                                  \
    for (auto it = clause.rbegin(); it != clause.rend(); it++) {  \
      assert((*it).first < label);                                \
      label = (*it).first;                                        \
      bool negated = (*it).second;                                \
                                                                  \
      c = sylvan_makenode(label,                                  \
                          negated ? sylvan_true : c,              \
                          negated ? c : sylvan_true);             \
    }                                                             \
                                                                  \
    sat_acc = sylvan_and(sat_acc, c);                             \
    sylvan_unprotect(&c);                                         \
  };                                                              \
                                                                  \
  const auto on_exists = [&](uint64_t var) -> void                \
  {                                                               \
    sat_acc = sylvan_exists(sat_acc, sylvan_ithvar(var));         \
  };                                                              \
                                                                  \
  const auto on_is_false = [&]() -> bool                          \
  {                                                               \
      return sat_acc == sylvan_false;                             \
  };                                                              \
                                                                  \
  const auto on_satcount = [&](uint64_t varcount) -> uint64_t     \
  {                                                               \
    return mtbdd_satcount(sat_acc, varcount);                     \
  };                                                              \
                                                                  \
  const auto on_size = [&]() -> uint64_t                          \
  {                                                               \
    return sylvan_nodecount(sat_acc);                             \
  };                                                              \
                                                                  \
  sat_solver<bdd_policy> solver(on_reset,                         \
                                on_and_clause,                    \
                                on_exists,                        \
                                on_is_false,                      \
                                on_satcount,                      \
                                on_size,                          \
                                varcount)

