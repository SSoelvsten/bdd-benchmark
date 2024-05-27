#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "../common/adapter.h"

#include "oxidd/bdd.hpp"
#include "oxidd/bcdd.hpp"
#include "oxidd/zbdd.hpp"

inline std::pair<size_t, size_t>
compute_init_size(unsigned cache_arity)
{
  // TODO: We should export these numbers in OxiDD, they are dependent on the
  // manager, apply cache implementation, and the maximum arity in use. Here, we
  // assume the index-based manager, direct-mapped apply cache, and an operator
  // arity of 2.
  constexpr double bytes_per_node = 16 + 8 / 0.75;
  double bytes_per_cache_entry    = 4 + 4 * cache_arity;
  constexpr double cache_ratio    = 64;

  // We need to maximize x and y in the following system of inequalities:
  // bytes_per_node * x + bytes_per_cache_entry * y <= M , x = y * CACHE_RATIO
  const size_t memory_bytes = static_cast<size_t>(M) * 1024 * 1024;
  const size_t x =
    memory_bytes / ((bytes_per_node * cache_ratio + bytes_per_cache_entry) / cache_ratio);
  const size_t y = x / cache_ratio;

  return { std::min(x, ((size_t)1 << 32) - 2), y };
}

class oxidd_bdd_adapter
{
public:
  static constexpr std::string_view name = "OxiDD";
  static constexpr std::string_view dd   = "BDD";

  static constexpr bool needs_extend     = false;
  static constexpr bool complement_edges = false;

  using dd_t         = oxidd::bdd_function;
  using build_node_t = oxidd::bdd_function;

private:
  oxidd::bdd_manager _manager;
  std::vector<oxidd::bdd_function> _vars;
  oxidd::bdd_function _latest_build;

  template <typename IT>
  inline oxidd::bdd_function
  make_cube(IT rbegin, IT rend)
  {
    oxidd::bdd_function cube = top();
    while (rbegin != rend) { cube &= _vars[*(rbegin++)]; }
    return cube;
  }

  inline oxidd::bdd_function
  make_cube(const std::function<bool(int)>& pred)
  {
    oxidd::bdd_function cube = top();
    for (size_t i = 0; i < _vars.size(); ++i) {
      if (pred(i)) cube &= _vars[i];
    }
    return cube;
  }

  // Init and Deinit
public:
  oxidd_bdd_adapter(uint32_t varcount)
    : _manager(compute_init_size(3).first, compute_init_size(3).second, threads)
  {
    _vars.reserve(varcount);
    for (uint32_t i = 0; i < varcount; i++) { _vars.emplace_back(_manager.new_var()); }
  }

  template <typename F>
  int
  run(const F& f)
  {
    return f();
  }

  // BDD Operations
public:
  inline oxidd::bdd_function
  top()
  {
    return _manager.t();
  }

  inline oxidd::bdd_function
  bot()
  {
    return _manager.f();
  }

  inline oxidd::bdd_function
  ithvar(uint32_t label)
  {
    return _vars[label];
  }

  inline oxidd::bdd_function
  nithvar(uint32_t label)
  {
    return ~_vars[label];
  }

  inline oxidd::bdd_function
  apply_and(const oxidd::bdd_function& f, const oxidd::bdd_function& g)
  {
    return f & g;
  }

  inline oxidd::bdd_function
  apply_or(const oxidd::bdd_function& f, const oxidd::bdd_function& g)
  {
    return f | g;
  }

  inline oxidd::bdd_function
  apply_diff(const oxidd::bdd_function& f, const oxidd::bdd_function& g)
  {
    return g.imp_strict(f);
  }

  inline oxidd::bdd_function
  apply_imp(const oxidd::bdd_function& f, const oxidd::bdd_function& g)
  {
    return f.imp(g);
  }

  inline oxidd::bdd_function
  apply_xor(const oxidd::bdd_function& f, const oxidd::bdd_function& g)
  {
    return f ^ g;
  }

  inline oxidd::bdd_function
  apply_xnor(const oxidd::bdd_function& f, const oxidd::bdd_function& g)
  {
    return f.equiv(g);
  }

