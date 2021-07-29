#ifndef SAT_SOLVER_H
#define SAT_SOLVER_H

#include <functional>

#include <vector>
#include <set>
#include <queue>

////////////////////////////////////////////////////////////////////////////////
/// A literal is the variable label and whether it is negated
typedef std::pair<uint64_t, bool> literal_t;

////////////////////////////////////////////////////////////////////////////////
/// A clause is then a list of literals, seen as a disjunction
typedef std::vector<literal_t> clause_t;

////////////////////////////////////////////////////////////////////////////////
/// A formula is then a list of clauses
typedef std::vector<clause_t> cnf_t;

////////////////////////////////////////////////////////////////////////////////
/// The SAT solver is reliant on hooks, which are provided as lambda functions.
/// While it would have been prettier with the Policy pattern, then the
/// functions of Sylvan cannot be called without local variables generated
/// during initialisation in the respective main function being in scope.
////////////////////////////////////////////////////////////////////////////////
template<typename mgr_t>
typename mgr_t::bdd_t bdd_from_clause(mgr_t &mgr, clause_t &clause)
{
  typename mgr_t::bdd_t c = mgr.leaf_false();

  uint64_t label = UINT64_MAX;

  for (auto it = clause.rbegin(); it != clause.rend(); it++)
    {
      assert((*it).first < label);
      label = (*it).first;
      bool negated = (*it).second;

      typename mgr_t::bdd_t v = negated ? mgr.nithvar(label) : mgr.ithvar(label);
      c = mgr.ite(v, mgr.leaf_true(), c);
    }

  return c;
}

template<typename mgr_t>
class sat_solver
{
private:
  uint64_t varcount;
  cnf_t clauses;

  mgr_t mgr;
  typename mgr_t::bdd_t acc;

  size_t number_of_quantifications = 0;
  size_t number_of_applies = 0;
  size_t largest_nodecount = 0;

public:
  sat_solver(uint64_t varcount)
    : varcount(varcount),
      mgr(varcount),
      acc(mgr.leaf_true())
  { }

  //////////////////////////////////////////////////////////////////////////////
  /// Adds a clause to the entire formula in CNF. About the given formula we
  /// expect the following:
  ///  - Every variable occurs at most once in the entire clause.
  ///  - The clause is sorted with respect to the variable numbering.
  //////////////////////////////////////////////////////////////////////////////
  void add_clause(clause_t clause)
  {
    clauses.push_back(clause);
  }

  size_t cnf_size()
  {
    return clauses.size();
  }

  void cnf_print()
  {
    for (clause_t &clause : clauses) {
      INFO("\t[ ");

      for (literal_t &literal : clause) {
        if (literal.second) {
          INFO("~");
        }
        INFO("%zu ", literal.first);
      }

      INFO("]\n");
    }
  }

  size_t var_count()
  {
    return varcount;
  }

  //////////////////////////////////////////////////////////////////////////////
private:
  void reset()
  {
    acc = mgr.leaf_true();
    number_of_applies = 0;
    number_of_quantifications = 0;
    largest_nodecount = 0;
  }

  //////////////////////////////////////////////////////////////////////////////
public:
  uint64_t check_satcount()
  {
    reset();

    for (clause_t &clause : clauses) {
      acc &= bdd_from_clause(mgr, clause);
      number_of_applies++;
      largest_nodecount = std::max(largest_nodecount, bdd_size());

      if (acc == mgr.leaf_false()) {
        return 0u;
      }
    }

    return mgr.satcount(acc);
  }

  bool check_satisfiable()
  {
    reset();

    std::sort(clauses.begin(), clauses.end(),
              [](clause_t &a, clause_t &b) {
                return a.back().first > b.back().first;
              });

    std::set<uint64_t> seen_labels;

    for (clause_t &clause : clauses)
    {
      while (!seen_labels.empty() && clause.back().first < *seen_labels.rbegin())
      {
        acc = mgr.exists(acc, *seen_labels.rbegin());
        number_of_quantifications++;
        largest_nodecount = std::max(largest_nodecount, mgr.nodecount(acc));

        seen_labels.erase(*seen_labels.rbegin());
      }

      for (literal_t &x : clause)
      {
        seen_labels.insert(x.first);
      }

      acc &= bdd_from_clause(mgr, clause);
      number_of_applies++;
      largest_nodecount = std::max(largest_nodecount, mgr.nodecount(acc));

      if (acc == mgr.leaf_false())
      {
        return false;
      }
    }

    return acc != mgr.leaf_false();
  }

  bool check_unsatisfiable()
  {
    return !check_satisfiable();
  }

  //////////////////////////////////////////////////////////////////////////////
  uint64_t bdd_largest_size()
  {
    return largest_nodecount;
  }

  uint64_t bdd_size()
  {
    return mgr.nodecount(acc);
  }

  uint64_t apply_count()
  {
    return number_of_applies;
  }

  uint64_t exists_count()
  {
    return number_of_quantifications;
  }
};

#endif // SAT_SOLVER_H