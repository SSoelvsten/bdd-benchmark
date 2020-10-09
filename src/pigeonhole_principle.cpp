#include <stdint.h>
#include <functional>
#include <utility>

#include <assert.h>

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
class sat_solver
{
private:
  cnf_t clauses;
  bool should_sort = false;

public:
  //////////////////////////////////////////////////////////////////////////////
  /// Adds a clause to the entire formula in CNF. About the given formula we
  /// expect the following:
  ///  - Every variable occurs at most once in the entire clause.
  ///  - The clause is sorted with respect to the variable numbering.
  //////////////////////////////////////////////////////////////////////////////
  void add_clause(clause_t clause)
  {
    clauses.push_back(clause);
    should_sort = true;
  }

  size_t cnf_size()
  {
    return clauses.size();
  }

  //////////////////////////////////////////////////////////////////////////////
  /// Checks whether the formula is unsatisfiable by adding one clause at a
  /// time, and seeing whether it collapses to the false sink.
  ///
  /// To allow use of different libraries, the OBDD manipulation are provided as
  /// lambda-functions, which essentially works as a compositional template
  /// pattern. The functions are:
  ///
  ///  - on_and_clause:
  ///      AND the given clause onto the intermediate result.
  ///
  ///  - on_quantify_variable:
  ///      Existentially quantify the variable with the given label. We
  ///      guarantee, this variable will never more be seen.
  ///
  ///  - on_is_false:
  ///      Answers whether the intermediate result has collapsed to the false
  ///      sink.
  ///
  /// Here, we assume that the intermediate results OBDD starts out to be the
  /// true sink.
  //////////////////////////////////////////////////////////////////////////////
  bool is_unsatisfiable(const std::function<void(clause_t&)> &on_and_clause,
                        const std::function<void(uint64_t)> &on_quantify_variable,
                        const std::function<bool()> &on_is_false)
  {
    if (should_sort)
    {
      // TODO: Based on the footnote in pigeonhole formulas, we may just want to
      // construct it in the same order they used?
      std::sort(clauses.begin(), clauses.end(),
                [](clause_t &a, clause_t &b) {
                  return a.back().first > b.back().first;
                });
    }

    std::set<uint64_t> unique_labels;
    std::priority_queue<uint64_t> priority_labels;

    for (clause_t &clause : clauses)
    {
      while (!priority_labels.empty() && clause.back().first < priority_labels.top())
      {
        on_quantify_variable(priority_labels.top());

        unique_labels.erase(priority_labels.top());
        priority_labels.pop();
      }

      for (literal_t &x : clause)
      {
        if (unique_labels.insert(x.first).second)
        {
          priority_labels.push(x.first);
        }
      }

      on_and_clause(clause);

      if (on_is_false())
      {
        return true;
      }
    }

    while (!priority_labels.empty())
    {
      on_quantify_variable(priority_labels.top());

      unique_labels.erase(priority_labels.top());
      priority_labels.pop();
    }

    return on_is_false();
  }

  //////////////////////////////////////////////////////////////////////////////
  /// Checks whether the constructed formula is satisfiable using the same hook
  /// functions as described for is_unsatisfiable above.
  //////////////////////////////////////////////////////////////////////////////
  bool is_satisfiable(const std::function<void(clause_t&)> &on_and_clause,
                      const std::function<void(uint64_t)> &on_quantify_variable,
                      const std::function<bool()> &on_is_false)
  {
    return !is_unsatisfiable(on_and_clause, on_quantify_variable, on_is_false);
  }
};

////////////////////////////////////////////////////////////////////////////////
uint64_t label_of_Pij(uint64_t i, uint64_t j, int N)
{
  assert(0 <= i && i <= N+1);
  assert(0 <= j && j <= N);
  return (N+1)*i + j;
}

////////////////////////////////////////////////////////////////////////////////
/// Constructs the CNF for the Pigeonhole Principle based on the paper by Olga
/// Tveretina, Carsten Sinz, and Hans Zantema "Ordered Binary Decision Diagrams,
/// Pigeonhole Formulas and Beyond".
////////////////////////////////////////////////////////////////////////////////
void construct_PHP_cnf(sat_solver &solver, int N)
{
  // PC_n
  for (uint64_t i = 1; i <= N+1; i++)
  {
    clause_t clause;
    for (uint64_t j = 1; j <= N; j++)
    {
      clause.push_back(literal_t (label_of_Pij(i,j,N), false));
    }
    solver.add_clause(clause);
  }

  // NC_n
  for (uint64_t i = 1; i < N+1; i++)
  {
    for (uint64_t j = i+1; j <= N+1; j++)
    {
      for (uint64_t k = 1; k <= N; k++)
      {
        clause_t clause;
        clause.push_back(literal_t (label_of_Pij(i,k,N), true));
        clause.push_back(literal_t (label_of_Pij(j,k,N), true));
        solver.add_clause(clause);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// TODO: Extended Pigeonhole Principle from the same paper...
