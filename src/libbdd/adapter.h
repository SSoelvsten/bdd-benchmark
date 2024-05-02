#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "../common/adapter.h"

#include "lib-bdd.h"

namespace lib_bdd
{

  class bdd_function;

  enum class opt_bool : int8_t
  {
    NONE  = -1,
    FALSE = 0,
    TRUE  = 1,
  };

  class assignment
  {
    const capi::bdd_assignment_t _assignment;

    assignment(capi::bdd_assignment_t a)
      : _assignment(a)
    {}

    assignment(assignment& other) = delete; // use as_vector() to copy
    friend class bdd_function;

  public:
    ~assignment()
    {
      bdd_assignment_free(_assignment);
    }

    const opt_bool*
    data() const
    {
      return (opt_bool*)_assignment.data;
    }

    size_t
    size() const
    {
      return _assignment.len;
    }

    const opt_bool*
    begin() const
    {
      return data();
    }

    const opt_bool*
    end() const
    {
      return data() + size();
    }

    const opt_bool*
    cbegin() const
    {
      return data();
    }

    const opt_bool*
    cend() const
    {
      return data() + size();
    }

    const opt_bool&
    operator[](size_t idx)
    {
      assert(idx < size());
      return data()[idx];
    }

    std::vector<opt_bool>
    as_vector() const
    {
      return std::vector<opt_bool>(begin(), end());
    }
  };

  class manager
  {
    capi::manager_t _manager;

    manager(capi::manager_t manager)
      : _manager(manager)
    {}

  public:
    manager() noexcept
    {
      _manager._p = nullptr;
    }

    manager(uint16_t num_vars, size_t max_nodes_total) noexcept
      : _manager(capi::manager_new(num_vars, max_nodes_total))
    {}

    manager(const manager& other) noexcept
      : _manager(other._manager)
    {
      capi::manager_ref(_manager);
    }

    manager(manager&& other) noexcept
      : _manager(other._manager)
    {
      other._manager._p = nullptr;
    }

    ~manager() noexcept
    {
      if (_manager._p != nullptr) capi::manager_unref(_manager);
    }

    bool
    is_invalid() noexcept
    {
      return !_manager._p;
    }

    size_t
    node_count() const noexcept
    {
      return capi::manager_node_count(_manager);
    }

    bdd_function
    ithvar(uint16_t var) const noexcept;
    bdd_function
    nithvar(uint16_t var) const noexcept;

    bdd_function
    top() const noexcept;
    bdd_function
    bot() const noexcept;
  };

  class bdd_function
  {
    capi::bdd_t _func;

    friend class manager;

    bdd_function(capi::bdd_t func) noexcept
      : _func(func)
    {}

  public:
    bdd_function() noexcept
      : _func({ ._p = nullptr })
    {}

    bdd_function(const bdd_function& other) noexcept
      : _func(other._func)
    {
      capi::bdd_ref(_func);
    }

    bdd_function(bdd_function&& other) noexcept
      : _func(other._func)
    {
      other._func._p = nullptr;
    }

    ~bdd_function() noexcept
    {
      if (_func._p != nullptr) capi::bdd_unref(_func);
    }

    bdd_function&
    operator=(const bdd_function& rhs) noexcept
    {
      if (_func._p != nullptr) capi::bdd_unref(_func);
      _func = rhs._func;
      if (_func._p != nullptr) capi::bdd_ref(_func);
      return *this;
    }

    bdd_function&
    operator=(bdd_function&& rhs) noexcept
    {
      if (_func._p != nullptr) capi::bdd_unref(_func);
      _func        = rhs._func;
      rhs._func._p = nullptr;
      return *this;
    }

    bool
    is_invalid() const noexcept
    {
      return _func._p == nullptr;
    }

    bool
    operator==(const bdd_function& rhs) const noexcept
    {
      return (_func._p && rhs._func._p && capi::bdd_eq(_func, rhs._func))
        || (!_func._p && !rhs._func._p);
    }

    bool
    operator!=(const bdd_function& rhs) const noexcept
    {
      return !(*this == rhs);
    }

    bdd_function
    operator~() const noexcept
    {
      return capi::bdd_not(_func);
    }

    bdd_function
    operator&(const bdd_function& rhs) const noexcept
    {
      assert(_func._p && rhs._func._p);
      return capi::bdd_and(_func, rhs._func);
    }

    bdd_function&
    operator&=(const bdd_function& rhs) noexcept
    {
      return (*this = *this & rhs);
    }

    bdd_function
    operator|(const bdd_function& rhs) const noexcept
    {
      assert(_func._p && rhs._func._p);
      return capi::bdd_or(_func, rhs._func);
    }