  inline oxidd::bdd_function
  ite(const oxidd::bdd_function& i, const oxidd::bdd_function& t, const oxidd::bdd_function& e)
  {
    return i.ite(t, e);
  }

  template <typename IT>
  inline oxidd::bdd_function
  extend(const oxidd::bdd_function& f, IT /*begin*/, IT /*end*/)
  {
    return f;
  }

  inline oxidd::bdd_function
  exists(const oxidd::bdd_function& b, int label)
  {
    return b.exist(_vars[label]);
  }

  inline oxidd::bdd_function
  exists(const oxidd::bdd_function& b, const std::function<bool(int)>& pred)
  {
    return b.exist(make_cube(pred));
  }

  template <typename IT>
  inline oxidd::bdd_function
  exists(const oxidd::bdd_function& b, IT rbegin, IT rend)
  {
    return b.exist(make_cube(rbegin, rend));
  }

  inline oxidd::bdd_function
  forall(const oxidd::bdd_function& b, int label)
  {
    return b.forall(_vars[label]);
  }

  inline oxidd::bdd_function
  forall(const oxidd::bdd_function& b, const std::function<bool(int)>& pred)
  {
    return b.forall(make_cube(pred));
  }

  template <typename IT>
  inline oxidd::bdd_function
  forall(const oxidd::bdd_function& b, IT rbegin, IT rend)
  {
    return b.forall(rbegin, rend);
  }

  inline uint64_t
  nodecount(const oxidd::bdd_function& f)
  {
    return f.node_count();
  }

  inline uint64_t
  satcount(const oxidd::bdd_function& f)
  {
    return f.sat_count_double(_vars.size());
  }

  inline uint64_t
  satcount(const oxidd::bdd_function& f, const size_t vc)
  {
    assert(vc <= _vars.size());
    return f.sat_count_double(vc);
  }

  inline std::vector<std::pair<uint32_t, char>>
  pickcube(const oxidd::bdd_function& f)
  {
    oxidd::util::assignment sat = f.pick_cube();

    std::vector<std::pair<uint32_t, char>> res;
    res.reserve(sat.size());
    for (uint32_t x = 0; x < sat.size(); ++x) {
      const oxidd::util::opt_bool val = sat[x];
      if (val == oxidd::util::opt_bool::NONE) continue;

      res.emplace_back(x, '0' + static_cast<char>(val));
    }

    return res;
  }

  void
  print_dot(const oxidd::bdd_function&, const std::string&)
  {
    std::cerr << "oxidd_bdd_adapter does not yet support dot export" << std::endl;
  }

  // BDD Build Operations
public:
  inline oxidd::bdd_function
  build_node(const bool value)
  {
    const oxidd::bdd_function res = value ? top() : bot();
    if (_latest_build.is_invalid() || _latest_build == top() || _latest_build == bot()) {
      _latest_build = res;
    }
    return res;
  }

  inline oxidd::bdd_function
  build_node(const uint32_t label, const oxidd::bdd_function& low, const oxidd::bdd_function& high)
  {
    return _latest_build = ite(ithvar(label), high, low);
  }

  inline oxidd::bdd_function
  build()
  {
    return std::move(_latest_build);
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return _manager.num_inner_nodes();
  }

  void
  print_stats()
  {
    std::cout << "OxiDD statistics:" << std::endl
              << "  inner nodes stored in manager: " << _manager.num_inner_nodes() << std::endl;
    oxidd::capi::oxidd_bdd_print_stats();
  }
};

class oxidd_bcdd_adapter
{
public:
  static constexpr std::string_view name = "OxiDD";
  static constexpr std::string_view dd   = "BCDD";

  static constexpr bool needs_extend     = false;
  static constexpr bool complement_edges = true;

  using dd_t         = oxidd::bcdd_function;
  using build_node_t = oxidd::bcdd_function;

private:
  oxidd::bcdd_manager _manager;
  std::vector<oxidd::bcdd_function> _vars;
  oxidd::bcdd_function _latest_build;

  template <typename IT>
  inline oxidd::bcdd_function
  make_cube(IT rbegin, IT rend)
  {
    oxidd::bcdd_function res = top();
    while (rbegin != rend) { res &= _vars[*(rbegin++)]; }
    return res;
  }

