// Assertions
#include <algorithm>
#include <cassert>

// Data structures
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

// Types
#include <cstdlib>

// For reading the CNF file
#include <filesystem>
#include <fstream>

#include "common/adapter.h"
#include "common/chrono.h"
#include "common/input.h"
#include "common/json.h"

#ifdef BDD_BENCHMARK_STATS
size_t largest_bdd = 0;
size_t total_nodes = 0;
#endif // BDD_BENCHMARK_STATS

// ========================================================================== //
std::string file;
bool satcount = false;

class parsing_policy
{
public:
  static constexpr std::string_view name = "CNF";
  static constexpr std::string_view args = "f:c";

  static constexpr std::string_view help_text =
    "        -c                    Count satisfying assignments\n"
    "        -f PATH               Path to '.cnf'/'.dimacs' file";

  // NOTE: One could add more options to this benchmark, e.g., to influence the
  // order in which the clauses are conjoined. The current implementation
  // interprets the linear clause ordering given in the input file as an
  // (approximately) balanced binary tree, e.g., `(c0 ∧ c1) ∧ (c2 ∧ (c3 ∧ c4))`.
  // Importantly, it does not commute any operands, i.e., we do not conjoin `c0`
  // and `c4` (or any dependant intermediate results) before `c1` has been
  // processed.
  //
  // In general, this benchmark allows quite important options to be tuned by
  // preprocessing the CNF. We can, e.g., apply preprocessing techniques from
  // #SAT solving using the tool
  // [pmc](https://www.cril.univ-artois.fr/KC/pmc.html), and compute variable
  // and clause orderings using [MINCE](http://www.aloul.net/Tools/mince/).
  // Implementing the algorithms of these tools here would require adding a lot
  // of code, or at least quite a few dependencies (a SAT solver, a hypergraph
  // cutter). Also, we want to be sure that we actually give the same clauses
  // in the same order to the different libraries. Hence, we would need to
  // ensure that the algorithms are deterministic. Therefore it is much easier
  // (and also less time-consuming) to do all the preprocessing steps
  // externally.

  /// Parse one command line option, where `c` is one of keys specified in the
  /// `args` constant of this class and `arg` is the option's value (if the
  /// option takes one).
  ///
  /// Returns `true` on error.
  static inline bool
  parse_input(const int c, const char* arg)
  {
    switch (c) {
    case 'f': {
      if (!std::filesystem::exists(arg)) {
        std::cerr << "File '" << arg << "' does not exist\n";
        return true;
      }
      if (!file.empty()) {
        std::cerr << "Only one file may be given\n";
        return true;
      }

      file = arg;
      return false;
    }
    case 'c': {
      satcount = true;
      return false;
    }
    default: return true;
    }
  }
};

// ========================================================================== //

/// Helper function to compute `|value|` as `unsigned` without causing undefined
/// behavior on `INT_MIN`
unsigned
unsigned_abs(int value)
{
  // from https://stackoverflow.com/a/32676663
  return value < 0 ? 0 - unsigned(value) : unsigned(value);
}

class CNF
{
  /// Literals of all clauses
  std::vector<int> _clause_data;

  /// Offsets of the clauses in `_clause_data`
  ///
  /// If there is at least one clause, then `_clause_offsets[0]` is 0. The size
  /// of this vector is exactly the number of clauses.
  std::vector<size_t> _clause_offsets;

