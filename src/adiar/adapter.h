#include <adiar/adiar.h>

class adiar_adapter
{
protected:
  const int _varcount;

  // Init and Deinit
protected:
  adiar_adapter(int vc) : _varcount(vc)
  {
    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;
    adiar::adiar_init(memory_bytes, temp_path);
    adiar::domain_set(vc);
  }

  ~adiar_adapter()
  {
    adiar::adiar_deinit();
  }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return 0; }

  void print_stats()
  {
    // Requires the "ADIAR_STATS" property to be ON in CMake
    std::cout << "\n";
    adiar::statistics_print();
  }
};

class adiar_bdd_adapter : public adiar_adapter
{
public:
  inline static const std::string NAME = "Adiar [BDD]";

  using dd_t   = adiar::bdd;
  using __dd_t = adiar::__bdd;

  using build_node_t = adiar::bdd_ptr;

private:
  adiar::bdd_builder _builder;

  // Init and Deinit
public:
  adiar_bdd_adapter(int vc) : adiar_adapter(vc)
  { }

  // BDD Operations
public:
  inline adiar::bdd
  top()
  { return adiar::bdd_top(); }

  inline adiar::bdd
  bot()
  { return adiar::bdd_bot(); }

  inline adiar::bdd
  ithvar(int i)
  { return adiar::bdd_ithvar(i); }

  inline adiar::bdd
  nithvar(int i)
  { return adiar::bdd_nithvar(i); }

  inline adiar::bdd
  ite(const adiar::bdd &i, const adiar::bdd &t, const adiar::bdd &e)
  { return adiar::bdd_ite(i,t,e); }

  inline adiar::bdd
  exists(const adiar::bdd &b, int i)
  { return adiar::bdd_exists(b, i); }

  inline adiar::bdd
  exists(const adiar::bdd &b, const std::function<bool(int)> &pred)
  { return adiar::bdd_exists(b, pred); }

  template<class IT>
  inline adiar::bdd
  exists(const adiar::bdd &b, IT rbegin, IT rend)
  { return adiar::bdd_exists(b, rbegin, rend); }

  inline adiar::bdd
  forall(const adiar::bdd &b, int i)
  { return adiar::bdd_forall(b, i); }

  inline adiar::bdd
  forall(const adiar::bdd &b, const std::function<bool(int)> &pred)
  { return adiar::bdd_forall(b, pred); }

  template<class IT>
  inline adiar::bdd
  forall(const adiar::bdd &b, IT rbegin, IT rend)
  { return adiar::bdd_forall(b, rbegin, rend); }

  inline uint64_t
  nodecount(const adiar::bdd &b)
  { return adiar::bdd_nodecount(b); }

  inline uint64_t
  satcount(const adiar::bdd &b)
  { return this->satcount(b, this->_varcount); }

  inline uint64_t
  satcount(const adiar::bdd &b, const size_t vc)
  { return adiar::bdd_satcount(b, vc); }

  inline std::vector<std::pair<int, char>>
  pickcube(const adiar::bdd &b)
  {
    // Unset domain temporarily to only get variables in the BDD.
    assert(adiar::domain_isset());
    const auto dom = adiar::domain_get();
    adiar::domain_unset();

    std::vector<std::pair<int, char>> res;

    adiar::bdd_satmin(b, [&res](const size_t x, const bool v) {
      res.push_back(std::make_pair(x, '0' + v));
    });

    adiar::domain_set(dom);
    return res;
  }

  void
  print_dot(const adiar::bdd &b, const std::string &filename)
  { return adiar::bdd_printdot(b, filename); }

  // BDD Build Operations
public:
  inline adiar::bdd_ptr build_node(const bool value)
  { return _builder.add_node(value); }

  inline adiar::bdd_ptr build_node(const int label,
                                   const adiar::bdd_ptr &low,
                                   const adiar::bdd_ptr &high)
  { return _builder.add_node(label, low, high); }

  inline adiar::bdd build()
  { return _builder.build(); }
};

class adiar_zdd_adapter : public adiar_adapter
{
public:
  inline static const std::string NAME = "Adiar [ZDD]";

  using dd_t   = adiar::zdd;
  using __dd_t = adiar::__zdd;

  using build_node_t = adiar::zdd_ptr;

private:
  adiar::zdd_builder _builder;

  // Init and Deinit
public:
  adiar_zdd_adapter(int vc) : adiar_adapter(vc)
  { }

  ~adiar_zdd_adapter()
  { }

  // ZDD Operations
public:
  inline adiar::zdd top()
  { return adiar::zdd_top(); }

  inline adiar::zdd bot()
  { return adiar::zdd_bot(); }

  inline adiar::zdd ithvar(int i)
  { return adiar::zdd_ithvar(i); }

  inline adiar::zdd nithvar(int i)
  { return adiar::zdd_nithvar(i); }

  inline adiar::zdd
  exists(const adiar::zdd &b, int i)
  { return adiar::zdd_project(b, [&i](int x){ return x != i; }); }

  inline adiar::zdd
  exists(const adiar::zdd &b, const std::function<bool(int)> &pred)
  { return adiar::zdd_project(b, [&pred](int x) { return !pred(x); }); }

  template<class IT>
  inline adiar::zdd
  exists(const adiar::zdd &b, IT rbegin, IT rend)
  {
    // To use `zdd_project`, flip the variables in the iterator.
    std::vector<typename IT::value_type> c;
    c.reserve(this->_varcount);

    for (int x = this->_varcount-1; 0 <= x; --x) {
      while (rbegin != rend && x < *rbegin) {
        rbegin++;
      }

      const int filter = rbegin == rend ? *rbegin : -1;
      if (x != filter) { c.push_back(x); }
    }

    // Project to remaining variables
    return adiar::zdd_project(b, c.cbegin(), c.cend());
  }

  inline adiar::zdd
  forall(const adiar::zdd &b, int i)
  { return ~(exists(~b, i)); }

  inline adiar::zdd
  forall(const adiar::zdd &b, const std::function<bool(int)> &pred)
  { return ~(exists(~b, pred)); }

  template<class IT>
  inline adiar::zdd
  forall(const adiar::zdd &b, IT rbegin, IT rend)
  { return ~(exists(~b, rbegin, rend)); }

  inline uint64_t nodecount(const adiar::zdd &z)
  { return adiar::zdd_nodecount(z); }

  inline uint64_t satcount(const adiar::zdd &z)
  { return this->satcount(z, this->_varcount); }

  inline uint64_t satcount(const adiar::zdd &z, const size_t/*vc*/)
  { return adiar::zdd_size(z); }

  inline std::vector<std::pair<int, char>>
  pickcube(const adiar::zdd &z)
  {
    std::vector<std::pair<int, char>> res;

    adiar::zdd_minelem(z, [&res](const size_t x) {
      res.push_back(std::make_pair(x, '1'));
    });

    return res;
  }

  void
  print_dot(const adiar::zdd &z, const std::string &filename)
  { return adiar::zdd_printdot(z, filename); }

  // ZDD Build Operations
public:
  inline adiar::zdd_ptr build_node(const bool value)
  { return _builder.add_node(value); }

  inline adiar::zdd_ptr build_node(const int label,
                                   const adiar::zdd_ptr &low,
                                   const adiar::zdd_ptr &high)
  { return _builder.add_node(label, low, high); }

  inline adiar::zdd build()
  { return _builder.build(); }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return 0; }
};
