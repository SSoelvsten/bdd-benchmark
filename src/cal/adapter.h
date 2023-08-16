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
  Cal _mgr;
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
    std::vector<int> i;
    i.push_back(label);
    return exists(b, i.begin(), i.end());
  }

  template<typename IT>
  inline BDD exists(const BDD &b, IT rbegin, IT rend)
  {
    const int assoc = convert_to_association_list(rbegin, rend);
    _mgr.AssociationSetCurrent(assoc);
    const BDD res = _mgr.Exists(b);
    _mgr.AssociationQuit(assoc);
    return res;
  }

  inline BDD forall(const BDD &b, int label)
  {
    std::vector<int> i;
    i.push_back(label);
    return forall(b, i.begin(), i.end());
  }

  template<typename IT>
  inline BDD forall(const BDD &b, IT rbegin, IT rend)
  {
    const int assoc = convert_to_association_list(rbegin, rend);
    _mgr.AssociationSetCurrent(assoc);
    const BDD res = _mgr.Exists(b);
    _mgr.AssociationQuit(assoc);
    return res;
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
    BDD support = _mgr.SatisfySupport(b);
    std::vector<std::pair<int, char>> res;

    while (support != _mgr.One() && support != _mgr.Zero()) {
      const int  i = support.Id()-1;
      const char v = '0' + (support.Then() != _mgr.Zero())
                         + (support.Then() == support.Else());

      res.push_back({i,v});

      support = v != '0' ? support.Then() : support.Else();
    }

    return res;
  }

private:
  bool is_complemented(const BDD &b)
  { return b != _mgr.Regular(b); }

  template<typename IT>
  int convert_to_association_list(IT begin, IT end)
  {
    std::vector<BDD> vec;
    vec.reserve(std::distance(begin, end));

    while (begin != end) {
      vec.push_back(ithvar(*(begin++)));
    }

    return _mgr.AssociationInit(vec.begin(), vec.end());
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
