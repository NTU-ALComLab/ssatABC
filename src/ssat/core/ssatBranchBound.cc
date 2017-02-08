/**CFile****************************************************************

  FileName    [ssatBranchBound.cc]

  SystemName  [ssatQesto]

  Synopsis    [Branch and bound method]

  Author      [Nian-Ze Lee]
  
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

void
SsatSolver::solveBranchBound( Abc_Ntk_t * pNtk )
{
   vec<Lit> eLits;
   Abc_Obj_t * pObj;
   double tempValue = 0.0;
   int i;

   _satPb = 0.0;
   ntkBuildPrefix( pNtk );
   _s1 = ntkBuildSolver( pNtk );
   // TODO: initialize eLits to 1111..1111
   do {
      tempValue = allSatModelCount( _s1 , eLits , _satPb );
      if ( _satPb < tempValue ) {
         printf( "  > find better solution! (%f)\n" , tempValue );
         _satPb = tempValue;
         eLits.copyTo( _erModel );
      }
   } while ( binaryDecrement( eLits ) );
   printf( "\n  > optimized value: %f\n" , _satPb );
   printf( "  > optimizing assignment to exist vars:\n" );
   dumpCla( _erModel );
}

/**Function*************************************************************

  Synopsis    [Build prefix structure from pNtk.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::ntkBuildPrefix( Abc_Ntk_t * pNtk )
{
   // TODO: initialize _rootVars, _quan, _level
}

/**Function*************************************************************

  Synopsis    [Build SAT solver from pNtk.]

  Description [Assume negative output for branch and bound.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Solver*
SsatSolver::ntkBuildSolver( Abc_Ntk_t * pNtk )
{
   // TODO: initialize _s1
   return NULL;
}

/**Function*************************************************************

  Synopsis    [Iterate all exist assignments.]

  Description [Decrement 1, return false if eLits = 00...00.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

bool
SsatSolver::binaryDecrement( vec<Lit> & eLits ) const
{
   // TODO: decrement exist assignment
   return false;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