  inline oxidd::bcdd_function
  make_cube(const std::function<bool(int)>& pred)
  {
    oxidd::bcdd_function cube = top();
    for (size_t i = 0; i < _vars.size(); ++i) {
      if (pred(i)) cube &= _vars[i];
    }
    return cube;
  }

  // Init and Deinit
public:
  oxidd_bcdd_adapter(uint32_t varcount)
    : _manager(compute_init_size(3).first, compute_init_size(3).second, threads)
  {
    _vars.reserve(varcount);
    for (uint32_t i = 0; i < varcount; i++) { _vars.emplace_back(_manager.new_var()); }
  }

  template <typename F>
  int
  run(const F& f)
  {
    return f();
  }

  // BDD Operations
public:
  inline oxidd::bcdd_function
  top()
  {
    return _manager.t();
  }

  inline oxidd::bcdd_function
  bot()
  {
    return _manager.f();
  }

  inline oxidd::bcdd_function
  ithvar(uint32_t label)
  {
    return _vars[label];
  }

  inline oxidd::bcdd_function
  nithvar(uint32_t label)
  {
    return ~_vars[label];
  }

  inline oxidd::bcdd_function
  apply_and(const oxidd::bcdd_function& f, const oxidd::bcdd_function& g)
  {
    return f & g;
  }

  inline oxidd::bcdd_function
  apply_or(const oxidd::bcdd_function& f, const oxidd::bcdd_function& g)
  {
    return f | g;
  }

  inline oxidd::bcdd_function
  apply_diff(const oxidd::bcdd_function& f, const oxidd::bcdd_function& g)
  {
    return g.imp_strict(f);
  }

  inline oxidd::bcdd_function
  apply_imp(const oxidd::bcdd_function& f, const oxidd::bcdd_function& g)
  {
    return f.imp(g);
  }

  inline oxidd::bcdd_function
  apply_xor(const oxidd::bcdd_function& f, const oxidd::bcdd_function& g)
  {
    return f ^ g;
  }

  inline oxidd::bcdd_function
  apply_xnor(const oxidd::bcdd_function& f, const oxidd::bcdd_function& g)
  {
    return f.equiv(g);
  }

  inline oxidd::bcdd_function
  ite(const oxidd::bcdd_function& i, const oxidd::bcdd_function& t, const oxidd::bcdd_function& e)
  {
    return i.ite(t, e);
  }

  template <typename IT>
  inline oxidd::bcdd_function
  extend(const oxidd::bcdd_function& f, IT /*begin*/, IT /*end*/)
  {
    return f;
  }

  inline oxidd::bcdd_function
  exists(const oxidd::bcdd_function& b, int label)
  {
    return b.exist(_vars[label]);
  }

  inline oxidd::bcdd_function
  exists(const oxidd::bcdd_function& b, const std::function<bool(int)>& pred)
  {
    return b.exist(make_cube(pred));
  }

  template <typename IT>
  inline oxidd::bcdd_function
  exists(const oxidd::bcdd_function& b, IT rbegin, IT rend)
  {
    return b.exist(make_cube(rbegin, rend));
  }

  inline oxidd::bcdd_function
  forall(const oxidd::bcdd_function& b, int label)
  {
    return b.forall(_vars[label]);
  }

  inline oxidd::bcdd_function
  forall(const oxidd::bcdd_function& b, const std::function<bool(int)>& pred)
  {
    return b.forall(make_cube(pred));
  }

  template <typename IT>
  inline oxidd::bcdd_function
  forall(const oxidd::bcdd_function& b, IT rbegin, IT rend)
  {
    return b.forall(rbegin, rend);
  }

  inline uint64_t
  nodecount(const oxidd::bcdd_function& f)
  {
    return f.node_count();
  }

  inline uint64_t
  satcount(const oxidd::bcdd_function& f)
  {
    return f.sat_count_double(_vars.size());
  }

  inline uint64_t
  satcount(const oxidd::bcdd_function& f, const size_t vc)
  {
    assert(vc <= _vars.size());
    return f.sat_count_double(vc);
  }

