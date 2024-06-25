#include <cassert>
#include <cstdlib>
#include <string>
#include <string_view>

#include "../common/adapter.h"

#include <adiar/adiar.h>

class adiar_adapter
{
protected:
  const int _varcount;

  // Init and Deinit
protected:
  adiar_adapter(int vc)
    : _varcount(vc)
  {
    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;
    adiar::adiar_init(memory_bytes, temp_path);
    adiar::domain_set(vc);
  }

  ~adiar_adapter()
  {
    adiar::adiar_deinit();
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
    return 0;
  }

  void
  print_stats()
  {
    // Requires the "ADIAR_STATS" property to be ON in CMake
    std::cout << "\n";
    adiar::statistics_print();
  }
};

class adiar_bdd_adapter : public adiar_adapter
{
public:
  static constexpr std::string_view name = "Adiar";
  static constexpr std::string_view dd   = "BDD";

  static constexpr bool needs_extend     = false;
  static constexpr bool needs_frame_rule = true;

  static constexpr bool complement_edges = false;

public:
  using dd_t   = adiar::bdd;
  using __dd_t = adiar::__bdd;

  using build_node_t = adiar::bdd_ptr;

private:
  adiar::bdd_builder _builder;

  // Init and Deinit
public:
  adiar_bdd_adapter(int vc)
    : adiar_adapter(vc)
  {}

  // BDD Operations
public:
  inline adiar::bdd
  top()
  {
    return adiar::bdd_top();
  }

  inline adiar::bdd
  bot()
  {
    return adiar::bdd_bot();
  }

  inline adiar::bdd
  ithvar(int i)
  {
    return adiar::bdd_ithvar(i);
  }

  inline adiar::bdd
  nithvar(int i)
  {
    return adiar::bdd_nithvar(i);
  }

  template <typename IT>
  inline adiar::bdd
  cube(IT rbegin, IT rend)
  {
    return adiar::bdd_cube(rbegin, rend);
  }

  inline adiar::bdd
  cube(const std::function<bool(int)>& pred)
  {
    const auto terminal_bot = this->build_node(false);
    const auto terminal_top = this->build_node(true);

    build_node_t root = terminal_top;
    for (int i = _varcount - 1; 0 <= i; --i) {
      if (pred(i)) { root = this->build_node(i, terminal_bot, root); }
    }

    return this->build();
  }

  inline adiar::bdd
  apply_and(const adiar::bdd& f, const adiar::bdd& g)
  {
    return adiar::bdd_and(f, g);
  }

  inline adiar::bdd
  apply_or(const adiar::bdd& f, const adiar::bdd& g)
  {
    return adiar::bdd_or(f, g);
  }

  inline adiar::bdd
  apply_diff(const adiar::bdd& f, const adiar::bdd& g)
  {
    return adiar::bdd_diff(f, g);
  }

  inline adiar::bdd
  apply_imp(const adiar::bdd& f, const adiar::bdd& g)
  {
    return adiar::bdd_imp(f, g);
  }

  inline adiar::bdd
  apply_xor(const adiar::bdd& f, const adiar::bdd& g)
  {
    return adiar::bdd_xor(f, g);
  }

  inline adiar::bdd
  apply_xnor(const adiar::bdd& f, const adiar::bdd& g)
  {
    return adiar::bdd_xnor(f, g);
  }

  inline adiar::bdd
  ite(const adiar::bdd& f, const adiar::bdd& g, const adiar::bdd& h)
  {
    return adiar::bdd_ite(f, g, h);
  }

  template <typename IT>
  inline adiar::bdd
  extend(const adiar::bdd& f, IT /*begin*/, IT /*end*/)
  {
    return f;
  }

  inline adiar::bdd
  exists(const adiar::bdd& f, int i)
  {
    return adiar::bdd_exists(f, i);
  }

  inline adiar::bdd
  exists(const adiar::bdd& f, const std::function<bool(int)>& pred)
  {
    return adiar::bdd_exists(f, pred);
  }

  template <class IT>
  inline adiar::bdd
  exists(const adiar::bdd& f, IT rbegin, IT rend)
  {
    return adiar::bdd_exists(f, rbegin, rend);
  }

  inline adiar::bdd
  forall(const adiar::bdd& f, int i)
  {
    return adiar::bdd_forall(f, i);
  }

