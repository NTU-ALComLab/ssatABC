/**CFile****************************************************************

  FileName    [probBddSp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [prob : probabilistic design operation.]

  Synopsis    [methods to compute signal probability by bdd.]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [May 16, 2016.]

***********************************************************************/

#include <stdlib.h>

#include "aig/saig/saig.h"
#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "bdd/extrab/extraBdd.h"
#include "proof/abs/abs.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// external methods
extern void* Abc_NtkBuildGlobalBdds(Abc_Ntk_t*, int, int, int, int);
extern DdNode* Abc_NodeGlobalBdds_rec(DdManager*, Abc_Obj_t*, int, int,
                                      ProgressBar*, int*, int);
extern MtrNode* Cudd_MakeTreeNode(DdManager*, unsigned int, unsigned int,
                                  unsigned int);
// main methods
float Pb_BddComputeSp(Abc_Ntk_t*, int, int, int, int);
float Pb_BddComputeRESp(Abc_Ntk_t*, int, int, int, int);
float Ssat_BddComputeRESp(Abc_Ntk_t*, DdManager*, int, int, int);
void Pb_BddComputeAllSp(Abc_Ntk_t*, int, int, int);
// helpers
DdManager* Ssat_NtkPoBuildGlobalBdd(Abc_Ntk_t*, int, int, int);
int Pb_BddShuffleGroup(DdManager*, int, int);
void Pb_BddResetProb(DdManager*, DdNode*);
void BddComputeSsat_rec(Abc_Ntk_t*, DdNode*);
float Ssat_BddComputeProb_rec(Abc_Ntk_t*, DdNode*, int, int);
void Nz_DebugBdd(DdNode* bFunc);
static DdManager* Pb_NtkBuildGlobalBdds(Abc_Ntk_t*, int, int);
static DdManager* Abc_NtkPoBuildGlobalBdd(Abc_Ntk_t*, int, int, int);
static void Pb_BddComputeProb(Abc_Ntk_t*, DdNode*, int, int);
static float Pb_BddComputeProb_rec(Abc_Ntk_t*, DdNode*, int, int);
static void Pb_BddPrintProb(Abc_Ntk_t*, DdNode*, int);
static void Pb_BddPrintExSol(Abc_Ntk_t*, DdNode*, int, int);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [compute signal prob by bdd]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

float Pb_BddComputeSp(Abc_Ntk_t* pNtk, int numPo, int numExist, int fGrp,
                      int fVerbose) {
  DdManager* dd;
  DdNode* bFunc;
  abctime clk;
  float prob;

  if (fVerbose)
    printf("  > Pb_BddComputeSp() : build bdd for %d-th Po\n", numPo);
  clk = Abc_Clock();
  dd = Abc_NtkPoBuildGlobalBdd(pNtk, numPo, numExist, fGrp);
  if (!dd) {
    Abc_Print(-1, "Bdd construction has failed.\n");
    return -1;
  }
  if (fVerbose) Abc_PrintTime(1, "  > Bdd construction", Abc_Clock() - clk);
  // NZ : check exist/random variables are correctly ordered
  if (numExist > 0) {
    if (Cudd_ReadInvPerm(dd, 0) > numExist - 1) {
      printf("  > AI(%d) is ordered before PI(%d) --> shuffle back!\n",
             Cudd_ReadInvPerm(dd, 0), numExist);
      if (Pb_BddShuffleGroup(dd, numExist, Abc_NtkPiNum(pNtk) - numExist) ==
          0) {
        Abc_Print(-1, "Bdd Shuffle has failed.\n");
        Abc_NtkFreeGlobalBdds(pNtk, 1);
        return -1;
      }
    }
  }
  bFunc = Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, numPo));
  clk = Abc_Clock();
  Pb_BddResetProb(dd, bFunc);
  Pb_BddComputeProb(pNtk, bFunc, numExist, Cudd_IsComplement(bFunc));
  if (fVerbose) Abc_PrintTime(1, "  > Prob computation", Abc_Clock() - clk);

  prob = Cudd_IsComplement(bFunc) ? 1.0 - Cudd_Regular(bFunc)->pMin
                                  : Cudd_Regular(bFunc)->pMax;
  if (fVerbose) {
    printf("  > %d-th Po", numPo);
    Pb_BddPrintProb(pNtk, bFunc, numExist);
  }
  Abc_NtkFreeGlobalBdds(pNtk, 1);
  return prob;
}

void Pb_BddComputeProb(Abc_Ntk_t* pNtk, DdNode* bFunc, int numExist, int fNot) {
  Pb_BddComputeProb_rec(pNtk, bFunc, numExist, fNot);
}

void Pb_BddResetProb(DdManager* dd, DdNode* bFunc) {
  DdGen* gen;
  DdNode* node;
  Cudd_ForeachNode(dd, bFunc, gen, node) Cudd_Regular(node)->pMax =
      Cudd_Regular(node)->pMin = Cudd_IsConstant(node) ? 1.0 : -1.0;
}