  /// Map from variables to levels
  std::vector<unsigned> _var_to_level;

public:
  /// Parse a DIMACS CNF file from `path`
  ///
  /// A sample file might look like this:
  ///
  /// ```
  /// c 2 b
  /// c 1 a
  /// c 3 c
  /// c 4 d
  /// p cnf 4 4
  /// -1 2 0
  /// -2 1 0
  /// 3 0
  /// -4 0
  /// ```
  ///
  /// The comment lines (starting with `c`) are used to specify the variable
  /// order. Here, variable 2 called "b" is at the top, then variable 1 "a"
  /// follows, and so on. The variable order is optional but if present, if must
  /// contain all variables.
  static std::optional<CNF>
  parse_dimacs_cnf(const std::string& path)
  {
    size_t nclauses;
    CNF cnf;
    std::ifstream input(path, std::ios::in);

    // Parse the variable order
    std::vector<unsigned> var_order;
    std::string tok;
    unsigned line = 0;
    while (input.good()) {
      const int c = input.get();
      ++line;
      if (c == 'c') {
        input >> tok;
        if (std::all_of(tok.begin(), tok.end(), ::isdigit)) {
          // Ignore lines where the token after `c` is not numeric. This allows extensions of the
          // format.

          const unsigned var_id = std::stoul(tok);
          if (var_id == 0) {
            std::cerr << "error: variable numbers must be > 0 (in variable order)\n";
            return {};
          }
          var_order.push_back(var_id - 1);
        }
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      } else if (c == 'p') {
        break;
      } else {
        std::cerr << "error: unexpected character '" << static_cast<char>(c)
                  << "' at beginning of line" << line << "\n";
        return {};
      }
    }

    // Read the problem line (`p cnf <#vars> <#clauses>`)
    std::string problem_type;
    unsigned nvars;
    input >> problem_type >> nvars >> nclauses;

    if (input.fail()) {
      std::cerr << "error: expected `p cnf #vars #clauses` (line " << line << ")\n";
      return {};
    }
    if (problem_type != "cnf") {
      std::cerr << "error: can only handle 'cnf' files\n";
      return {};
    }
    if (nvars >= unsigned(std::numeric_limits<int>::max())) {
      std::cerr << "error: too many variables\n";
      return {};
    }
    if (var_order.size() == 0) { // Allow no variable order to be given
      cnf._var_to_level.reserve(nvars);
      for (unsigned i = 0; i < nvars; i++) { cnf._var_to_level.push_back(i); }
    } else { // Case where a variable order is given via the comment lines
      if (nvars != var_order.size()) {
        std::cerr << "error: number of variables does not match\n";
        return {};
      }

      constexpr unsigned NO_VAR = std::numeric_limits<unsigned>::max();
      cnf._var_to_level.resize(nvars, NO_VAR);
      for (unsigned i = 0; i < nvars; i++) {
        if (cnf._var_to_level[var_order[i]] != NO_VAR) {
          std::cerr << "error: variable " << cnf._var_to_level[var_order[i]]
                    << "occurs twice in order\n";
          return {};
        }
        cnf._var_to_level[var_order[i]] = i;
      }

      // Holds by the pigeonhole principle
      assert(std::all_of(cnf._var_to_level.begin(), cnf._var_to_level.end(), [](unsigned level) {
        return level != NO_VAR;
      }));
    }

    // Read the clauses
    size_t clause_offset = 0;
    cnf._clause_data.reserve(nclauses * 4);
    cnf._clause_offsets.reserve(nclauses);
    while (input.good()) {
      int literal;
      input >> literal;
      if (input.fail()) {
        if (input.eof())
          break; // this branch is taken if there is whitespace at the end of the file
        std::cerr << "error: expected an integer\n";
        return {};
      }
      if (literal == 0) {
        cnf._clause_offsets.push_back(clause_offset);
        clause_offset = cnf._clause_data.size();
      } else if (unsigned_abs(literal) > nvars) {
        std::cerr << "error: found literal " << literal << " but there are only " << nvars
                  << "variables\n";
        return {};
      } else {
        assert(literal != 0);
        cnf._clause_data.push_back(literal);
      }
    }
    // The last 0 could be omitted in the input. It could also be that the last clause is empty; in
    // that case `cnf._clause_data.size() > clause_offset` would be false.
    if (cnf._clause_data.size() > clause_offset || cnf._clause_offsets.size() == nclauses - 1) {
      cnf._clause_offsets.push_back(clause_offset);
    }

    if (cnf._clause_offsets.size() != nclauses) {
      std::cerr << "error: number of clauses does not match (" << nclauses
                << " in header, actual: " << cnf._clause_offsets.size() << ")\n";
      return {};
    }

    if (input.bad()) {
      std::cerr << "error: reading from the input file failed\n";
      return {};
    }

    return cnf;
  }

  /// Returns `true` iff there is an empty clause
  bool
  has_empty_clause() const
  {
    size_t last_offset = std::numeric_limits<size_t>::max();
    for (size_t offset : _clause_offsets) {
      if (offset == last_offset) return true;
      last_offset = offset;
    }
    return false;
  }

