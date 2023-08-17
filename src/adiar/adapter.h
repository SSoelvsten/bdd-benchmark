#include <adiar/adiar.h>

class adiar_adapter
{
protected:
  const int varcount;

  // Init and Deinit
protected:
  adiar_adapter(int vc) : varcount(vc)
  {
    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;
    adiar::adiar_init(memory_bytes, temp_path);
    adiar::adiar_set_domain(vc);
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
    adiar::adiar_printstat();
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
  { return adiar::bdd_true(); }

  inline adiar::bdd
  bot()
  { return adiar::bdd_false(); }

  inline adiar::bdd
  ithvar(int label)
  { return adiar::bdd_ithvar(label); }

  inline adiar::bdd
  negate(const adiar::bdd &b)
  { return ~b; }

  inline adiar::bdd
  ite(const adiar::bdd &i, const adiar::bdd &t, const adiar::bdd &e)
  { return adiar::bdd_ite(i,t,e); }

  inline adiar::bdd
  exists(const adiar::bdd &b, int label)
  { return adiar::bdd_exists(b,label); }

  template<class IT>
  inline adiar::bdd
  exists(const adiar::bdd &b, IT rbegin, IT rend)
  { return adiar::bdd_exists(b, rbegin, rend); }

  inline adiar::bdd
  forall(const adiar::bdd &b, int label)
  { return adiar::bdd_forall(b,label); }

  template<class IT>
  inline adiar::bdd
  forall(const adiar::bdd &b, IT rbegin, IT rend)
  { return adiar::bdd_forall(b, rbegin, rend); }

  inline uint64_t
  nodecount(const adiar::bdd &b)
  { return adiar::bdd_nodecount(b); }

  inline uint64_t
  satcount(const adiar::bdd &b)
  { return adiar::bdd_satcount(b, varcount); }

  inline std::vector<std::pair<int, char>>
  pickcube(const adiar::bdd &b)
  {
    std::vector<std::pair<int, char>> res;

    adiar::bdd_satmin(b, [&res](const size_t x, const bool v) {
      res.push_back(std::make_pair(x, '0' + v));
    });

    return res;
  }

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
  { return adiar::zdd_powerset(adiar::adiar_get_domain()); }

  inline adiar::zdd bot()
  { return adiar::zdd_empty(); }

  inline adiar::zdd ithvar(int i)
  { return adiar::zdd_ithvar(i); }

  inline adiar::zdd negate(const adiar::zdd &z)
  { return ~z; }

  inline uint64_t nodecount(const adiar::zdd &z)
  { return adiar::zdd_nodecount(z); }

  inline uint64_t satcount(const adiar::zdd &z)
  { return adiar::zdd_size(z); }

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

  void print_stats()
  {
    // Requires the "ADIAR_STATS" and/or "ADIAR_STATS_EXTRA" property to be ON in CMake
    std::cout << "\n";
    adiar::adiar_printstat();
  }
};