// FIXME: this is buggy!!!
#if 1
float Pb_BddComputeProb_rec(Abc_Ntk_t* pNtk, DdNode* bFunc, int numExist,
                            int fNot) {
  float prob, pThenMax, pElseMax, pThenMin, pElseMin;
  int numPi, fComp;

  numPi = Cudd_Regular(bFunc)->index;
  fComp = Cudd_IsComplement(bFunc);

  if (Cudd_IsConstant(bFunc)) return Cudd_IsComplement(bFunc) ? 0.0 : 1.0;
  if (Cudd_Regular(bFunc)->pMax == -1.0) {
    // compute value for this node if it is not visited
    if (numPi < numExist) {
      if (fNot) {  // forall var -> min
        pThenMin = Pb_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numExist, fNot);
        pElseMin = Pb_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numExist, fNot);
        pThenMax = Cudd_IsComplement(Cudd_T(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_T(bFunc))->pMin
                       : Cudd_Regular(Cudd_T(bFunc))->pMax;
        pElseMax = Cudd_IsComplement(Cudd_E(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_E(bFunc))->pMin
                       : Cudd_Regular(Cudd_E(bFunc))->pMax;
      } else {  // exist var ->max
        pThenMax = Pb_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numExist, fNot);
        pElseMax = Pb_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numExist, fNot);
        pThenMin = Cudd_IsComplement(Cudd_T(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_T(bFunc))->pMax
                       : Cudd_Regular(Cudd_T(bFunc))->pMin;
        pElseMin = Cudd_IsComplement(Cudd_E(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_E(bFunc))->pMax
                       : Cudd_Regular(Cudd_E(bFunc))->pMin;
        // printf( "pThenMax = %f , pThenMin = %f , pElseMax = %f , pElseMin =
        // %f\n" , pThenMax , pThenMin , pElseMax , pElseMin ); printf( "   >
        // (exist) %s(%d-th) , pMax = %f , pMin = %f " , Abc_ObjName(
        // Abc_NtkPi(pNtk,numPi) ) , numPi , Cudd_Regular(bFunc)->pMax ,
        // Cudd_Regular(bFunc)->pMin ); printf( " cMax = %d , cMin = %d\n" ,
        // Cudd_Regular(bFunc)->cMax , Cudd_Regular(bFunc)->cMin );
      }
      Cudd_Regular(bFunc)->pMax = (pThenMax >= pElseMax) ? pThenMax : pElseMax;
      Cudd_Regular(bFunc)->pMin = (pThenMin <= pElseMin) ? pThenMin : pElseMin;
      Cudd_Regular(bFunc)->cMax = (pThenMax >= pElseMax) ? 1 : 0;
      Cudd_Regular(bFunc)->cMin = (pThenMin <= pElseMin) ? 1 : 0;
    } else {  // random var -> avg
      prob = Abc_NtkPi(pNtk, numPi)->dTemp;
      Cudd_Regular(bFunc)->pMax = Cudd_Regular(bFunc)->pMin =
          prob * Pb_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numExist, fNot) +
          (1.0 - prob) *
              Pb_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numExist, fNot);
      // printf( "   > (random) %s , pMax = %f\n" , Abc_ObjName(
      // Abc_NtkPi(pNtk,numPi) ) , Cudd_Regular(bFunc)->pMax );
    }
  }
  if (fNot)
    return fComp ? 1.0 - Cudd_Regular(bFunc)->pMax : Cudd_Regular(bFunc)->pMin;
  else
    return fComp ? 1.0 - Cudd_Regular(bFunc)->pMin : Cudd_Regular(bFunc)->pMax;
}
#else
float Pb_BddComputeProb_rec(Abc_Ntk_t* pNtk, DdNode* bFunc, int numExist) {
  float prob, pThenMax, pElseMax, pThenMin, pElseMin;
  int numPi, fComp;

  numPi = Cudd_Regular(bFunc)->index;
  fComp = Cudd_IsComplement(bFunc);

  if (Cudd_IsConstant(bFunc)) return Cudd_IsComplement(bFunc) ? 0.0 : 1.0;
  if (Cudd_Regular(bFunc)->pMax == -1.0) {  // unvisited node
    if (numPi < numExist) {                 // exist var -> max / min
      pThenMax = Pb_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numExist);
      pElseMax = Pb_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numExist);
      pThenMin = Cudd_IsComplement(Cudd_T(bFunc))
                     ? 1.0 - Cudd_Regular(Cudd_T(bFunc))->pMax
                     : Cudd_Regular(Cudd_T(bFunc))->pMin;
      pElseMin = Cudd_IsComplement(Cudd_E(bFunc))
                     ? 1.0 - Cudd_Regular(Cudd_E(bFunc))->pMax
                     : Cudd_Regular(Cudd_E(bFunc))->pMin;
      // printf( "pThenMax = %f , pThenMin = %f , pElseMax = %f , pElseMin =
      // %f\n" , pThenMax , pThenMin , pElseMax , pElseMin );
      Cudd_Regular(bFunc)->pMax = (pThenMax >= pElseMax) ? pThenMax : pElseMax;
      Cudd_Regular(bFunc)->pMin = (pThenMin <= pElseMin) ? pThenMin : pElseMin;
      Cudd_Regular(bFunc)->cMax = (pThenMax >= pElseMax) ? 1 : 0;
      Cudd_Regular(bFunc)->cMin = (pThenMin <= pElseMin) ? 1 : 0;
      // printf( "   > (exist) %s(%d-th) , pMax = %f , pMin = %f " ,
      // Abc_ObjName( Abc_NtkPi(pNtk,numPi) ) , numPi ,
      // Cudd_Regular(bFunc)->pMax , Cudd_Regular(bFunc)->pMin ); printf( " cMax
      // = %d , cMin = %d\n" , Cudd_Regular(bFunc)->cMax ,
      // Cudd_Regular(bFunc)->cMin );
    } else {  // random var -> avg
      prob = Abc_NtkPi(pNtk, numPi)->dTemp;
      Cudd_Regular(bFunc)->pMax = Cudd_Regular(bFunc)->pMin =
          prob * Pb_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numExist) +
          (1.0 - prob) * Pb_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numExist);
      // printf( "   > (random) %s , pMax = %f\n" , Abc_ObjName(
      // Abc_NtkPi(pNtk,numPi) ) , Cudd_Regular(bFunc)->pMax );
    }
  }
  return fComp ? 1.0 - Cudd_Regular(bFunc)->pMin : Cudd_Regular(bFunc)->pMax;
}
#endif

