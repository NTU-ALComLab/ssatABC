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
#include <math.h>
#include "ssat/utils/ParseUtils.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/core/Solver.h"
#include "ssat/core/Dimacs.h"
#include "ssat/core/SsatSolver.h"
#include "ssat/mtl/Sort.h"

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
   vec<Lit> eLits( _rootVars[0].size() ) , sBkCla , parLits;
   vec<int> ClasInd;
   double subvalue;
   int dropIndex, dropHead, dropTail;
   int nCachet = 0;
   _erModel.capacity( _rootVars[0].size() ); _erModel.clear();
   abctime clk = Abc_Clock();
   for(;;) {
      if ( !_s2->solve() ) {
         printf( "\n  > optimizing assignment to exist vars:\n" );
         dumpCla( _erModel );
         printf( "  > number of calls to Cachet:%10d\n" , nCachet );
         return _satPb;
      }
      for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
         eLits[i] = ( _s2->modelValue(_rootVars[0][i]) == l_True ) ? mkLit(_rootVars[0][i]) : ~mkLit(_rootVars[0][i]);
      if ( !_s1->solve(eLits) ) { // UNSAT case
         sBkCla.clear();
         // miniUnsatCore( _s1->conflict , sBkCla );
         // _s2->addClause( sBkCla );
         _s2->addClause( _s1->conflict );
      }
      else { // SAT case
         dropIndex = eLits.size();
         // FIXME
         if ( _s1->nClauses() == 0 ) {
            Abc_Print( -1 , "  > There is no clause left ...\n" );
            Abc_Print( -1 , "  > Should look at unit assumption to compute value ...\n" );
         }
         subvalue  = fBdd ? clauseToNetwork() : countModels( eLits , dropIndex );
         ++nCachet;
         if ( subvalue == 1 ) {
            printf( "\n  > optimizing assignment to exist vars:\n" );
            dumpCla( eLits );
            printf( "  > number of calls to Cachet:%10d\n" , nCachet );
            _satPb = 1;
            return 1;
         }
         if ( subvalue >= _satPb ) {
            fflush(stdout);
            _satPb = subvalue;
            eLits.copyTo( _erModel );
         }

         sBkCla.clear();
         ClasInd.clear();
         parLits.clear();
         collectBkClaER( sBkCla , ClasInd , dropIndex );
         
         sBkCla.copyTo( parLits );
         for ( int i = 0 ; i < parLits.size() ; ++i )
            parLits[i] = ~parLits[i];

         dropIndex = parLits.size();
         int countdrop = 0, countCachet = 0;
         if ( dropIndex >= 1 ) {
         for(;;) {
            while ( !dropLit( parLits , ClasInd , --dropIndex , subvalue ) );

            ++nCachet;
            subvalue = countModels( parLits , dropIndex );
            if ( subvalue > _satPb )
               break;
         }
         ++dropIndex;
         }

         sBkCla.clear();
         for ( int i = 0 ; i < dropIndex; ++i )
            sBkCla.push( ~parLits[i] );

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

bool
SsatSolver::dropLit( vec<Lit>& parLits, vec<int>& ClasInd, int dropIndex, double& subvalue )
{
   bool dropCla;
   double claValue;
   vec<int> newClasInd;
   
   for ( int i = 0 , n = ClasInd.size() ; i < n ; ++i ) {
      Clause & c = _s1->ca[_s1->clauses[ClasInd[i]]];
      dropCla = false;
      claValue = 1;
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( isEVar(var(c[j])) && !dropCla ) {
            for ( int k = dropIndex ; k < parLits.size() ; ++k ) {
               if ( var(c[j]) == var(parLits[k]) ) {
                  dropCla = true;
                  break;
               }
            }
         }
         else if ( isRVar(var(c[j])) ) {
            if ( _s1->value(c[j]) == l_True )
               claValue *= _quan[var(c[j])];
            else
               claValue *= 1 - _quan[var(c[j])];
         }
      }
      if ( dropCla )
         subvalue += claValue;
      else
         newClasInd.push(ClasInd[i]);
   }
   if ( newClasInd.size() == ClasInd.size() )
      return false;
   newClasInd.copyTo(ClasInd);
   if ( subvalue <= _satPb )
      return false;
   return true;
}

void
SsatSolver::collectParLits( vec<Lit> & parLits, vec<int> & ClasInd )
{
   vec<Lit> tmpLits;
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
         ClasInd.push(i);
         for ( int j = 0 ; j < c.size() ; ++j ) {
            if ( isEVar(var(c[j])) && _level[var(c[j])] == 0 )
               tmpLits.push (~c[j]);
         }
      }
   }

   sort(tmpLits);
   Lit p; int i, j;
   int num[500] = {0}, max = -1, maxIndex;
   for (i = j = 0, p = lit_Undef; i < tmpLits.size(); i++)
      if (tmpLits[i] != p) {
         tmpLits[j++] = p = tmpLits[i];
      }
      else 
         num[j-1]++;
   tmpLits.shrink(i - j);
   for(;;) {
      for ( int i = 0 ; i < tmpLits.size() ; ++i )
      {
         if ( num[i] > max ) {
            max = num[i];
            maxIndex = i;
         }
      }
      if ( max == -1 ) break;
      parLits.push(tmpLits[ maxIndex ]);
      num[maxIndex] = -1;
      max = -1;
   }
}

