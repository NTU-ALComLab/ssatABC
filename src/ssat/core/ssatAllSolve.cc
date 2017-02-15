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

extern SsatTimer timer;

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
SsatSolver::aSolve( double range , int upper , int lower , bool fMini )
{
   _s2 = buildAllSelector();
   if ( isAVar( _rootVars[0][0] ) ) return aSolve2QBF();
   else                             return aSolve2SSAT( range , upper , lower , fMini );
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
   for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
      while ( _rootVars[0][i] >= S->nVars() ) S->newVar();
   return S;
}

/**Function*************************************************************

  Synopsis    [All-Sat 2QBF solving internal function]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

bool
SsatSolver::aSolve2QBF()
{
   // TODO
   printf( "  > Under construction...\n" );
   return false;
}

/**Function*************************************************************

  Synopsis    [All-Sat 2SSAT solving internal function]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::aSolve2SSAT( double range , int upper , int lower , bool fMini )
{
   vec<Lit> rLits( _rootVars[0].size() ) , sBkCla;
   abctime clk = Abc_Clock();
   initCubeNetwork( upper , lower , true );
   sBkCla.capacity(_rootVars[0].size());
   while ( 1.0 - _unsatPb - _satPb > range ) {
      clk = Abc_Clock();
      if ( !_s2->solve() ) {
         _unsatPb = cubeToNetwork(false);
         //return (_satPb = cubeToNetwork(true));
         return _satPb;
      }
      timer.timeS2 += Abc_Clock()-clk;
      for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
         rLits[i] = ( _s2->modelValue(_rootVars[0][i]) == l_True ) ? mkLit(_rootVars[0][i]) : ~mkLit(_rootVars[0][i]);
      clk = Abc_Clock();
      if ( !_s1->solve(rLits) ) { // UNSAT case
         _unsatClause.push();
         if ( fMini ) {
            sBkCla.clear();
            miniUnsatCore( _s1->conflict , sBkCla );
            sBkCla.copyTo( _unsatClause.last() );
            if ( !sBkCla.size() ) { // FIXME: temp sol for UNSAT matrix
               _unsatPb = 1.0;
               _satPb   = 0.0;
               return _satPb;
            }
            _s2->addClause( sBkCla );
         }
         else {
            _s1->conflict.copyTo( _unsatClause.last() );
            _s2->addClause( _s1->conflict );
         }
#if 0
         if ( unsatCubeListFull() ) {
            //printf( "  > Collect %d UNSAT cubes, convert to network\n" , _upperLimit );
            _unsatPb = cubeToNetwork(false);
            printf( "  > current Upper bound = %e\n" , 1-_unsatPb );
            Abc_PrintTime( 1 , "  > Time elapsed" , Abc_Clock() - clk );
            fflush(stdout);
         }
#endif
      }
      else { // SAT case
         sBkCla.clear();
         miniHitSet( sBkCla , 0 ); // random var at Lv.0
         _satClause.push();
         sBkCla.copyTo( _satClause.last() );
         _s2->addClause( sBkCla );
#if 0
         if ( satCubeListFull() ) {
            //printf( "  > Collect %d SAT cubes, convert to network\n" , _lowerLimit );
            _satPb = cubeToNetwork(true);
            printf( "\t\t\t\t\t\t  > current Lower bound = %e\n" , _satPb );
            Abc_PrintTime( 1 , "\t\t\t\t\t\t  > Time elasped" , Abc_Clock() - clk );
            fflush(stdout);
         }
#endif
      }
      timer.timeS1 += Abc_Clock()-clk;
   }
   return _satPb; // lower bound
}

/**Function*************************************************************

  Synopsis    [Minimum hitting set to generalize SAT solutions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::miniHitSet( vec<Lit> & sBkCla , int randLv ) const
{
   vec<Lit> minterm;
   minterm.capacity(_rootVars[0].size());
   vec<bool> pick( _s1->nVars() , false );
   miniHitOneHotLit  ( sBkCla , pick );
   miniHitCollectLit ( sBkCla , minterm , pick );
   if ( minterm.size() )
      miniHitDropLit ( sBkCla , minterm , pick );
   // sanity check: avoid duplicated lits --> invalid write!
   if ( sBkCla.size() > _rootVars[randLv].size() ) {
      Abc_Print( -1 , "Wrong hitting set!!!\n" );
      dumpCla(sBkCla);
      exit(1);
   }
}

void
SsatSolver::miniHitOneHotLit( vec<Lit> & sBkCla , vec<bool> & pick ) const
{
   Lit lit;
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      Clause & c  = _s1->ca[_s1->clauses[i]];
      short find  = 0;
      bool claSat = false;
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( _s1->modelValue(c[j]) == l_True ) {
            if ( pick[var(c[j])] ) {
               claSat = true;
               break;
            }
            ++find;
            if ( find == 1 ) lit = c[j];
            else break;
         }
      }
      if ( claSat ) continue;
      if ( find == 1 ) {
         pick[var(lit)] = true;
         if ( isRVar(var(lit)) ) sBkCla.push( ~lit );
      }
   }
}

void
SsatSolver::miniHitCollectLit( vec<Lit> & sBkCla , vec<Lit> & minterm , vec<bool> & pick ) const
{
   vec<Lit> rLits , eLits;
   rLits.capacity(8) , eLits.capacity(8);
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      Clause & c  = _s1->ca[_s1->clauses[i]];
      bool claSat = false;
      rLits.clear() , eLits.clear();
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( _s1->modelValue(c[j]) == l_True ) {
            if ( pick[var(c[j])] ) {
               claSat = true;
               break;
            }
            isRVar(var(c[j])) ? rLits.push(c[j]) : eLits.push(c[j]);
         }
      }
      if ( !claSat ) {
         if ( eLits.size() ) {
            pick[var(eLits[0])] = true;
            continue;
         }
         assert( rLits.size() > 1 );
         for ( int j = 0 ; j < rLits.size() ; ++j ) {
            assert( !pick[var(rLits[j])] );
            pick[var(rLits[j])] = true;
            minterm.push(rLits[j]);
         }
      }
   }
}

void
SsatSolver::miniHitDropLit( vec<Lit> & sBkCla , vec<Lit> & minterm , vec<bool> & pick ) const
{
   for ( int i = 0 ; i < minterm.size() ; ++i ) {
      pick[var(minterm[i])] = false;
      bool cnfSat = true;
      for ( int j = 0 ; j < _s1->nClauses() ; ++j ) {
         Clause & c  = _s1->ca[_s1->clauses[j]];
         bool claSat = false;
         for ( int k = 0 ; k < c.size() ; ++k ) {
            if ( pick[var(c[k])] && _s1->modelValue(c[k]) == l_True ) {
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
         pick[var(minterm[i])] = true;
         sBkCla.push( ~minterm[i] );
      }
   }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