void Pb_BddPrintProb(Abc_Ntk_t* pNtk, DdNode* bFunc, int numExist) {
  // int i , n;
  printf(" , prob = %e\n", Cudd_IsComplement(bFunc)
                               ? 1.0 - Cudd_Regular(bFunc)->pMin
                               : Cudd_Regular(bFunc)->pMax);
  /*if ( Cudd_Regular( bFunc )->index < numExist ) { // exist var
     if ( pNtk->pModel ) printf( "  > [Warning] Reset model\n" );
          else {
                  printf( "  > Create model\n" );
                  pNtk->pModel = ABC_ALLOC( int , Abc_NtkPiNum( pNtk ) );
          }
          for ( i = 0 , n = Abc_NtkPiNum( pNtk ) ; i < n ; ++i ) pNtk->pModel[i]
  = -1; Pb_BddPrintExSol( pNtk , bFunc , numExist , 1 );
  }*/
}

void Pb_BddPrintExSol(Abc_Ntk_t* pNtk, DdNode* bFunc, int numExist, int max) {
  // > max = 1 -> find max ; max = 0 -> find min
  int sol;
  max = max ^ Cudd_IsComplement(bFunc);
  sol = max ? Cudd_Regular(bFunc)->cMax : Cudd_Regular(bFunc)->cMin;
  if (!Cudd_IsConstant(bFunc) && Cudd_Regular(bFunc)->index < numExist) {
    printf("  > %d-th Pi , sol = %d\n", Cudd_Regular(bFunc)->index, sol);
    pNtk->pModel[Cudd_Regular(bFunc)->index] = sol;
    sol ? Pb_BddPrintExSol(pNtk, Cudd_T(bFunc), numExist, max)
        : Pb_BddPrintExSol(pNtk, Cudd_E(bFunc), numExist, max);
  }
}

