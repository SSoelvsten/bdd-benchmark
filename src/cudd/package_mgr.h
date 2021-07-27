#include <cstddef>
#include <cstdio>

#include "cudd.h"
#include "cuddObj.hh"

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
    : __mgr(varcount, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, M*1024*1024),
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

  inline BDD ithvar(size_t label)
  { return __mgr.bddVar(label); }

  inline BDD nithvar(size_t label)
  { return ~__mgr.bddVar(label); }

  inline BDD ite(const BDD &f, const BDD &g, const BDD &h)
  { return f.Ite(g,h); }

  inline BDD exists(const BDD &b, size_t label)
  { return b.ExistAbstract(__mgr.bddVar(label)); }

  inline uint64_t nodecount(const BDD &b)
  { return b.nodeCount(); }

public:
  inline uint64_t satcount(const BDD &b)
  { return b.CountMinterm(varcount); }
};
