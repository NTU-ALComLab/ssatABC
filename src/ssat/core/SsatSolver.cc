/**CFile****************************************************************

  FileName    [SsatSolver.cc]

  SystemName  [ssatQesto]

  Synopsis    [Implementations of member functions for ssatSolver]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [16, Dec., 2016]

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

  Synopsis    [Constructor/Destructor]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

SsatSolver::~SsatSolver()
{
   if ( _s1 ) {
      delete _s1;
      _s1 = NULL;
   }
   if ( _s2 ) {
      delete _s2;
      _s2 = NULL;
   }
   if ( _pNtkCube ) {
      Abc_NtkDelete( _pNtkCube );
      _pNtkCube = NULL;
   }
   if ( _vMapVars ) {
      Vec_PtrFree( _vMapVars );
      _vMapVars = NULL;
   }
}

/**Function*************************************************************

  Synopsis    [Build two solvers for qesto-like SSAT solving]

  Description [Interface function]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::readSSAT( gzFile & input_stream )
{
   _s1 = parse_SDIMACS( input_stream );
}

/**Function*************************************************************

  Synopsis    [SSAT formula parser]

  Description [Build quantification and _s1 (root clauses)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Solver*
SsatSolver::parse_SDIMACS( gzFile & input_stream ) 
{
    StreamBuffer in( input_stream );
    vec<Lit> lits;
    Solver * ptrS = new Solver;
    Solver & S    = *ptrS;
    double parsed_prob;
    int vars    = 0;
    int clauses = 0;
    int cnt     = 0;
    int level   = 0;
    int quan    = 0; // 1: random, 2:exist, 3: forall, 0:none

    _rootVars.push();
    for (;;) {
       skipWhitespace(in);
       if ( *in == EOF ) break;
       else if ( *in == 'p' ) { // p cnf #var #clause
          if ( eagerMatch( in , "p cnf" ) ) {
             vars    = parseInt(in);
             clauses = parseInt(in);
          }
          else
             printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
       } 
       else if ( *in == 'r' ) { // r p x_i  <-- randomly quantified variable x_i w.p. p 
          ++in;
          parsed_prob = parseFloat(in);
          readPrefix( in , S , parsed_prob , 1 , quan , level );
       }
       else if ( *in == 'e' ) { // e y_j    <-- existentially quantified variable y_j
          ++in;
          readPrefix( in , S , EXIST , 2 , quan , level );
       }
       else if ( *in == 'a' ) { // a z_k    <-- universally quantified variable z_k
          ++in;
          readPrefix( in , S , FORALL , 3 , quan , level );
       }
       else {
          if ( *in == 'c' || *in == 'p' )
             skipLine(in);
          else {
             cnt++;
             readClause(in, S, lits);
             S.addClause_(lits); 
          }
       }
    }
    _numLv = level + 1;
    if ( vars != S.nVars() )
       fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
    if ( cnt  != clauses )
       fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
    return ptrS;
}

void 
SsatSolver::readPrefix( StreamBuffer & in , Solver & S , double prob , 
                        int cur , int & quan , int & level ) 
{
   vec<int> parsed;
   int parsed_int , var , max = -1;

   if ( quan != 0 && cur != quan ) {
      _rootVars.push();
      ++level;
   }
   quan = cur;
   for (;;) {
      parsed_int = parseInt(in);
      if ( parsed_int == 0 ) break;
      parsed.push(parsed_int);
      if ( parsed_int > max ) max = parsed_int;
      var = parsed_int - 1;
      while ( var >= S.nVars() ) S.newVar();
   }
   _quan.growTo  ( max , -1 );
   _level.growTo ( max , -1 );
   for ( int i = 0 ; i < parsed.size() ; ++i ) {
      (_rootVars.last()).push(parsed[i]-1);
      _quan[parsed[i]-1]  = prob;
      _level[parsed[i]-1] = level;
   }
}

/**Function*************************************************************

  Synopsis    [SsatSolve write to cnf file with weighted variables]

  Description []
               
  SideEffects []

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
   if ( _s2->satisfied(c) ) return;

   for ( int i = 0 ; i < c.size() ; ++i )
      if ( _s2->value(c[i]) != l_False )
         fprintf( f , "%s%d " , sign(c[i]) ? "-" : "" , mapVar( var(c[i]) , map , max ) + 1 );
   fprintf( f , "0\n" );
}

void
SsatSolver::toDimacsWeighted( FILE * f , vec<double> & weights , Var & max )
{
   for ( int i = 0 ; i < weights.size() ; ++i ) {
      fprintf( f , "w %d %f\n" , i + 1 , weights[i] );
   }
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
   // Handle case when solver is in contradictory state:
   if ( !_s2->ok ) {
      fprintf( f , "p cnf 1 2\n1 0\n-1 0\n" );
      return;
   }

   Var max = 0, tmpVar;
   int cnt = 0;
   vec<Var> map;
   vec<double> weights;
   
   // map var to 0 ~ max
   // map 0 ~ max to original weight
   for ( int i = 0 ; i < _s2->clauses.size() ; ++i )
      if ( !_s2->satisfied( _s2->ca[_s2->clauses[i]]) ) {
         cnt++;
         Clause& c = _s2->ca[_s2->clauses[i]];
         for ( int j = 0 ; j < c.size(); ++j )
            if ( _s2->value(c[j]) != l_False ) {
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

   for ( int i = 0 ; i < _s2->clauses.size() ; ++i )
      toDimacs( f , _s2->ca[_s2->clauses[i]] , map , max );

   toDimacsWeighted( f , weights , max );
}

/**Function*************************************************************

  Synopsis    [Solving process entrance]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::ssolve()
{
   if ( _numLv != 2 || !isEVar( _rootVars[1][0] ) ) {
      fprintf( stderr , "WARNING! Currently only support \"AE 2QBF\" or \"RE 2SSAT\"...\n" );
      return false;
   }
   _s1->simplify(); // avoid clause deletion after solving
   _s2 = buildSelectSolver();
   initSelLitMark(); // avoid repeat selection vars in blocking clause
   if ( isAVar( _rootVars[0][0] ) ) return ssolve2QBF();
   else                             return ssolve2SSAT();
}

/**Function*************************************************************

  Synopsis    [Build _s2 (clause selection solver)]

  Description [Initialize forall vars and build selection clauses]
               
  SideEffects [forall vars have exactly the same IDs as _s1]

  SeeAlso     []

***********************************************************************/