  inline adiar::bdd
  forall(const adiar::bdd& f, const std::function<bool(int)>& pred)
  {
    return adiar::bdd_forall(f, pred);
  }

  template <class IT>
  inline adiar::bdd
  forall(const adiar::bdd& f, IT rbegin, IT rend)
  {
    return adiar::bdd_forall(f, rbegin, rend);
  }

  inline adiar::bdd
  relnext(const adiar::bdd& states, const adiar::bdd& rel, const adiar::bdd& /*rel_support*/)
  {
    return adiar::bdd_relnext(
      states,
      rel,
      [](const int x) -> adiar::optional<int> {
        return (x % 2) == 0 ? adiar::make_optional<int>() : adiar::make_optional<int>(x - 1);
      },
      adiar::replace_type::Shift);
  }

  inline adiar::bdd
  relprev(const adiar::bdd& states, const adiar::bdd& rel, const adiar::bdd& /*rel_support*/)
  {
    return adiar::bdd_relprev(
      states,
      rel,
      [](const int x) -> adiar::optional<int> {
        return (x % 2) == 1 ? adiar::make_optional<int>() : adiar::make_optional<int>(x + 1);
      },
      adiar::replace_type::Shift);
  }

  inline uint64_t
  nodecount(const adiar::bdd& f)
  {
    const uint64_t c = adiar::bdd_nodecount(f);
    // Adiar does not count terminal nodes. So, we'll compensate to make it consistent with the
    // numbers from other BDD packages.
    return c == 0 ? 1 : c + 2;
  }

  inline adiar::bdd
  satone(const adiar::bdd& f)
  {
    return adiar::bdd_satmin(f);
  }

  inline adiar::bdd
  satone(const adiar::bdd& f, const adiar::bdd& c)
  {
    return adiar::bdd_satmin(f, c);
  }

  inline uint64_t
  satcount(const adiar::bdd& f)
  {
    return this->satcount(f, this->_varcount);
  }

  inline uint64_t
  satcount(const adiar::bdd& f, const size_t vc)
  {
    return adiar::bdd_satcount(f, vc);
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const adiar::bdd& f)
  {
    // Unset domain temporarily to only get variables in the BDD.
    assert(adiar::domain_isset());
    const auto dom = adiar::domain_get();
    adiar::domain_unset();

    std::vector<std::pair<int, char>> res;

    adiar::bdd_satmin(f, [&res](const adiar::pair<adiar::bdd::label_type, bool>& xv) {
      res.push_back(std::make_pair(xv.first, '0' + xv.second));
    });

    adiar::domain_set(dom);
    return res;
  }

  void
  print_dot(const adiar::bdd& f, const std::string& filename)
  {
    return adiar::bdd_printdot(f, filename);
  }

  // BDD Build Operations
public:
  inline adiar::bdd_ptr
  build_node(const bool value)
  {
    return _builder.add_node(value);
  }

  inline adiar::bdd_ptr
  build_node(const int label, const adiar::bdd_ptr& low, const adiar::bdd_ptr& high)
  {
    return _builder.add_node(label, low, high);
  }

  inline adiar::bdd
  build()
  {
    return _builder.build();
  }
};

class adiar_zdd_adapter : public adiar_adapter
{
public:
  static constexpr std::string_view name = "Adiar";
  static constexpr std::string_view dd   = "ZDD";

  static constexpr bool needs_extend     = true;
  static constexpr bool complement_edges = false;

public:
  using dd_t   = adiar::zdd;
  using __dd_t = adiar::__zdd;

  using build_node_t = adiar::zdd_ptr;

private:
  adiar::zdd_builder _builder;

  // Init and Deinit
public:
  adiar_zdd_adapter(int vc)
    : adiar_adapter(vc)
  {}

  ~adiar_zdd_adapter()
  {}

  // ZDD Operations
public:
  inline adiar::zdd
  top()
  {
    return adiar::zdd_top();
  }

  inline adiar::zdd
  bot()
  {
    return adiar::zdd_bot();
  }

  inline adiar::zdd
  ithvar(int i)
  {
    return adiar::zdd_ithvar(i);
  }

  inline adiar::zdd
  nithvar(int i)
  {
    return adiar::zdd_nithvar(i);
  }