/**Function*************************************************************

  Synopsis    [shuffle AI and PI to respect quantification order]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

int Pb_BddShuffleGroup(DdManager* dd, int numExist, int numRand) {
  // printf( "Pb_BddShuffleGroup() : shuffle AI and PI\n" );
  int *perm, RetValue, i, n;

  perm = ABC_ALLOC(int, numExist + numRand);

  for (i = 0, n = numRand; i < n; ++i)
    perm[numExist + i] = Cudd_ReadInvPerm(dd, i);
  for (i = 0, n = numExist; i < n; ++i)
    perm[i] = Cudd_ReadInvPerm(dd, numRand + i);

  // for ( i = 0 , n = numExist + numRand ; i < n ; ++i )
  // printf( "perm[%d] = %d\n" , i , perm[i] );
  // NZ : free group before shuffling
  Cudd_FreeTree(dd);
  RetValue = Cudd_ShuffleHeap(dd, perm);
  ABC_FREE(perm);
  return RetValue;
}

/**Function*************************************************************

  Synopsis    [build a global bdd for one Po]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

DdManager* Abc_NtkPoBuildGlobalBdd(Abc_Ntk_t* pNtk, int numPo, int numExist,
                                   int fGrp) {
  ProgressBar* pProgress;
  Abc_Obj_t *pObj, *pFanin;
  Vec_Att_t* pAttMan;
  DdManager* dd;
  DdNode* bFunc;
  int nBddSizeMax, fReorder, fDropInternal, fVerbose, Counter, k, i;

  // set defaults
  nBddSizeMax = ABC_INFINITY;
  fReorder = (numExist > 0) ? 0 : 1;
  fDropInternal = 1;
  fVerbose = 0;

  // remove dangling nodes
  Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);

  // start the manager
  assert(!Abc_NtkGlobalBdd(pNtk));
  dd = Cudd_Init(Abc_NtkPiNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
  // NZ : group variables
  if (numExist > 0 && fGrp) {
    printf("  >  Start grouping PI and AI variables\n");
    Cudd_MakeTreeNode(dd, 0, numExist, MTR_DEFAULT);
    Cudd_MakeTreeNode(dd, numExist, Abc_NtkPiNum(pNtk) - numExist, MTR_DEFAULT);
    fReorder = 1;
    printf("  >  Done\n");
  }
  pAttMan = Vec_AttAlloc(Abc_NtkObjNumMax(pNtk) + 1, dd,
                         (void (*)(void*))Extra_StopManager, NULL,
                         (void (*)(void*, void*))Cudd_RecursiveDeref);
  Vec_PtrWriteEntry(pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan);

  if (fReorder) Cudd_AutodynEnable(dd, CUDD_REORDER_SYMM_SIFT);

  // assign the constant node BDD
  pObj = Abc_AigConst1(pNtk);
  if (Abc_ObjFanoutNum(pObj) > 0) {
    bFunc = dd->one;
    Abc_ObjSetGlobalBdd(pObj, bFunc);
    Cudd_Ref(bFunc);
  }
  // set the elementary variables (Pi`s)
  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (Abc_ObjFanoutNum(pObj) > 0) {
      bFunc = dd->vars[i];
      Abc_ObjSetGlobalBdd(pObj, bFunc);
      Cudd_Ref(bFunc);
    }
  }
  // construct the BDD for numPo
  Counter = 0;
  pProgress = Extra_ProgressBarStart(stdout, Abc_NtkNodeNum(pNtk));
  pObj = Abc_NtkPo(pNtk, numPo);

  extern DdNode* Abc_NodeGlobalBdds_rec(DdManager*, Abc_Obj_t*, int, int,
                                        ProgressBar*, int*, int);
  bFunc = Abc_NodeGlobalBdds_rec(dd, Abc_ObjFanin0(pObj), nBddSizeMax,
                                 fDropInternal, pProgress, &Counter, fVerbose);
  if (!bFunc) {
    if (fVerbose) printf("Constructing global BDDs is aborted.\n");
    Abc_NtkFreeGlobalBdds(pNtk, 0);
    Cudd_Quit(dd);
    // reset references
    Abc_NtkForEachObj(pNtk, pObj,
                      i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj))
        pObj->vFanouts.nSize = 0;
    Abc_NtkForEachObj(pNtk, pObj,
                      i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj))
        Abc_ObjForEachFanin(pObj, pFanin, k) pFanin->vFanouts.nSize++;
    return NULL;
  }
  bFunc = Cudd_NotCond(bFunc, (int)Abc_ObjFaninC0(pObj));
  Cudd_Ref(bFunc);
  Abc_ObjSetGlobalBdd(pObj, bFunc);
  Extra_ProgressBarStop(pProgress);
  // reset references
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj))
      pObj->vFanouts.nSize = 0;
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj))
      Abc_ObjForEachFanin(pObj, pFanin, k) pFanin->vFanouts.nSize++;
  // reorder one more time
  if (fReorder) {
    Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT, 1);
    Cudd_AutodynDisable(dd);
  }
  return dd;
}

/**Function*************************************************************

  Synopsis    [compute signal prob for every Po]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Pb_BddComputeAllSp(Abc_Ntk_t* pNtk, int numExist, int fGrp, int fVerbose) {
  DdManager* dd;
  // DdNode * bFunc;
  // Abc_Obj_t * pObj;
  abctime clk;
  int fReorder, maxId;  // , i;
  float maxProb;        // , temp;

  fReorder = (numExist > 0) ? 0 : 1;
  maxId = -1;
  maxProb = 0.0;

  if (fVerbose) printf("  > Pb_BddComputeAllSp() : build bdds for every Po\n");
  clk = Abc_Clock();
  dd = Pb_NtkBuildGlobalBdds(pNtk, numExist, fGrp);
  if (!dd) {
    Abc_Print(-1, "Bdd construction has failed.\n");
    return;
  }
#if 0
	if ( fVerbose ) Abc_PrintTime( 1 , "  > Bdd construction" , Abc_Clock() - clk );
   // NZ : check exist/random variables are correctly ordered
   if ( numExist > 0 ) {
      if ( Cudd_ReadInvPerm( dd , 0 ) > numExist-1 ) {
         printf( "  > AI(%d) is ordered before PI(%d) --> shuffle back!\n" , Cudd_ReadInvPerm(dd , 0) , numExist );
         if ( Pb_BddShuffleGroup( dd , numExist , Abc_NtkPiNum(pNtk)-numExist ) == 0 ) {
            Abc_Print( -1 , "Bdd Shuffle has failed.\n" );
	         Abc_NtkFreeGlobalBdds( pNtk , 1 );
            return;
         }
      }
   }
	clk = Abc_Clock();
	Abc_NtkForEachPo( pNtk , pObj , i )
	{
	   bFunc = Abc_ObjGlobalBdd( Abc_NtkPo( pNtk , i ) );
      Pb_BddResetProb( dd , bFunc ); 
	   if ( fVerbose ) printf( "Pb_BddComputeSp() : compute prob for %d-th Po\n" , i );
	   clk = Abc_Clock();
      Pb_BddComputeProb( pNtk , bFunc , numExist );
	   if ( fVerbose ) {
         Abc_PrintTime( 1 , "  > Probability computation" , Abc_Clock() - clk );
	      printf( "Bddsp : numPo = %d " , i );
      }
		temp = Cudd_IsComplement( bFunc ) ? 1.0-Cudd_Regular(bFunc)->pMin : Cudd_Regular(bFunc)->pMax;
		if ( temp > maxProb ) {
		   maxProb = temp;
         maxId   = i;
		}
      if ( fVerbose ) Pb_BddPrintProb( pNtk , bFunc , numExist );
	}
	if ( fVerbose ) Abc_PrintTime( 1 , "  > Probability computation" , Abc_Clock() - clk );
	printf( "  > max prob = %f , numPo = %d\n" , maxProb , maxId );
#endif
  Abc_NtkFreeGlobalBdds(pNtk, 1);
}

/**Function*************************************************************

  Synopsis    [Derives global BDDs for the POs of the network.]

  Description [Group PI and AI variables]

  SideEffects []

  SeeAlso     []

***********************************************************************/