  inline std::vector<std::pair<uint32_t, char>>
  pickcube(const oxidd::bcdd_function& f)
  {
    oxidd::util::assignment sat = f.pick_cube();

    std::vector<std::pair<uint32_t, char>> res;
    res.reserve(sat.size());
    for (uint32_t x = 0; x < sat.size(); ++x) {
      const oxidd::util::opt_bool val = sat[x];
      if (val == oxidd::util::opt_bool::NONE) continue;

      res.emplace_back(x, '0' + static_cast<char>(val));
    }

    return res;
  }

  void
  print_dot(const oxidd::bcdd_function&, const std::string&)
  {
    std::cerr << "oxidd_bdd_adapter does not yet support dot export" << std::endl;
  }

  // BDD Build Operations
public:
  inline oxidd::bcdd_function
  build_node(const bool value)
  {
    const oxidd::bcdd_function res = value ? top() : bot();
    if (_latest_build.is_invalid() || _latest_build == top() || _latest_build == bot()) {
      _latest_build = res;
    }
    return res;
  }

  inline oxidd::bcdd_function
  build_node(const uint32_t label,
             const oxidd::bcdd_function& low,
             const oxidd::bcdd_function& high)
  {
    return _latest_build = ite(ithvar(label), high, low);
  }

  inline oxidd::bcdd_function
  build()
  {
    return std::move(_latest_build);
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return _manager.num_inner_nodes();
  }

  void
  print_stats()
  {
    std::cout << "OxiDD statistics:" << std::endl
              << "  inner nodes stored in manager: " << _manager.num_inner_nodes() << std::endl;
    oxidd::capi::oxidd_bdd_print_stats();
  }
};

class oxidd_zdd_adapter
{
public:
  static constexpr std::string_view name = "OxiDD";
  static constexpr std::string_view dd   = "ZDD";

  static constexpr bool needs_extend     = true;
  static constexpr bool complement_edges = false;

public:
  using dd_t         = oxidd::zbdd_function;
  using build_node_t = oxidd::zbdd_function;

private:
  oxidd::zbdd_manager _manager;
  std::vector<oxidd::zbdd_function> _vars;
  oxidd::zbdd_function _latest_build;

  // Init and Deinit
public:
  oxidd_zdd_adapter(uint32_t varcount)
    : _manager(compute_init_size(3).first, compute_init_size(3).second, threads)
  {
    _vars.reserve(varcount);
    for (uint32_t i = 0; i < varcount; i++) { _vars.emplace_back(_manager.new_singleton()); }
  }

  template <typename F>
  int
  run(const F& f)
  {
    return f();
  }

  // ZDD Operations
public:
  inline oxidd::zbdd_function
  top()
  {
    return _manager.t();
  }

  inline oxidd::zbdd_function
  bot()
  {
    return _manager.f();
  }

  inline oxidd::zbdd_function
  ithvar(const int i)
  {
    return _vars[i];
  }

  inline oxidd::zbdd_function
  nithvar(const int i)
  {
    return ~_vars[i];
  }

  inline oxidd::zbdd_function
  apply_and(const oxidd::zbdd_function& f, const oxidd::zbdd_function& g)
  {
    return f & g;
  }

  inline oxidd::zbdd_function
  apply_or(const oxidd::zbdd_function& f, const oxidd::zbdd_function& g)
  {
    return f | g;
  }

  inline oxidd::zbdd_function
  apply_diff(const oxidd::zbdd_function& f, const oxidd::zbdd_function& g)
  {
    return f - g;
  }

  inline oxidd::zbdd_function
  apply_imp(const oxidd::zbdd_function& f, const oxidd::zbdd_function& g)
  {
    return f.imp(g);
  }

  inline oxidd::zbdd_function
  apply_xor(const oxidd::zbdd_function& f, const oxidd::zbdd_function& g)
  {
    return f ^ g;
  }

  inline oxidd::zbdd_function
  apply_xnor(const oxidd::zbdd_function& f, const oxidd::zbdd_function& g)
  {
    return f.equiv(g);
  }

