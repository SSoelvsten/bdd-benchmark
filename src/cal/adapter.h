#include <cstddef>
#include <cstdio>

#include <calObj.hh>

class cal_bdd_adapter
{
 public:
  inline static const std::string NAME = "CAL [BDD]";

  // Variable type
 public:
  typedef BDD dd_t;
  typedef BDD build_node_t;

private:
  Cal __mgr;
  const int varcount;
  BDD _latest_build;

public:
  cal_bdd_adapter(const int bdd_varcount)
    : __mgr(bdd_varcount),
      varcount(bdd_varcount)
  {
    // Disable dynamic variable reordering
    __mgr.DynamicReordering(Cal::ReorderTechnique::NONE);

    _latest_build = leaf_false();
  }

  ~cal_bdd_adapter()
  { /* Do nothing */ }

  // BDD Operations
 public:
  inline BDD leaf(bool val)
  { return val ? leaf_true() : leaf_false(); }

  inline BDD leaf_true()
  { return __mgr.One(); }

  inline BDD leaf_false()
  { return __mgr.Zero(); }

  inline BDD ithvar(int i)
  { return __mgr.Id(i+1); }

  inline BDD nithvar(int i)
  { return ~__mgr.Id(i+1); }

  inline BDD negate(BDD f)
  { return ~f; }

  inline BDD ite(BDD i, BDD t, BDD e)
  { return __mgr.ITE(i,t,e); }

  inline uint64_t nodecount(BDD f)
  { return __mgr.Size(f); }

  inline uint64_t satcount(BDD f)
  {
    const double satFrac = __mgr.SatisfyingFraction(f);
    const double numVars = varcount;
    return std::pow(2, numVars) * satFrac;
  }

  // BDD Build Operations
public:
  inline BDD build_node(const bool value)
  {
    const BDD res = value ? leaf_true() : leaf_false();
    if (_latest_build == leaf_false()) { _latest_build = res; }
    return res;
  }

  inline BDD build_node(const int label, const BDD &low, const BDD &high)
  {
    _latest_build = __mgr.ITE(__mgr.Id(label+1), high, low);
    return _latest_build;
  }

  inline BDD build()
  {
    const BDD res = _latest_build;
    _latest_build = leaf_false(); // <-- Reset and free builder reference
    return res;
  }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return __mgr.GetNumNodes(); }

  void print_stats()
  {
    INFO("\n");
    __mgr.Stats(stdout);
  }
};
