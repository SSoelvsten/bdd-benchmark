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

private:
  Cal __mgr;
  const int varcount;

public:
  cal_bdd_adapter(const int bdd_varcount)
    : __mgr(bdd_varcount),
      varcount(bdd_varcount)
  {
    // Disable dynamic variable reordering
    __mgr.DynamicReordering(Cal::ReorderTechnique::NONE);
  }

  ~cal_bdd_adapter()
  { /* Do nothing */ }

  // BDD Operations
 public:
  inline BDD leaf_true()
  { return __mgr.One(); }

  inline BDD leaf_false()
  { return __mgr.Zero(); }

  inline BDD ithvar(int i)
  { return __mgr.Id(i+1); }

  inline BDD nithvar(int i)
  { return ~__mgr.Id(i+1); }

  inline BDD make_node(int label, BDD F, BDD T)
  { return __mgr.ITE(__mgr.Id(label+1), T, F); }

  inline BDD negate(BDD f)
  { return ~f; }

  inline uint64_t nodecount(BDD f)
  { return __mgr.Size(f); }

  inline uint64_t satcount(BDD f)
  {
    const double satFrac = __mgr.SatisfyingFraction(f);
    const double numVars = varcount;
    return std::pow(2, numVars) * satFrac;
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
