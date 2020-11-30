#include <stdint.h>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
uint64_t label_of_Pij(uint64_t i, uint64_t j, int N)
{
  assert(0 <= i && i <= N+1);
  assert(0 <= j && j <= N);
  return (N+1)*i + j;
}

////////////////////////////////////////////////////////////////////////////////
/// Constructs the CNF for the Pigeonhole Principle based on the paper by Olga
/// Tveretina, Carsten Sinz, and Hans Zantema "Ordered Binary Decision Diagrams,
/// Pigeonhole Formulas and Beyond".
////////////////////////////////////////////////////////////////////////////////
template<typename bdd_policy>
void construct_PHP_cnf(sat_solver<bdd_policy> &solver, int N)
{
  // PC_n
  for (int i = 1; i <= N+1; i++)
  {
    clause_t clause;
    for (int j = 1; j <= N; j++)
    {
      clause.push_back(literal_t (label_of_Pij(i,j,N), false));
    }
    solver.add_clause(clause);
  }

  // NC_n
  for (int i = 1; i < N+1; i++)
  {
    for (int j = i+1; j <= N+1; j++)
    {
      for (int k = 1; k <= N; k++)
      {
        clause_t clause;
        clause.push_back(literal_t (label_of_Pij(i,k,N), true));
        clause.push_back(literal_t (label_of_Pij(j,k,N), true));
        solver.add_clause(clause);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// TODO: Extended Pigeonhole Principle from the same paper...