DdManager* Pb_NtkBuildGlobalBdds(Abc_Ntk_t* pNtk, int numExist, int fGrp) {
  ProgressBar* pProgress;
  Abc_Obj_t *pObj, *pFanin;
  Vec_Att_t* pAttMan;
  DdManager* dd;
  DdNode* bFunc;
  int nBddSizeMax, fReorder, fDropInternal, fVerbose, Counter, k, i;

  // set defaults
  nBddSizeMax = ABC_INFINITY;
  fReorder = (numExist > 0) ? 0 : 1;
  fDropInternal = 1;
  fVerbose = 0;
  // remove dangling nodes
  Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
  // start the manager
  assert(Abc_NtkGlobalBdd(pNtk) == NULL);
  dd = Cudd_Init(Abc_NtkPiNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
  // NZ : group variables
  if (numExist > 0 && fGrp) {
    printf(" >  Start grouping PI and AI variables\n");
    Cudd_MakeTreeNode(dd, 0, numExist, MTR_DEFAULT);
    Cudd_MakeTreeNode(dd, numExist, Abc_NtkPiNum(pNtk) - numExist, MTR_DEFAULT);
    fReorder = 1;
    printf(" >  Done\n");
  }
  pAttMan = Vec_AttAlloc(Abc_NtkObjNumMax(pNtk) + 1, dd,
                         (void (*)(void*))Extra_StopManager, NULL,
                         (void (*)(void*, void*))Cudd_RecursiveDeref);
  Vec_PtrWriteEntry(pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan);
  // set reordering
  if (fReorder) Cudd_AutodynEnable(dd, CUDD_REORDER_SYMM_SIFT);
  // assign the constant node BDD
  pObj = Abc_AigConst1(pNtk);
  if (Abc_ObjFanoutNum(pObj) > 0) {
    bFunc = dd->one;
    Abc_ObjSetGlobalBdd(pObj, bFunc);
    Cudd_Ref(bFunc);
  }
  // set the elementary variables
  Abc_NtkForEachPi(pNtk, pObj, i) if (Abc_ObjFanoutNum(pObj) > 0) {
    bFunc = dd->vars[i];
    Abc_ObjSetGlobalBdd(pObj, bFunc);
    Cudd_Ref(bFunc);
  }
  // collect the global functions of the COs
  Counter = 0;
  // construct the BDDs
  pProgress = Extra_ProgressBarStart(stdout, Abc_NtkNodeNum(pNtk));
  Abc_NtkForEachPo(pNtk, pObj, i) {
    // NZ : group variables if they were free
    if (numExist > 0 && fGrp && !dd->tree) {
      Cudd_MakeTreeNode(dd, 0, numExist, MTR_DEFAULT);
      Cudd_MakeTreeNode(dd, numExist, Abc_NtkPiNum(pNtk) - numExist,
                        MTR_DEFAULT);
    }
    bFunc =
        Abc_NodeGlobalBdds_rec(dd, Abc_ObjFanin0(pObj), nBddSizeMax,
                               fDropInternal, pProgress, &Counter, fVerbose);
    if (bFunc == NULL) {
      if (fVerbose) printf("Constructing global BDDs is aborted.\n");
      Abc_NtkFreeGlobalBdds(pNtk, 0);
      Cudd_Quit(dd);
      // reset references
      Abc_NtkForEachObj(pNtk, pObj,
                        i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj))
          pObj->vFanouts.nSize = 0;
      Abc_NtkForEachObj(pNtk, pObj,
                        i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj))
          Abc_ObjForEachFanin(pObj, pFanin, k) pFanin->vFanouts.nSize++;
      return NULL;
    }
    bFunc = Cudd_NotCond(bFunc, (int)Abc_ObjFaninC0(pObj));
    Cudd_Ref(bFunc);
    Abc_ObjSetGlobalBdd(pObj, bFunc);
  }
  Extra_ProgressBarStop(pProgress);
  // reset references
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj))
      pObj->vFanouts.nSize = 0;
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj))
      Abc_ObjForEachFanin(pObj, pFanin, k) pFanin->vFanouts.nSize++;
  // reorder one more time
  if (fReorder) {
    Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT, 1);
    Cudd_AutodynDisable(dd);
  }
  //    Cudd_PrintInfo( dd, stdout );
  return dd;
}

