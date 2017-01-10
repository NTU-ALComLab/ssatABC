/**CFile****************************************************************

  FileName    [ssatAllSolve.cc]

  SystemName  [ssatQesto]

  Synopsis    [All-SAT enumeration solve]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [10, Jan., 2017]

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

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Solving process entrance]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::aSolve( double range , int cLimit , bool fMini )
{
   _s2 = buildAllSelector();
   assert( isRVar( _rootVars[0][0] ) ); // only support 2SSAT
   return aSolve2SSAT( range , cLimit , fMini );
}

/**Function*************************************************************

  Synopsis    [Build _s2 (All-Sat Lv.1 vars selector)]

  Description [forall vars have exactly the same IDs as _s1]
               
  SideEffects [Initialize as tautology (no clause)]

  SeeAlso     []

***********************************************************************/

Solver*
SsatSolver::buildAllSelector()
{
   Solver * S = new Solver;
   vec<Lit> uLits;
   
   for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
      while ( _rootVars[0][i] >= S->nVars() ) S->newVar();
   return S;
}

/**Function*************************************************************

  Synopsis    [All-Sat 2SSAT solving internal function]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::aSolve2SSAT( double range , int cLimit , bool fMini )
{
   vec<Lit> rLits( _rootVars[0].size() ) , sBkCla;
   abctime clk = Abc_Clock();
   initCubeNetwork( cLimit , true );
   while ( 1.0 - _unsatPb - _satPb > range ) {
      if ( !_s2->solve() ) {
         _unsatPb = cubeToNetwork(false);
         return (_satPb = cubeToNetwork(true));
      }
      for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
         rLits[i] = ( _s2->modelValue(_rootVars[0][i]) == l_True ) ? mkLit(_rootVars[0][i]) : ~mkLit(_rootVars[0][i]);
      if ( !_s1->solve(rLits) ) { // UNSAT case
         if ( fMini ) {
            sBkCla.clear();
            miniUnsatCore( _s1->conflict , sBkCla );
            _unsatClause.push();
            sBkCla.copyTo( _unsatClause.last() );
            _s2->addClause( sBkCla );
         }
         else {
            _unsatClause.push();
            _s1->conflict.copyTo( _unsatClause.last() );
            _s2->addClause( _s1->conflict );
         }
         if ( unsatCubeListFull() ) {
            _unsatPb = cubeToNetwork(false);
            printf( "  > current unsat prob = %f\n" , _unsatPb );
            Abc_PrintTime( 1 , "  > current time" , Abc_Clock() - clk );
            fflush(stdout);
         }
      }
      else { // SAT case
         sBkCla.clear();
         //for ( int i = 0 ; i < rLits.size() ; ++i ) sBkCla.push( ~rLits[i] );
         miniHitSet( rLits , sBkCla );
         _satClause.push();
         sBkCla.copyTo( _satClause.last() );
         _s2->addClause( sBkCla );
         if ( satCubeListFull() ) {
            _satPb = cubeToNetwork(true);
            printf( "  > current sat prob = %f\n" , _satPb );
            Abc_PrintTime( 1 , "  > current time" , Abc_Clock() - clk );
            fflush(stdout);
         }
      }
   }
   return _satPb; // lower bound
}

void
SsatSolver::miniHitSet( const vec<Lit> & minterm , vec<Lit> & sBkCla )
{
#if 1
   //printf( "  > minterm:\n" );
   //dumpCla( minterm );
   vec<bool> drop( _s1->nVars() , false );
   for ( int i = 0 ; i < minterm.size() ; ++i ) {
      drop[var(minterm[i])] = true;
      bool cnfSat = true;
      for ( int j = 0 ; j < _s1->nClauses() ; ++j ) {
         Clause & c = _s1->ca[_s1->clauses[j]];
         bool claSat = false;
         for ( int k = 0 ; k < c.size() ; ++k ) {
            if ( !drop[var(c[k])] && _s1->modelValue(c[k]) == l_True ) {
               claSat = true;
               break;
            }
         }
         if ( !claSat ) {
            cnfSat = false;
            break;
         }
      }
      if ( !cnfSat ) {
         drop[var(minterm[i])] = false;
         sBkCla.push( ~minterm[i] );
      }
   }
#endif
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
