/**CFile****************************************************************

  FileName    [ssatModelCount.cc]

  SystemName  [ssatQesto]

  Synopsis    [All-SAT enumeration model counting]

  Author      [Yen-Shi Wang]
  
  Affiliation [NTU]

  Date        [8, Feb., 2017]

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

  Synopsis    [All-sat model counting entrance point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

double
SsatSolver::allSatModelCount( Solver * s , const vec<Lit> & eLits , double curVal )
{
   vec<Lit> rLits ( _rootVars[1].size() ) , rHits , bkCla;
   vec<CRef> bkClaRef;
   double count = 0.0 , prob;

   while ( s->solve( eLits ) ) {
      for ( int i = 0 ; i < _rootVars[1].size() ; ++i )
         rLits[i] = ( s->modelValue(_rootVars[1][i]) == l_True ) ? mkLit(_rootVars[1][i]) : ~mkLit(_rootVars[1][i]);
     
      // TODO: try to drop some rLits
      // miniHitSet( s , rLits , rHits );       // get rHits
      // use rLits to debug
      rLits.copyTo( rHits );
      
      prob = 1;
      for ( int i = 0 ; i < rHits.size() ; ++i )
         prob *= sign(rHits[i]) ? 1 - _quan[var(rHits[i])] : _quan[var(rHits[i])];
      
      count += prob;
      if ( count >= 1 - curVal ) break;
      bkCla.clear();
      for ( int i = 0 ; i < rHits.size() ; ++i ) bkCla.push( ~rHits[i] );
      s->addClause( bkCla );
      bkClaRef.push( s->clauses[s->nClauses()-1] );
   }
   for ( int i = 0 ; i < bkClaRef.size() ; ++i )
      s->removeClause( bkClaRef[i] );
   return 1 - count;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
