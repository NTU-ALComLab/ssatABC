/**CFile****************************************************************

  FileName    [ssatModelCount.cc]

  SystemName  [ssatQesto]

  Synopsis    [All-SAT enumeration model counting]

  Author      [Yen-Shi Wang]

  Affiliation [NTU]

  Date        [8, Feb., 2017]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include <iostream>

#include "ssat/core/Dimacs.h"
#include "ssat/core/Solver.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/core/SsatSolver.h"
#include "ssat/utils/ParseUtils.h"

using namespace Minisat;
using namespace std;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//#define DEBUG

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [All-sat model counting entrance point.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

double SsatSolver::allSatModelCount(Solver* s, const vec<Lit>& assump,
                                    double curVal) {
#ifdef DEBUG
  printf("  > current exist assignment:\n");
  dumpCla(assump);
  fflush(stdout);
#endif
  vec<Lit> rLits(_rootVars[1].size()), rHits, eLits;
  eLits.capacity(assump.size() + _control.size() + 1);
  assump.copyTo(eLits);
  _control.push(s->newVar());
  for (int i = 0; i < _control.size(); ++i) eLits.push(mkLit(_control[i]));
  eLits.last() = ~eLits.last();
  double count = 0.0, prob;

  while (s->solve(eLits)) {
#ifdef DEBUG
    for (int i = 0; i < _rootVars[1].size(); ++i)
      rLits[i] = (s->modelValue(_rootVars[1][i]) == l_True)
                     ? mkLit(_rootVars[1][i])
                     : ~mkLit(_rootVars[1][i]);
    printf("  > current random assignment:\n");
    dumpCla(rLits);
    fflush(stdout);
#endif
    rHits.clear();
    miniHitSet(rHits, 1);  // rand vars at Lv.1
#ifdef DEBUG
    printf("  > hitting set:\n");
    dumpCla(rHits);
    fflush(stdout);
#endif
    prob = 1;
    for (int i = 0; i < rHits.size(); ++i)
      prob *= sign(rHits[i]) ? _quan[var(rHits[i])] : 1 - _quan[var(rHits[i])];
    count += prob;
    if (count >= 1 - curVal) {
#ifdef DEBUG
      printf("  > upper bound less than current value, stop!\n");
#endif
      break;
    } else {
      rHits.push(mkLit(_control.last()));
      s->addClause(rHits);
    }
  }
#ifdef DEBUG
  printf("  > current value: %f\n", 1 - count);
  fflush(stdout);
#endif
  return 1 - count;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
