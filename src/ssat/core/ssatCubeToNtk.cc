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

// external functions from ABC
extern "C" {
   void        Abc_NtkShow     ( Abc_Ntk_t * , int , int , int );
   Abc_Ntk_t * Abc_NtkDarToCnf ( Abc_Ntk_t * , char * , int , int , int );
   int         Abc_NtkDSat     ( Abc_Ntk_t * , ABC_INT64_T , ABC_INT64_T , int , int , int , int , int , int , int );
};
// helper functions
Abc_Obj_t * Ssat_SopAnd2Obj   ( Abc_Obj_t * , Abc_Obj_t * );
void        Ssat_DumpCubeNtk  ( Abc_Ntk_t * );
static Abc_Obj_t * Ssat_SopOr2Obj    ( Abc_Obj_t * , Abc_Obj_t * );
static void        Pb_WriteWMCCla    ( FILE * , Abc_Ntk_t * );

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
SsatSolver::initCubeNetwork( bool fAll )
{
   char name[32];
   _pNtkCube = Abc_NtkAlloc( ABC_NTK_LOGIC , ABC_FUNC_SOP , 1 );
   sprintf( name , "qesto_cubes_network" );
   _pNtkCube->pName = Extra_UtilStrsav( name );
   _vMapVars = Vec_PtrStart( _s2->nVars() );
   ntkCreatePi( _pNtkCube , _vMapVars ); 
   ntkCreatePo( _pNtkCube ); 
   if ( !fAll ) ntkCreateSelDef ( _pNtkCube , _vMapVars );
}

double
SsatSolver::cubeToNetwork( bool sat )
{
   vec< vec<Lit> > & learntClause = sat ? _satClause : _unsatClause;
   if ( !learntClause.size() ) return (sat ? _satPb : _unsatPb);
   
   Abc_Obj_t * pObjCube = ntkCreateNode( _pNtkCube , _vMapVars , sat );
   ntkPatchPoCheck( _pNtkCube ,  pObjCube , sat );
   learntClause.clear();
   return ntkBddComputeSp( _pNtkCube , sat );
}

void
SsatSolver::ntkCreatePi( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars )
{
   Abc_Obj_t * pPi;
   char name[1024];
   for ( int i = 0 ; i < _rootVars[0].size() ; ++i ) {
      pPi = Abc_NtkCreatePi( pNtkCube );
      sprintf( name , "r%d" , _rootVars[0][i]+1 );
      Abc_ObjAssignName( pPi , name , "" );
      Vec_PtrWriteEntry( vMapVars , _rootVars[0][i] , pPi );
   }
}

void
SsatSolver::ntkCreatePo( Abc_Ntk_t * pNtkCube )
{
   Abc_Obj_t * pPo;
   _pConst0  = Abc_NtkCreateNodeConst0( pNtkCube );
   _pConst1  = Abc_NtkCreateNodeConst1( pNtkCube );
   Abc_ObjAssignName( pPo = Abc_NtkCreatePo(pNtkCube) , "unsat_cubes" , "" );
   Abc_ObjAddFanin( pPo , _pConst0 );
   Abc_ObjAssignName( pPo = Abc_NtkCreatePo(pNtkCube) , "sat_cubes"   , "" );
   Abc_ObjAddFanin( pPo , _pConst0 );
}

void
SsatSolver::ntkCreateSelDef( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars )
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
      if ( uLits.size() > 1 ) { // create gate for selection var
         pObj = Abc_NtkCreateNode( pNtkCube );
         sprintf( name , "s%d" , i );
         Abc_ObjAssignName( pObj , name , "" );
         for ( int j = 0 ; j < uLits.size() ; ++j ) {
            Abc_ObjAddFanin( pObj , (Abc_Obj_t*)Vec_PtrEntry( vMapVars , var(uLits[j]) ) );
            pfCompl[j] = sign(uLits[j]) ? 0 : 1;
         }
         Abc_ObjSetData( pObj , Abc_SopCreateAnd( (Mem_Flex_t*)pNtkCube->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
         Vec_PtrWriteEntry( vMapVars , var(_claLits[i]) , pObj );
      }
   }
   delete[] pfCompl;
}

