#include "../tic_tac_toe_zdd.cpp"

#include "adapter.h"


// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<>
adiar::zdd construct_init(adiar_zdd_adapter &/*mgr*/)
{
  adiar::label_file dom;

  {
    adiar::label_writer w(dom);
    for (int i = 0; i < 64; i++) { w << i; }
  }

  return adiar::zdd_sized_sets<std::equal_to<>>(dom, N, std::equal_to<>());
}

template<>
adiar::zdd construct_is_not_winning(adiar_zdd_adapter &/* mgr */,
                                    std::array<int, 4>& line)
{
  adiar::node_file out;
  adiar::node_writer out_writer(out);

  typename adiar::ptr_t root = adiar::create_sink_ptr(true);

  // Post "don't care" chain
  for (int curr_level = 63; curr_level > line[3]; curr_level--) {
    const adiar::node_t n = adiar::create_node(curr_level, 0, root, root);

    out_writer << n;
    root = n.uid;
  }

  // Three chains, checking at least one is set to true and one to false
  int line_idx = 4-1;

  typename adiar::ptr_t safe = root;

  typename adiar::ptr_t only_Xs = adiar::create_sink_ptr(false);
  typename adiar::ptr_t no_Xs = adiar::create_sink_ptr(false);

  for (int curr_level = line[3]; curr_level > line[0]; curr_level--) {
    if (curr_level == line[line_idx]) {
      const adiar::node_t n_no = adiar::create_node(curr_level, 2, no_Xs, safe);
      no_Xs = n_no.uid;

      out_writer << n_no;

      const adiar::node_t n_only = adiar::create_node(curr_level, 1, safe, only_Xs);
      only_Xs = n_only.uid;

      out_writer << n_only;

      line_idx--;
    } else if (curr_level <= line[3]) {
      const adiar::node_t n_no = adiar::create_node(curr_level, 2, no_Xs, no_Xs);
      no_Xs = n_no.uid;

      out_writer << n_no;

      const adiar::node_t n_only = adiar::create_node(curr_level, 1, only_Xs, only_Xs);
      only_Xs = n_only.uid;

      out_writer << n_only;
    }

    if (curr_level > line[1]) {
      const adiar::node_t n_safe = adiar::create_node(curr_level, 0, safe, safe);
      safe = n_safe.uid;

      out_writer << n_safe;
    }
  }

  // Split for both
  {
    const adiar::node_t n = adiar::create_node(line[0], 0, no_Xs, only_Xs);
    root = n.uid;

    out_writer << n;
  }

  // Pre "don't care" chain
  for (int curr_level = line[0] - 1; curr_level >= 0; curr_level--) {
    const adiar::node_t n = adiar::create_node(curr_level, 0, root, root);
    root = n.uid;

    out_writer << n;
  }

  return out;
}

// ========================================================================== //
int main(int argc, char** argv)
{
  run_tic_tac_toe<adiar_zdd_adapter>(argc, argv);
}

