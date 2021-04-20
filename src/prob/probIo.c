/**CFile****************************************************************

  FileName    [probIo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [prob : probabilistic design operation.]

  Synopsis    [I/O methods file.]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [May 14, 2016.]

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
extern float randProb();
// main methods
void Pb_GenProbNtk(Abc_Ntk_t*, float, float);
void Pb_WritePBN(Abc_Ntk_t*, char*, int, int);
// helpers
static void Pb_CountErrorGates(Abc_Ntk_t*);
static void Pb_WritePBNPio(FILE*, Abc_Ntk_t*, int);
static void Pb_WritePBNGate(FILE*, Abc_Ntk_t*, int);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [generate prob ntk with given error/defect rates]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Pb_GenProbNtk(Abc_Ntk_t* pNtk, float error, float defect) {
  printf("Pb_GenProbNtk() : error rate = %f , defect rate = %f\n", error,
         defect);
  Abc_Obj_t* pObj;
  int i;

  Abc_NtkForEachObj(pNtk, pObj, i) pObj->dTemp = 0.0;
  Abc_NtkForEachPi(pNtk, pObj, i) pObj->dTemp = 0.5;

  Abc_NtkForEachNode(pNtk, pObj, i) {
    if (randProb() <= defect) {
      printf("\t> gate %s is assigned to be erroneous!\n", Abc_ObjName(pObj));
      pObj->dTemp = error;
    } else
      pObj->dTemp = 0.0;
  }
  Pb_CountErrorGates(pNtk);
}

void Pb_CountErrorGates(Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  float ratio;
  int numErr, i;

  numErr = 0;
  Abc_NtkForEachNode(pNtk, pObj, i) if (pObj->dTemp != 0.0)++ numErr;

  ratio = (float)numErr / Abc_NtkNodeNum(pNtk);
  printf("\t> #error gates = %d / #total gates = %d (%f)\n", numErr,
         Abc_NtkNodeNum(pNtk), ratio);
}

/**Function*************************************************************

  Synopsis    [write out PBN files]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Pb_WritePBN(Abc_Ntk_t* pNtk, char* name, int numPIs, int fStandard) {
  FILE* out;

  out = fopen(name, "w");
  if (!out) {
    Abc_Print(-1, "Cannot open file %s\n", name);
    return;
  }
  fprintf(out, "# PBN %s written by probABC\n", Abc_NtkName(pNtk));
  fprintf(out, ".model %s\n", Abc_NtkName(pNtk));

  fprintf(out, "# .numpi %d\n", numPIs);
  Pb_WritePBNPio(out, pNtk, fStandard);
  Pb_WritePBNGate(out, pNtk, fStandard);

  fprintf(out, ".end\n");
  fclose(out);
  Abc_Print(-2, "File %s is written.\n", name);
}

void Pb_WritePBNPio(FILE* out, Abc_Ntk_t* pNtk, int fStandard) {
  Abc_Obj_t* pObj;
  int i;

  if (fStandard) {
    Abc_NtkForEachPi(pNtk, pObj, i)
        fprintf(out, "# .prob %s %f\n", Abc_ObjName(pObj), pObj->dTemp);
    Abc_NtkForEachPi(pNtk, pObj, i)
        fprintf(out, ".inputs %s\n", Abc_ObjName(pObj));
  } else {
    Abc_NtkForEachPi(pNtk, pObj, i)
        fprintf(out, ".inputs %s %f\n", Abc_ObjName(pObj), pObj->dTemp);
  }
  Abc_NtkForEachPo(pNtk, pObj, i)
      fprintf(out, ".outputs %s\n", Abc_ObjName(pObj));
}

void Pb_WritePBNGate(FILE* out, Abc_Ntk_t* pNtk, int fStandard) {
  Abc_Obj_t* pObj;
  int i;

  Abc_NtkForEachNode(pNtk, pObj, i) {
    if (!Abc_ObjIsPi(Abc_ObjFanin0(pObj)) &&
        !Abc_ObjIsPi(Abc_ObjFanin1(pObj))) {
      fprintf(out, ".names %d %d", Abc_ObjId(Abc_ObjFanin0(pObj)),
              Abc_ObjId(Abc_ObjFanin1(pObj)));
    } else if (Abc_ObjIsPi(Abc_ObjFanin0(pObj))) {
      if (Abc_ObjIsPi(Abc_ObjFanin1(pObj))) {
        fprintf(out, ".names %s %s", Abc_ObjName(Abc_ObjFanin0(pObj)),
                Abc_ObjName(Abc_ObjFanin1(pObj)));
      } else {
        fprintf(out, ".names %s %d", Abc_ObjName(Abc_ObjFanin0(pObj)),
                Abc_ObjId(Abc_ObjFanin1(pObj)));
      }
    } else {
      fprintf(out, ".names %d %s", Abc_ObjId(Abc_ObjFanin0(pObj)),
              Abc_ObjName(Abc_ObjFanin1(pObj)));
    }
    if (fStandard) {
      fprintf(out, " %d\n", Abc_ObjId(pObj));
    } else {
      fprintf(out, " %d %f\n", Abc_ObjId(pObj), pObj->dTemp);
    }
    if (!Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) fprintf(out, "11 1\n");
    if (Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) fprintf(out, "01 1\n");
    if (!Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) fprintf(out, "10 1\n");
    if (Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) fprintf(out, "00 1\n");
  }

  Abc_NtkForEachPo(pNtk, pObj, i) {
    fprintf(out, ".names %d %s\n", Abc_ObjId(Abc_ObjFanin0(pObj)),
            Abc_ObjName(pObj));
    if (!Abc_ObjFaninC0(pObj))
      fprintf(out, "1 1\n");
    else
      fprintf(out, "0 1\n");
  }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
