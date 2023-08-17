#include <cstddef>
#include <cstdio>

#include "cudd.h"
#include "cuddObj.hh"

unsigned int
cudd_cachesize(int varcount)
{
  const size_t number_of_buckets = CUDD_UNIQUE_SLOTS * varcount;

  constexpr size_t sizeof_DdSubtable = 8 + 9 * 4 + 8;
  const size_t buckets_bytes = number_of_buckets * sizeof_DdSubtable;

  const size_t bytes_remaining = static_cast<size_t>(M)*1024u*1024u - buckets_bytes;

  // CUDD may increase the number of buckets, thereby decreasing the space left
  // for nodes. This will skew the ratio in favour of the cache, but this is the
  // best we can do.
  constexpr size_t sizeof_DdNode = 2*4 + 3*8;
  constexpr size_t sizeof_DdCache = 4 * 8;

  // We need to maximise x and y in the following system of inequalities:
  //              24x + 32y <= M , x = y * CACHE_RATIO
  const size_t x = bytes_remaining / ((sizeof_DdNode * CACHE_RATIO + sizeof_DdCache) / CACHE_RATIO);
  const size_t y = x / CACHE_RATIO;

  return y;
}

unsigned long
cudd_memorysize()
{
  constexpr size_t CUDD_MAX = std::numeric_limits<unsigned long>::max();
  return std::min(static_cast<size_t>(M), CUDD_MAX / (1024 * 1024)) * 1024 * 1024;
}

class cudd_adapter
{
protected:
  Cudd _mgr;
  const int _varcount;

protected:
  cudd_adapter(const int bdd_varcount, const int zdd_varcount)
    : _mgr(bdd_varcount, zdd_varcount,
           CUDD_UNIQUE_SLOTS,
           cudd_cachesize(bdd_varcount + zdd_varcount),
           cudd_memorysize()),
      _varcount(bdd_varcount + zdd_varcount)
  { }

  ~cudd_adapter()
  { /* Do nothing */ }


  // Statistics
public:
  inline size_t allocated_nodes()
  { return _mgr.ReadKeys(); }

  void print_stats()
  {
    std::cout << "\n"
              << "CUDD Statistics:\n"

              << "   Table:\n"
              << "   | peak node count:     " << _mgr.ReadPeakNodeCount() << "\n"
              << "   | node count (bdd):    " << _mgr.ReadNodeCount() << "\n"
              << "   | node count (zdd):    " << _mgr.zddReadNodeCount() << "\n"
              << "   | keys:                " << _mgr.ReadKeys() << "\n"
              << "   | dead:                " <<  _mgr.ReadDead() << "\n"

    // Commented lines are only available if 'DD_STATS' flag is set in CUDD compilation

    // INFO(" | Cache:\n");
    // INFO(" | | slots:               %zu\n", _mgr.ReadCacheUsedSlots());
    // INFO(" | | lookups:             %zu\n", _mgr.ReadCacheLookUps());
    // INFO(" | | hits:                %zu\n", _mgr.ReadCacheHits());

              << "   Garbage Collections:\n"
              << "   | runs:                " << _mgr.ReadGarbageCollections() << "\n"
              << "   | time (ms):           " << _mgr.ReadGarbageCollectionTime() << "\n";
  }
};

class cudd_bdd_adapter : public cudd_adapter
{
public:
  inline static const std::string NAME = "CUDD [BDD]";
  typedef BDD dd_t;
  typedef BDD build_node_t;

private:
  BDD _latest_build;

  // Init and Deinit
public:
  cudd_bdd_adapter(int varcount) : cudd_adapter(varcount, 0)
  { // Disable dynamic ordering
    _mgr.AutodynDisable();

    _latest_build = bot();
  }

private:
  template<typename IT>
  inline BDD make_cube(IT rbegin, IT rend)
  {
    BDD res = top();
    while (rbegin != rend) {
      res = _mgr.bddVar(*(rbegin++)).Ite(res, bot());
    }
    return res;
  }

  inline BDD make_cube(const std::function<bool(int)> &pred)
  {
    BDD res = top();
    for (int i = _varcount-1; 0 <= i; --i) {
      if (pred(i)) {
        res = _mgr.bddVar(i).Ite(res, bot());
      }
    }
    return res;
  }

