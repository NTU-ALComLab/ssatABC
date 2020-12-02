/**CFile****************************************************************

  FileName    [probMiter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [prob : probabilistic design operation.]

  Synopsis    [Prob miter building methods file.]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [May 26, 2016.]

***********************************************************************/

#include <stdlib.h>

#include "aig/saig/saig.h"
#include "base/main/mainInt.h"
#include "proof/abs/abs.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// external methods
// main methods
Abc_Ntk_t* Pb_ProbMiter(Abc_Ntk_t*, Abc_Ntk_t*, int);
// helpers

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Build the probabilistic miter]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

Abc_Ntk_t* Pb_ProbMiter(Abc_Ntk_t* pNtkProb, Abc_Ntk_t* pNtkGolden,
                        int fMulti) {
  Abc_Ntk_t* pNtkMiter;
  Abc_Obj_t* pObj;
  float* prob;
  int fRemove, numPi, numAi, i;

  fRemove = 0;
  prob = ABC_ALLOC(float, Abc_NtkPiNum(pNtkProb));
  Abc_NtkForEachPi(pNtkProb, pObj, i) prob[i] = pObj->dTemp;

  if (!Abc_NtkIsComb(pNtkGolden)) Abc_NtkMakeComb(pNtkGolden, 0);
  if (!Abc_NtkIsStrash(pNtkGolden)) {
    Abc_Print(0, "The golden network is strashed.\n");
    pNtkGolden = Abc_NtkStrash(pNtkGolden, 0, 1, 0);
    fRemove = 1;
  }
  numPi = Abc_NtkPiNum(pNtkGolden);
  numAi = Abc_NtkPiNum(pNtkProb) - Abc_NtkPiNum(pNtkGolden);
  for (i = 0; i < numAi; ++i) {
    pObj = Abc_NtkCreatePi(pNtkGolden);
    Abc_ObjAssignName(pObj, Abc_ObjName(Abc_NtkPi(pNtkProb, numPi + i)),
                      "_pseudo");
  }
  Abc_NtkShortNames(pNtkProb);
  Abc_NtkShortNames(pNtkGolden);
  pNtkMiter = Abc_NtkMiter(pNtkProb, pNtkGolden, 1, 0, 0, fMulti);
  Abc_NtkForEachPi(pNtkMiter, pObj, i) pObj->dTemp = prob[i];
  ABC_FREE(prob);
  if (fRemove) Abc_NtkDelete(pNtkGolden);
  return pNtkMiter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