  inline oxidd::zbdd_function
  ite(const oxidd::zbdd_function& f, const oxidd::zbdd_function& g, const oxidd::zbdd_function& h)
  {
    return f.ite(g, h);
  }

  template <typename IT>
  inline oxidd::zbdd_function
  extend(oxidd::zbdd_function& f, IT /* begin*/, IT /*end*/)
  {
    throw std::logic_error("No support to 'Extend' ZDDs with Don't Cares (?)");
  }

  inline oxidd::zbdd_function
  exists(const oxidd::zbdd_function& f, int x)
  {
    throw std::logic_error("No support to 'Exists' for ZDDs");
    // One should think this may work, but since the variable is left in the output, then we cannot
    // recreate the BDD semantics.
    //
    // `return f.Subset0(x).Union(f.Subset1(x));`
  }

  inline oxidd::zbdd_function
  exists(oxidd::zbdd_function f, const std::function<bool(int)>& pred)
  {
    throw std::logic_error("No support to 'Exists' for ZDDs");
  }

  template <typename IT>
  inline oxidd::zbdd_function
  exists(oxidd::zbdd_function f, IT begin, IT end)
  {
    throw std::logic_error("No support to 'Exists' for ZDDs");
  }

  inline oxidd::zbdd_function
  forall(const oxidd::zbdd_function& f, int x)
  {
    throw std::logic_error("No support to 'Forall' for ZDDs");
    // One should think this may work, but since the variable is left in the output, then we cannot
    // recreate the BDD semantics.
    //
    // `return f.Subset0(x).Intersect(f.Subset1(x));`
  }

  inline oxidd::zbdd_function
  forall(oxidd::zbdd_function f, const std::function<bool(int)>& pred)
  {
    throw std::logic_error("No support to 'Forall' for ZDDs");
  }

  template <typename IT>
  inline oxidd::zbdd_function
  forall(oxidd::zbdd_function f, IT begin, IT end)
  {
    throw std::logic_error("No support to 'Forall' for ZDDs");
  }

  inline uint64_t
  nodecount(const oxidd::zbdd_function& f)
  {
    return f.node_count();
  }

  inline uint64_t
  satcount(const oxidd::zbdd_function& f)
  {
    return f.sat_count_double(_vars.size());
  }

  inline uint64_t
  satcount(const oxidd::zbdd_function& f, const size_t vc)
  {
    return f.sat_count_double(vc);
  }

  inline std::vector<std::pair<uint32_t, char>>
  pickcube(const oxidd::zbdd_function& f)
  {
    oxidd::util::assignment sat = f.pick_cube();

    std::vector<std::pair<uint32_t, char>> res;
    res.reserve(sat.size());
    for (uint32_t x = 0; x < sat.size(); ++x) {
      const oxidd::util::opt_bool val = sat[x];
      if (val == oxidd::util::opt_bool::NONE) continue;

      res.emplace_back(x, '0' + static_cast<char>(val));
    }

    return res;
  }

  void
  print_dot(const oxidd::zbdd_function&, const std::string&)
  {
    std::cerr << "oxidd_zdd_adapter does not yet support dot export" << std::endl;
  }

  // ZDD Build operations
public:
  inline oxidd::zbdd_function
  build_node(const bool value)
  {
    const oxidd::zbdd_function res = value ? _manager.base() : _manager.empty();
    if (_latest_build.is_invalid() || _latest_build == top() || _latest_build == bot()) {
      _latest_build = res;
    }
    return res;
  }

  inline oxidd::zbdd_function
  build_node(const int label, const oxidd::zbdd_function& low, const oxidd::zbdd_function& high)
  {
    return _latest_build =
             _vars[label].make_node(oxidd::zbdd_function(high), oxidd::zbdd_function(low));
  }

  inline oxidd::zbdd_function
  build()
  {
    return std::move(_latest_build);
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return _manager.num_inner_nodes();
  }

  void
  print_stats()
  {
    std::cout << "OxiDD statistics:" << std::endl
              << "  inner nodes stored in manager: " << _manager.num_inner_nodes() << std::endl;
    oxidd::capi::oxidd_bdd_print_stats();
  }
};
