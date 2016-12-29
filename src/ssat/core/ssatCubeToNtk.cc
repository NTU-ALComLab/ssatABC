/**CFile****************************************************************

  FileName    [ssatCubeToNtk.cc]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [2SSAT solving by Qesto and model counting.]

  Synopsis    [Function to construct circuits from cubes in SsatSolver.]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [28, Dec., 2016.]

***********************************************************************/

#include "ssat/core/SsatSolver.h"
using namespace Minisat;


//ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// main function
void Ssat_CubeToNtk              ( SsatSolver & );

// helper functions
void Ssat_CubeToNtkCreatePi      ( Abc_Ntk_t * , Vec_Ptr_t * , SsatSolver & );
void Ssat_CubeToNtkCreateNode    ( Abc_Ntk_t * , Vec_Ptr_t * , SsatSolver & );
void Ssat_CubeToNtkWriteWcnf     ( Abc_Ntk_t * , SsatSolver & );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Construct circuits from cubes in SsatSolver]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void 
Ssat_CubeToNtk( SsatSolver & S )
{
   char name[32];
   Abc_Ntk_t * pNtkCube;
   Vec_Ptr_t * vMapVars; // mapping var to obj
   
   pNtkCube = Abc_NtkAlloc( ABC_NTK_LOGIC , ABC_FUNC_SOP , 1 );
   sprintf( name , "cubes_network" );
   pNtkCube->pName = Extra_UtilStrsav( name );
   vMapVars = Vec_PtrStart( (S._s2)->nVars() );

   Ssat_CubeToNtkCreatePi    ( pNtkCube , vMapVars , S );
   Ssat_CubeToNtkCreateNode  ( pNtkCube , vMapVars , S );
   Ssat_CubeToNtkWriteWcnf   ( pNtkCube , S );
   
   Abc_NtkDelete ( pNtkCube );
   Vec_PtrFree   ( vMapVars );
}

void 
Ssat_CubeToNtkCreatePi( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars , SsatSolver & S )
{
   for ( int i = 0 ; i < S._rootVars[0].size() ; ++i ) {
      printf( "  > Create Pi for variable %d\n" , S._rootVars[0][i]+1 );
      Vec_PtrWriteEntry( vMapVars , S._rootVars[0][i] , Abc_NtkCreatePi( pNtkCube ) );
      printf( "  > Var %d map to Obj %p\n" , S._rootVars[0][i]+1 , Vec_PtrEntry( vMapVars , S._rootVars[0][i] ) );
   }
}

void 
Ssat_CubeToNtkCreateNode( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars , SsatSolver & S )
{
   // Create nodes for selection vars
   Solver * s1 = S._s1;
   vec<Lit> uLits;
   for ( int i = 0 ; i < s1->nClauses() ; ++i ) {
      Clause & c  = s1->ca[s1->clauses[i]];
      uLits.clear();
      for ( int j = 0 ; j < c.size() ; ++j )
         if ( S.isAVar(var(c[j])) || S.isRVar(var(c[j])) ) uLits.push(c[j]);
      if ( uLits.size() > 1 ) { // allocate new selection var
         printf( "  > Create selection var for clause %d\n" , i );
      }
   }
}

void 
Ssat_CubeToNtkWriteWcnf( Abc_Ntk_t * pNtkCube , SsatSolver & S )
{
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


//ABC_NAMESPACE_IMPL_END
