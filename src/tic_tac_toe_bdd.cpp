#include "tic_tac_toe.cpp"

// ========================================================================== //
//                           EXACTLY N CONSTRAINT                             //
template<typename adapter_t>
typename adapter_t::dd_t construct_init(adapter_t &adapter)
{
  std::vector<typename adapter_t::dd_t> init_parts(N+1, adapter.leaf_false());
  init_parts.at(N) = adapter.leaf_true();

  for (int curr_level = 63; curr_level >= 0; curr_level--) {
    int min_idx = curr_level > 63 - N ? N - (63 - curr_level + 1) : 0;
    int max_idx = std::min(curr_level, N);

    for (int curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
      const typename adapter_t::dd_t low = init_parts.at(curr_idx);
      const typename adapter_t::dd_t high = curr_idx < N
        ? init_parts.at(curr_idx + 1)
        : adapter.leaf_false();

      init_parts.at(curr_idx) = adapter.make_node(curr_level, low, high);
    }
  }

  return init_parts.at(0);
}

// ========================================================================== //
//                              LINE CONSTRAINT                               //
template<typename adapter_t>
typename adapter_t::dd_t construct_is_not_winning(adapter_t &adapter,
                                                  std::array<int, 4>& line)
{
  typename adapter_t::dd_t no_Xs = adapter.leaf_false();
  typename adapter_t::dd_t only_Xs = adapter.leaf_false();

  for (int idx = 4-1; idx >= 0; idx--) {
    no_Xs = adapter.make_node(line[idx],
                              no_Xs,
                              idx == 0 ? only_Xs : adapter.leaf_true());

    if (idx > 0) {
      only_Xs = adapter.make_node(line[idx],
                                  adapter.leaf_true(),
                                  only_Xs);
    }
  }

  return no_Xs;
}
