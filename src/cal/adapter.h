#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/adapter.h"

#include <calObj.hh>

class cal_bcdd_adapter
{
public:
  static constexpr std::string_view name = "CAL";
  static constexpr std::string_view dd   = "BCDD";

  static constexpr bool needs_extend     = false;
  static constexpr bool needs_frame_rule = true;

  static constexpr bool complement_edges = true;

  // Variable type
public:
  typedef BDD dd_t;
  typedef BDD build_node_t;

private:
  Cal _mgr;
  const int _varcount;
  BDD _latest_build;

  int _relnext_vars  = -1;
  int _relnext_pairs = -1;
  int _relprev_vars  = -1;
  int _relprev_pairs = -1;

public:
  cal_bcdd_adapter(const int bdd_varcount)
    : _mgr(bdd_varcount)
    , _varcount(bdd_varcount)
  {
    _mgr.DynamicReordering(enable_reordering ? Cal::ReorderTechnique::Sift
                                             : Cal::ReorderTechnique::None);

    _latest_build = bot();
  }

  ~cal_bcdd_adapter()
  {
    if (this->_relnext_vars != -1) { this->_mgr.AssociationQuit(this->_relnext_vars); }
    if (this->_relnext_pairs != -1) { this->_mgr.AssociationQuit(this->_relnext_pairs); }
    if (this->_relprev_vars != -1) { this->_mgr.AssociationQuit(this->_relprev_vars); }
    if (this->_relprev_pairs != -1) { this->_mgr.AssociationQuit(this->_relprev_pairs); }
  }

public:
  template <typename F>
  int
  run(const F& f)
  {
    return f();
  }

  // BDD Operations
public:
  inline BDD
  top()
  {
    return _mgr.One();
  }

  inline BDD
  bot()
  {
    return _mgr.Zero();
  }

  inline BDD
  ithvar(int i)
  {
    return _mgr.Id(i + 1);
  }

  inline BDD
  nithvar(int i)
  {
    return ~_mgr.Id(i + 1);
  }

  template <typename IT>
  inline BDD
  cube(IT rbegin, IT rend)
  {
    BDD res = top();
    while (rbegin != rend) { res = ite(ithvar(*(rbegin++)), res, bot()); }
    return res;
  }

  inline BDD
  cube(const std::function<bool(int)>& pred)
  {
    BDD res = top();
    for (int i = _varcount - 1; 0 <= i; --i) {
      if (pred(i)) { res = ite(ithvar(i), res, bot()); }
    }
    return res;
  }

  inline BDD
  apply_and(const BDD& f, const BDD& g)
  {
    return f.And(g);
  }

  inline BDD
  apply_or(const BDD& f, const BDD& g)
  {
    return f.Or(g);
  }

  inline BDD
  apply_diff(const BDD& f, const BDD& g)
  {
    return f.And(g.Not());
  }

  inline BDD
  apply_imp(const BDD& f, const BDD& g)
  {
    return f.Not().Or(g);
  }

  inline BDD
  apply_xor(const BDD& f, const BDD& g)
  {
    return f.Xor(g);
  }

  inline BDD
  apply_xnor(const BDD& f, const BDD& g)
  {
    return f.Xnor(g);
  }

  inline BDD
  ite(const BDD& f, const BDD& g, const BDD& h)
  {
    return _mgr.ITE(f, g, h);
  }

  template <typename IT>
  inline BDD
  extend(const BDD& f, IT /*begin*/, IT /*end*/)
  {
    return f;
  }

  inline BDD
  exists(const BDD& f, int i)
  {
    std::vector<int> is = { i };
    return exists(f, is.begin(), is.end());
  }

  inline BDD
  exists(const BDD& f, const std::function<bool(int)>& pred)
  {
    _mgr.AssociationSetCurrent(new_temp_vars(pred));
    return _mgr.Exists(f);
  }

  template <typename IT>
  inline BDD
  exists(const BDD& f, IT rbegin, IT rend)
  {
    _mgr.AssociationSetCurrent(new_temp_vars(rbegin, rend));
    return _mgr.Exists(f);
  }

  inline BDD
  forall(const BDD& f, int i)
  {
    std::vector<int> is = { i };
    return forall(f, is.begin(), is.end());
  }

  inline BDD
  forall(const BDD& f, const std::function<bool(int)>& pred)
  {
    _mgr.AssociationSetCurrent(new_temp_vars(pred));
    return _mgr.ForAll(f);
  }

  template <typename IT>
  inline BDD
  forall(const BDD& f, IT rbegin, IT rend)
  {
    _mgr.AssociationSetCurrent(new_temp_vars(rbegin, rend));
    return _mgr.ForAll(f);
  }

  inline BDD
  relnext(const BDD& states, const BDD& rel, const BDD& /*rel_support*/)
  {
    if (_relnext_vars == -1) {
      std::vector<int> vars;
      for (int i = 0; i < this->_varcount; i += 2) { vars.push_back(i); }
      _relnext_vars = new_assoc_vars(vars.begin(), vars.end());
    }
    _mgr.AssociationSetCurrent(_relnext_vars);

    const BDD unshifted_quantified_product = _mgr.RelProd(states, rel);

    if (_relnext_pairs == -1) {
      std::vector<std::pair<int, int>> pairs;
      for (int i = 0; i < this->_varcount; i += 2) { pairs.push_back({ i + 1, i }); }
      _relnext_pairs = new_assoc_pairs(pairs.begin(), pairs.end());
    }
    _mgr.AssociationSetCurrent(_relnext_pairs);

    return _mgr.VarSubstitute(std::move(unshifted_quantified_product));
  }