    bdd_function&
    operator|=(const bdd_function& rhs) noexcept
    {
      return (*this = *this | rhs);
    }

    bdd_function
    operator^(const bdd_function& rhs) const noexcept
    {
      assert(_func._p && rhs._func._p);
      return capi::bdd_xor(_func, rhs._func);
    }

    bdd_function&
    operator^=(const bdd_function& rhs) noexcept
    {
      return (*this = *this | rhs);
    }

    bdd_function
    imp(const bdd_function& rhs) const noexcept
    {
      assert(_func._p && rhs._func._p);
      return capi::bdd_imp(_func, rhs._func);
    }

    bdd_function
    iff(const bdd_function& rhs) const noexcept
    {
      assert(_func._p && rhs._func._p);
      return capi::bdd_iff(_func, rhs._func);
    }

    bdd_function
    and_not(const bdd_function& rhs) const noexcept
    {
      assert(_func._p && rhs._func._p);
      return capi::bdd_and_not(_func, rhs._func);
    }

    bdd_function
    ite(const bdd_function& t, const bdd_function& e) const noexcept
    {
      assert(_func._p && t._func._p && e._func._p);
      return capi::bdd_ite(_func, t._func, e._func);
    }

    bdd_function
    var_forall(uint16_t var) const noexcept
    {
      assert(_func._p);
      return capi::bdd_var_forall(_func, var);
    }

    bdd_function
    var_exists(uint16_t var) const noexcept
    {
      assert(_func._p);
      return capi::bdd_var_exists(_func, var);
    }

    bdd_function
    forall(const uint16_t* vars, size_t num_vars) const noexcept
    {
      assert(_func._p);
      return capi::bdd_forall(_func, vars, num_vars);
    }

    bdd_function
    exists(const uint16_t* vars, size_t num_vars) const noexcept
    {
      assert(_func._p);
      return capi::bdd_exists(_func, vars, num_vars);
    }

    uint64_t
    node_count() const noexcept
    {
      assert(_func._p);
      return capi::bdd_nodecount(_func);
    }

    double
    sat_count() const noexcept
    {
      assert(_func._p);
      return capi::bdd_satcount(_func);
    }

    assignment
    pickcube() const noexcept
    {
      assert(_func._p);
      return capi::bdd_pickcube(_func);
    }
  };

  inline bdd_function
  manager::ithvar(uint16_t var) const noexcept
  {
    assert(_manager._p);
    return capi::manager_ithvar(_manager, var);
  }

  inline bdd_function
  manager::nithvar(uint16_t var) const noexcept
  {
    assert(_manager._p);
    return capi::manager_nithvar(_manager, var);
  }

  inline bdd_function
  manager::top() const noexcept
  {
    assert(_manager._p);
    return capi::manager_true(_manager);
  }

  inline bdd_function
  manager::bot() const noexcept
  {
    assert(_manager._p);
    return capi::manager_false(_manager);
  }
} // namespace lib_bdd

class libbdd_bdd_adapter
{
public:
  static constexpr std::string_view name = "LibBDD";
  static constexpr std::string_view dd   = "BDD";

  static constexpr bool needs_extend     = false;
  static constexpr bool complement_edges = false;

  using dd_t         = lib_bdd::bdd_function;
  using build_node_t = lib_bdd::bdd_function;

private:
  const int _varcount;
  lib_bdd::manager _manager;
  lib_bdd::bdd_function _latest_build;

  // Init and Deinit
public:
  libbdd_bdd_adapter(int varcount)
    : _varcount(varcount)
    , _manager(lib_bdd::manager(varcount, static_cast<size_t>(M) * 1024 * 1024 / 16))
  {}

  template <typename F>
  int
  run(const F& f)
  {
    return f();
  }

  // BDD Operations
  inline lib_bdd::bdd_function
  top()
  {
    return _manager.top();
  }

  inline lib_bdd::bdd_function
  bot()
  {
    return _manager.bot();
  }

  inline lib_bdd::bdd_function
  ithvar(uint32_t label)
  {
    return _manager.ithvar(label);
  }

  inline lib_bdd::bdd_function
  nithvar(uint32_t label)
  {
    return _manager.nithvar(label);
  }

  inline lib_bdd::bdd_function
  apply_and(const lib_bdd::bdd_function& f, const lib_bdd::bdd_function& g)
  {
    return f & g;
  }

  inline lib_bdd::bdd_function
  apply_or(const lib_bdd::bdd_function& f, const lib_bdd::bdd_function& g)
  {
    return f | g;
  }

