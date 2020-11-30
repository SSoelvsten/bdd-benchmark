#include <cmath>
#include <algorithm>
#include <assert.h>

// =============================================================================
// A few chrono wrappers to improve readability
#include <chrono>

inline std::chrono::steady_clock::time_point get_timestamp() {
  return std::chrono::steady_clock::now();
}

inline unsigned long int duration_of(std::chrono::steady_clock::time_point &before,
                                     std::chrono::steady_clock::time_point &after) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
}

// =============================================================================
// Common printing macros
#define INFO(s, ...) fprintf(stdout, s, ##__VA_ARGS__)
#define Abort(...) { fprintf(stderr, __VA_ARGS__); exit(-1); }


// =============================================================================
// Input parsing
#include <iostream>       // std::cerr
#include <stdexcept>      // std::invalid_argument

void parse_input(int &argc, char* argv[], size_t &N, size_t &M)
{
  try {
    if (argc > 1) {
      int N_ = std::atoi(argv[1]);
      if (N_ < 0) {
        Abort("N (first argument) should be nonnegative\n");
      }
      N = N_;
    }

    if (argc > 2) {
      int M_ = std::atoi(argv[2]);
      if (M_ <= 0) {
        Abort("M (second argument) should be positive\n");
      }
      M = M_;
    }
  } catch (std::invalid_argument const &ex) {
    Abort("Invalid number: %s\n", argv[1]);
  } catch (std::out_of_range const &ex) {
    Abort("Number out of range: %s\n", argv[1]);
  }
}

// =============================================================================
// SAT Solver
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
template <typename bdd_policy>
class sat_solver : private bdd_policy
{
private:
  uint64_t varcount;
  cnf_t clauses;

  size_t number_of_quantifications = 0;
  size_t number_of_applies = 0;
  size_t largest_nodecount = 0;

public:
  sat_solver(uint64_t var_count) : varcount(var_count) { }

  sat_solver(std::function<void()> on_reset,
             std::function<void(clause_t&)> on_and_clause,
             std::function<void(uint64_t)> on_exists,
             std::function<bool()> on_is_false,
             std::function<uint64_t(uint64_t)> on_satcount,
             std::function<uint64_t()> on_size,
             uint64_t var_count) : bdd_policy(on_reset, on_and_clause, on_exists, on_is_false, on_satcount, on_size),
                                   varcount(var_count)
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
    bdd_policy::reset();
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
      bdd_policy::and_clause(clause);
      number_of_applies++;
      largest_nodecount = std::max(largest_nodecount, bdd_policy::size());

      if (bdd_policy::is_false()) {
        return 0u;
      }
    }

    return bdd_policy::satcount(varcount);
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
        bdd_policy::quantify_variable(*seen_labels.rbegin());
        number_of_quantifications++;
        largest_nodecount = std::max(largest_nodecount, bdd_policy::size());

        seen_labels.erase(*seen_labels.rbegin());
      }

      for (literal_t &x : clause)
      {
        seen_labels.insert(x.first);
      }

      bdd_policy::and_clause(clause);
      number_of_applies++;
      largest_nodecount = std::max(largest_nodecount, bdd_policy::size());

      if (bdd_policy::is_false())
      {
        return false;
      }
    }

    return !(bdd_policy::is_false());
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
    return bdd_policy::size();
  }

  uint64_t apply_count()
  {
    return number_of_applies;
  }

  uint64_t exists_count()
  {
    return number_of_quantifications;
  }

private:
  using bdd_policy::reset;
  using bdd_policy::and_clause;
  using bdd_policy::quantify_variable;
  using bdd_policy::is_false;
  using bdd_policy::satcount;
  using bdd_policy::size;
};

////////////////////////////////////////////////////////////////////////////////
/// A BDD policy definable by use of lambda functions
////////////////////////////////////////////////////////////////////////////////
class bdd_policy
{
private:
  std::function<void()> on_reset;
  std::function<void(clause_t&)> on_and_clause;
  std::function<void(uint64_t)> on_exists;
  std::function<bool()> on_is_false;
  std::function<uint64_t(uint64_t)> on_satcount;
  std::function<uint64_t()> on_size;

public:
  bdd_policy(std::function<void()> on_reset,
             std::function<void(clause_t&)> on_and_clause,
             std::function<void(uint64_t)> on_exists,
             std::function<bool()> on_is_false,
             std::function<uint64_t(uint64_t)> on_satcount,
             std::function<uint64_t()> on_size) :
    on_reset(on_reset),
    on_and_clause(on_and_clause),
    on_exists(on_exists),
    on_is_false(on_is_false),
    on_satcount(on_satcount),
    on_size(on_size) { }

  void reset() { on_reset(); }

  void and_clause(clause_t &clause) { on_and_clause(clause); }

  void quantify_variable(uint64_t var) { on_exists(var); }

  bool is_false() { return on_is_false(); }

  uint64_t satcount(uint64_t varcount) { return on_satcount(varcount); }

  uint64_t size() { return on_size(); }
};
