#include "tic_tac_toe.cpp"

// ========================================================================== //
//                           EXACTLY N CONSTRAINT                             //
template<typename adapter_t>
typename adapter_t::dd_t construct_init(adapter_t &adapter)
{
  std::vector<typename adapter_t::build_node_t> init_parts(N+1, adapter.build_node(false));
  init_parts.at(N) = adapter.build_node(true);

  for (int curr_level = 63; curr_level >= 0; curr_level--) {
    int min_idx = curr_level > 63 - N ? N - (63 - curr_level + 1) : 0;
    int max_idx = std::min(curr_level, N);

    for (int curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
      const auto low = init_parts.at(curr_idx);
      const auto high = curr_idx < N
        ? init_parts.at(curr_idx + 1)
        : adapter.build_node(false);

      init_parts.at(curr_idx) = adapter.build_node(curr_level, low, high);
    }
  }

  typename adapter_t::dd_t out = adapter.build();
  total_nodes += adapter.nodecount(out);
  return out;
}

// ========================================================================== //
//                              LINE CONSTRAINT                               //
template<typename adapter_t>
typename adapter_t::dd_t construct_is_not_winning(adapter_t &adapter,
                                                  std::array<int, 4>& line)
{
  auto no_Xs = adapter.build_node(false);
  auto only_Xs = adapter.build_node(false);

  for (int idx = 4-1; idx >= 0; idx--) {
    no_Xs = adapter.build_node(line[idx],
                               no_Xs,
                               idx == 0 ? only_Xs : adapter.build_node(true));

    if (idx > 0) {
      only_Xs = adapter.build_node(line[idx],
                                   adapter.build_node(true),
                                   only_Xs);
    }
  }

  typename adapter_t::dd_t out = adapter.build();
  total_nodes += adapter.nodecount(out);
  return out;
}
