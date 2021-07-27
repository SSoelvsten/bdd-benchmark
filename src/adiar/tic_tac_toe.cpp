#include "../tic_tac_toe.cpp"

#include "package_mgr.h"

// ========================================================================== //
//                            SQUARE CONSTRUCTION                             //
template<>
adiar::bdd construct_init(adiar_mgr &/*mgr*/)
{
  return adiar::bdd_counter(0, 63, N);
}

template<>
adiar::bdd construct_is_not_winning(adiar_mgr &/* mgr */, std::array<uint64_t, 4>& line)
{
  size_t idx = 4 - 1;

  adiar::ptr_t no_Xs_false = adiar::create_sink_ptr(false);
  adiar::ptr_t no_Xs_true = adiar::create_sink_ptr(true);

  adiar::ptr_t some_Xs_true = adiar::create_sink_ptr(false);

  adiar::node_file out;
  adiar::node_writer out_writer(out);

  do {
    adiar::node_t some_Xs = adiar::create_node(line[idx], 1,
                                               adiar::create_sink_ptr(true),
                                               some_Xs_true);

    if (idx != 0) {
      out_writer << some_Xs;
    }

    adiar::node_t no_Xs = adiar::create_node(line[idx], 0,
                                             no_Xs_false,
                                             no_Xs_true);

    out_writer << no_Xs;

    no_Xs_false = no_Xs.uid;
    if (idx == 1) { // The next is the root?
      no_Xs_true = some_Xs.uid;
    }

    some_Xs_true = some_Xs.uid;
  } while (idx-- > 0);

  return out;
}

// ========================================================================== //
int main(int argc, char** argv)
{
  run_tic_tac_toe<adiar_mgr>(argc, argv);
}
