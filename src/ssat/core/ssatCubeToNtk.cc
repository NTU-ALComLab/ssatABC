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
static Abc_Obj_t * Ssat_SopAnd2Obj   ( Abc_Obj_t * , Abc_Obj_t * );
static Abc_Obj_t * Ssat_SopOr2Obj    ( Abc_Obj_t * , Abc_Obj_t * );
static void        Pb_WriteWMCCla    ( FILE * , Abc_Ntk_t * );
static void        Ssat_DumpCubeNtk  ( Abc_Ntk_t * );

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
   Vec_Ptr_t * vMapVars; // mapping Var to Abc_Obj_t
   Abc_Obj_t * pObjCube; // gate of cubes
   
   pNtkCube = Abc_NtkAlloc( ABC_NTK_LOGIC , ABC_FUNC_SOP , 1 );
   sprintf( name , "qesto_cubes_network" );
   pNtkCube->pName = Extra_UtilStrsav( name );
   vMapVars = Vec_PtrStart( _s2->nVars() );

   ntkCreatePi                ( pNtkCube , vMapVars );
   ntkCreateSelDef            ( pNtkCube , vMapVars );
   pObjCube = ntkCreateNode   ( pNtkCube , vMapVars );
   ntkCreatePoCheck           ( pNtkCube , pObjCube );
   
   ntkBddComputeSp            ( pNtkCube );
   ntkWriteWcnf               ( pNtkCube );
   
   Abc_NtkDelete ( pNtkCube );
   Vec_PtrFree   ( vMapVars );
}

void
SsatSolver::ntkCreatePi( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars ) const
{
   Abc_Obj_t * pPi;
   char name[1024];
   for ( int i = 0 ; i < _rootVars[0].size() ; ++i ) {
      printf( "  > Create Pi for randome variable %d\n" , _rootVars[0][i]+1 );
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
      if ( uLits.size() > _rootVars[0].size() ) {
         Abc_Print( -1 , "Clause %d has %d rand lits, more than total(%d)\n" , i , uLits.size() , _rootVars[0].size() );
         exit(1);
      }
      if ( uLits.size() > 1 ) { // create gate for selection var
         pObj = Abc_NtkCreateNode( pNtkCube );
         sprintf( name , "s%d" , i );
         Abc_ObjAssignName( pObj , name , "" );
         for ( int j = 0 ; j < uLits.size() ; ++j ) {
            Abc_ObjAddFanin( pObj , (Abc_Obj_t*)Vec_PtrEntry( vMapVars , var(uLits[j]) ) );
            pfCompl[j] = sign(uLits[j]) ? 0 : 1;
         }
         Abc_ObjSetData( pObj , Abc_SopCreateAnd( (Mem_Flex_t*)pNtkCube->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
         if ( Vec_PtrEntry( vMapVars , var(_claLits[i]) ) ) {
           Abc_Print( -1 , "Selection var for clause %d is redefined\n" , i );
           exit(1); 
         }
         Vec_PtrWriteEntry( vMapVars , var(_claLits[i]) , pObj );
      }
   }
   delete[] pfCompl;
}

Abc_Obj_t*
SsatSolver::ntkCreateNode( Abc_Ntk_t * pNtkCube , Vec_Ptr_t * vMapVars ) const
{
   Abc_Obj_t * pObj , * pObjCube , * pConst0 , * pConst1;
   char name[1024];
   int * pfCompl = new int[Abc_NtkObjNumMax(pNtkCube)];
   pObjCube = NULL;
   pConst0  = Abc_NtkCreateNodeConst0( pNtkCube );
   pConst1  = Abc_NtkCreateNodeConst1( pNtkCube );
   for ( int i = _learntClause.size()-1 ; i > -1 ; --i ) {
      printf( "  > %3d-th learnt clause , type = %s\n" , i , _learntType[i] ? "SAT" : "UNSAT" );
      dumpCla( _learntClause[i] );
      if ( _learntClause[i].size() ) {
         pObj = Abc_NtkCreateNode( pNtkCube );
         sprintf( name , "c%d_%s" , i , _learntType[i] ? "SAT" : "UNSAT" );
         Abc_ObjAssignName( pObj , name , "" );
         for ( int j = 0 ; j < _learntClause[i].size() ; ++j ) {
            Abc_ObjAddFanin( pObj , (Abc_Obj_t*)Vec_PtrEntry( vMapVars , var(_learntClause[i][j]) ) );
            pfCompl[j] = sign(_learntClause[i][j]) ^ _learntType[i] ? 1 : 0;
         }
      }
      else pObj = _learntType[i] ? pConst1 : pConst0;
      if ( _learntType[i] ) { // SAT blocking clause
         Abc_ObjSetData( pObj , Abc_SopCreateAnd( (Mem_Flex_t*)pNtkCube->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
         pObjCube = Ssat_SopOr2Obj( pObjCube , pObj );
      }
      else { // UNSAT conflict clause
         Abc_ObjSetData( pObj , Abc_SopCreateOr( (Mem_Flex_t*)pNtkCube->pManFunc , Abc_ObjFaninNum(pObj) , pfCompl ) );
         pObjCube = Ssat_SopAnd2Obj( pObjCube , pObj );
      }
   }
   delete[] pfCompl;
   return pObjCube;
}

void
SsatSolver::ntkCreatePoCheck( Abc_Ntk_t * pNtkCube , Abc_Obj_t * pObjCube ) const
{
   Abc_Obj_t * pPo = Abc_NtkCreatePo( pNtkCube );
   Abc_ObjAssignName( pPo , "cube_network_Po" , "" );
   Abc_ObjAddFanin( pPo , pObjCube );
   if ( !Abc_NtkCheck( pNtkCube ) ) {
      Abc_Print( -1 , "Something wrong with cubes to network ...\n" );
      Abc_NtkDelete( pNtkCube );
      exit(1);
   }
   Ssat_DumpCubeNtk( pNtkCube );
   //Abc_NtkShow( pNtkCube , 0 , 0 , 1 );
}

/**Function*************************************************************

  Synopsis    [Compute SSAT value by weighted model counting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::ntkWriteWcnf( Abc_Ntk_t * pNtkCube ) const
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

void
SsatSolver::ntkBddComputeSp( Abc_Ntk_t * pNtkCube ) const
{
   Fraig_Params_t Params , * pParams = &Params;
   Abc_Ntk_t * pNtk;
   Abc_Obj_t * pObj;
   int i;
   
   Fraig_ParamsSetDefault( pParams );
   pNtkCube = Abc_NtkStrash ( pNtkCube , 0 , 1 , 0 );
   pNtk     = Abc_NtkFraig  ( pNtkCube , pParams , 0 , 0 );
	Abc_NtkForEachPi( pNtk , pObj , i )
	   pObj->dTemp = (float)_quan[_rootVars[0][i]];

   Pb_BddComputeSp( pNtk , 0 , 0 , 1 );
   Abc_NtkDelete( pNtkCube );
   Abc_NtkDelete( pNtk );
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

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
