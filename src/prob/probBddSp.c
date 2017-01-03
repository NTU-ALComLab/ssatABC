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

#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "aig/saig/saig.h"
#include "proof/abs/abs.h"
#include "sat/bmc/bmc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// external methods
extern void*   Abc_NtkBuildGlobalBdds ( Abc_Ntk_t * , int , int , int , int );
extern DdNode* Abc_NodeGlobalBdds_rec ( DdManager * , Abc_Obj_t * , int , int , ProgressBar * , int * , int );
extern MtrNode* Cudd_MakeTreeNode     ( DdManager * , unsigned int , unsigned int , unsigned int );
// main methods
void  Pb_BddComputeSp           ( Abc_Ntk_t * , int , int , int );
void  Pb_BddComputeAllSp        ( Abc_Ntk_t * , int , int );
// helpers
DdManager* Pb_NtkBuildGlobalBdds     ( Abc_Ntk_t * , int , int );
DdManager* Abc_NtkPoBuildGlobalBdd   ( Abc_Ntk_t * , int , int , int );
void       Pb_BddResetProb           ( DdNode * );
void       Pb_BddComputeProb         ( Abc_Ntk_t * , DdNode * , int );
float      Pb_BddComputeProb_rec     ( Abc_Ntk_t * , DdNode * , int );
void       Pb_BddPrintProb           ( Abc_Ntk_t * , DdNode * , int );
void       Pb_BddPrintExSol          ( Abc_Ntk_t * , DdNode * , int , int );
int        Pb_BddShuffleGroup        ( DdManager * , int , int );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [compute signal prob by bdd]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
Pb_BddComputeSp( Abc_Ntk_t * pNtk , int numPo , int numExist , int fGrp )
{
   DdManager * dd;
	DdNode * bFunc;
	abctime clk;

	printf( "Pb_BddComputeSp() : build bdd for %d-th Po\n" , numPo );
	clk = Abc_Clock();
   dd  = Abc_NtkPoBuildGlobalBdd( pNtk , numPo , numExist , fGrp );
	if ( !dd ) {
	   Abc_Print( -1 , "Bdd construction has failed.\n" );
		return;
	}
	Abc_PrintTime( 1 , "  > Bdd construction" , Abc_Clock() - clk );
   
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

	printf( "Pb_BddComputeSp() : compute prob for %d-th Po (%s) on its bdd\n" , numPo , Abc_ObjName(Abc_NtkPo(pNtk,numPo)));
	bFunc = Abc_ObjGlobalBdd( Abc_NtkPo( pNtk , numPo ) );
	//printf( "bFunc = %p\n" , bFunc );
	//printf( "Pb_BddComputeSp() : compute prob for %d-th Po (%s) on its bdd\n" , numPo , Abc_ObjName(Abc_NtkPo(pNtk,numPo)));
	clk = Abc_Clock();
	Pb_BddResetProb( bFunc );
   Pb_BddComputeProb( pNtk , bFunc , numExist );
	Abc_PrintTime( 1 , "  > Probability computation" , Abc_Clock() - clk );
	
	printf( "Bddsp : numPo = %d " , numPo );
   Pb_BddPrintProb( pNtk , bFunc , numExist );
	Abc_NtkFreeGlobalBdds( pNtk , 1 );
}

void
Pb_BddComputeProb( Abc_Ntk_t * pNtk , DdNode * bFunc , int numExist )
{
	Pb_BddComputeProb_rec( pNtk , bFunc , numExist );
}

void
Pb_BddResetProb( DdNode * bFunc )
{
	if ( !Cudd_IsConstant( bFunc ) ) {
      if ( Cudd_Regular( bFunc )->pMax == -1.0 );  // FIXME: bad practice! use uninitialized values
		else {
         Cudd_Regular( bFunc )->pMax = Cudd_Regular( bFunc )->pMin = -1.0;
		   Pb_BddResetProb( Cudd_T( bFunc ) );
		   Pb_BddResetProb( Cudd_E( bFunc ) );
		}
	}
	else Cudd_Regular(bFunc)->pMax = Cudd_Regular(bFunc)->pMin = 1.0;
}

