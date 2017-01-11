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
            printf( "  > Collect %d UNSAT cubes, convert to network\n" , _cubeLimit );
            _unsatPb = cubeToNetwork(false);
            printf( "  > current unsat prob = %f\n" , _unsatPb );
            Abc_PrintTime( 1 , "  > current time" , Abc_Clock() - clk );
            fflush(stdout);
         }
      }
      else { // SAT case
         sBkCla.clear();
         miniHitSet( rLits , sBkCla );
         _satClause.push();
         sBkCla.copyTo( _satClause.last() );
         _s2->addClause( sBkCla );
         if ( satCubeListFull() ) {
            printf( "  > Collect %d SAT cubes, convert to network\n" , _cubeLimit );
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
SsatSolver::miniHitSet( const vec<Lit> & model , vec<Lit> & sBkCla )
{
   //printf( "  > original model:\n" );
   //dumpCla( model );
   vec<Lit> rLits , eLits;
   rLits.capacity(32);
   eLits.capacity(32);
   vec<short> pick( _s1->nVars() , 0 ); // 1:picked ; 0:undecided ; -1:dropped
   Lit lit;
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      short find = 0;
      Clause & c = _s1->ca[_s1->clauses[i]];
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( _s1->modelValue(c[j]) == l_True ) {
            ++find;
            if ( find == 1 ) lit  = c[j];
            else break;
         }
      }
      if ( find == 1 && pick[var(lit)] != 1 ) {
         pick[var(lit)] = 1;
         if ( isRVar(var(lit)) ) sBkCla.push( ~lit );
      }
   }
   //printf( "  > must pick:\n" );
   //dumpCla( sBkCla );
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      bool picked = false;
      rLits.clear();
      eLits.clear();
      Clause & c = _s1->ca[_s1->clauses[i]];
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( _s1->modelValue(c[j]) == l_True ) {
            if ( pick[var(c[j])] ) {
               picked = true;
               break;
            }
            if ( isRVar(var(c[j])) ) rLits.push(c[j]);
            else                     eLits.push(c[j]);
         }
      }
      if ( !picked ) {
         if ( eLits.size() ) {
            pick[var(eLits[0])] = 1;
            continue;
         }
         assert( rLits.size() > 1 );
         pick[var(rLits[0])] = 1;
         sBkCla.push( ~rLits[0] );
      }
   }
   if ( sBkCla.size() > _rootVars[0].size() ) {
      Abc_Print( -1 , "Wrong hitting set!!!\n" );
      dumpCla(model);
      dumpCla(sBkCla);
      exit(1);
   }
   //printf( "  > minimal set:\n" );
   //dumpCla( sBkCla );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
