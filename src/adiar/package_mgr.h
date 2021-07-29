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
#ifndef GRENDEL
    adiar::adiar_init(M);
#else
    adiar::adiar_init(M, temp_path);
#endif
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

  inline adiar::bdd exists(const adiar::bdd &b, int label)
  { return adiar::bdd_exists(b,label); }

  inline uint64_t nodecount(const adiar::bdd &b)
  { return adiar::bdd_nodecount(b); }

  inline uint64_t satcount(const adiar::bdd &b)
  { return adiar::bdd_satcount(b); }
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