  /// Get the number of clauses
  unsigned
  num_clauses() const
  {
    return _clause_offsets.size();
  }

  /// Get the variable to level mapping
  const std::vector<unsigned>&
  var_to_level() const
  {
    return _var_to_level;
  }

  /// Call `f(begin, end)` for each clause, where `begin` and `end` are random access iterators over
  /// the literals of the clause. Each literal `l` is an `int` unequal to 0, where `l` refers to
  /// variable `|l| - 1`.
  template <typename F>
  void
  foreach_clause(F f) const
  {
    unsigned n = _clause_offsets.size();
    for (unsigned i = 0; i < n; ++i) {
      f(_clause_data.begin() + _clause_offsets[i],
        i == n - 1 ? _clause_data.end() : _clause_data.begin() + _clause_offsets[i + 1]);
    }
  }
};

// ========================================================================== //

/// Construct the clauses of `cnf`
///
/// This will filter out clauses equivalent to `⊤`, so the resulting vector
/// might be shorter than `cnf.num_clauses()`.
///
/// Precondition: there is no empty clause
template <typename Adapter>
std::vector<typename Adapter::dd_t>
construct_clauses(Adapter& adapter, const CNF& cnf)
{
  std::vector<typename Adapter::dd_t> clauses;
  clauses.reserve(cnf.num_clauses());
  const std::vector<unsigned>& var_to_level = cnf.var_to_level();

  // polarity of variables in the current clause
  std::vector<signed char> polarities(var_to_level.size());

  // We directly construct the clauses using the builder (and do not use the
  // disjunction operator) for better performance on time-forward processing
  // implementations

  cnf.foreach_clause([&adapter, &var_to_level, &polarities, &clauses](auto begin, auto end) {
    // minimum and maximum variable defined in the clause
    unsigned min_level = var_to_level.size(), max_level = 0;

    // Fill the `polarities` table
    for (; begin != end; ++begin) {
      const int literal          = *begin;
      const unsigned level       = var_to_level[unsigned_abs(literal) - 1];
      const signed char polarity = literal < 0 ? -1 : 1, prev = polarities[level];
      if (prev != 0 && prev != polarity) {
        // x ∨ ¬x ≡ ⊤, just clean the polarities and continue with the next clause
        std::fill(polarities.begin() + min_level, polarities.begin() + max_level + 1, 0);
        return;
      } else {
        polarities[level] = polarity;
        if (level > max_level) max_level = level;
        if (level < min_level) min_level = level;
      }
    }

    assert(min_level <= max_level); // holds by the function's precondition

    // ==============================
    // Construct the clause bottom-up
    unsigned level = var_to_level.size() - 1;

    // nodes below `max_level`
    typename Adapter::build_node_t tautology = adapter.build_node(true);
    if (Adapter::needs_extend) {
      // This is needed for ZDDs, and would not be incorrect for BDDs, still we
      // skip the unnecessary computations.
      for (; level != max_level; --level) {
        tautology = adapter.build_node(level, tautology, tautology);
      }
      assert(level == max_level);
    } else {
      level = max_level;
    }

    // nodes for `max_level`
    assert(polarities[level] == 1 || polarities[level] == -1);
    typename Adapter::build_node_t clause_build = polarities[level] == 1
      ? adapter.build_node(level, /* lo */ adapter.build_node(false), tautology)
      : adapter.build_node(level, /* lo */ tautology, adapter.build_node(false));
    if (Adapter::needs_extend && level > min_level)
      tautology = adapter.build_node(level, tautology, tautology);
    polarities[level] = 0;

    // nodes above `max_level`
    while (level-- != 0) {
      const signed char pol = polarities[level];
      if (pol == 0) {
        if (Adapter::needs_extend)
          clause_build = adapter.build_node(level, clause_build, clause_build);
      } else {
        polarities[level] = 0;
        clause_build      = pol == 1 ? adapter.build_node(level, /* lo */ clause_build, tautology)
                                     : adapter.build_node(level, /* lo */ tautology, clause_build);
      }
      if (Adapter::needs_extend && level > min_level)
        tautology = adapter.build_node(level, tautology, tautology);
    }

    clauses.push_back(adapter.build());
  });

  return clauses;
}

