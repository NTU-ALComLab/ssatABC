/**CFile****************************************************************
  FileName    [main_test.cc]
  SystemName  []
  PackageName [Catch2 package.]
  Synopsis    [Catch2 is an unit test framework.
               This file add a new commend 'utest' in ABC to use the test suit.]
  Author      [Kuan-Hua Tu]

  Affiliation [NTU]
  Date        [2018.04.20]
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

// ABC command to run Catch
static int UnitTest_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
  Catch::Session().run(argc, argv);
  return 0;
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "Alcom", "utest", UnitTest_Command, 0);
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc) {}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t UnitTest_frame_initializer = {init, destroy};

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct UnitTest_registrar {
  UnitTest_registrar() { Abc_FrameAddInitializer(&UnitTest_frame_initializer); }
} unitTest_registrar_;

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