  // BDD Operations
public:
  inline BDD top()
  { return _mgr.bddOne(); }

  inline BDD bot()
  { return _mgr.bddZero(); }

  inline BDD ithvar(int label)
  { return _mgr.bddVar(label); }

  inline BDD nithvar(int label)
  { return ~_mgr.bddVar(label); }

  inline BDD negate(const BDD &b)
  { return ~b; }

  inline BDD ite(const BDD &i, const BDD &t, const BDD &e)
  { return i.Ite(t,e); }

  inline BDD exists(const BDD &b, int label)
  { return b.ExistAbstract(_mgr.bddVar(label)); }

  inline BDD exists(const BDD &b, const std::function<bool(int)> &pred)
  { return b.ExistAbstract(make_cube(pred)); }

  template<typename IT>
  inline BDD exists(const BDD &b, IT rbegin, IT rend)
  { return b.ExistAbstract(make_cube(rbegin, rend)); }

  inline BDD forall(const BDD &b, int label)
  { return b.UnivAbstract(_mgr.bddVar(label)); }

  inline BDD forall(const BDD &b, const std::function<bool(int)> &pred)
  { return b.UnivAbstract(make_cube(pred)); }

  template<typename IT>
  inline BDD forall(const BDD &b, IT rbegin, IT rend)
  { return b.UnivAbstract(make_cube(rbegin, rend)); }

  inline uint64_t nodecount(const BDD &b)
  { return b.nodeCount(); }

  inline uint64_t satcount(const BDD &b)
  { return b.CountMinterm(_varcount); }

  inline std::vector<std::pair<int, char>>
  pickcube(const BDD &b)
  {
    std::string cudd_res(_varcount, '_');
    b.PickOneCube(cudd_res.data());

    std::vector<std::pair<int, char>> res;
    for (int x = 0; x < _varcount; ++x) {
      const char cudd_val = cudd_res.at(x);
      if (x == '_') { continue; }

      res.push_back({ x, '0'+cudd_val });
    }

    return res;
  }

  // BDD Build operations
public:
  inline BDD build_node(const bool value)
  {
    const BDD res = value ? top() : bot();
    if (_latest_build == bot()) { _latest_build = res; }
    return res;
  }

  inline BDD build_node(const int label, const BDD &low, const BDD &high)
  {
    _latest_build = _mgr.makeBddNode(label, high, low);
    return _latest_build;
  }

  inline BDD build()
  {
    const BDD res = _latest_build;
    _latest_build = bot(); // <-- Reset and free builder reference
    return res;
  }
};

class cudd_zdd_adapter : public cudd_adapter
{
public:
  inline static const std::string NAME = "CUDD [ZDD]";
  typedef ZDD dd_t;
  typedef ZDD build_node_t;

private:
  ZDD _leaf0;
  ZDD _leaf1;

  ZDD _latest_build;

  // Init and Deinit
public:
  cudd_zdd_adapter(int varcount) : cudd_adapter(0, varcount)
  { // Disable dynamic ordering
    _mgr.AutodynDisableZdd();

    _leaf0 = _mgr.zddZero();
    _leaf1 = _mgr.zddOne(std::numeric_limits<int>::max());
    _latest_build = _mgr.zddZero();
  }

  // ZDD Operations
public:
  inline ZDD top()
  { return _mgr.zddOne(0); }

  inline ZDD bot()
  { return _leaf0; }

  inline ZDD ithvar(const int i)
  { return _mgr.zddVar(i); }

  inline ZDD negate(const ZDD &z)
  { return top() - z; }

  inline uint64_t nodecount(const ZDD &b)
  { return b.nodeCount(); }

  inline uint64_t satcount(const ZDD &b)
  { return b.CountMinterm(_varcount); }

  // ZDD Build operations
public:
  inline ZDD build_node(const bool value)
  {
    const ZDD res = value ? _leaf1 : _leaf0;
    if (_latest_build == _leaf0) { _latest_build = res; }
    return res;
  }

  inline ZDD build_node(const int label, const ZDD &low, const ZDD &high)
  {
    return _latest_build = _mgr.makeZddNode(label, high, low);
  }

  inline ZDD build()
  {
    const ZDD res = _latest_build;
    _latest_build = _leaf0; // <-- Reset and free builder reference
    return res;
  }
};
