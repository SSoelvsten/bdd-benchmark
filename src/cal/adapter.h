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
  // HACK: the destructor of the 'Cal' class creates a segmentation fault.
  //       Hence, we wrap it such that we create a memory leak by NOT calling
  //       the destructor.
  //
  //       See also: https://github.com/SSoelvsten/cal/issues/3
  union {
    Cal _mgr;
  };
  const int _varcount;
  BDD _latest_build;

public:
  cal_bdd_adapter(const int bdd_varcount)
    : _mgr(bdd_varcount), _varcount(bdd_varcount)
  {
    // Disable dynamic variable reordering
    _mgr.DynamicReordering(Cal::ReorderTechnique::NONE);

    _latest_build = bot();
  }

  ~cal_bdd_adapter()
  { /* Do nothing */ }

  // BDD Operations
 public:
  inline BDD top()
  { return _mgr.One(); }

  inline BDD bot()
  { return _mgr.Zero(); }

  inline BDD ithvar(int i)
  { return _mgr.Id(i+1); }

  inline BDD nithvar(int i)
  { return ~_mgr.Id(i+1); }

  inline BDD negate(BDD f)
  { return ~f; }

  inline BDD ite(BDD i, BDD t, BDD e)
  { return _mgr.ITE(i,t,e); }

  inline BDD exists(const BDD &b, int label)
  {
    throw std::exception();
  }

  template<typename IT>
  inline BDD exists(const BDD &b, IT rbegin, IT rend)
  {
    throw std::exception();
  }

  inline BDD forall(const BDD &b, int label)
  {
    throw std::exception();
  }

  template<typename IT>
  inline BDD forall(const BDD &b, IT rbegin, IT rend)
  {
    throw std::exception();
  }

  inline uint64_t nodecount(BDD f)
  { return _mgr.Size(f); }

  inline uint64_t satcount(BDD f)
  {
    const double satFrac = _mgr.SatisfyingFraction(f);
    const double numVars = _varcount;
    return std::pow(2, numVars) * satFrac;
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const BDD &b)
  {
    throw std::exception();
  }

  // BDD Build Operations
public:
  inline BDD build_node(const bool value)
  {
    const BDD res = value ? top() : bot();
    if (_latest_build == bot()) { _latest_build = res; }
    return res;
  }

  inline BDD build_node(const int label, const BDD &low, const BDD &high)
  {
    _latest_build = _mgr.ITE(_mgr.Id(label+1), high, low);
    return _latest_build;
  }

  inline BDD build()
  {
    const BDD res = _latest_build;
    _latest_build = bot(); // <-- Reset and free builder reference
    return res;
  }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return _mgr.GetNumNodes(); }

  void print_stats()
  {
    std::cout << "\n";
    _mgr.Stats(stdout);
  }
};