/**Function*************************************************************

  Synopsis    [compute signal prob by bdd]

  Description [For RE-2SSAT.]

  SideEffects [Lots of codes are similar, FIXME later!]

  SeeAlso     []

***********************************************************************/

float Pb_BddComputeRESp(Abc_Ntk_t* pNtk, int numPo, int numRand, int fGrp,
                        int fVerbose) {
  DdManager* dd;
  DdNode* bFunc;
  abctime clk;
  float prob;

  if (fVerbose)
    printf("  > Pb_BddComputeSp() : build bdd for %d-th Po\n", numPo);
  clk = Abc_Clock();
  dd = Ssat_NtkPoBuildGlobalBdd(pNtk, numPo, numRand, fGrp);
  if (!dd) {
    Abc_Print(-1, "Bdd construction has failed.\n");
    return -1;
  }
  if (fVerbose) Abc_PrintTime(1, "  > Bdd construction", Abc_Clock() - clk);
  // NZ : check random/exist variables are correctly ordered
  if (numRand < Abc_NtkPiNum(pNtk)) {
    if (Cudd_ReadInvPerm(dd, 0) > numRand - 1) {
      if (fVerbose)
        printf(
            "  > exist var(%d) is ordered before random var --> shuffle "
            "back!\n",
            Cudd_ReadInvPerm(dd, 0));
      if (Pb_BddShuffleGroup(dd, numRand, Abc_NtkPiNum(pNtk) - numRand) == 0) {
        Abc_Print(-1, "Bdd Shuffle has failed.\n");
        Abc_NtkFreeGlobalBdds(pNtk, 1);
        return -1;
      }
    }
  }
  bFunc = Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, numPo));
  clk = Abc_Clock();
  Pb_BddResetProb(dd, bFunc);
  BddComputeSsat_rec(pNtk, bFunc);
  if (fVerbose) Abc_PrintTime(1, "  > Prob computation", Abc_Clock() - clk);
  prob = Cudd_IsComplement(bFunc) ? 1.0 - Cudd_Regular(bFunc)->pMin
                                  : Cudd_Regular(bFunc)->pMax;
  if (fVerbose) printf("  > %d-th Po, sat prob = %f\n", numPo, prob);
  Abc_NtkFreeGlobalBdds(pNtk, 1);
  return prob;
}

void Nz_DebugBdd(DdNode* bFunc) {
  if (Cudd_IsConstant(bFunc))
    printf("  > visiting const node %d\n", Cudd_IsComplement(bFunc) ? 0 : 1);
  else {
    printf("  > node %p , decision var %3d , ", Cudd_Regular(bFunc),
           Cudd_Regular(bFunc)->index);
    printf(" pMax = %f , pMin = %f\n", Cudd_Regular(bFunc)->pMax,
           Cudd_Regular(bFunc)->pMin);
    printf("    > Then-Child of %p\n", Cudd_Regular(bFunc));
    Nz_DebugBdd(Cudd_T(bFunc));
    printf("    > Else-Child of %p\n", Cudd_Regular(bFunc));
    Nz_DebugBdd(Cudd_E(bFunc));
  }
}