float
Pb_BddComputeProb_rec( Abc_Ntk_t * pNtk , DdNode * bFunc , int numExist )
{
	float prob , pThenMax , pElseMax , pThenMin , pElseMin;
	int numPi , fComp;
	
	numPi = Cudd_Regular( bFunc )->index;
   fComp = Cudd_IsComplement( bFunc );

	if ( Cudd_IsConstant( bFunc ) ) return Cudd_IsComplement( bFunc ) ? 0.0 : 1.0;
	if ( Cudd_Regular( bFunc )->pMax != -1.0 ) {} // computed node
   else {
		// compute value for this node if it is not visited
		if ( numPi < numExist ) { // exist var -> max / min
	      pThenMax = Pb_BddComputeProb_rec( pNtk , Cudd_T( bFunc ) , numExist );    
	      pElseMax = Pb_BddComputeProb_rec( pNtk , Cudd_E( bFunc ) , numExist );
			pThenMin = Cudd_IsComplement(Cudd_T(bFunc)) ? 1.0-Cudd_Regular(Cudd_T(bFunc))->pMax : Cudd_Regular(Cudd_T(bFunc))->pMin;
			pElseMin = Cudd_IsComplement(Cudd_E(bFunc)) ? 1.0-Cudd_Regular(Cudd_E(bFunc))->pMax : Cudd_Regular(Cudd_E(bFunc))->pMin;
			//printf( "pThenMax = %f , pThenMin = %f , pElseMax = %f , pElseMin = %f\n" , pThenMax , pThenMin , pElseMax , pElseMin );
		   Cudd_Regular( bFunc )->pMax = ( pThenMax >= pElseMax ) ? pThenMax : pElseMax;
		   Cudd_Regular( bFunc )->pMin = ( pThenMin <= pElseMin ) ? pThenMin : pElseMin;
		   Cudd_Regular( bFunc )->cMax = ( pThenMax >= pElseMax ) ? 1 : 0;
		   Cudd_Regular( bFunc )->cMin = ( pThenMin <= pElseMin ) ? 1 : 0;
			//printf( "   > (exist) %s(%d-th) , pMax = %f , pMin = %f " , Abc_ObjName( Abc_NtkPi(pNtk,numPi) ) , numPi , Cudd_Regular(bFunc)->pMax , Cudd_Regular(bFunc)->pMin );
			//printf( " cMax = %d , cMin = %d\n" , Cudd_Regular(bFunc)->cMax , Cudd_Regular(bFunc)->cMin );
		}
		else { // random var -> avg
	      prob  = Abc_NtkPi( pNtk , numPi )->dTemp;
		   Cudd_Regular( bFunc )->pMax = Cudd_Regular( bFunc )->pMin = 
				                           prob        * Pb_BddComputeProb_rec( pNtk , Cudd_T( bFunc ) , numExist ) +
		                                 (1.0-prob)  * Pb_BddComputeProb_rec( pNtk , Cudd_E( bFunc ) , numExist );
			//printf( "   > (random) %s , pMax = %f\n" , Abc_ObjName( Abc_NtkPi(pNtk,numPi) ) , Cudd_Regular(bFunc)->pMax );
		}
	}
	return fComp ? 1.0-Cudd_Regular( bFunc )->pMin : Cudd_Regular( bFunc )->pMax;
}

void  
Pb_BddPrintProb( Abc_Ntk_t * pNtk , DdNode * bFunc , int numExist )
{
	int i , n;
   printf( " , prob = %f\n" , Cudd_IsComplement( bFunc ) ? 1.0-Cudd_Regular(bFunc)->pMin : Cudd_Regular(bFunc)->pMax );
	if ( Cudd_Regular( bFunc )->index < numExist ) { // exist var
	   if ( pNtk->pModel ) printf( "  > [Warning] Reset model\n" );
		else {
			printf( "  > Create model\n" );
			pNtk->pModel = ABC_ALLOC( int , Abc_NtkPiNum( pNtk ) );
		}
		for ( i = 0 , n = Abc_NtkPiNum( pNtk ) ; i < n ; ++i ) pNtk->pModel[i] = -1;
		Pb_BddPrintExSol( pNtk , bFunc , numExist , 1 );
	}
}

void
Pb_BddPrintExSol( Abc_Ntk_t * pNtk , DdNode * bFunc , int numExist , int max )
{
	// > max = 1 -> find max ; max = 0 -> find min
	int sol;
	max = max ^ Cudd_IsComplement( bFunc );
   sol = max ? Cudd_Regular( bFunc )->cMax : Cudd_Regular( bFunc )->cMin;
	if ( !Cudd_IsConstant( bFunc ) && Cudd_Regular( bFunc )->index < numExist ) {
	   printf( "  > %d-th Pi , sol = %d\n" , Cudd_Regular( bFunc )->index , sol );
      pNtk->pModel[Cudd_Regular(bFunc)->index] = sol;
		sol ? Pb_BddPrintExSol(pNtk, Cudd_T(bFunc), numExist, max) : Pb_BddPrintExSol(pNtk, Cudd_E(bFunc), numExist, max);
	}
}

