#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

#include "../common/adapter.h"

#include "cudd.h"
#include "cuddObj.hh"

inline unsigned long
cudd_memorysize()
{
  const size_t max_value = std::numeric_limits<unsigned long>::max() / (1024 * 1024);
  return std::min(static_cast<size_t>(M), max_value) * 1024 * 1024;
}

class cudd_adapter
{
protected:
  Cudd _mgr;
  const int _varcount;

protected:
  cudd_adapter(const int bdd_varcount, const int zdd_varcount)
    : _mgr(bdd_varcount, zdd_varcount, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, cudd_memorysize())
    , _varcount(bdd_varcount + zdd_varcount)
  {}

  ~cudd_adapter()
  { /* Do nothing */
  }

public:
  template <typename F>
  int
  run(const F& f)
  {
    return f();
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return _mgr.ReadKeys();
  }

  void
  print_stats()
  {
    std::cout << "\n"
              << "CUDD Statistics:\n"

              << "   Table:\n"
              << "   | peak node count:     " << _mgr.ReadPeakNodeCount() << "\n"
              << "   | node count (bdd):    " << _mgr.ReadNodeCount() << "\n"
              << "   | node count (zdd):    " << _mgr.zddReadNodeCount() << "\n"
              << "   | keys:                " << _mgr.ReadKeys() << "\n"
              << "   | dead:                " << _mgr.ReadDead()
              << "\n"

              // Commented lines are only available if 'DD_STATS' flag is set in CUDD compilation

              // INFO(" | Cache:\n");
              // INFO(" | | slots:               %zu\n", _mgr.ReadCacheUsedSlots());
              // INFO(" | | lookups:             %zu\n", _mgr.ReadCacheLookUps());
              // INFO(" | | hits:                %zu\n", _mgr.ReadCacheHits());

              << "   Garbage Collections:\n"
              << "   | runs:                " << _mgr.ReadGarbageCollections() << "\n"
              << "   | time (ms):           " << _mgr.ReadGarbageCollectionTime() << "\n";
  }
};

class cudd_bcdd_adapter : public cudd_adapter
{
public:
  static constexpr std::string_view name = "CUDD";
  static constexpr std::string_view dd   = "BCDD";

  static constexpr bool needs_extend     = false;
  static constexpr bool needs_frame_rule = true;

  static constexpr bool complement_edges = true;

public:
  typedef BDD dd_t;
  typedef BDD build_node_t;

private:
  BDD _latest_build;

  BDD _vars_relnext;
  std::vector<int> _permute_relnext;

  BDD _vars_relprev;
  std::vector<int> _permute_relprev;

  // Init and Deinit
public:
  cudd_bcdd_adapter(int varcount)
    : cudd_adapter(varcount, 0)
  { // Disable dynamic ordering
    if (!enable_reordering) { _mgr.AutodynDisable(); }

    _latest_build = bot();
  }

  // BDD Operations
public:
  inline BDD
  top()
  {
    return _mgr.bddOne();
  }

  inline BDD
  bot()
  {
    return _mgr.bddZero();
  }

  inline BDD
  ithvar(int i)
  {
    return _mgr.bddVar(i);
  }

  inline BDD
  nithvar(int i)
  {
    return ~_mgr.bddVar(i);
  }

  template <typename IT>
  inline BDD
  cube(IT rbegin, IT rend)
  {
    BDD res = top();
    while (rbegin != rend) { res = _mgr.bddVar(*(rbegin++)).Ite(res, bot()); }
    return res;
  }

