/**CFile****************************************************************

  FileName    [ssatCubeToNtk.cc]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [2SSAT solving by Qesto and model counting.]

  Synopsis    [Function to construct circuits from cubes in SsatSolver.]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [28, Dec., 2016.]

***********************************************************************/

#include "ssat/core/SsatSolver.h"
using namespace Minisat;


//ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// main function, global namespace
void Ssat_CubeToNtk( SsatSolver& );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Construct circuits from cubes in SsatSolver]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void 
Ssat_CubeToNtk( SsatSolver & ssat )
{
   printf( "  > In Ssat_CubeToNtk():\n" );
   
   for ( int  i = 0 ; i < (ssat._rootVars).size() ; ++i ) {
      for ( int j = 0 ; j < (ssat._rootVars[i]).size() ; ++j ) {
         printf( "  > %d lv %d-th var = %d\n" , i , j , (ssat._rootVars[i])[j]+1 );
      }
   }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


//ABC_NAMESPACE_IMPL_END