/**Function*************************************************************

  Synopsis    [build a global bdd for one Po]

  Description [For RE-2SSAT.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

DdManager* Ssat_NtkPoBuildGlobalBdd(Abc_Ntk_t* pNtk, int numPo, int numRand,
                                    int fGrp) {
  ProgressBar* pProgress;
  Abc_Obj_t *pObj, *pFanin;
  Vec_Att_t* pAttMan;
  DdManager* dd;
  DdNode* bFunc;
  int nBddSizeMax, fReorder, fDropInternal, fVerbose, Counter, k, i;

  // set defaults
  nBddSizeMax = ABC_INFINITY;
  fReorder = (numRand < Abc_NtkPiNum(pNtk)) ? 0 : 1;
  fDropInternal = 1;
  fVerbose = 0;

  // remove dangling nodes
  Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);

  // start the manager
  assert(!Abc_NtkGlobalBdd(pNtk));
  dd = Cudd_Init(Abc_NtkPiNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
  // NZ : group variables
  if (numRand < Abc_NtkPiNum(pNtk) && fGrp) {
    // printf( "  >  Start grouping random and exist variables\n" );
    Cudd_MakeTreeNode(dd, 0, numRand, MTR_DEFAULT);
    Cudd_MakeTreeNode(dd, numRand, Abc_NtkPiNum(pNtk) - numRand, MTR_DEFAULT);
    fReorder = 1;
    // printf( "  >  Grouping done\n" );
  }
  pAttMan = Vec_AttAlloc(Abc_NtkObjNumMax(pNtk) + 1, dd,
                         (void (*)(void*))Extra_StopManager, NULL,
                         (void (*)(void*, void*))Cudd_RecursiveDeref);
  Vec_PtrWriteEntry(pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan);

  if (fReorder) Cudd_AutodynEnable(dd, CUDD_REORDER_SYMM_SIFT);

  // assign the constant node BDD
  pObj = Abc_AigConst1(pNtk);
  if (Abc_ObjFanoutNum(pObj) > 0) {
    bFunc = dd->one;
    Abc_ObjSetGlobalBdd(pObj, bFunc);
    Cudd_Ref(bFunc);
  }
  // set the elementary variables (Pi`s)
  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (Abc_ObjFanoutNum(pObj) > 0) {
      bFunc = dd->vars[i];
      Abc_ObjSetGlobalBdd(pObj, bFunc);
      Cudd_Ref(bFunc);
    }
  }
  // construct the BDD for numPo
  Counter = 0;
  pProgress = Extra_ProgressBarStart(stdout, Abc_NtkNodeNum(pNtk));
  pObj = Abc_NtkPo(pNtk, numPo);

  extern DdNode* Abc_NodeGlobalBdds_rec(DdManager*, Abc_Obj_t*, int, int,
                                        ProgressBar*, int*, int);
  bFunc = Abc_NodeGlobalBdds_rec(dd, Abc_ObjFanin0(pObj), nBddSizeMax,
                                 fDropInternal, pProgress, &Counter, fVerbose);
  if (!bFunc) {
    if (fVerbose) printf("Constructing global BDDs is aborted.\n");
    Abc_NtkFreeGlobalBdds(pNtk, 0);
    Cudd_Quit(dd);
    // reset references
    Abc_NtkForEachObj(pNtk, pObj,
                      i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj))
        pObj->vFanouts.nSize = 0;
    Abc_NtkForEachObj(pNtk, pObj,
                      i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj))
        Abc_ObjForEachFanin(pObj, pFanin, k) pFanin->vFanouts.nSize++;
    return NULL;
  }
  bFunc = Cudd_NotCond(bFunc, (int)Abc_ObjFaninC0(pObj));
  Cudd_Ref(bFunc);
  Abc_ObjSetGlobalBdd(pObj, bFunc);
  Extra_ProgressBarStop(pProgress);
  // reset references
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj))
      pObj->vFanouts.nSize = 0;
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj))
      Abc_ObjForEachFanin(pObj, pFanin, k) pFanin->vFanouts.nSize++;
  // reorder one more time
  if (fReorder) {
    Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT, 1);
    Cudd_AutodynDisable(dd);
  }
  return dd;
}

/**Function*************************************************************

  Synopsis    [Compute signal prob on BDD.]

  Description [For RE-2SSAT.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

// FIXME: this is buggy!!! See BddComputeSsat_rec
float Ssat_BddComputeProb_rec(Abc_Ntk_t* pNtk, DdNode* bFunc, int numRand,
                              int fNot) {
  float prob, pThenMax, pElseMax, pThenMin, pElseMin;
  int numPi, fComp;

  numPi = Cudd_Regular(bFunc)->index;
  fComp = Cudd_IsComplement(bFunc);

  if (Cudd_IsConstant(bFunc)) return Cudd_IsComplement(bFunc) ? 0.0 : 1.0;
  if (Cudd_Regular(bFunc)->pMax == -1.0) {  // unvisited node
    if (numPi >= numRand) {
      if (fNot) {  // forall var -> min
        pThenMin = Ssat_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numRand, fNot);
        pElseMin = Ssat_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numRand, fNot);
        pThenMax = Cudd_IsComplement(Cudd_T(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_T(bFunc))->pMin
                       : Cudd_Regular(Cudd_T(bFunc))->pMax;
        pElseMax = Cudd_IsComplement(Cudd_E(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_E(bFunc))->pMin
                       : Cudd_Regular(Cudd_E(bFunc))->pMax;
      } else {  // exist var -> max
        pThenMax = Ssat_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numRand, fNot);
        pElseMax = Ssat_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numRand, fNot);
        pThenMin = Cudd_IsComplement(Cudd_T(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_T(bFunc))->pMax
                       : Cudd_Regular(Cudd_T(bFunc))->pMin;
        pElseMin = Cudd_IsComplement(Cudd_E(bFunc))
                       ? 1.0 - Cudd_Regular(Cudd_E(bFunc))->pMax
                       : Cudd_Regular(Cudd_E(bFunc))->pMin;
      }
      Cudd_Regular(bFunc)->pMax = (pThenMax >= pElseMax) ? pThenMax : pElseMax;
      Cudd_Regular(bFunc)->pMin = (pThenMin <= pElseMin) ? pThenMin : pElseMin;
      Cudd_Regular(bFunc)->cMax = (pThenMax >= pElseMax) ? 1 : 0;
      Cudd_Regular(bFunc)->cMin = (pThenMin <= pElseMin) ? 1 : 0;
    } else {  // random var -> avg
      prob = Abc_NtkPi(pNtk, numPi)->dTemp;
      Cudd_Regular(bFunc)->pMax = Cudd_Regular(bFunc)->pMin =
          prob * Ssat_BddComputeProb_rec(pNtk, Cudd_T(bFunc), numRand, fNot) +
          (1.0 - prob) *
              Ssat_BddComputeProb_rec(pNtk, Cudd_E(bFunc), numRand, fNot);
    }
  }
  if (fNot)  // forall quantified!
    return fComp ? 1.0 - Cudd_Regular(bFunc)->pMax : Cudd_Regular(bFunc)->pMin;
  else
    return fComp ? 1.0 - Cudd_Regular(bFunc)->pMin : Cudd_Regular(bFunc)->pMax;
}

/**Function*************************************************************

  Synopsis    [Compute SSAT value on BDD.]

  Description [For general SSAT.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void BddComputeSsat_rec(Abc_Ntk_t* pNtk, DdNode* bFunc) {
  float prob, pThenMax, pElseMax, pThenMin, pElseMin;
  int numPi, fComp;

  numPi = Cudd_Regular(bFunc)->index;
  fComp = Cudd_IsComplement(bFunc);

  if (Cudd_IsConstant(bFunc) || Cudd_Regular(bFunc)->pMax != -1.0) return;
  BddComputeSsat_rec(pNtk, Cudd_T(bFunc));
  BddComputeSsat_rec(pNtk, Cudd_E(bFunc));
  pThenMax = Cudd_IsComplement(Cudd_T(bFunc))
                 ? 1.0 - Cudd_Regular(Cudd_T(bFunc))->pMin
                 : Cudd_Regular(Cudd_T(bFunc))->pMax;
  pElseMax = Cudd_IsComplement(Cudd_E(bFunc))
                 ? 1.0 - Cudd_Regular(Cudd_E(bFunc))->pMin
                 : Cudd_Regular(Cudd_E(bFunc))->pMax;
  pThenMin = Cudd_IsComplement(Cudd_T(bFunc))
                 ? 1.0 - Cudd_Regular(Cudd_T(bFunc))->pMax
                 : Cudd_Regular(Cudd_T(bFunc))->pMin;
  pElseMin = Cudd_IsComplement(Cudd_E(bFunc))
                 ? 1.0 - Cudd_Regular(Cudd_E(bFunc))->pMax
                 : Cudd_Regular(Cudd_E(bFunc))->pMin;
  prob = Abc_NtkPi(pNtk, numPi)->dTemp;
  if (prob == -1.0) {  // exist var
    Cudd_Regular(bFunc)->pMax = (pThenMax >= pElseMax) ? pThenMax : pElseMax;
    Cudd_Regular(bFunc)->cMax = (pThenMax >= pElseMax) ? 1 : 0;
    Cudd_Regular(bFunc)->pMin = (pThenMin <= pElseMin) ? pThenMin : pElseMin;
    Cudd_Regular(bFunc)->cMin = (pThenMin <= pElseMin) ? 1 : 0;
  } else {  // random var
    Cudd_Regular(bFunc)->pMax = prob * pThenMax + (1.0 - prob) * pElseMax;
    Cudd_Regular(bFunc)->pMin = prob * pThenMin + (1.0 - prob) * pElseMin;
  }
}

/**Function*************************************************************

  Synopsis    [compute signal prob by bdd incrementally]

  Description [For RE-2SSAT.]

  SideEffects [Lots of codes are similar, FIXME later!]

  SeeAlso     []

***********************************************************************/

