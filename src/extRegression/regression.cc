/**CFile****************************************************************
  FileName    [regression.cc]
  SystemName  []
  PackageName [Catch2: C++ Test Framework]
  Synopsis    [This file adds a new command 'regression-test'.]
  Author      [Kuan-Hua Tu and Nian-Ze Lee]

  Affiliation [NTU]
  Date        [2020.12.03]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#define CATCH_CONFIG_RUNNER
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "catch.hpp"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// ABC command to run Catch2
static int RegressionTest_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
  return Catch::Session().run(argc, argv);
}

static void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "Catch2", "regression-test", RegressionTest_Command, 0);
}

static void destroy(Abc_Frame_t* pAbc) {}

Abc_FrameInitializer_t RegressionTest_frame_initializer = {init, destroy};

struct RegressionTest_registrar {
  RegressionTest_registrar() {
    Abc_FrameAddInitializer(&RegressionTest_frame_initializer);
  }
} regressionTest_registrar_;

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
