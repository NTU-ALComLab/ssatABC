/**CFile****************************************************************

  FileName    [SsatERSolver.cc]

  SystemName  [ssatQesto]

  Synopsis    [Implementations of member functions for ssatSolver]

  Author      []
  
  Affiliation [NTU]

  Date        [14, Jan., 2017]

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

  Synopsis    [Solve ER or ERE type of SSAT.]

  Description [Use BDD or Cachet to evaluate weight.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::erSolve2SSAT( bool fBdd )
{
   _s1->simplify();
   _s2 = buildERSelector();
   if ( fBdd ) initClauseNetwork();
   
   vec<Lit> eLits( _rootVars[0].size() ) , sBkCla;
   double subvalue;
   _erModel.capacity( _rootVars[0].size() ); _erModel.clear();
   abctime clk = Abc_Clock();

   for(;;) {
      if ( !_s2->solve() ) {
         printf( "  > optimizing assignment to exist vars:\n" );
         dumpCla( _erModel );
         return _satPb;
      }
      for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
         eLits[i] = ( _s2->modelValue(_rootVars[0][i]) == l_True ) ? mkLit(_rootVars[0][i]) : ~mkLit(_rootVars[0][i]);
      if ( !_s1->solve(eLits) ) { // UNSAT case
         sBkCla.clear();
         miniUnsatCore( _s1->conflict , sBkCla );
         _s2->addClause( sBkCla );
      }
      else { // SAT case
         if ( fBdd ) {
            subvalue = clauseToNetwork( eLits );
         }
         else subvalue = countModels( eLits );
         if ( subvalue > _satPb ) {
            printf( "  > find a better solution!\n" );
            Abc_PrintTime( 1, "  > Time consumed" , Abc_Clock() - clk );
            _satPb = subvalue;
            eLits.copyTo( _erModel );
         }
         sBkCla.clear();
         collectBkClaER( sBkCla );
         _s2->addClause( sBkCla );
      }
   }
   return _satPb;
}

Solver*
SsatSolver::buildERSelector()
{
   return buildAllSelector();
}

void
SsatSolver::collectBkClaER( vec<Lit> & sBkCla )
{
   bool block;
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      block = true;
      Clause & c = _s1->ca[_s1->clauses[i]];
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( isEVar(var(c[j])) && _level[var(c[j])] == 0 && _s1->modelValue(c[j]) == l_True ) {
            block = false;
            break;
         }
      }
      if ( block ) {
         for ( int j = 0 ; j < c.size() ; ++j ) {
            if ( isEVar(var(c[j])) && _level[var(c[j])] == 0 )
               sBkCla.push (c[j]);
         }
      }
   }
}

/**Function*************************************************************

  Synopsis    [Counting models under an exist assignment]

  Description []
               
  SideEffects [Cachet: variables ids must be consecutive!]

  SeeAlso     []

***********************************************************************/

double
SsatSolver::countModels( const vec<Lit> & sBkCla )
{
   FILE * file;
   int length = 1024;
   char prob_str[length] , cmdModelCount[length];

   // vec<Lit> assump( sBkCla.size() );
   // for ( int i = 0 ; i < sBkCla.size() ; ++i ) assump[i] = ~sBkCla[i];
   
   // toDimacsWeighted( "temp.wcnf" , assump );
   toDimacsWeighted( "temp.wcnf" , sBkCla );
   sprintf( cmdModelCount , "bin/cachet temp.wcnf > tmp.log");
   if ( system( cmdModelCount ) ) {
      fprintf( stderr , "Error! Problems with cachet execution...\n" );
      exit(1);
   }

   sprintf( cmdModelCount , "cat tmp.log | grep \"Satisfying\" | awk '{print $3}' > satProb.log" );
   system( cmdModelCount );
   
   file = fopen( "satProb.log" , "r" );
   if ( file == NULL ) {
      fprintf( stderr , "Error! Problems with reading probability from \"satProb.log\"\n" );
      exit(1);
   }
   fgets( prob_str , length , file );
   fclose( file );
   return atof(prob_str);
}

