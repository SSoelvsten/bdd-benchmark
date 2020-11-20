#include <sylvan.h>
#include <sylvan_table.h>

#include <sylvan_obj.hpp>

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
  sylvan_set_limits(M * 1024 * 1024, 2, 0);\
  sylvan_init_package();\
  sylvan_set_granularity(1);\
  sylvan_init_bdd();

#define SYLVAN_DEINIT\
  sylvan_quit();\
  lace_exit();