/// Conjoin the clauses with an (approximately) balanced bracketing
/// `(c0 ∧ c1) ∧ (c2 ∧ (c3 ∧ c4))`
///
/// In the paper [Configuring BDD Compilation Techniques for Feature
/// Models](https://doi.org/10.1145/3646548.3676538), this was shown to be much
/// better than a left-deep clause bracketing `((c1 ∧ c2) ∧ c3) ∧ c4`.
///
/// Importantly, this function does not commute any operands, i.e., we do not
/// conjoin `c0` and `c4` (or any dependant intermediate results) before `c1`
/// has been processed.
template <typename Adapter, typename IT>
typename Adapter::dd_t
conjoin(Adapter& adapter, const IT begin, const IT end)
{
  if (begin == end) return adapter.top();
  auto d = std::distance(begin, end);
  if (d == 1) return *begin;

  const IT mid = begin + d / 2;
  return conjoin(adapter, begin, mid) & conjoin(adapter, mid, end);
}

// ========================================================================== //
template <typename Adapter>
int
run_cnf(int argc, char** argv)
{
  bool should_exit = parse_input<parsing_policy>(argc, argv);
  if (should_exit) { return -1; }

  if (file.empty()) {
    std::cerr << "Input file not specified\n";
    return -1;
  }

  // Read the file
  std::optional<CNF> cnf = CNF::parse_dimacs_cnf(file);
  if (!cnf) {
    return -1; // Parser error has been printed
  }
  if (cnf->has_empty_clause()) {
    std::cerr << "The CNF contains an empty clause and is thus trivially unsatisfiable\n";
    return -1;
  }

  // =========================================================================
  // Initialise BDD manager
  return run<Adapter>("cnf", cnf->var_to_level().size(), [&cnf](Adapter& adapter) {
    uint64_t solutions;

    // ========================================================================
    // Construct a BDD for each clause

    std::cout << json::field("clauses") << json::brace_open << json::endl << json::flush;

    const time_point t1                         = now();
    std::vector<typename Adapter::dd_t> clauses = construct_clauses(adapter, *cnf);
    const time_point t2                         = now();

    const time_duration clause_cons_time = duration_ms(t1, t2);
    std::cout << json::field("amount") << json::value(clauses.size()) << json::comma << json::endl;
    std::cout << json::field("time (ms)") << json::value(clause_cons_time) << json::endl;

    std::cout << json::brace_close << json::comma << json::endl;

    // ========================================================================
    // Compute conjunction
    std::cout << json::field("apply") << json::brace_open << json::endl << json::flush;

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("intermediate results") << json::brace_open << json::endl;
#endif // BDD_BENCHMARK_STATS

    const time_point t3        = now();
    typename Adapter::dd_t res = conjoin(adapter, clauses.cbegin(), clauses.cend());
    const time_point t4        = now();

    const time_duration apply_time = duration_ms(t3, t4);

#ifdef BDD_BENCHMARK_STATS
    std::cout << json::field("total processed (nodes)") << json::value(total_nodes) << json::comma
              << json::endl;
    std::cout << json::field("largest size (nodes)") << json::value(largest_bdd) << json::endl;
    std::cout << json::brace_close << json::comma << json::endl;
#endif // BDD_BENCHMARK_STATS
    std::cout << json::field("final size (nodes)") << json::value(adapter.nodecount(res))
              << json::comma << json::endl;
    std::cout << json::field("time (ms)") << json::value(apply_time) << json::endl;
    std::cout << json::brace_close << json::comma << json::endl << json::flush;

    // ========================================================================
    // Count number of solutions
    time_duration counting_time = 0;
    if (satcount) {
      std::cout << json::field("satcount") << json::brace_open << json::endl << json::flush;

      const time_point t5 = now();
      solutions           = adapter.satcount(res);
      const time_point t6 = now();

      counting_time = duration_ms(t5, t6);

      std::cout << json::field("result") << json::value(solutions) << json::comma << json::endl;
      std::cout << json::field("time (ms)") << json::value(counting_time) << json::endl;
      std::cout << json::brace_close << json::endl << json::flush;
    }

    // ========================================================================
    std::cout << json::field("total time (ms)")
              << json::value(init_time + clause_cons_time + apply_time + counting_time)
              << json::endl
              << json::flush;

    return 0;
  });
}
