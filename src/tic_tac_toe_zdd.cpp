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
    int max_idx = std::min(curr_level, N-1);

    for (int curr_idx = min_idx; curr_idx <= max_idx; curr_idx++) {
      const auto low = init_parts.at(curr_idx);
      const auto high = init_parts.at(curr_idx + 1);

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
  auto root = adapter.build_node(true);

  // Post "don't care" chain
  for (int curr_level = 63; curr_level > line[3]; curr_level--) {
    root = adapter.build_node(curr_level, root, root);
  }

  // Three chains, checking at least one is set to true and one to false
  int line_idx = 4-1;

  auto safe = root;

  auto only_Xs = adapter.build_node(false);
  auto no_Xs = adapter.build_node(false);

  for (int curr_level = line[3]; curr_level > line[0]; curr_level--) {
    if (curr_level == line[line_idx]) {
      no_Xs = adapter.build_node(curr_level, no_Xs, safe);
      only_Xs = adapter.build_node(curr_level, safe, only_Xs);

      line_idx--;
    } else if (curr_level <= line[3]) {
      no_Xs = adapter.build_node(curr_level, no_Xs, no_Xs);
      only_Xs = adapter.build_node(curr_level, only_Xs, only_Xs);
    }

    if (curr_level > line[1]) {
      safe = adapter.build_node(curr_level, safe, safe);
    }
  }

  // Split for both
  root = adapter.build_node(line[0], no_Xs, only_Xs);

  // Pre "don't care" chain
  for (int curr_level = line[0] - 1; curr_level >= 0; curr_level--) {
    root = adapter.build_node(curr_level, root, root);
  }

  typename adapter_t::dd_t out = adapter.build();
  total_nodes += adapter.nodecount(out);
  return out;
}
