/**CFile****************************************************************

  FileName    [ssatBddSolve.cc]

  SystemName  [ssatABC]

  Synopsis    [Solve general SSAT by BDD]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [16, Mar., 2017]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdio.h>
#include "ssat/utils/ParseUtils.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/core/Solver.h"
#include "ssat/core/Dimacs.h"
#include "ssat/core/SsatSolver.h"

using namespace Minisat;
using namespace std;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Abc_Obj_t * Ssat_SopAnd2Obj   ( Abc_Obj_t * , Abc_Obj_t * );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::bddSolveSsat( bool fGroup , bool fReorder )
{
   if ( _fVerbose ) {
      printf( "  > grouping = %s , reordering = %s\n" , fGroup?"yes":"no", fReorder?"yes":"no" );
      printf( "  > Under construction ...\n" );
   }
   // TODO
   initCnfNetwork();
   buildBddFromNtk( fGroup , fReorder );
}

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::initCnfNetwork()
{
   Abc_Obj_t * pObjCla , * pObj;
   char name[32];
   int i;
   _pNtkCnf = Abc_NtkAlloc( ABC_NTK_LOGIC , ABC_FUNC_SOP , 1 );
   sprintf( name , "root_clause_ntk" );
   _pNtkCnf->pName = Extra_UtilStrsav( name );
   _varToPi.growTo( _s1->nVars() , -1 );
   cnfNtkCreatePi   ( _pNtkCnf , _varToPi ); 
   //Abc_NtkForEachPi( _pNtkCnf , pObj , i ) printf( "  > Id = %d\n" , Abc_ObjId(pObj) );
   pObjCla = cnfNtkCreateNode ( _pNtkCnf , _varToPi ); 
   cnfNtkCreatePo   ( _pNtkCnf , pObjCla ); 
   if ( !Abc_NtkCheck( _pNtkCnf ) ) {
      Abc_Print( -1 , "Something wrong with cnf to network ...\n" );
      Abc_NtkDelete( _pNtkCnf );
      exit(1);
   }
}

void
SsatSolver::cnfNtkCreatePi( Abc_Ntk_t * pNtkCnf , vec<int> & varToPi )
{
   Abc_Obj_t * pPi;
   char name[1024];
   for ( int i = 0 ; i < _rootVars.size() ; ++i ) {
      for ( int j = 0 ; j < _rootVars[i].size() ; ++j ) {
         pPi = Abc_NtkCreatePi( pNtkCnf );
         sprintf( name , "var_%d_%d" , _rootVars[i][j] , Abc_NtkPiNum(pNtkCnf)-1 );
         Abc_ObjAssignName( pPi , name , "" );
         //printf( "  > var %d --> id %d\n" , _rootVars[i][j] , Abc_ObjId(pPi) );
         varToPi[_rootVars[i][j]] = Abc_NtkPiNum(pNtkCnf)-1;
      }
   }
}

Abc_Obj_t*
SsatSolver::cnfNtkCreateNode( Abc_Ntk_t * pNtkCnf , const vec<int> & varToPi )
{
   Abc_Obj_t * pObj , * pObjCla = NULL;
   int * pfCompl = new int[_s1->nVars()];
   char name[1024];

   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      Clause & c = _s1->ca[_s1->clauses[i]];
      pObj = Abc_NtkCreateNode( pNtkCnf );
      sprintf( name , "c%d" , i );
      Abc_ObjAssignName( pObj , name , "" );
      for ( int j = 0 ; j < c.size() ; ++j ) {
         //printf( "  > add var %d as fanin , id = %d\n" , var(c[j]) , varToPi[var(c[j])] );
         Abc_ObjAddFanin( pObj , Abc_NtkPi( pNtkCnf , varToPi[var(c[j])] ) );
         pfCompl[Abc_ObjFaninNum(pObj)-1] = sign(c[j]) ? 1 : 0;
      }
      Abc_ObjSetData( pObj , Abc_SopCreateOr( (Mem_Flex_t*)pNtkCnf->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
      pObjCla = Ssat_SopAnd2Obj( pObjCla , pObj );
   } 
   delete[] pfCompl;
   return pObjCla;
}

void
SsatSolver::cnfNtkCreatePo( Abc_Ntk_t * pNtkCnf , Abc_Obj_t * pObjCla )
{
   Abc_ObjAssignName( Abc_NtkCreatePo(pNtkCnf) , "clause_output" , "" );
   Abc_ObjAddFanin( Abc_NtkPo( pNtkCnf , 0 ) , pObjCla );
}

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::buildBddFromNtk( bool fGroup , bool fReorder )
{
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
