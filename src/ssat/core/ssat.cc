/**CFile****************************************************************

  FileName    [ssat.cc]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [2SSAT solving by Qesto and model counting.]

  Synopsis    [Command file.]

  Author      [Nian-Ze Lee]
  
  Affiliation [NTU]

  Date        [28, Dec., 2016.]

***********************************************************************/

#include "ssat/core/SsatSolver.h"
using namespace Minisat;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern "C" void Ssat_Init ( Abc_Frame_t * );
extern "C" void Ssat_End  ( Abc_Frame_t * );

static int SsatCommandSSAT   ( Abc_Frame_t * pAbc , int argc , char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start / Stop the eda package]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void 
Ssat_Init( Abc_Frame_t * pAbc )
{
   Cmd_CommandAdd( pAbc , "z SSAT" , "ssat" , SsatCommandSSAT , 0 );
}

void 
Ssat_End( Abc_Frame_t * pAbc )
{
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int 
SsatCommandSSAT( Abc_Frame_t * pAbc , int argc , char ** argv )
{
   SsatSolver * pSsat;
   gzFile in;
   abctime clk;
   int c;

   Extra_UtilGetoptReset();
   while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
   {
      switch ( c )
      {
         case 'h':
            goto usage;
         default:
            goto usage;
      }
   }
   if ( argc != globalUtilOptind + 1 ) goto usage;
   in = gzopen( argv[globalUtilOptind] , "rb" );
   pSsat = new SsatSolver;
   pSsat->readSSAT(in);
   gzclose(in);
   clk = Abc_Clock();
   pSsat->ssolve();
   Abc_PrintTime( 1 , "  > Time" , Abc_Clock() - clk );
   delete pSsat;
   return 0;

usage:
   Abc_Print( -2 , "usage: ssat [-h] <file>\n" );
   Abc_Print( -2 , "\t        Solve 2SSAT by Qesto and model counting\n" );
   Abc_Print( -2 , "\t-h    : prints the command summary\n" );
   Abc_Print( -2 , "\tfile  : the sdimacs file\n" );
   return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

