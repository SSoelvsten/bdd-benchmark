#include <adiar/adiar.h>

class adiar_zdd_adapter
{
private:
  const int varcount;

public:
  inline static const std::string NAME = "Adiar";

  // Variable type
public:
  typedef adiar::zdd bdd_t;

  // Init and Deinit
public:
  adiar_zdd_adapter(int vc) : varcount(vc)
  {
    const size_t memory_bytes = static_cast<size_t>(M) * 1024u * 1024u;
    adiar::adiar_init(memory_bytes, temp_path);
  }

  ~adiar_zdd_adapter()
  {
    adiar::adiar_deinit();
  }

  // ZDD Operations
public:
  inline uint64_t nodecount(const adiar::zdd &z)
  { return adiar::zdd_nodecount(z); }

  inline uint64_t satcount(const adiar::zdd &z)
  { return adiar::zdd_size(z); }

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