  inline BDD
  cube(const std::function<bool(int)>& pred)
  {
    BDD res = top();
    for (int i = _varcount - 1; 0 <= i; --i) {
      if (pred(i)) { res = _mgr.bddVar(i).Ite(res, bot()); }
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
    return f.And(!g);
  }

  inline BDD
  apply_imp(const BDD& f, const BDD& g)
  {
    return f.Ite(g, _mgr.bddOne());
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
    return f.Ite(g, h);
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
    return f.ExistAbstract(_mgr.bddVar(i));
  }

  inline BDD
  exists(const BDD& f, const std::function<bool(int)>& pred)
  {
    return f.ExistAbstract(cube(pred));
  }

  template <typename IT>
  inline BDD
  exists(const BDD& f, IT rbegin, IT rend)
  {
    return f.ExistAbstract(cube(rbegin, rend));
  }

  inline BDD
  forall(const BDD& f, int i)
  {
    return f.UnivAbstract(_mgr.bddVar(i));
  }

  inline BDD
  forall(const BDD& f, const std::function<bool(int)>& pred)
  {
    return f.UnivAbstract(cube(pred));
  }

  template <typename IT>
  inline BDD
  forall(const BDD& f, IT rbegin, IT rend)
  {
    return f.UnivAbstract(cube(rbegin, rend));
  }

  inline BDD
  relnext(const BDD& states, const BDD& rel, const BDD& /*rel_support*/)
  {
    if (_permute_relnext.empty()) {
      _vars_relnext = cube([](int x) { return x % 2 == 0; });

      _permute_relnext.reserve(_varcount);
      for (int x = 0; x < _varcount; ++x) { _permute_relnext.push_back(x & ~1); }
    }

    return states.AndAbstract(rel, _vars_relnext).Permute(_permute_relnext.data());
  }

  inline BDD
  relprev(const BDD& states, const BDD& rel, const BDD& /*rel_support*/)
  {
    if (_permute_relprev.empty()) {
      _vars_relprev = cube([](int x) { return x % 2 == 1; });

      _permute_relprev.reserve(_varcount);
      for (int x = 0; x < _varcount; ++x) { _permute_relprev.push_back(x | 1); }
    }

    return states.Permute(_permute_relprev.data()).AndAbstract(rel, _vars_relprev);
  }

  inline uint64_t
  nodecount(const BDD& f)
  {
    return f.nodeCount();
  }

  inline uint64_t
  satcount(const BDD& f)
  {
    return this->satcount(f, _varcount);
  }

  inline uint64_t
  satcount(const BDD& f, const size_t vc)
  {
    return f.CountMinterm(vc);
  }

  inline BDD
  satone(const BDD& f)
  {
    return satone(f, f);
  }

  inline BDD
  satone(const BDD& f, const BDD& c)
  {
    std::vector<unsigned int> support_int = c.SupportIndices();
    std::vector<BDD> support_bdd;
    support_bdd.reserve(support_int.size());
    for (const auto x : support_int) { support_bdd.push_back(this->ithvar(x)); }
    return f.PickOneMinterm(support_bdd);
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const BDD& f)
  {
    std::string cudd_res(_varcount, '_');
    f.PickOneCube(cudd_res.data());

    std::vector<std::pair<int, char>> res;
    for (int x = 0; x < _varcount; ++x) {
      const char cudd_val = cudd_res.at(x);
      if (cudd_val == '_' || cudd_val == 2) { continue; }

      res.push_back({ x, '0' + cudd_val });
    }

    return res;
  }

  void
  print_dot(const BDD&, const std::string&)
  {
    std::cerr << "CUDD::PrintDot does not exist." << std::endl;
  }

  // BDD Build operations
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
    _latest_build = _mgr.makeBddNode(label, high, low);
    return _latest_build;
  }

  inline BDD
  build()
  {
    const BDD res = _latest_build;
    _latest_build = bot(); // <-- Reset and free builder reference
    return res;
  }
};

class cudd_zdd_adapter : public cudd_adapter
{
public:
  static constexpr std::string_view name = "CUDD";
  static constexpr std::string_view dd   = "zdd";

  static constexpr bool needs_extend     = true;
  static constexpr bool needs_frame_rule = true;

  static constexpr bool complement_edges = false;

public:
  typedef ZDD dd_t;
  typedef ZDD build_node_t;

private:
  ZDD _leaf0;
  ZDD _leaf1;

  ZDD _latest_build;

  // Init and Deinit
public:
  cudd_zdd_adapter(int varcount)
    : cudd_adapter(0, varcount)
  { // Disable dynamic ordering
    if (!enable_reordering) { _mgr.AutodynDisableZdd(); }

    _leaf0        = _mgr.zddZero();
    _leaf1        = _mgr.zddOne(std::numeric_limits<int>::max());
    _latest_build = _mgr.zddZero();
  }

