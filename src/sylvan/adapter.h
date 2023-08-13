#include <sylvan.h>
#include <sylvan_table.h>

#include <sylvan_obj.hpp>

////////////////////////////////////////////////////////////////////////////////
/// Initialisation of Sylvan.
///
/// From 'sylvan_commons.h' we know that every node takes up 24 bytes of memory
/// and every operation cache entry takes up 36 bytes.
///
/// Lace initialisation
/// - lace_start:             Initializes LACE given the number of threads and
///                           the size of the task queue.
///
/// Sylvan initialisation:
///   Nodes table size: 24 bytes * nodes
///   Cache table size: 36 bytes * cache entries
///
/// - sylvan_set_limit:       Set the memory limit, the (exponent of the) ratio
///                           between node table and cache, and lastly make the
///                           table sizes be as big as possible.
///
/// - sylvan_set_granularity: 1 for "use cache for every operation".
////////////////////////////////////////////////////////////////////////////////
size_t log2(size_t n)
{
  size_t exp = 1u;
  size_t val = 2u; // 2^1
  while (val < n) {
    val <<= 1u;
    exp++;
  }
  return exp;
}


class sylvan_bdd_adapter
{
public:
  inline static const std::string NAME = "Sylvan";
  typedef sylvan::Bdd dd_t;
  typedef sylvan::Bdd build_node_t;

private:
  int varcount;
  sylvan::Bdd _latest_build;

  // Init and Deinit
public:
  sylvan_bdd_adapter(int varcount) : varcount(varcount)
  {
    // Init LACE
    lace_start(1, 1000000);

    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;

    // Init Sylvan
    sylvan::sylvan_set_limits(memory_bytes,      // Set memory limit
                              log2(CACHE_RATIO), // Set (exponent) of cache ratio
                              0);                // Initialise unique node table to full size
    sylvan::sylvan_set_granularity(1);
    sylvan::sylvan_init_package();
    sylvan::sylvan_init_bdd();

    _latest_build = bot();
  }

  ~sylvan_bdd_adapter()
  {
    sylvan::sylvan_quit();
    lace_stop();
  }

private:
  template<typename IT>
  inline sylvan::Bdd make_cube(IT rbegin, IT rend)
  {
    sylvan::Bdd res = top();
    while (rbegin != rend) {
      res = sylvan::Bdd::bddVar(*(rbegin++)).Ite(res, bot());
    }
    return res;
  }

  // BDD Operations
public:
  inline sylvan::Bdd top()
  { return sylvan::Bdd::bddOne(); }

  inline sylvan::Bdd bot()
  { return sylvan::Bdd::bddZero(); }

  inline sylvan::Bdd ithvar(int label)
  { return sylvan::Bdd::bddVar(label); }

  inline sylvan::Bdd nithvar(int label)
  { return ~sylvan::Bdd::bddVar(label); }

  inline sylvan::Bdd negate(const sylvan::Bdd &b)
  { return ~b; }

  inline sylvan::Bdd ite(const sylvan::Bdd &i,
                         const sylvan::Bdd &t,
                         const sylvan::Bdd &e)
  { return i.Ite(t,e); }

  inline sylvan::Bdd exists(const sylvan::Bdd &b, int label)
  { return b.ExistAbstract(sylvan::Bdd::bddVar(label)); }

  template<typename IT>
  inline sylvan::Bdd exists(const sylvan::Bdd &b, IT rbegin, IT rend)
  { return b.ExistAbstract(make_cube(rbegin, rend)); }

  inline sylvan::Bdd forall(const sylvan::Bdd &b, int label)
  { return b.UnivAbstract(sylvan::Bdd::bddVar(label)); }

  template<typename IT>
  inline sylvan::Bdd forall(const sylvan::Bdd &b, IT rbegin, IT rend)
  { return b.UnivAbstract(make_cube(rbegin, rend)); }

  inline uint64_t nodecount(const sylvan::Bdd &b)
  { return b.NodeCount() - 1; }

  inline uint64_t satcount(const sylvan::Bdd &b)
  { return b.SatCount(varcount); }

  inline std::vector<std::pair<int, char>>
  pickcube(const sylvan::Bdd &b)
  {
    std::vector<std::pair<int, char>> res;

    sylvan::Bdd sat = b.PickOneCube();
    while (!sat.isOne() && !sat.isZero()) {
      const int var = sat.TopVar();
      const sylvan::Bdd sat_low = sat.Else();
      const sylvan::Bdd sat_high = sat.Then();

      const bool go_high = !sat_high.isZero();
      if (sat_low == sat_high) {
        res.push_back({ var, '2' });
      } else if (go_high) {
        res.push_back({ var, '1' });
      } else { // !go_high
        res.push_back({ var, '0' });
      }
      sat = go_high ? sat_high : sat_low;
    }
    return res;
  }

  // BDD Build operations
public:
  inline sylvan::Bdd build_node(const bool value)
  {
    const sylvan::Bdd res = value ? top() : bot();
    if (_latest_build == bot()) { _latest_build = res; }
    return res;
  }

  inline sylvan::Bdd build_node(const int label,
                                const sylvan::Bdd &low,
                                const sylvan::Bdd &high)
  {
    _latest_build = sylvan::Bdd::bddVar(label).Ite(high, low);
    return _latest_build;
  }

  inline sylvan::Bdd build()
  {
    const sylvan::Bdd res = _latest_build;
    _latest_build = bot(); // <-- Reset and free builder reference
    return res;
  }

  // Statistics
public:
  inline size_t allocated_nodes()
  { return 0; }

  void print_stats()
  {
    // Requires the "SYLVAN_STATS" property to be set in CMake
    std::cout << "\n";
    sylvan::sylvan_stats_report(stdout);
  }
};
