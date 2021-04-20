/**CFile****************************************************************

  FileName    [probSsat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [prob : probabilistic design operation.]

  Synopsis    [methods to generate ssat files.]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [May 15, 2016.]

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
extern void Pb_WriteWMCCla(FILE*, Abc_Ntk_t*, int);
// main methods
void Pb_WriteSSAT(Abc_Ntk_t*, char*, int, int, int);
// helpers
static void Pb_WriteQdimacsPrefix(FILE*, Abc_Ntk_t*, int, int);
static void Pb_WriteSSATprefix(FILE*, Abc_Ntk_t*, int, int);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [write out WMC files]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Pb_WriteSSAT(Abc_Ntk_t* pNtk, char* name, int numPo, int numExist,
                  int fQdimacs) {
  FILE* out;
  int numVar, numCla;

  out = fopen(name, "w");
  if (!out) {
    Abc_Print(-1, "Cannot open file %s\n", name);
    return;
  }
  numVar = Abc_NtkObjNum(pNtk) - 1;  // substract constant node
  numCla = 3 * Abc_NtkNodeNum(pNtk) + 2 +
           1;  // 2 for Po connection , 1 for Po assertion
  if (fQdimacs) {
    fprintf(out, "c SSAT file for PBN %s written by probABC\n",
            Abc_NtkName(pNtk));
    fprintf(out, "p cnf %d %d\n", numVar, numCla);
    Pb_WriteQdimacsPrefix(out, pNtk, numPo, numExist);
  } else {
    fprintf(out, "%d\n", numVar);
    fprintf(out, "%d\n", numCla);
    Pb_WriteSSATprefix(out, pNtk, numPo, numExist);
  }
  Pb_WriteWMCCla(out, pNtk, numPo);
  fclose(out);
  Abc_Print(-2, "File %s is written.\n", name);
}

void Pb_WriteQdimacsPrefix(FILE* out, Abc_Ntk_t* pNtk, int numPo,
                           int numExist) {
  Abc_Obj_t* pObj;
  int i;

  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (i < numExist)
      fprintf(out, "e %d 0\n", Abc_ObjId(pObj));
    else
      fprintf(out, "r %f %d 0\n", pObj->dTemp, Abc_ObjId(pObj));
  }
  Abc_NtkForEachNode(pNtk, pObj, i) fprintf(out, "e %d 0\n", Abc_ObjId(pObj));
  Abc_NtkForEachPo(pNtk, pObj, i) fprintf(out, "e %d 0\n", Abc_ObjId(pObj));
}

void Pb_WriteSSATprefix(FILE* out, Abc_Ntk_t* pNtk, int numPo, int numExist) {
  Abc_Obj_t* pObj;
  int i;

  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (i < numExist)
      fprintf(out, "%d x%d E\n", Abc_ObjId(pObj), Abc_ObjId(pObj));
    else
      fprintf(out, "%d x%d R %f\n", Abc_ObjId(pObj), Abc_ObjId(pObj),
              pObj->dTemp);
  }
  Abc_NtkForEachNode(pNtk, pObj, i)
      fprintf(out, "%d x%d E\n", Abc_ObjId(pObj), Abc_ObjId(pObj));
  Abc_NtkForEachPo(pNtk, pObj, i)
      fprintf(out, "%d x%d E\n", Abc_ObjId(pObj), Abc_ObjId(pObj));
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