  // ZDD Operations
public:
  inline ZDD
  top()
  {
    return _mgr.zddOne(0);
  }

  inline ZDD
  bot()
  {
    return _leaf0;
  }

  inline ZDD
  ithvar(const int i)
  {
    return _mgr.zddVar(i);
  }

  inline ZDD
  nithvar(const int i)
  {
    return ~_mgr.zddVar(i);
  }

  inline ZDD
  apply_and(const ZDD& f, const ZDD& g)
  {
    return f.Intersect(g);
  }

  inline ZDD
  apply_or(const ZDD& f, const ZDD& g)
  {
    return f.Union(g);
  }

  inline ZDD
  apply_diff(const ZDD& f, const ZDD& g)
  {
    return f.Diff(g);
  }

  inline ZDD
  apply_imp(const ZDD& f, const ZDD& g)
  {
    return f.Complement().Union(g);
  }

  inline ZDD
  apply_xor(const ZDD& f, const ZDD& g)
  {
    return (f.Union(g)).Diff(f.Intersect(g));
  }

  inline ZDD
  apply_xnor(const ZDD& f, const ZDD& g)
  {
    return this->apply_xor(f, g).Complement();
  }

  inline ZDD
  ite(const ZDD& f, const ZDD& g, const ZDD& h)
  {
    return f.Ite(g, h);
  }

  template <typename IT>
  inline ZDD
  extend(ZDD& f, IT /* begin*/, IT /*end*/)
  {
    throw std::logic_error("No support to 'Extend' ZDDs with Don't Cares (?)");
  }

  inline ZDD
  exists(const ZDD& f, int x)
  {
    throw std::logic_error("No support to 'Exists' for ZDDs");
    // One should think this may work, but since the variable is left in the output, then we cannot
    // recreate the BDD semantics.
    //
    // `return f.Subset0(x).Union(f.Subset1(x));`
  }

  inline ZDD
  exists(ZDD f, const std::function<bool(int)>& pred)
  {
    for (int x = _varcount - 1; 0 <= x; --x) {
      if (pred(x)) { f = this->exists(f, x); }
    }
    return f;
  }

  template <typename IT>
  inline ZDD
  exists(ZDD f, IT begin, IT end)
  {
    for (; begin != end; ++begin) { f = this->exists(*begin); }
    return f;
  }

  inline ZDD
  forall(const ZDD& f, int x)
  {
    throw std::logic_error("No support to 'Forall' for ZDDs");
    // One should think this may work, but since the variable is left in the output, then we cannot
    // recreate the BDD semantics.
    //
    // `return f.Subset0(x).Intersect(f.Subset1(x));`
  }

  inline ZDD
  forall(ZDD f, const std::function<bool(int)>& pred)
  {
    for (int x = _varcount - 1; 0 <= x; --x) {
      if (pred(x)) { f = this->forall(f, x); }
    }
    return f;
  }

  template <typename IT>
  inline ZDD
  forall(ZDD f, IT begin, IT end)
  {
    for (; begin != end; ++begin) { f = this->forall(*begin); }
    return f;
  }

  inline uint64_t
  nodecount(const ZDD& f)
  {
    return f.nodeCount();
  }

  inline uint64_t
  satcount(const ZDD& f)
  {
    return this->satcount(f, _varcount);
  }

  inline uint64_t
  satcount(const ZDD& f, const size_t vc)
  {
    return f.CountMinterm(vc);
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const ZDD& f)
  {
    return {};
  }

  void
  print_dot(const ZDD&, const std::string&)
  {
    std::cerr << "CUDD::PrintDot does not exist." << std::endl;
  }

  // ZDD Build operations
public:
  inline ZDD
  build_node(const bool value)
  {
    const ZDD res = value ? _leaf1 : _leaf0;
    if (_latest_build == _leaf0) { _latest_build = res; }
    return res;
  }

  inline ZDD
  build_node(const int label, const ZDD& low, const ZDD& high)
  {
    return _latest_build = _mgr.makeZddNode(label, high, low);
  }

  inline ZDD
  build()
  {
    const ZDD res = _latest_build;
    _latest_build = _leaf0; // <-- Reset and free builder reference
    return res;
  }
};