  inline adiar::zdd
  apply_and(const adiar::zdd& f, const adiar::zdd& g)
  {
    return adiar::zdd_intsec(f, g);
  }

  inline adiar::zdd
  apply_or(const adiar::zdd& f, const adiar::zdd& g)
  {
    return adiar::zdd_union(f, g);
  }

  inline adiar::zdd
  apply_diff(const adiar::zdd& f, const adiar::zdd& g)
  {
    return adiar::zdd_diff(f, g);
  }

  inline adiar::zdd
  apply_imp(const adiar::zdd& f, const adiar::zdd& g)
  {
    return adiar::zdd_union(adiar::zdd_complement(f), g);
  }

  inline adiar::zdd
  apply_xor(const adiar::zdd& f, const adiar::zdd& g)
  {
    return adiar::zdd_diff(adiar::zdd_union(f, g), adiar::zdd_intsec(f, g));
  }

  inline adiar::zdd
  apply_xnor(const adiar::zdd& f, const adiar::zdd& g)
  {
    return adiar::zdd_complement(this->apply_xor(f, g));
  }

  inline adiar::zdd
  ite(const adiar::zdd& f, const adiar::zdd& g, const adiar::zdd& h)
  {
    return adiar::zdd_union(adiar::zdd_intsec(f, g),
                            adiar::zdd_intsec(adiar::zdd_complement(f), h));
  }

  template <typename IT>
  inline adiar::zdd
  extend(const adiar::zdd& f, IT begin, IT end)
  {
    return adiar::zdd_expand(f, begin, end);
  }

  inline adiar::zdd
  exists(const adiar::zdd& f, int i)
  {
    return adiar::zdd_project(f, [&i](int x) { return x != i; });
  }

  inline adiar::zdd
  exists(const adiar::zdd& f, const std::function<bool(int)>& pred)
  {
    return adiar::zdd_project(f, [&pred](int x) { return !pred(x); });
  }

  template <class IT>
  inline adiar::zdd
  exists(const adiar::zdd& f, IT rbegin, IT rend)
  {
    // To use `zdd_project`, flip the variables in the iterator.
    std::vector<typename IT::value_type> c;
    c.reserve(this->_varcount);

    for (int x = this->_varcount - 1; 0 <= x; --x) {
      while (rbegin != rend && x < *rbegin) { rbegin++; }

      const int filter = rbegin == rend ? *rbegin : -1;
      if (x != filter) { c.push_back(x); }
    }

    // Project to remaining variables
    return adiar::zdd_project(f, c.cbegin(), c.cend());
  }

  inline adiar::zdd
  forall(const adiar::zdd& f, int i)
  {
    return ~(exists(~f, i));
  }

  inline adiar::zdd
  forall(const adiar::zdd& f, const std::function<bool(int)>& pred)
  {
    return ~(exists(~f, pred));
  }

  template <class IT>
  inline adiar::zdd
  forall(const adiar::zdd& f, IT rbegin, IT rend)
  {
    return ~(exists(~f, rbegin, rend));
  }

  inline uint64_t
  nodecount(const adiar::zdd& f)
  {
    return adiar::zdd_nodecount(f);
  }

  inline uint64_t
  satcount(const adiar::zdd& f)
  {
    return this->satcount(f, this->_varcount);
  }

  inline uint64_t
  satcount(const adiar::zdd& f, const size_t /*vc*/)
  {
    return adiar::zdd_size(f);
  }

  inline std::vector<std::pair<int, char>>
  pickcube(const adiar::zdd& f)
  {
    std::vector<std::pair<int, char>> res;

    adiar::zdd_minelem(f, [&res](const size_t x) { res.push_back(std::make_pair(x, '1')); });

    return res;
  }

  void
  print_dot(const adiar::zdd& z, const std::string& filename)
  {
    return adiar::zdd_printdot(z, filename);
  }

  // ZDD Build Operations
public:
  inline adiar::zdd_ptr
  build_node(const bool value)
  {
    return _builder.add_node(value);
  }

  inline adiar::zdd_ptr
  build_node(const int label, const adiar::zdd_ptr& low, const adiar::zdd_ptr& high)
  {
    return _builder.add_node(label, low, high);
  }

  inline adiar::zdd
  build()
  {
    return _builder.build();
  }

  // Statistics
public:
  inline size_t
  allocated_nodes()
  {
    return 0;
  }
};