float Ssat_BddComputeRESp(Abc_Ntk_t* pNtk, DdManager* dd, int numPo,
                          int numRand, int fGrp) {
  ProgressBar* pProgress;
  Abc_Obj_t *pObj, *pFanin;
  DdNode* bFunc;
  float prob;
  int fDropInternal, Counter, i, k;

  fDropInternal = 1;
  Counter = 0;
  pProgress = Extra_ProgressBarStart(stdout, Abc_NtkNodeNum(pNtk));
  pObj = Abc_NtkPo(pNtk, numPo);
  bFunc = Abc_NodeGlobalBdds_rec(dd, Abc_ObjFanin0(pObj), ABC_INFINITY,
                                 fDropInternal, pProgress, &Counter, 0);
  if (!bFunc) {
    printf("Constructing global BDDs is aborted.\n");
    exit(1);
  }
  bFunc = Cudd_NotCond(bFunc, (int)Abc_ObjFaninC0(pObj));
  Cudd_Ref(bFunc);
  Abc_ObjSetGlobalBdd(pObj, bFunc);
  Extra_ProgressBarStop(pProgress);
  // reset references
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj))
      pObj->vFanouts.nSize = 0;
  Abc_NtkForEachObj(pNtk, pObj,
                    i) if (!Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj))
      Abc_ObjForEachFanin(pObj, pFanin, k) pFanin->vFanouts.nSize++;
  // NZ : check random/exist variables are correctly ordered
  if (numRand < Abc_NtkPiNum(pNtk)) {
    if (Cudd_ReadInvPerm(dd, 0) > numRand - 1) {
      if (Pb_BddShuffleGroup(dd, numRand, Abc_NtkPiNum(pNtk) - numRand) == 0) {
        Abc_Print(-1, "Bdd Shuffle has failed.\n");
        Abc_NtkFreeGlobalBdds(pNtk, 1);
        return -1;
      }
    }
  }
  bFunc = Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, numPo));
  Pb_BddResetProb(dd, bFunc);
  BddComputeSsat_rec(pNtk, bFunc);
  prob = Cudd_IsComplement(bFunc) ? 1.0 - Cudd_Regular(bFunc)->pMin
                                  : Cudd_Regular(bFunc)->pMax;
  return prob;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