void
SsatSolver::collectBkClaER( vec<Lit> & sBkCla , vec<int> & ClasInd , int dropIndex )
{
   vec<bool> drop( _s1->nVars() , false );
   for ( int i = dropIndex ; i < _rootVars[0].size() ; ++i ) drop[_rootVars[0][i]] = true;

   bool block;
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      block = true;
      Clause & c = _s1->ca[_s1->clauses[i]];
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( drop[var(c[j])] || isEVar(var(c[j])) && _level[var(c[j])] == 0 && _s1->modelValue(c[j]) == l_True ) {
            block = false;
            break;
         }
      }
      if ( block ) {
         ClasInd.push(i);
         for ( int j = 0 ; j < c.size() ; ++j ) {
            // cout << ( sign(c[j]) ? "-": "" ) << var(c[j])+1 << " ";
            if ( isEVar(var(c[j])) && _level[var(c[j])] == 0 )
               sBkCla.push (c[j]);
         }
         // cout << "\n";
      }
   }
   
    sort(sBkCla);
    Lit p; int i, j;
    for (i = j = 0, p = lit_Undef; i < sBkCla.size(); i++)
        if (_s1->value(sBkCla[i]) != l_False && sBkCla[i] != p)
            sBkCla[j++] = p = sBkCla[i];
    sBkCla.shrink(i - j);
    
    // cout << "  > Clause after tidying up. " << endl;
    // dumpCla( sBkCla );
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

//TODO
double
SsatSolver::countModels( const vec<Lit> & sBkCla , int dropIndex )
{
   FILE * file;
   int length = 1024;
   char prob_str[length] , cmdModelCount[length];

   // vec<Lit> assump( sBkCla.size() );
   // for ( int i = 0 ; i < sBkCla.size() ; ++i ) assump[i] = ~sBkCla[i];
   
   // toDimacsWeighted( "temp.wcnf" , assump );
   // toDimacsWeighted( "temp.wcnf" , sBkCla );
   // cout << "dropIndex: " << dropIndex << " , clause: ";
   // dumpCla(sBkCla);
   toDimacsWeighted( "temp.wcnf" , sBkCla , dropIndex );
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
SsatSolver::toDimacs( FILE * f , Clause & c , vec<Var> & map , Var & max , int dropIndex )
{
   Solver * S = _s1;
   // if ( S->satisfied(c) ) return;

   for ( int i = 0 ; i < c.size() ; ++i )
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
SsatSolver::toDimacsWeighted( const char * file , const vec<Lit> & assumps , int dropIndex )
{
   FILE * f = fopen( file , "wr" );
   if ( f == NULL )
      fprintf( stderr , "could not open file %s\n" , file ), exit(1);
   toDimacsWeighted( f , assumps , dropIndex );
   fclose(f);
}

void
SsatSolver::toDimacsWeighted( FILE * f , const vec<Lit> & assumps , int dropIndex )
{
   Solver * S = _s1;

   vec<bool> drop( _s1->nVars() , false );
   for ( int i = dropIndex ; i < _rootVars[0].size() ; ++i ) drop[_rootVars[0][i]] = true;
   
   // Handle case when solver is in contradictory state:
   if ( !S->ok ) {
      fprintf( f , "p cnf 1 2\n1 0\n-1 0\n" );
      return;
   }

   Var max = 0, tmpVar;
   int cnt = 0;
   vec<Var> map;
   vec<double> weights;
   bool select = true;
   
   // map var to 0 ~ max
   // map 0 ~ max to original weight
   for ( int i = 0 ; i < S->clauses.size() ; ++i ) {
      Clause& c = S->ca[S->clauses[i]];
      for ( int j = 0 ; j < c.size(); ++j ) {
      // cout << "dropIndex : " << dropIndex << endl;
      // cout << "var : " << var(c[j]) << endl;
         if ( drop[var(c[j])] || isEVar(var(c[j])) && _level[var(c[j])] == 0 && _s1->modelValue(c[j]) == l_True ) {
            select = false;
         }
      }
      if ( select == true ) {
         cnt++;
         Clause& c = S->ca[S->clauses[i]];
         for ( int j = 0 ; j < c.size(); ++j ) {
               tmpVar = mapVar( var(c[j]) , map , max );
               mapWeight( tmpVar, weights, ( isRVar(var(c[j])) ? _quan[var(c[j])] : -1 ) );
         }
      }
      select = true;
   }

   // Assumptions are added as unit clauses:
   cnt += dropIndex;
   for ( int i = 0 ; i < dropIndex ; ++i ) {
      tmpVar = mapVar( var(assumps[i]) , map , max );
      mapWeight( tmpVar , weights , ( isRVar(var(assumps[i])) ? _quan[var(assumps[i])] : -1 ) );
   }

   fprintf(f, "p cnf %d %d\n", max, cnt);

   for ( int i = 0 ; i < dropIndex ; ++i ) {
      fprintf( f , "%s%d 0\n" , sign(assumps[i]) ? "-" : "" , mapVar( var(assumps[i]) , map , max ) + 1 );
   }

   // for ( int i = 0 ; i < S->clauses.size() ; ++i )
      // toDimacs( f , S->ca[S->clauses[i]] , map , max , dropIndex );
   select = true;
   for ( int i = 0 ; i < S->clauses.size() ; ++i ) {
      Clause& c = S->ca[S->clauses[i]];
      for ( int j = 0 ; j < c.size(); ++j )
         if ( drop[var(c[j])] || isEVar(var(c[j])) && _level[var(c[j])] == 0 && _s1->modelValue(c[j]) == l_True ) {
            select = false;
         }
      if ( select == true ) {
         toDimacs( f , S->ca[S->clauses[i]] , map , max , dropIndex );
      }
      select = true;
   }

   toDimacsWeighted( f , weights , max );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