Solver*
SsatSolver::buildSelectSolver()
{
   Solver * S = new Solver;
   vec<Lit> uLits;
   
   for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
      while ( _rootVars[0][i] >= S->nVars() ) S->newVar();
   _claLits.growTo( _s1->nClauses() , lit_Undef );
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      Clause & c  = _s1->ca[_s1->clauses[i]];
      uLits.clear();
      for ( int j = 0 ; j < c.size() ; ++j )
         if ( isAVar(var(c[j])) || isRVar(var(c[j])) ) uLits.push(c[j]);
      if ( uLits.size() > 1 ) { // allocate new selection var
         _claLits[i] = mkLit( S->newVar() );
         addSelectCla( *S , _claLits[i] , uLits );
      }
      else if ( uLits.size() == 1 ) // reuse the forall var as selection var
         _claLits[i] = ~uLits[0];
   }
   return S;
}

void
SsatSolver::addSelectCla( Solver & S , const Lit & x , const vec<Lit> & uLits )
{
   vec<Lit> cla( 1 + uLits.size() );
   for ( int i = 0 ; i < uLits.size() ; ++i )
      S.addClause( ~x , ~uLits[i] );
   cla[0] = x;
   for ( int i = 0 ; i < uLits.size() ; ++i )
      cla[i+1] = uLits[i];
   S.addClause( cla );
}

