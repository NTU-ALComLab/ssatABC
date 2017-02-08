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
   Abc_Obj_t * pObj;
   int i;

   Abc_NtkForEachPi( pNtk , pObj , i )
      printf( "  > Pi %s , prob = %f\n" , Abc_ObjName(pObj) , pObj->dTemp );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
