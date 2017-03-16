/**CFile****************************************************************

  FileName    [ssatBddSolve.cc]

  SystemName  [ssatABC]

  Synopsis    [Solve general SSAT by BDD]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [16, Mar., 2017]

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

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::bddSolveSsat( bool fGroup , bool fReorder )
{
   if ( _fVerbose ) {
      printf( "  > grouping = %s , reordering = %s\n" , fGroup?"yes":"no", fReorder?"yes":"no" );
      printf( "  > Under construction ...\n" );
   }
   // TODO
   initCnfNetwork();
   buildBddFromNtk();
}

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::initCnfNetwork()
{
}

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
SsatSolver::buildBddFromNtk()
{
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
