/**CFile****************************************************************

  FileName    [ssatCubeToNtk.cc]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [2SSAT solving by Qesto and model counting.]

  Synopsis    [Member functions to construct circuits from cubes.]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [28, Dec., 2016.]

***********************************************************************/

#include "ssat/core/SsatSolver.h"
using namespace Minisat;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Construct circuits from cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::cubeToNetwork() const
{
   char name[32];
   Abc_Ntk_t * pNtkCube;
   Vec_Ptr_t * vMapVars; // mapping var to obj
   
   pNtkCube = Abc_NtkAlloc( ABC_NTK_LOGIC , ABC_FUNC_SOP , 1 );
   sprintf( name , "cubes_network" );
   pNtkCube->pName = Extra_UtilStrsav( name );
   vMapVars = Vec_PtrStart( _s2->nVars() );

   ntkCreatePi   ( pNtkCube , vMapVars );
   ntkCreateNode ( pNtkCube , vMapVars );
   ntkWriteWcnf  ();
   
   Abc_NtkDelete ( pNtkCube );
   Vec_PtrFree   ( vMapVars );
}

void
SsatSolver::ntkCreatePi( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars ) const
{
   for ( int i = 0 ; i < _rootVars[0].size() ; ++i ) {
      printf( "  > Create Pi for variable %d\n" , _rootVars[0][i]+1 );
      Vec_PtrWriteEntry( vMapVars , _rootVars[0][i] , Abc_NtkCreatePi( pNtkCube ) );
      printf( "  > Var %d map to Obj %p\n" , _rootVars[0][i]+1 , Vec_PtrEntry( vMapVars , _rootVars[0][i] ) );
   }
}

void
SsatSolver::ntkCreateNode( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars ) const
{
   vec<Lit> uLits;
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      Clause & c  = _s1->ca[_s1->clauses[i]];
      uLits.clear();
      for ( int j = 0 ; j < c.size() ; ++j )
         if ( isAVar(var(c[j])) || isRVar(var(c[j])) ) uLits.push(c[j]);
      if ( uLits.size() > 1 ) { // create gate
         printf( "  > Create gate for selection var of clause %d\n" , i );
      }
   }
}

void
SsatSolver::ntkWriteWcnf() const
{
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

