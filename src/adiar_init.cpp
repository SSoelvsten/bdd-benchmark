#include <adiar/adiar.h>
using namespace adiar;

#ifndef GRENDEL
#    define ADIAR_INIT(M)                       \
  coom_init(M)
#else
#    define ADIAR_INIT(M)                       \
  coom_init(M,"/scratch")
#endif

#define ADIAR_DEINIT                            \
  coom_deinit()                                 \

////////////////////////////////////////////////////////////////////////////////
class adiar_sat_policy
{
private:
  bdd sat_acc = bdd_true();

public:
  void reset()
  {
    sat_acc = bdd_true();
  }

  void and_clause(clause_t &clause)
  {
    node_file clause_bdd;

    { // All bdd functions require that no writer is attached to a file. So, we
      // garbage collect the writer before the bdd_apply call.
      node_writer clause_writer(clause_bdd);

      node_t n = create_sink(false);

      uint64_t label = UINT64_MAX;

      for (auto it = clause.rbegin(); it != clause.rend(); it++) {
        assert((*it).first < label);
        label = (*it).first;
        bool negated = (*it).second;

        n = create_node(label, 0,
                        negated ? create_sink(true) : n,
                        negated ? n : create_sink(true));

        clause_writer << n;
      }
    }

    sat_acc &= clause_bdd;
  }

  void quantify_variable(uint64_t var)
  {
    sat_acc = bdd_exists(sat_acc, var);
  }

  bool is_false()
  {
    return is_sink(sat_acc, adiar::is_false);
  }

  uint64_t satcount(uint64_t varcount)
  {
    return bdd_satcount(sat_acc, varcount);
  }

  uint64_t size()
  {
    return bdd_nodecount(sat_acc);
  }
};

typedef sat_solver<adiar_sat_policy> adiar_sat_solver;
