#include <coom/coom.h>

////////////////////////////////////////////////////////////////////////////////
class coom_sat_policy
{
private:
  coom::bdd sat_acc = coom::bdd_true();

public:
  void and_clause(clause_t &clause)
  {
    coom::node_file clause_bdd;

    { // All bdd functions require that no writer is attached to a file. So, we
      // garbage collect the writer before the bdd_apply call.
      coom::node_writer clause_writer(clause_bdd);

      coom::ptr_t next = coom::create_sink_ptr(false);

      for (auto it = clause.rbegin(); it != clause.rend(); it++) {
        literal_t v = *it;

        coom::node n = coom::create_node(v.first, 0,
                                         v.second ? coom::create_sink_ptr(true) : next,
                                         v.second ? next : coom::create_sink_ptr(true));

        next = n.uid;

        clause_writer << n;
      }
    }

    sat_acc &= clause_bdd;
  }

  void quantify_variable(uint64_t var)
  {
    sat_acc = coom::bdd_exists(sat_acc, var);
  }

  bool is_false()
  {
    return coom::is_sink(sat_acc, coom::is_false);
  }

  uint64_t satcount(uint64_t varcount)
  {
    return coom::bdd_satcount(sat_acc, varcount);
  }

  uint64_t size()
  {
    return coom::bdd_nodecount(sat_acc);
  }
};

typedef sat_solver<coom_sat_policy> coom_sat_solver;