/**Function*************************************************************

  Synopsis    [SsatSolve write to cnf file with weighted variables]

  Description [For Weighted Model Counting use.]
               
  SideEffects [Using S to switch which solver to write.]

  SeeAlso     []

***********************************************************************/

static Var
mapVar( Var x , vec<Var> & map , Var & max )
{
   if ( map.size() <= x || map[x] == -1) {
      map.growTo( x + 1 , -1 );
      map[x] = max++;
   }
   return map[x];
}

static double
mapWeight( Var x , vec<double> & weights , double weight )
{
   if ( weights.size() <= x || weights[x] != weight ) {
      weights.growTo( x + 1 , -1 );
      weights[x] = weight;
   }
   return weights[x];
}

void
SsatSolver::toDimacs( FILE * f , Clause & c , vec<Var> & map , Var & max )
{
   Solver * S = _s1;
   if ( S->satisfied(c) ) return;

   for ( int i = 0 ; i < c.size() ; ++i )
      if ( S->value(c[i]) != l_False )
         fprintf( f , "%s%d " , sign(c[i]) ? "-" : "" , mapVar( var(c[i]) , map , max ) + 1 );
   fprintf( f , "0\n" );
}

void
SsatSolver::toDimacsWeighted( FILE * f , vec<double> & weights , Var & max )
{
   for ( int i = 0 ; i < weights.size() ; ++i )
      fprintf( f , "w %d %f\n" , i + 1 , weights[i] );
}

void
SsatSolver::toDimacsWeighted( const char * file , const vec<Lit> & assumps )
{
   FILE * f = fopen( file , "wr" );
   if ( f == NULL )
      fprintf( stderr , "could not open file %s\n" , file ), exit(1);
   toDimacsWeighted( f , assumps );
   fclose(f);
}

void
SsatSolver::toDimacsWeighted( FILE * f , const vec<Lit> & assumps )
{
   Solver * S = _s1;

   // Handle case when solver is in contradictory state:
   if ( !S->ok ) {
      fprintf( f , "p cnf 1 2\n1 0\n-1 0\n" );
      return;
   }

   Var max = 0, tmpVar;
   int cnt = 0;
   vec<Var> map;
   vec<double> weights;
   
   // map var to 0 ~ max
   // map 0 ~ max to original weight
   for ( int i = 0 ; i < S->clauses.size() ; ++i )
      if ( !S->satisfied( S->ca[S->clauses[i]]) ) {
         cnt++;
         Clause& c = S->ca[S->clauses[i]];
         for ( int j = 0 ; j < c.size(); ++j )
            if ( S->value(c[j]) != l_False ) {
               tmpVar = mapVar( var(c[j]) , map , max );
               mapWeight( tmpVar, weights, ( isRVar(var(c[j])) ? _quan[var(c[j])] : -1 ) );
            }
      }

   // Assumptions are added as unit clauses:
   cnt += assumps.size();
   for ( int i = 0 ; i < assumps.size() ; ++i ) {
      tmpVar = mapVar( var(assumps[i]) , map , max );
      mapWeight( tmpVar , weights , ( isRVar(var(assumps[i])) ? _quan[var(assumps[i])] : -1 ) );
   }

   fprintf(f, "p cnf %d %d\n", max, cnt);

   for ( int i = 0 ; i < assumps.size() ; ++i ) {
      fprintf( f , "%s%d 0\n" , sign(assumps[i]) ? "-" : "" , mapVar( var(assumps[i]) , map , max ) + 1 );
   }

   for ( int i = 0 ; i < S->clauses.size() ; ++i )
      toDimacs( f , S->ca[S->clauses[i]] , map , max );

   toDimacsWeighted( f , weights , max );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