Abc_Obj_t*
SsatSolver::ntkCreateNode( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars , bool sat )
{
   vec< vec<Lit> > & learntClause = sat ? _satClause : _unsatClause;
   Abc_Obj_t * pObj , * pObjCube;
   char name[1024];
   int * pfCompl = new int[_s2->nVars()];
   pObjCube = NULL;
   for ( int i = 0 ; i < learntClause.size() ; ++i ) {
      if ( learntClause[i].size() ) {
         assert( learntClause[i].size() <= _s2->nVars() );
         pObj = Abc_NtkCreateNode( pNtkCube );
         sprintf( name , "c_%s_%d" , sat ? "SAT" : "UNSAT" , Abc_NtkObjNum(pNtkCube) );
         Abc_ObjAssignName( pObj , name , "" );
         for ( int j = 0 ; j < learntClause[i].size() ; ++j ) {
            Abc_ObjAddFanin( pObj , (Abc_Obj_t*)Vec_PtrEntry( vMapVars , var(learntClause[i][j]) ) );
            pfCompl[j] = sign(learntClause[i][j]) ? 0 : 1;
         }
         Abc_ObjSetData( pObj , Abc_SopCreateAnd( (Mem_Flex_t*)pNtkCube->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
      }
      else pObj = sat ? _pConst1 : _pConst0;
      pObjCube = Ssat_SopOr2Obj( pObjCube , pObj );
   }
   delete[] pfCompl;
   return pObjCube;
}

void
SsatSolver::ntkPatchPoCheck( Abc_Ntk_t * pNtkCube , Abc_Obj_t * pObjCube , bool sat )
{
   Abc_Obj_t * pPo = sat ? Abc_NtkPo( pNtkCube , 1 ) : Abc_NtkPo( pNtkCube , 0 );
   if ( !Abc_ObjFaninNum( pPo ) ) // add fanin
      Abc_ObjAddFanin( pPo , pObjCube );
   else // OR previous output with pObjCube
      Abc_ObjPatchFanin( pPo , Abc_ObjFanin0(pPo) , Ssat_SopOr2Obj( Abc_ObjFanin0(pPo) , pObjCube ) );
   if ( !Abc_NtkCheck( pNtkCube ) ) {
      Abc_Print( -1 , "Something wrong with cubes to network ...\n" );
      Abc_NtkDelete( pNtkCube );
      exit(1);
   }
}

/**Function*************************************************************

  Synopsis    [Compute SSAT value by weighted model counting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::ntkWriteWcnf( Abc_Ntk_t * pNtkCube )
{
   // FIXME: Known bug with constant pNtkCube, Id+1??
   FILE * out;
   Abc_Ntk_t * pNtk;
   Abc_Obj_t * pObj;
   int nVars , nClas , i;
   
   Fraig_Params_t Params , * pParams = &Params;
   Fraig_ParamsSetDefault( pParams );
   pNtkCube = Abc_NtkStrash ( pNtkCube , 0 , 1 , 0 );
   pNtk     = Abc_NtkFraig  ( pNtkCube , pParams , 0 , 0 );
   //Abc_NtkDarToCnf( pNtk , "cubeNtk.cnf" , 0 , 0 , 0 );
   
   out      = fopen( "cubeNtk.wcnf" , "w" ); 
	nVars    = Abc_NtkPiNum(pNtk) + Abc_NtkNodeNum(pNtk) + 1; // only 1 Po
	nClas    = 3 * Abc_NtkNodeNum( pNtk ) + 2 + 1; // 2 for Po connection , 1 for Po assertion
	fprintf( out , "p cnf %d %d\n" , nVars , nClas );
   
	Pb_WriteWMCCla( out , pNtk );
	Abc_NtkForEachPi( pNtk , pObj , i )
	   fprintf( out , "w %d %f\n" , Abc_ObjId(pObj) , _quan[_rootVars[0][i]] );
	Abc_NtkForEachNode( pNtk , pObj , i )
	   fprintf( out , "w %d %d\n" , Abc_ObjId(pObj) , -1 );
	Abc_NtkForEachPo( pNtk , pObj , i )
	   fprintf( out , "w %d %d\n" , Abc_ObjId(pObj) , -1 );
   
   Abc_NtkDelete( pNtkCube );
   Abc_NtkDelete( pNtk );
   fclose( out );
}

void
Pb_WriteWMCCla( FILE * out , Abc_Ntk_t * pNtk )
{
	Abc_Obj_t * pObj , * pFanin0 , * pFanin1;
	int i;

	Abc_NtkForEachNode( pNtk , pObj , i )
	{
		pFanin0 = Abc_ObjFanin0( pObj );
		pFanin1 = Abc_ObjFanin1( pObj );
		if ( !Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj) ) {
			fprintf( out , "%d -%d -%d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) , Abc_ObjId(pFanin1) );
			fprintf( out , "-%d %d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) );
			fprintf( out , "-%d %d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin1) );
		}
		if ( Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj) ) {
			fprintf( out , "%d %d -%d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) , Abc_ObjId(pFanin1) );
			fprintf( out , "-%d -%d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) );
			fprintf( out , "-%d %d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin1) );
		}
		if ( !Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj) ) {
			fprintf( out , "%d -%d %d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) , Abc_ObjId(pFanin1) );
			fprintf( out , "-%d %d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) );
			fprintf( out , "-%d -%d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin1) );
		}
		if ( Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj) ) {
			fprintf( out , "%d %d %d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) , Abc_ObjId(pFanin1) );
			fprintf( out , "-%d -%d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin0) );
			fprintf( out , "-%d -%d 0\n" , Abc_ObjId(pObj) , Abc_ObjId(pFanin1) );
		}
	}
	pObj    = Abc_NtkPo( pNtk , 0 );
	pFanin0 = Abc_ObjFanin0( pObj );
	if ( !Abc_ObjFaninC0( pObj ) ) {
	   fprintf( out , "%d -%d 0\n" , Abc_ObjId( pObj ) , Abc_ObjId( pFanin0 ) );
	   fprintf( out , "-%d %d 0\n" , Abc_ObjId( pObj ) , Abc_ObjId( pFanin0 ) );
	}
	else {
	   fprintf( out , "%d %d 0\n"   , Abc_ObjId( pObj ) , Abc_ObjId( pFanin0 ) );
	   fprintf( out , "-%d -%d 0\n" , Abc_ObjId( pObj ) , Abc_ObjId( pFanin0 ) );
	}
	fprintf( out , "%d 0\n" , Abc_ObjId( pObj ) );
}

/**Function*************************************************************

  Synopsis    [Compute SSAT value by BDD signal probability.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// FIXME: disable fraig due to complexity
// SOP to BDD ?? (not via AIG)
// in Abc_NtkCollapse, always strash ...
double
SsatSolver::ntkBddComputeSp( Abc_Ntk_t * pNtkCube , bool sat )
{
   //Fraig_Params_t Params , * pParams = &Params;
   Abc_Ntk_t * pNtk , * pNtkCopy , * pNtkAig;
   Abc_Obj_t * pObj;
   double prob;
   int i;
   
   pNtkCopy = Abc_NtkDup( pNtkCube );
   //Fraig_ParamsSetDefault( pParams );
   pNtkAig = Abc_NtkStrash ( pNtkCopy , 0 , 1 , 0 );
   //pNtk    = Abc_NtkFraig  ( pNtkAig , pParams , 0 , 0 );
	pNtk = pNtkAig;
   Abc_NtkForEachPi( pNtk , pObj , i )
	   pObj->dTemp = (float)_quan[_rootVars[0][i]];

   prob = (double)Pb_BddComputeSp( pNtk , (int)sat , 0 , 0 , 0 );
   Abc_NtkDelete( pNtkCopy );
   //Abc_NtkDelete( pNtkAig );
   Abc_NtkDelete( pNtk );
   return prob;
}

/**Function*************************************************************

  Synopsis    [Helper functions to manipulate SOP gates.]

  Description [Return pObj2 if pObj1 == NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Abc_Obj_t*
Ssat_SopAnd2Obj( Abc_Obj_t * pObj1 , Abc_Obj_t * pObj2 )
{
   assert( pObj2 );
   if ( !pObj1 ) return pObj2;
   assert( Abc_ObjNtk(pObj1) == Abc_ObjNtk(pObj2) );

   Abc_Obj_t * pObjAnd;
   Vec_Ptr_t * vFanins = Vec_PtrStart( 2 );
   Vec_PtrWriteEntry( vFanins , 0 , pObj1 );
   Vec_PtrWriteEntry( vFanins , 1 , pObj2 );
   pObjAnd = Abc_NtkCreateNodeAnd( Abc_ObjNtk(pObj1) , vFanins );
   Vec_PtrFree( vFanins );
   return pObjAnd;
}

Abc_Obj_t*
Ssat_SopOr2Obj( Abc_Obj_t * pObj1 , Abc_Obj_t * pObj2 )
{
   assert( pObj2 );
   if ( !pObj1 ) return pObj2;
   assert( Abc_ObjNtk(pObj1) == Abc_ObjNtk(pObj2) );

   Abc_Obj_t * pObjOr;
   Vec_Ptr_t * vFanins = Vec_PtrStart( 2 );
   Vec_PtrWriteEntry( vFanins , 0 , pObj1 );
   Vec_PtrWriteEntry( vFanins , 1 , pObj2 );
   pObjOr = Abc_NtkCreateNodeOr( Abc_ObjNtk(pObj1) , vFanins );
   Vec_PtrFree( vFanins );
   return pObjOr;
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

/**Function*************************************************************

  Synopsis    [Construct circuits from clauses.]

  Description [For ER-2SSAT.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::initClauseNetwork()
{
   Abc_Obj_t * pObj;
   int i;
   char name[32];
   _pNtkCube = Abc_NtkAlloc( ABC_NTK_LOGIC , ABC_FUNC_SOP , 1 );
   sprintf( name , "er_clauses_network" );
   _pNtkCube->pName = Extra_UtilStrsav( name );
   _vMapVars = Vec_PtrStart( _s1->nVars() );
   erNtkCreatePi( _pNtkCube , _vMapVars ); 
   _pNtkAig = Abc_NtkStartFrom( _pNtkCube , ABC_NTK_STRASH , ABC_FUNC_AIG );
   Abc_NtkForEachPi( _pNtkAig , pObj , i )
	   pObj->dTemp = ( i < _rootVars[1].size() ) ? (float)_quan[_rootVars[1][i]] : -1.0;
   _dd = erInitCudd( _pNtkAig , _rootVars[1].size() , 1 );
   erNtkCreatePo( _pNtkCube ); 
}

void
SsatSolver::erNtkCreatePi( Abc_Ntk_t * pNtkClause , Vec_Ptr_t * vMapVars )
{
   Abc_Obj_t * pPi;
   char name[1024];
   for ( int i = 0 ; i < _rootVars[1].size() ; ++i ) {
      pPi = Abc_NtkCreatePi( pNtkClause );
      sprintf( name , "r%d" , _rootVars[1][i]+1 );
      Abc_ObjAssignName( pPi , name , "" );
      Vec_PtrWriteEntry( vMapVars , _rootVars[1][i] , pPi );
   }
   if ( _numLv == 2 ) return;
   else if ( _numLv == 3 ) {
      for ( int i = 0 ; i < _rootVars[2].size() ; ++i ) {
         pPi = Abc_NtkCreatePi( pNtkClause );
         sprintf( name , "e%d" , _rootVars[2][i]+1 );
         Abc_ObjAssignName( pPi , name , "" );
         Vec_PtrWriteEntry( vMapVars , _rootVars[2][i] , pPi );
      }
   }
   else {
      Abc_Print( -1 , "Unexpected number of levels (%d)\n" , _numLv );
      exit(1);
   }
}

void
SsatSolver::erNtkCreatePo( Abc_Ntk_t * pNtkClause )
{
   Abc_ObjAssignName( Abc_NtkCreatePo(pNtkClause) , "clause_output" , "" );
}

double
SsatSolver::clauseToNetwork( const vec<Lit> & eLits , int dropIndex )
{
   Abc_Obj_t * pObj , * pObjCla;
   int i;
   Abc_NtkForEachNode( _pNtkCube , pObj , i ) Abc_NtkDeleteObj( pObj );
   pObjCla = erNtkCreateNode( _pNtkCube , _vMapVars , eLits , dropIndex );
   if ( pObjCla ) {
      erNtkPatchPoCheck( _pNtkCube , pObjCla );
      return erNtkBddComputeSp( _pNtkCube );
   }
   return 1.0;
}

Abc_Obj_t*
SsatSolver::erNtkCreateNode( Abc_Ntk_t * pNtkClause , Vec_Ptr_t * vMapVars , const vec<Lit> & eLits , int dropIndex )
{
   vec<bool> drop( _s1->nVars() , false );
   Abc_Obj_t * pObj , * pObjCla = NULL;
   int * pfCompl = new int[_s1->nVars()];
   char name[1024];
   bool select;

   for ( int i = dropIndex ; i < eLits.size() ; ++i ) drop[var(eLits[i])] = true;
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      select = true;
      Clause & c = _s1->ca[_s1->clauses[i]];
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( drop[var(c[j])] || isEVar(var(c[j])) && _level[var(c[j])] == 0 && _s1->modelValue(c[j]) == l_True ) {
            select = false;
            break;
         }
      }
      if ( select ) {
         pObj = Abc_NtkCreateNode( pNtkClause );
         sprintf( name , "c%d" , i );
         Abc_ObjAssignName( pObj , name , "" );
         for ( int j = 0 ; j < c.size() ; ++j ) {
            if ( _level[var(c[j])] && _s1->value(c[j]) != l_False ) {
               Abc_ObjAddFanin( pObj , (Abc_Obj_t*)Vec_PtrEntry( vMapVars , var(c[j]) ) );
               pfCompl[Abc_ObjFaninNum(pObj)-1] = sign(c[j]) ? 1 : 0;
            }
         }
         Abc_ObjSetData( pObj , Abc_SopCreateOr( (Mem_Flex_t*)pNtkClause->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
         pObjCla = Ssat_SopAnd2Obj( pObjCla , pObj );
      }
   }
   delete[] pfCompl;
   return pObjCla;
}

void
SsatSolver::erNtkPatchPoCheck( Abc_Ntk_t * pNtkClause , Abc_Obj_t * pObjCla )
{
   assert( !Abc_ObjFaninNum( Abc_NtkPo( pNtkClause , 0 ) ) );
   Abc_ObjAddFanin( Abc_NtkPo( pNtkClause , 0 ) , pObjCla );
   if ( !Abc_NtkCheck( pNtkClause ) ) {
      Abc_Print( -1 , "Something wrong with clauses to network ...\n" );
      Abc_NtkDelete( pNtkClause );
      exit(1);
   }
}

/**Function*************************************************************

  Synopsis    [Compute SSAT value by BDD signal probability.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::erNtkBddComputeSp( Abc_Ntk_t * pNtkClause )
{
   Abc_Ntk_t * pNtkCopy , * pNtkAig;
   Abc_Obj_t * pObj;
   double prob;
   int i;
   
   pNtkCopy = Abc_NtkDup( pNtkClause );
   pNtkAig  = Abc_NtkStrash( pNtkCopy , 0 , 1 , 0 );
   erNtkMergeIntoAig( pNtkAig );
   
   //prob = (double)Pb_BddComputeRESp( pNtkAig , 0 , _rootVars[1].size() , 1 , 0 );
   Abc_NtkDelete( pNtkCopy );
   Abc_NtkDelete( pNtkAig );
   //return prob;
   return 0.0;
}

void
SsatSolver::erNtkMergeIntoAig( Abc_Ntk_t * pNtkMerge )
{
   Abc_Obj_t * pObj;
   int i;
   
   Abc_AigConst1( pNtkMerge )->pCopy = Abc_AigConst1( _pNtkAig );
   Abc_NtkForEachPi( pNtkMerge , pObj , i )
      pObj->pCopy = Abc_NtkPi( _pNtkAig , i );
   Abc_NtkPo( pNtkMerge , 0 )->pCopy = Abc_NtkCreatePo( _pNtkAig );
   Abc_AigForEachAnd( pNtkMerge , pObj , i )
      pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)_pNtkAig->pManFunc , Abc_ObjChild0Copy(pObj) , Abc_ObjChild1Copy(pObj) );
   Abc_ObjAddFanin( Abc_NtkPo(pNtkMerge,0)->pCopy , Abc_ObjChild0Copy(Abc_NtkPo(pNtkMerge,0)) );
}

DdManager*
SsatSolver::erInitCudd( Abc_Ntk_t * pNtk , int numRand , int fGrp )
{
    Abc_Obj_t * pObj;
    Vec_Att_t * pAttMan;
    DdManager * dd;
    DdNode * bFunc;
    int nBddSizeMax , fReorder , fDropInternal , fVerbose , i;

    // set defaults
	 nBddSizeMax   = ABC_INFINITY;
	 fReorder      = ( numRand < Abc_NtkPiNum( pNtk ) ) ? 0 : 1;
	 fDropInternal = 1;
	 fVerbose      = 0;
    // remove dangling nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );
    // start the manager
    assert( !Abc_NtkGlobalBdd( pNtk ) );
    dd = Cudd_Init( Abc_NtkPiNum( pNtk ) , 0 , CUDD_UNIQUE_SLOTS , CUDD_CACHE_SLOTS , 0 );
    // NZ : group variables
    if ( numRand < Abc_NtkPiNum( pNtk ) && fGrp ) {
       //printf( "  >  Start grouping random and exist variables\n" );
       Cudd_MakeTreeNode( dd , 0 , numRand , MTR_DEFAULT );
       Cudd_MakeTreeNode( dd , numRand , Abc_NtkPiNum(pNtk)-numRand , MTR_DEFAULT );
       fReorder = 1;
       //printf( "  >  Grouping done\n" );
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
          Abc_ObjSetGlobalBdd( pObj , bFunc );  
			 Cudd_Ref( bFunc );
       }
	 }
    return dd;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
