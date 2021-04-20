/**CFile****************************************************************

  FileName    [probWmc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [prob : probabilistic design operation.]

  Synopsis    [methods to generate wmc files for cachet.]

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
// main methods
void Pb_WriteWMC(Abc_Ntk_t*, char*, int, int);
// helpers
void Pb_WriteWMCCla(FILE*, Abc_Ntk_t*, int);
static void Pb_WriteWMCWeight(FILE*, Abc_Ntk_t*, int, int);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [write out WMC files]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Pb_WriteWMC(Abc_Ntk_t* pNtk, char* name, int numPo, int fModel) {
  FILE* out;
  int numVar, numCla;

  out = fopen(name, "w");
  if (!out) {
    Abc_Print(-1, "Cannot open file %s\n", name);
    return;
  }
  numVar = Abc_NtkObjNumMax(pNtk);
  numCla = 3 * Abc_NtkNodeNum(pNtk) + 2 +
           1;  // 2 for Po connection , 1 for Po assertion
  fprintf(out, "c WMC file for PBN %s written by probABC\n", Abc_NtkName(pNtk));
  fprintf(out, "p cnf %d %d\n", numVar, numCla);

  Pb_WriteWMCCla(out, pNtk, numPo);
  Pb_WriteWMCWeight(out, pNtk, numPo, fModel);
  fclose(out);
  Abc_Print(-2, "File %s is written.\n", name);
}

void Pb_WriteWMCCla(FILE* out, Abc_Ntk_t* pNtk, int numPo) {
  Abc_Obj_t *pObj, *pFanin0, *pFanin1;
  int i;

  Abc_NtkForEachNode(pNtk, pObj, i) {
    pFanin0 = Abc_ObjFanin0(pObj);
    pFanin1 = Abc_ObjFanin1(pObj);
    if (!Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (!Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
  }

  pObj = Abc_NtkPo(pNtk, numPo);
  pFanin0 = Abc_ObjFanin0(pObj);
  if (!Abc_ObjFaninC0(pObj)) {
    fprintf(out, "%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  } else {
    fprintf(out, "%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  }
  fprintf(out, "%d 0\n", Abc_ObjId(pObj));
}

void Pb_WriteWMCWeight(FILE* out, Abc_Ntk_t* pNtk, int numPo, int fModel) {
  Abc_Obj_t* pObj;
  int i;

  if (fModel) {
    if (!pNtk->pModel)
      printf("  > [Error] Model does not exist! (output is invalid)\n");
    else {
      Abc_NtkForEachPi(pNtk, pObj, i) {
        if (pNtk->pModel[i] == 1)
          fprintf(out, "%d 0\n", Abc_ObjId(pObj));
        else if (pNtk->pModel[i] == 0)
          fprintf(out, "-%d 0\n", Abc_ObjId(pObj));
        else
          ;
      }
      Abc_NtkForEachPi(pNtk, pObj, i) {
        if (pNtk->pModel[i] == -1)
          fprintf(out, "w %d %f\n", Abc_ObjId(pObj), pObj->dTemp);
        else
          fprintf(out, "w %d -1\n", Abc_ObjId(pObj));
      }
    }
  } else {
    Abc_NtkForEachPi(pNtk, pObj, i)
        fprintf(out, "w %d %f\n", Abc_ObjId(pObj), pObj->dTemp);
  }
  Abc_NtkForEachNode(pNtk, pObj, i)
      fprintf(out, "w %d %d\n", Abc_ObjId(pObj), -1);
  Abc_NtkForEachPo(pNtk, pObj, i)
      fprintf(out, "w %d %d\n", Abc_ObjId(pObj), -1);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
