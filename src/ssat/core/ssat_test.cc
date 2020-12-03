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

#include "SsatSolver.h"
#include "extRegression/catch.hpp"
using namespace Minisat;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void initParams(Ssat_Params_t*);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

TEST_CASE("toliet_1", "[planning]") {
  const char* targetFile =
      "benchmarks/ssatER/planning/ToiletA/sdimacs/toilet_a_08_01.2.sdimacs";
  abctime clk = Abc_Clock();
  Ssat_Params_t Params, *pParams = &Params;
  initParams(pParams);
  pParams->fPart = false;
  printf("  > Testing target: %s\n", targetFile);
  gzFile in = gzopen(targetFile, "rb");
  SsatSolver* pSsat = new SsatSolver(pParams->fTimer, pParams->fVerbose);
  pSsat->readSSAT(in);
  gzclose(in);
  pSsat->solveSsat(pParams);
  double answer = pSsat->lowerBound();
  REQUIRE(answer == Approx(0.0078125));
  delete pSsat;
  Abc_PrintTime(1, "  > Elasped time", Abc_Clock() - clk);
  printf("-------------------------------------------\n");
}

TEST_CASE("toliet_2", "[planning]") {
  const char* targetFile =
      "benchmarks/ssatER/planning/ToiletA/sdimacs/toilet_a_08_01.4.sdimacs";
  abctime clk = Abc_Clock();
  Ssat_Params_t Params, *pParams = &Params;
  initParams(pParams);
  printf("  > Testing target: %s\n", targetFile);
  gzFile in = gzopen(targetFile, "rb");
  SsatSolver* pSsat = new SsatSolver(pParams->fTimer, pParams->fVerbose);
  pSsat->readSSAT(in);
  gzclose(in);
  pSsat->solveSsat(pParams);
  double answer = pSsat->lowerBound();
  REQUIRE(answer == Approx(0.015625));
  delete pSsat;
  Abc_PrintTime(1, "  > Elasped time", Abc_Clock() - clk);
  printf("-------------------------------------------\n");
}

TEST_CASE("toliet_3", "[planning]") {
  const char* targetFile =
      "benchmarks/ssatER/planning/ToiletA/sdimacs/toilet_a_10_01.2.sdimacs";
  abctime clk = Abc_Clock();
  Ssat_Params_t Params, *pParams = &Params;
  initParams(pParams);
  pParams->fPart = false;
  pParams->fCkt = false;
  printf("  > Testing target: %s\n", targetFile);
  gzFile in = gzopen(targetFile, "rb");
  SsatSolver* pSsat = new SsatSolver(pParams->fTimer, pParams->fVerbose);
  pSsat->readSSAT(in);
  gzclose(in);
  pSsat->solveSsat(pParams);
  double answer = pSsat->lowerBound();
  REQUIRE(answer == Approx(0.001953125));
  delete pSsat;
  Abc_PrintTime(1, "  > Elasped time", Abc_Clock() - clk);
  printf("-------------------------------------------\n");
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