/**Function*************************************************************

  Synopsis    [Qesto 2QBF solving internal function]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

bool
SsatSolver::ssolve2QBF()
{
   vec<Lit> uLits( _rootVars[0].size() ) , sBkCla;
   for (;;) {
      if ( !_s2->solve() ) return true;
      for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
         uLits[i] = ( _s2->modelValue(_rootVars[0][i]) == l_True ) ? mkLit(_rootVars[0][i]) : ~mkLit(_rootVars[0][i]);
      if ( !_s1->solve(uLits) ) return false;
      sBkCla.clear();
      collectBkCla(sBkCla);
      _s2->addClause(sBkCla);
   }
}

/**Function*************************************************************

  Synopsis    [Qesto-like 2SSAT solving internal function]

  Description [Invoking cachet via system call]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::ssolve2SSAT()
{
   vec<Lit> rLits( _rootVars[0].size() ) , sBkCla;
   initCubeNetwork();
   initCubeCollect();
   for ( ;; ) {
      if ( !_s2->solve() ) {
         cubeToNetwork(); 
         return 0.0;
      }
      for ( int i = 0 ; i < _rootVars[0].size() ; ++i )
         rLits[i] = ( _s2->modelValue(_rootVars[0][i]) == l_True ) ? mkLit(_rootVars[0][i]) : ~mkLit(_rootVars[0][i]);
      if ( !_s1->solve(rLits) ) {
         _learntClause.push();
         _s1->conflict.copyTo( _learntClause.last() );
         if ( _learntClause.size() == _cubeLimit )
            cubeToNetwork();
         _s2->addClause( _s1->conflict );
         continue;
      }
      sBkCla.clear();
      collectBkCla(sBkCla);
      _s2->addClause(sBkCla);
   }
}

void
SsatSolver::collectBkCla( vec<Lit> & sBkCla )
{
   unmarkSelLit();
   bool block;
   for ( int i = 0 ; i < _s1->nClauses() ; ++i ) {
      if ( _claLits[i] == lit_Undef ) continue;
      Clause & c = _s1->ca[_s1->clauses[i]];
      block = true;
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( isEVar(var(c[j])) && _s1->modelValue(c[j]) == l_True ) {
            block = false;
            break;
         }
      }
      if ( block && _s2->value(_claLits[i]) != l_False && !isSelLitMarked(_claLits[i]) ) {
         sBkCla.push (_claLits[i]);
         markSelLit  (_claLits[i]);
      }
   }
}

double
SsatSolver::baseProb() const
{
   double subspace_base = 1.0;
   for( int i = 0 ; i < _rootVars[0].size() ; ++i ) {
      if ( _s2->value( _rootVars[0][i] ) == l_True ) 
         subspace_base *= _quan[_rootVars[0][i]];
      else if ( _s2->value( _rootVars[0][i] ) == l_False )
         subspace_base *= 1 - _quan[_rootVars[0][i]];
   }
   return subspace_base;
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

   vec<Lit> assump( sBkCla.size() );
   for ( int i = 0 ; i < sBkCla.size() ; ++i ) assump[i] = ~sBkCla[i];
   
   toDimacsWeighted( "temp.wcnf" , assump );
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

  Synopsis    [Dump content for debugging]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::dumpCla( Solver & S ) const
{
   for ( int i = 0 ; i < S.nClauses() ; ++i ) {
      CRef     cr = S.clauses[i];
      Clause & c  = S.ca[cr];
      for ( int j = 0 ; j < c.size() ; ++j ) {
         if ( S.value(c[j]) != l_False )
            cout << ( sign(c[j]) ? "-": "" ) << var(c[j])+1 << " ";
      }
      cout << "0\n";
   }
   cout << endl;
}

void
SsatSolver::dumpCla( const vec<Lit> & c ) const
{
   for ( int i = 0 ; i < c.size() ; ++i )
      cout << ( sign(c[i]) ? "-": "" ) << var(c[i])+1 << " ";
   cout << "0\n";
}

/**Function*************************************************************

  Synopsis    [SsatSolve testing interface]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::test() const
{
   printf( "\nPrefix structure , numLv = %d\n\n" , _numLv );
   for ( int i = 0 ; i < _rootVars.size() ; ++i ) {
      printf( "Lv%d vars:\n" , i );
      for ( int j = 0 ; j < _rootVars[i].size() ; ++j ) {
         if ( isRVar(_rootVars[i][j]) ) 
            printf( "r %f %d\n" , _quan[_rootVars[i][j]] , _rootVars[i][j]+1 );
         else if ( isEVar(_rootVars[i][j]) ) 
            printf( "e %d\n" , _rootVars[i][j]+1 );
         else if ( isAVar(_rootVars[i][j]) ) 
            printf( "a %d\n" , _rootVars[i][j]+1 );
         else {
            fprintf( stderr , "Error! Unknown quantifier for var %d\n" , _rootVars[i][j]+1 );
            assert(0);
         }
      }
      printf( "\n" );
   }
   if ( _s1 ) {
      printf( "  > _s1 clauses:\n\n" );
      dumpCla( *_s1 );
   }
   if ( _s2 ) {
      printf( "  > _s2 clauses:\n\n" );
      dumpCla( *_s2 );
   }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
