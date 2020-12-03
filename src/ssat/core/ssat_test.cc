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

static void runTestWithParamsAndVerdict(const char* pTestCase,
                                        Ssat_Params_t* pParams,
                                        const double pVerdict) {
  abctime clk = Abc_Clock();
  Abc_Print(-2, "[INFO] Input sdimacs file: %s\n", pTestCase);
  gzFile in = gzopen(pTestCase, "rb");
  SsatSolver* pSsat = new SsatSolver(pParams->fTimer, pParams->fVerbose);
  pSsat->readSSAT(in);
  gzclose(in);
  pSsat->solveSsat(pParams);
  pSsat->reportSolvingResults();
  Abc_PrintTime(1, "  > Time", Abc_Clock() - clk);
  double answer = pSsat->exactSatProb();
  delete pSsat;
  printf("-------------------------------------------\n");
  REQUIRE(answer == Approx(pVerdict));
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

TEST_CASE("toilet_1", "[planning]") {
  const char* testCase =
      "benchmarks/ssatER/planning/ToiletA/sdimacs/toilet_a_08_01.2.sdimacs";
  double verdict = 0.0078125;
  Ssat_Params_t Params, *pParams = &Params;
  initParams(pParams);
  pParams->fPart = false;
  runTestWithParamsAndVerdict(testCase, pParams, verdict);
}

TEST_CASE("toilet_2", "[planning]") {
  const char* testCase =
      "benchmarks/ssatER/planning/ToiletA/sdimacs/toilet_a_08_01.4.sdimacs";
  double verdict = 0.015625;
  Ssat_Params_t Params, *pParams = &Params;
  initParams(pParams);
  runTestWithParamsAndVerdict(testCase, pParams, verdict);
}

TEST_CASE("toilet_3", "[planning]") {
  const char* testCase =
      "benchmarks/ssatER/planning/ToiletA/sdimacs/toilet_a_10_01.2.sdimacs";
  double verdict = 0.001953125;
  Ssat_Params_t Params, *pParams = &Params;
  initParams(pParams);
  pParams->fPart = false;
  pParams->fCkt = false;
  runTestWithParamsAndVerdict(testCase, pParams, verdict);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
