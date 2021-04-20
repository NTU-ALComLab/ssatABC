/**CFile****************************************************************

  FileName    [probPmc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [prob : probabilistic design operation.]

  Synopsis    [methods to generate pmc files for ApproxMC.]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [April 12, 2021.]

***********************************************************************/

#include <stdlib.h>

#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// external methods
extern void Pb_WriteWMCCla(FILE*, Abc_Ntk_t*, int);
// main methods
void Pb_WritePMC(Abc_Ntk_t*, char*, int);
// helpers
void Pb_CollectIndVar(Abc_Ntk_t*, Vec_Int_t*);
void Pb_WritePMCIndVar(FILE*, Vec_Int_t*);
void Pb_RewritePMC(FILE*, Abc_Ntk_t*, Vec_Int_t*);
void Pb_RewriteOneAndGate(FILE*, int, int, int, int);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [write out PMC files]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Pb_WritePMC(Abc_Ntk_t* pNtk, char* name, int numPo) {
  FILE* out = fopen(name, "w");
  if (!out) {
    Abc_Print(-1, "Cannot open file %s\n", name);
    return;
  }
  int numCktVar, numRewriteVar, numCktCla, numRewriteCla;

  Vec_Int_t* vIndVar = Vec_IntAlloc(Abc_NtkPiNum(pNtk));
  Pb_CollectIndVar(pNtk, vIndVar);
  numCktVar = Abc_NtkObjNumMax(pNtk) - 1;
  // 2 clauses for Po connection, 1 clause for Po assertion
  numCktCla = 3 * Abc_NtkNodeNum(pNtk) + 2 + 1;
  // The number of additional variables = 2*(#independent var - #Pi)
  numRewriteVar = 2 * (Vec_IntSize(vIndVar) - Abc_NtkPiNum(pNtk));
  // The number of additional AND gates = #independent var - #Pi
  numRewriteCla = 3 * (Vec_IntSize(vIndVar) - Abc_NtkPiNum(pNtk));
  fprintf(out, "c PMC file for PBN %s written by probABC\n", Abc_NtkName(pNtk));
  fprintf(out, "p cnf %d %d\n", numCktVar + numRewriteVar,
          numCktCla + numRewriteCla);
  Pb_WritePMCIndVar(out, vIndVar);
  Pb_WriteWMCCla(out, pNtk, numPo);
  Pb_RewritePMC(out, pNtk, vIndVar);
  Vec_IntFree(vIndVar);
  fclose(out);
  Abc_Print(-2, "File %s is written.\n", name);
}

void Pb_CollectIndVar(Abc_Ntk_t* pNtk, Vec_Int_t* vIndVar) {
  Abc_Obj_t* pPi;
  float weight;
  int idIndependetVar = Abc_NtkObjNumMax(pNtk);
  int i;

  Abc_NtkForEachPi(pNtk, pPi, i) {
    weight = pPi->dTemp;
    if (weight == 0.5) {
      Vec_IntPush(vIndVar, Abc_ObjId(pPi));
      continue;
    }
    while (weight != 0.5) {
      if (weight > 0.5) {
        weight = 1 - weight;
      }
      weight *= 2;
      Vec_IntPush(vIndVar, idIndependetVar++);
    }
    Vec_IntPush(vIndVar, idIndependetVar++);
  }
}

void Pb_WritePMCIndVar(FILE* pOut, Vec_Int_t* vIndVar) {
  int Entry, i;
  fprintf(pOut, "c ind");
  Vec_IntForEachEntry(vIndVar, Entry, i) { fprintf(pOut, " %d", Entry); }
  fprintf(pOut, " 0\n");
}

void Pb_RewritePMC(FILE* pOut, Abc_Ntk_t* pNtk, Vec_Int_t* vIndVar) {
  Abc_Obj_t* pPi;
  float weight;
  int idIndependetVar = Abc_NtkObjNumMax(pNtk);
  int idNewAndGateVar = Vec_IntEntryLast(vIndVar) + 1;
  int var, inv, i;

  Abc_NtkForEachPi(pNtk, pPi, i) {
    var = Abc_ObjId(pPi);
    weight = pPi->dTemp;
    while (weight != 0.5) {
      inv = 0;
      if (weight > 0.5) {
        weight = 1 - weight;
        inv = 1;
      }
      if (weight == 0.25) {
        Pb_RewriteOneAndGate(pOut, var, idIndependetVar, idIndependetVar + 1,
                             inv);
        idIndependetVar += 2;
        break;
      } else {
        Pb_RewriteOneAndGate(pOut, var, idIndependetVar, idNewAndGateVar, inv);
        var = idNewAndGateVar;
        weight *= 2;
        ++idIndependetVar;
        ++idNewAndGateVar;
      }
    }
  }
}

void Pb_RewriteOneAndGate(FILE* pOut, int var, int idIndependetVar,
                          int idNewAndGateVar, int inv) {
  fprintf(pOut, "%s%d -%d -%d 0\n", inv ? "-" : "", var, idIndependetVar,
          idNewAndGateVar);
  fprintf(pOut, "%s%d %d 0\n", inv ? "" : "-", var, idIndependetVar);
  fprintf(pOut, "%s%d %d 0\n", inv ? "" : "-", var, idNewAndGateVar);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
