#include <cstddef>
#include <cstdio>

#include <calObj.hh>

class cal_bdd_adapter
{
 public:
  inline static const std::string NAME = "CAL [BDD]";

  static constexpr bool needs_extend = false;

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

  inline BDD ite(BDD f, BDD g, BDD h)
  { return _mgr.ITE(f,g,h); }

  template <typename IT>
  inline BDD
  extend(const BDD &f, IT /*begin*/, IT /*end*/)
  { return f; }

  inline BDD exists(const BDD &f, int i)
  {
    std::vector<int> is = {i};
    return exists(f, is.begin(), is.end());
  }

  inline BDD exists(const BDD &f, const std::function<bool(int)> &pred)
  {
    set_temp_association(pred);
    return _mgr.Exists(f);
  }

  template<typename IT>
  inline BDD exists(const BDD &f, IT rbegin, IT rend)
  {
    set_temp_association(rbegin, rend);
    return _mgr.Exists(f);
  }

  inline BDD forall(const BDD &f, int i)
  {
    std::vector<int> is = {i};
    return forall(f, is.begin(), is.end());
  }

  inline BDD forall(const BDD &f, const std::function<bool(int)> &pred)
  {
    set_temp_association(pred);
    return _mgr.ForAll(f);
  }

  template<typename IT>
  inline BDD forall(const BDD &f, IT rbegin, IT rend)
  {
    set_temp_association(rbegin, rend);
    return _mgr.ForAll(f);
  }

  inline uint64_t nodecount(BDD f)
  { return _mgr.Size(f); }

  inline uint64_t
  satcount(BDD f)
  { return this->satcount(f, this->_varcount); }

  inline uint64_t
  satcount(BDD f, const size_t vc)
  {
    const double satFrac = _mgr.SatisfyingFraction(f);
    const double numVars = vc;
    return std::pow(2, numVars) * satFrac;
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const BDD &f)
  {
    std::vector<std::pair<int, char>> res;

    BDD sat = _mgr.Satisfy(f);
    while (sat != _mgr.One() && sat != _mgr.Zero()) {
      const int  var       = sat.Id()-1;

      const BDD  sat_low  = sat.Else();
      const BDD  sat_high = sat.Then();

      const bool go_high = sat.Then() != _mgr.Zero();
      res.push_back({ var, '0'+go_high });

      sat = go_high ? sat.Then() : sat.Else();
    }

    return res;
  }

  void
  print_dot(const BDD &, const std::string &)
  {
    std::cerr << "CAL::PrintDot does not exist (SSoelvsten/Cal#6)." << std::endl;
  }

private:
  bool is_complemented(const BDD &f)
  { return f != _mgr.Regular(f); }

  template<typename IT>
  void set_temp_association(IT begin, IT end)
  {
    std::vector<BDD> vec;
    vec.reserve(std::distance(begin, end));

    while (begin != end) {
      vec.push_back(ithvar(*(begin++)));
    }

    _mgr.TempAssociationInit(vec.begin(), vec.end());
  }

  void set_temp_association(const std::function<bool(int)> &pred)
  {
    std::vector<BDD> vec;

    for (int i = 0; i < _varcount; ++i) {
      if (pred(i)) {
        vec.push_back(ithvar(i));
      }
    }

    _mgr.TempAssociationInit(vec.begin(), vec.end());
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
