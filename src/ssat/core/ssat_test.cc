/**CFile****************************************************************
  FileName    [ssat_test.cc]
  SystemName  []
  PackageName []
  Synopsis    [An example file to use Catch2 writing test case]
  Author      [Kuan-Hua Tu]
  
  Affiliation [NTU]
  Date        [2018.04.20]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "extUnitTest/catch.hpp"
#include "SsatSolver.h"
using namespace Minisat;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

TEST_CASE( "toliet" , "[planning]" )
{
   gzFile in = gzopen( "/home/users/nianze/ssat/ssatABC/expSsat/ssatER/planning/ToiletA/toilet_a_10_01.2.qdimacs" , "rb" );
   SsatSolver * pSsat = new SsatSolver( true , false );
   pSsat->readSSAT(in);
   gzclose(in);
   pSsat->solveSsat();
   double answer = pSsat->lowerBound();
   REQUIRE( answer == Approx(0.001953125) );
   delete pSsat;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
