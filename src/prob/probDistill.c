/**CFile****************************************************************

  FileName    [probDistill.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [prob : probabilistic design operation.]

  Synopsis    [Distillation (standardized PBN) methods file.]

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
// main methods
Abc_Ntk_t* Pb_DistillNtk(Abc_Ntk_t*);
void Pb_PrintDistNtk(Abc_Ntk_t*);
// helpers
static void Pb_DistillCreatePio(Abc_Ntk_t*, Abc_Ntk_t*);
static void Pb_DistillCreateGate(Abc_Ntk_t*, Abc_Ntk_t*);
static void Pb_DistillFinalize(Abc_Ntk_t*, Abc_Ntk_t*);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Distillate a probabilistic network]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

Abc_Ntk_t* Pb_DistillNtk(Abc_Ntk_t* pNtk) {
  char Buffer[1000];
  Abc_Ntk_t* pNtkDist;

  pNtkDist = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
  sprintf(Buffer, "%s_distill", pNtk->pName);
  pNtkDist->pName = Extra_UtilStrsav(Buffer);

  Pb_DistillCreatePio(pNtkDist, pNtk);
  Pb_DistillCreateGate(pNtkDist, pNtk);
  Pb_DistillFinalize(pNtkDist, pNtk);
  pNtkDist->pModel = NULL;
  // Pb_PrintDistNtk( pNtkDist );

  // Check disable since it will clean pCopy field! --> reset prob QQ
  /*if ( !Abc_NtkCheck( pNtkDist ) ) {
     printf( "Pb_DistillNtk() : The network check has failed.\n" );
          Abc_NtkDelete( pNtkDist );
          return NULL;
  }*/
  return pNtkDist;
}

void Pb_DistillCreatePio(Abc_Ntk_t* pNtkDist, Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;

  Abc_AigConst1(pNtk)->pData = Abc_AigConst1(pNtkDist);

  Abc_NtkForEachPi(pNtk, pObj, i) {
    pObj->pData = Abc_NtkCreatePi(pNtkDist);
    Abc_ObjAssignName(pObj->pData, Abc_ObjName(pObj), "_d");
    ((Abc_Obj_t*)(pObj->pData))->dTemp = pObj->dTemp;
  }

  Abc_NtkForEachPo(pNtk, pObj, i) {
    pObj->pData = Abc_NtkCreatePo(pNtkDist);
    Abc_ObjAssignName(pObj->pData, Abc_ObjName(pObj), "_d");
  }
}

void Pb_DistillCreateGate(Abc_Ntk_t* pNtkDist, Abc_Ntk_t* pNtk) {
  Abc_Obj_t *pObj, *pObjAi;
  int i;

  Abc_AigForEachAnd(pNtk, pObj, i) {
    pObj->pData = Abc_AigAnd(pNtkDist->pManFunc, Abc_ObjChild0Data(pObj),
                             Abc_ObjChild1Data(pObj));
    if (pObj->dTemp != 0.0) {
      // distillation process
      pObjAi = Abc_NtkCreatePi(pNtkDist);
      Abc_ObjAssignName(pObjAi, Abc_ObjName(pObj), "_ai");
      pObjAi->dTemp = pObj->dTemp;
      pObj->pData = Abc_AigXor(pNtkDist->pManFunc, pObj->pData, pObjAi);
    }
  }
}

void Pb_DistillFinalize(Abc_Ntk_t* pNtkDist, Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;

  Abc_NtkForEachPo(pNtk, pObj, i)
      Abc_ObjAddFanin(pObj->pData, Abc_ObjChild0Data(pObj));
}

/**Function*************************************************************

  Synopsis    [print distillated networks]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Pb_PrintDistNtk(Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;

  printf("Ntk name = %s , input list:\n", pNtk->pName);
  Abc_NtkForEachPi(pNtk, pObj, i)
      printf("Obj Id = %d , name = %s , prob = %f\n", Abc_ObjId(pObj),
             Abc_ObjName(pObj), pObj->dTemp);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
