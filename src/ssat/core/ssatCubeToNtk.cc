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

static void Ssat_DumpCubeNtk( Abc_Ntk_t * );

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

   ntkCreatePi     ( pNtkCube , vMapVars );
   ntkCreateSelDef ( pNtkCube , vMapVars );
   ntkCreateNode   ( pNtkCube , vMapVars );
   ntkWriteWcnf    ();
   
   Abc_NtkDelete ( pNtkCube );
   Vec_PtrFree   ( vMapVars );
}

void
SsatSolver::ntkCreatePi( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars ) const
{
   Abc_Obj_t * pPi;
   char name[1024];
   for ( int i = 0 ; i < _rootVars[0].size() ; ++i ) {
      printf( "  > Create Pi for variable %d\n" , _rootVars[0][i]+1 );
      pPi = Abc_NtkCreatePi( pNtkCube );
      sprintf( name , "r%d" , _rootVars[0][i]+1 );
      Abc_ObjAssignName( pPi , name , "" );
      Vec_PtrWriteEntry( vMapVars , _rootVars[0][i] , pPi );
      printf( "  > Var %d map to Obj %s\n" , _rootVars[0][i]+1 , Abc_ObjName(pPi) );
   }
}

void
SsatSolver::ntkCreateSelDef( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars ) const
{
   Abc_Obj_t * pObj;
   vec<Lit> uLits;
   char name[1024];
   int * pfCompl = new int[_rootVars[0].size()];
   
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      Clause & c  = _s1->ca[_s1->clauses[i]];
      uLits.clear();
      for ( int j = 0 ; j < c.size() ; ++j )
         if ( isAVar(var(c[j])) || isRVar(var(c[j])) ) uLits.push(c[j]);
      if ( uLits.size() > 1 ) { // create gate
         printf( "  > Create gate for selection var of clause %d\n" , i );
         pObj = Abc_NtkCreateNode( pNtkCube );
         sprintf( name , "s%d" , i );
         Abc_ObjAssignName( pObj , name , "" );
         for ( int j = 0 ; j < uLits.size() ; ++j ) {
            Abc_ObjAddFanin( pObj , (Abc_Obj_t*)Vec_PtrEntry( vMapVars , var(uLits[j]) ) );
            pfCompl[j] = sign(uLits[j]) ? 0 : 1;
         }
         Abc_ObjSetData( pObj, Abc_SopCreateAnd( (Mem_Flex_t *)pNtkCube->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
         Vec_PtrWriteEntry( vMapVars , var(_claLits[i]) , pObj );
      }
   }
   Ssat_DumpCubeNtk( pNtkCube );
   delete[] pfCompl;
}

void
SsatSolver::ntkCreateNode( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars ) const
{
}

void
SsatSolver::ntkWriteWcnf() const
{
}

/**Function*************************************************************

  Synopsis    [Dump constructed circuits for debug.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
Ssat_DumpCubeNtk( Abc_Ntk_t * pNtk )
{
   Abc_Obj_t * pObj , * pFanin;
   int i , j;
   printf( "\n  > print nodes in the cube network\n" );
   Abc_NtkForEachNode( pNtk , pObj , i )
   {
      printf( ".names" );
      Abc_ObjForEachFanin( pObj , pFanin , j )
         printf( " %s" , Abc_ObjName( pFanin ) );
      printf( " %s\n%s" , Abc_ObjName(pObj), (char*)Abc_ObjData(pObj) );
   }
   printf( "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