  inline lib_bdd::bdd_function
  apply_diff(const lib_bdd::bdd_function& f, const lib_bdd::bdd_function& g)
  {
    return f.and_not(g);
  }

  inline lib_bdd::bdd_function
  apply_imp(const lib_bdd::bdd_function& f, const lib_bdd::bdd_function& g)
  {
    return f.imp(g);
  }

  inline lib_bdd::bdd_function
  apply_xor(const lib_bdd::bdd_function& f, const lib_bdd::bdd_function& g)
  {
    return f ^ g;
  }

  inline lib_bdd::bdd_function
  apply_xnor(const lib_bdd::bdd_function& f, const lib_bdd::bdd_function& g)
  {
    return f.iff(g);
  }

  inline lib_bdd::bdd_function
  ite(const lib_bdd::bdd_function& i,
      const lib_bdd::bdd_function& t,
      const lib_bdd::bdd_function& e)
  {
    return i.ite(t, e);
  }

  template <typename IT>
  inline lib_bdd::bdd_function
  extend(const lib_bdd::bdd_function& f, IT /*begin*/, IT /*end*/)
  {
    return f;
  }

  inline lib_bdd::bdd_function
  exists(const lib_bdd::bdd_function& b, int label)
  {
    return b.var_exists(label);
  }

  inline lib_bdd::bdd_function
  exists(const lib_bdd::bdd_function& b, const std::function<bool(int)>& pred)
  {
    std::vector<uint16_t> vars;
    vars.reserve(_varcount);
    for (uint16_t i = 0; i < _varcount; ++i) {
      if (pred(i)) vars.push_back(i);
    }
    return b.exists(vars.data(), vars.size());
  }

  template <typename IT>
  inline lib_bdd::bdd_function
  exists(const lib_bdd::bdd_function& b, IT rbegin, IT rend)
  {
    std::vector<uint16_t> vars(rbegin, rend);
    return b.exists(vars.data(), vars.size());
  }

  inline lib_bdd::bdd_function
  forall(const lib_bdd::bdd_function& b, int label)
  {
    return b.var_forall(label);
  }

  inline lib_bdd::bdd_function
  forall(const lib_bdd::bdd_function& b, const std::function<bool(int)>& pred)
  {
    std::vector<uint16_t> vars;
    vars.reserve(_varcount);
    for (uint16_t i = 0; i < _varcount; ++i) {
      if (pred(i)) vars.push_back(i);
    }
    return b.forall(vars.data(), vars.size());
  }

  template <typename IT>
  inline lib_bdd::bdd_function
  forall(const lib_bdd::bdd_function& b, IT rbegin, IT rend)
  {
    std::vector<uint16_t> vars(rbegin, rend);
    return b.forall(vars.data(), vars.size());
  }

  inline uint64_t
  nodecount(const lib_bdd::bdd_function& f)
  {
    return f.node_count();
  }

  inline uint64_t
  satcount(const lib_bdd::bdd_function& f)
  {
    return f.sat_count();
  }

  inline uint64_t
  satcount(const lib_bdd::bdd_function& f, const size_t vc)
  {
    assert(vc <= this->_varcount);

    const double excess_variables = static_cast<double>(this->_varcount) - static_cast<double>(vc);

    return f.sat_count() / std::pow(2, excess_variables);
  }

  inline std::vector<std::pair<uint32_t, char>>
  pickcube(const lib_bdd::bdd_function& f)
  {
    lib_bdd::assignment sat = f.pickcube();

    std::vector<std::pair<uint32_t, char>> res;
    res.reserve(sat.size());
    for (uint32_t x = 0; x < sat.size(); ++x) {
      const lib_bdd::opt_bool val = sat[x];
      if (val == lib_bdd::opt_bool::NONE) continue;

      res.emplace_back(x, '0' + static_cast<char>(val));
    }

    return res;
  }

  void
  print_dot(const lib_bdd::bdd_function&, const std::string&)
  {
    std::cerr << "libbdd_bdd_adapter does not support dot export" << std::endl;
  }

  // BDD Build Operations
public:
  inline lib_bdd::bdd_function
  build_node(const bool value)
  {
    const lib_bdd::bdd_function res = value ? top() : bot();
    if (_latest_build.is_invalid()) { _latest_build = res; }
    return res;
  }

  inline lib_bdd::bdd_function
  build_node(const uint32_t label,
             const lib_bdd::bdd_function& low,
             const lib_bdd::bdd_function& high)
  {
    return _latest_build = ite(ithvar(label), high, low);
  }

  inline lib_bdd::bdd_function
  build()
  {
    return std::move(_latest_build);
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return _manager.node_count();
  }

  void
  print_stats()
  {}
};
