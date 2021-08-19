#include <adiar/adiar.h>

class adiar_mgr
{
public:
  inline static const std::string NAME = "Adiar";

  // Variable type
public:
  typedef adiar::bdd bdd_t;

  // Init and Deinit
public:
  adiar_mgr(int /* varcount */)
  {
    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;
    adiar::adiar_init(memory_bytes, temp_path);
  }

  ~adiar_mgr()
  {
    adiar::adiar_deinit();
  }

  // BDD Operations
public:
  inline adiar::bdd leaf_true()
  { return adiar::bdd_true(); }

  inline adiar::bdd leaf_false()
  { return adiar::bdd_false(); }

  inline adiar::bdd ithvar(int label)
  { return adiar::bdd_ithvar(label); }

  inline adiar::bdd negate(const adiar::bdd &b)
  { return ~b; }

  inline adiar::bdd exists(const adiar::bdd &b, int label)
  { return adiar::bdd_exists(b,label); }

  inline uint64_t nodecount(const adiar::bdd &b)
  { return adiar::bdd_nodecount(b); }

  inline uint64_t satcount(const adiar::bdd &b)
  { return adiar::bdd_satcount(b); }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return 0; }

  void print_stats()
  {
    // Requires the "ADIAR_STATS" and/or "ADIAR_STATS_EXTRA" property to be ON in CMake
    INFO("\n");
    adiar::adiar_printstat();
  }
};

#include "../sat_solver.h"

template<>
adiar::bdd bdd_from_clause(adiar_mgr &/* mgr */, clause_t &clause)
{
  adiar::node_file clause_bdd;
  adiar::node_writer clause_writer(clause_bdd);

  adiar::node_t n = adiar::create_sink(false);

  uint64_t label = UINT64_MAX;

  for (auto it = clause.rbegin(); it != clause.rend(); it++) {
    assert((*it).first < label);
    label = (*it).first;
    bool negated = (*it).second;

    n = create_node(label, 0,
                    negated ? adiar::create_sink(true) : n,
                    negated ? n : adiar::create_sink(true));

    clause_writer << n;
  }

  return clause_bdd;
}