  inline BDD
  relprev(const BDD& states, const BDD& rel, const BDD& /*rel_support*/)
  {
    if (_relprev_pairs == -1) {
      std::vector<std::pair<int, int>> pairs;
      for (int i = 0; i < this->_varcount; i += 2) { pairs.push_back({ i, i + 1 }); }
      _relprev_pairs = new_assoc_pairs(pairs.begin(), pairs.end());
    }
    _mgr.AssociationSetCurrent(_relprev_pairs);

    const BDD shifted_states = _mgr.VarSubstitute(states);

    if (_relprev_vars == -1) {
      std::vector<int> vars;
      for (int i = 0; i < this->_varcount; i += 2) { vars.push_back(i + 1); }
      _relprev_vars = new_assoc_vars(vars.begin(), vars.end());
    }
    _mgr.AssociationSetCurrent(_relprev_vars);

    return _mgr.RelProd(std::move(shifted_states), rel);
  }

  inline uint64_t
  nodecount(BDD f)
  {
    return _mgr.Size(f, true);
  }

  inline uint64_t
  satcount(BDD f)
  {
    return this->satcount(f, _varcount);
  }

  inline uint64_t
  satcount(BDD f, const size_t vc)
  {
    const double satFrac = _mgr.SatisfyingFraction(f);
    const double numVars = vc;
    return std::pow(2, numVars) * satFrac;
  }

  inline BDD
  satone(const BDD& f)
  {
    return f.Satisfy();
  }

  inline BDD
  satone(const BDD& f, BDD c)
  {
    _mgr.AssociationSetCurrent(new_temp_vars(c));
    return f.SatisfySupport();
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const BDD& f)
  {
    std::vector<std::pair<int, char>> res;

    BDD sat = f;
    while (!sat.IsConst()) {
      const int var = sat.Id() - 1;

      const BDD sat_low  = sat.Else();
      const BDD sat_high = sat.Then();

      const bool go_high = !sat_high.IsZero();
      res.push_back({ var, '0' + go_high });

      sat = go_high ? sat.Then() : sat.Else();
    }

    return res;
  }

  void
  print_dot(const BDD&, const std::string&)
  {
    std::cerr << "'CAL::PrintDot()' does not exist (SSoelvsten/Cal#6)." << std::endl;
  }

  void
  save(const BDD&, const std::string&)
  {
    std::cerr << "'CAL::DumpBdd()' does not exist (SSoelvsten/Cal#8)." << std::endl;
  }

private:
  bool
  is_complemented(const BDD& f)
  {
    return f != _mgr.Regular(f);
  }

  template <typename IT>
  int
  new_assoc_pairs(IT begin, IT end)
  {
    std::vector<BDD> vec;
    vec.reserve(std::distance(begin, end));

    while (begin != end) {
      const auto& iter_value = *begin++;
      vec.push_back(ithvar(iter_value.first));
      vec.push_back(ithvar(iter_value.second));
    }

    return _mgr.AssociationInit(vec.begin(), vec.end(), true);
  }

  template <typename IT>
  int
  new_assoc_vars(IT begin, IT end)
  {
    std::vector<BDD> vec;
    vec.reserve(std::distance(begin, end));

    while (begin != end) { vec.push_back(ithvar(*(begin++))); }

    return _mgr.AssociationInit(vec.begin(), vec.end());
  }

  int
  new_assoc_vars(const std::function<bool(int)>& pred)
  {
    std::vector<BDD> vec;

    for (int i = 0; i < _varcount; ++i) {
      if (pred(i)) { vec.push_back(ithvar(i)); }
    }

    return _mgr.AssociationInit(vec.begin(), vec.end());
  }

  template <typename IT>
  int
  new_temp_vars(IT begin, IT end)
  {
    std::vector<BDD> vec;
    vec.reserve(std::distance(begin, end));

    while (begin != end) { vec.push_back(ithvar(*(begin++))); }

    _mgr.TempAssociationInit(vec.begin(), vec.end());
    return -1;
  }

  int
  new_temp_vars(const std::function<bool(int)>& pred)
  {
    std::vector<BDD> vec;

    for (int i = 0; i < _varcount; ++i) {
      if (pred(i)) { vec.push_back(ithvar(i)); }
    }

    _mgr.TempAssociationInit(vec.begin(), vec.end());
    return -1;
  }

  int
  new_temp_vars(BDD c)
  {
    assert(c.IsCube());

    // TODO: Use CAL's Support function instead!?!
    std::vector<BDD> vec;

    while (!c.IsConst()) {
      vec.push_back(_mgr.Id(c.Id()));
      c = !c.Then().IsZero() ? c.Then() : c.Else();
    }

    _mgr.TempAssociationInit(vec.begin(), vec.end());
    return -1;
  }

  // BDD Build Operations
public:
  inline BDD
  build_node(const bool value)
  {
    const BDD res = value ? top() : bot();
    if (_latest_build == bot()) { _latest_build = res; }
    return res;
  }

  inline BDD
  build_node(const int label, const BDD& low, const BDD& high)
  {
    _latest_build = _mgr.ITE(_mgr.Id(label + 1), high, low);
    return _latest_build;
  }

  inline BDD
  build()
  {
    const BDD res = _latest_build;
    _latest_build = bot(); // <-- Reset and free builder reference
    return res;
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return _mgr.Nodes();
  }

  void
  print_stats()
  {
    std::cout << "\n";
    _mgr.Stats(stdout);
  }
};