/**Function*************************************************************

  Synopsis    [shuffle AI and PI to respect quantification order]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int
Pb_BddShuffleGroup( DdManager * dd , int numExist , int numRand )
{
   printf( "Pb_BddShuffleGroup() : shuffle AI and PI\n" );
   int * perm , RetValue , i , n;

   perm = ABC_ALLOC( int , numExist+numRand );

   for ( i = 0 , n = numRand ; i < n ; ++i )
      perm[numExist+i] = Cudd_ReadInvPerm( dd , i );
   for ( i = 0 , n = numExist ; i < n ; ++i )
      perm[i] = Cudd_ReadInvPerm( dd , numRand+i );

   //for ( i = 0 , n = numExist + numRand ; i < n ; ++i )
     // printf( "perm[%d] = %d\n" , i , perm[i] );
   // NZ : free group before shuffling
   Cudd_FreeTree( dd );
   RetValue = Cudd_ShuffleHeap( dd , perm );
   ABC_FREE( perm );
   return RetValue;
}

/**Function*************************************************************

  Synopsis    [build a global bdd for one Po]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

DdManager* 
Abc_NtkPoBuildGlobalBdd( Abc_Ntk_t * pNtk , int numPo , int numExist , int fGrp )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pObj, * pFanin;
    Vec_Att_t * pAttMan;
    DdManager * dd;
    DdNode * bFunc;
    int nBddSizeMax , fReorder , fDropInternal , fVerbose , Counter , k , i;

    // set defaults
	 nBddSizeMax   = ABC_INFINITY;
	 fReorder      = ( numExist > 0 ) ? 0 : 1;
	 fDropInternal = 1;
	 fVerbose      = 0;

    // remove dangling nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );

    // start the manager
    assert( !Abc_NtkGlobalBdd( pNtk ) );
    dd = Cudd_Init( Abc_NtkPiNum( pNtk ) , 0 , CUDD_UNIQUE_SLOTS , CUDD_CACHE_SLOTS , 0 );
    // NZ : group variables
    if ( numExist > 0 && fGrp ) {
       printf( "  >  Start grouping PI and AI variables\n" );
       Cudd_MakeTreeNode( dd , 0 , numExist , MTR_DEFAULT );
       Cudd_MakeTreeNode( dd , numExist , Abc_NtkPiNum(pNtk)-numExist , MTR_DEFAULT );
       fReorder = 1;
       printf( "  >  Done\n" );
    }
    pAttMan = Vec_AttAlloc( Abc_NtkObjNumMax( pNtk ) + 1, dd, (void (*)(void*))Extra_StopManager, NULL, (void (*)(void*,void*))Cudd_RecursiveDeref );
    Vec_PtrWriteEntry( pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan );

    if ( fReorder ) Cudd_AutodynEnable( dd , CUDD_REORDER_SYMM_SIFT );

    // assign the constant node BDD
    pObj = Abc_AigConst1( pNtk );
    if ( Abc_ObjFanoutNum(pObj) > 0 ) {
       bFunc = dd->one;
       Abc_ObjSetGlobalBdd( pObj , bFunc );   
		 Cudd_Ref( bFunc );
    }
    // set the elementary variables (Pi`s)
    Abc_NtkForEachPi( pNtk , pObj , i )
	 {
       if ( Abc_ObjFanoutNum(pObj) > 0 ) {
          bFunc = dd->vars[i];
			 //Cudd_Regular( bFunc )->trueP = pObj->dTemp;
          Abc_ObjSetGlobalBdd( pObj , bFunc );  
			 Cudd_Ref( bFunc );
       }
	 }

    // construct the BDD for numPo
    Counter   = 0;
    pProgress = Extra_ProgressBarStart( stdout , Abc_NtkNodeNum( pNtk ) );
    pObj      = Abc_NtkPo( pNtk , numPo );
    
	 extern DdNode* Abc_NodeGlobalBdds_rec ( DdManager * , Abc_Obj_t * , int , int , ProgressBar * , int * , int );
    bFunc     = Abc_NodeGlobalBdds_rec( dd, Abc_ObjFanin0( pObj ), nBddSizeMax, fDropInternal, pProgress, &Counter, fVerbose );
    if ( !bFunc ) {
       if ( fVerbose ) printf( "Constructing global BDDs is aborted.\n" );
       Abc_NtkFreeGlobalBdds( pNtk , 0 );
       Cudd_Quit( dd ); 
       // reset references
       Abc_NtkForEachObj( pNtk, pObj, i )
          if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
             pObj->vFanouts.nSize = 0;
       Abc_NtkForEachObj( pNtk, pObj, i )
          if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
             Abc_ObjForEachFanin( pObj, pFanin, k )
                        pFanin->vFanouts.nSize++;
            return NULL;
    }
    bFunc = Cudd_NotCond( bFunc , (int)Abc_ObjFaninC0(pObj) );  
	 Cudd_Ref( bFunc ); 
    Abc_ObjSetGlobalBdd( pObj , bFunc );
    Extra_ProgressBarStop( pProgress );

    // reset references
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
            pObj->vFanouts.nSize = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                pFanin->vFanouts.nSize++;

    // reorder one more time
    if ( fReorder ) {
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
        Cudd_AutodynDisable( dd );
    }
    return dd;
}

/**Function*************************************************************

  Synopsis    [compute signal prob for every Po]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
Pb_BddComputeAllSp( Abc_Ntk_t * pNtk , int numExist , int fGrp )
{
   DdManager * dd;
	DdNode * bFunc;
	Abc_Obj_t * pObj;
	abctime clk;
	int fReorder , maxId , i;
	float maxProb , temp;

	fReorder = ( numExist > 0 ) ? 0 : 1;
	maxId    = -1;
	maxProb  = 0.0;

	printf( "Pb_BddComputeAllSp() : build bdds for every Po\n" );
	clk = Abc_Clock();
   dd  = Pb_NtkBuildGlobalBdds( pNtk , numExist , fGrp );
   if ( !dd ) {
	   Abc_Print( -1 , "Bdd construction has failed.\n" );
		return;
	}
#if 0
	Abc_PrintTime( 1 , "  > Bdd construction" , Abc_Clock() - clk );
   
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
   
	Abc_NtkForEachPo( pNtk , pObj , i )
	{
		bFunc = Abc_ObjGlobalBdd( Abc_NtkPo( pNtk , i ) );
      Pb_BddResetProb( bFunc );	   
	}

	clk = Abc_Clock();
	Abc_NtkForEachPo( pNtk , pObj , i )
	{
	   bFunc = Abc_ObjGlobalBdd( Abc_NtkPo( pNtk , i ) );
	   printf( "Pb_BddComputeSp() : compute prob for %d-th Po\n" , i );
	   //clk = Abc_Clock();
      Pb_BddComputeProb( pNtk , bFunc , numExist );
	   //Abc_PrintTime( 1 , "  > Probability computation" , Abc_Clock() - clk );
	   printf( "Bddsp : numPo = %d " , i );
		temp = Cudd_IsComplement( bFunc ) ? 1.0-Cudd_Regular(bFunc)->pMin : Cudd_Regular(bFunc)->pMax;
		if ( temp > maxProb ) {
		   maxProb = temp;
			maxId   = i;
		}
		Pb_BddPrintProb( pNtk , bFunc , numExist );
	}
	Abc_PrintTime( 1 , "  > Probability computation" , Abc_Clock() - clk );
	printf( "  > max prob = %f , numPo = %d\n" , maxProb , maxId );
#endif
	Abc_NtkFreeGlobalBdds( pNtk , 1 );
}

/**Function*************************************************************

  Synopsis    [Derives global BDDs for the POs of the network.]

  Description [Group PI and AI variables]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

DdManager* 
Pb_NtkBuildGlobalBdds( Abc_Ntk_t * pNtk , int numExist , int fGrp )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pObj, * pFanin;
    Vec_Att_t * pAttMan;
    DdManager * dd;
    DdNode * bFunc;
    int nBddSizeMax , fReorder , fDropInternal , fVerbose , Counter , k , i;
    int maxId;
    float temp , maxProb;

    // set defaults
	 nBddSizeMax   = ABC_INFINITY;
	 fReorder      = ( numExist > 0 ) ? 0 : 1;
	 fDropInternal = 1;
	 fVerbose      = 0;
    maxId         = -1;
    maxProb       = 0.0;

    // remove dangling nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );

    // start the manager
    assert( Abc_NtkGlobalBdd(pNtk) == NULL );
    dd = Cudd_Init( Abc_NtkPiNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    
    // NZ : group variables
    if ( numExist > 0 && fGrp ) {
       printf( " >  Start grouping PI and AI variables\n" );
       Cudd_MakeTreeNode( dd , 0 , numExist , MTR_DEFAULT );
       Cudd_MakeTreeNode( dd , numExist , Abc_NtkPiNum(pNtk)-numExist , MTR_DEFAULT );
       fReorder = 1;
       printf( " >  Done\n" );
    }
    
    pAttMan = Vec_AttAlloc( Abc_NtkObjNumMax(pNtk) + 1, dd, (void (*)(void*))Extra_StopManager, NULL, (void (*)(void*,void*))Cudd_RecursiveDeref );
    Vec_PtrWriteEntry( pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan );

    // set reordering
    if ( fReorder ) Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // assign the constant node BDD
    pObj = Abc_AigConst1(pNtk);
    if ( Abc_ObjFanoutNum(pObj) > 0 )
    {
        bFunc = dd->one;
        Abc_ObjSetGlobalBdd( pObj, bFunc );   Cudd_Ref( bFunc );
    }
    // set the elementary variables
    Abc_NtkForEachPi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
        {
            bFunc = dd->vars[i];
//            bFunc = dd->vars[Abc_NtkCiNum(pNtk) - 1 - i];
            Abc_ObjSetGlobalBdd( pObj, bFunc );  Cudd_Ref( bFunc );
        }

    // collect the global functions of the COs
    Counter = 0;
    // construct the BDDs
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkNodeNum(pNtk) );
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
       printf( "i = %d " , i);
       fflush( stdout );
        // NZ : group variables if they were free
        if ( numExist > 0 && fGrp && !dd->tree ) {
           Cudd_MakeTreeNode( dd , 0 , numExist , MTR_DEFAULT );
           Cudd_MakeTreeNode( dd , numExist , Abc_NtkPiNum(pNtk)-numExist , MTR_DEFAULT );
        }
        bFunc = Abc_NodeGlobalBdds_rec( dd, Abc_ObjFanin0(pObj), nBddSizeMax, fDropInternal, pProgress, &Counter, fVerbose );
        if ( bFunc == NULL )
        {
            if ( fVerbose )
            printf( "Constructing global BDDs is aborted.\n" );
            Abc_NtkFreeGlobalBdds( pNtk, 0 );
            Cudd_Quit( dd ); 

            // reset references
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
                    pObj->vFanouts.nSize = 0;
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
                    Abc_ObjForEachFanin( pObj, pFanin, k )
                        pFanin->vFanouts.nSize++;
            return NULL;
        }
        bFunc = Cudd_NotCond( bFunc, (int)Abc_ObjFaninC0(pObj) );  Cudd_Ref( bFunc ); 
        Abc_ObjSetGlobalBdd( pObj, bFunc );
        // NZ : compute signal prob
        if ( numExist > 0 ) {
           if ( Cudd_ReadInvPerm( dd , 0 ) > numExist-1 ) {
              printf( "  > AI(%d) is ordered before PI(%d) --> shuffle back!\n" , Cudd_ReadInvPerm(dd , 0) , numExist );
              if ( Pb_BddShuffleGroup( dd , numExist , Abc_NtkPiNum(pNtk)-numExist ) == 0 ) {
                 Abc_Print( -1 , "Bdd Shuffle has failed.\n" );
                 Abc_NtkFreeGlobalBdds( pNtk , 0 );
                 Cudd_Quit( dd ); 
                 return NULL;
              }
           }
        }
        printf( "Computing %d-th Po : " , i );
        Pb_BddResetProb( bFunc );
        Pb_BddComputeProb( pNtk , bFunc , numExist );
        temp = Cudd_IsComplement( bFunc ) ? 1.0-Cudd_Regular(bFunc)->pMin : Cudd_Regular(bFunc)->pMax;
        if ( temp >= maxProb ) {
           maxProb = temp;
           maxId   = i;
        }
		  printf( "prob = %f , cur max = %f , numPo = %d\n" , temp , maxProb , maxId );
        fflush( stdout );
    }
    // NZ : print max prob
	 printf( "  > max prob = %f , numPo = %d\n" , maxProb , maxId );

    Extra_ProgressBarStop( pProgress );

    // reset references
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
            pObj->vFanouts.nSize = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                pFanin->vFanouts.nSize++;

    // reorder one more time
    if ( fReorder )
    {
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
        Cudd_AutodynDisable( dd );
    }
//    Cudd_PrintInfo( dd, stdout );
    return dd;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

