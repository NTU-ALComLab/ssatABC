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
   double range;
   int cLimit , c;
   bool fAll , fMini;

   range  = 0.01;
   cLimit = 1;
   fAll   = false;
   fMini  = true;
   Extra_UtilGetoptReset();
   while ( ( c = Extra_UtilGetopt( argc, argv, "RLamh" ) ) != EOF )
   {
      switch ( c )
      {
         case 'R':
            if ( globalUtilOptind >= argc ) {
                Abc_Print( -1 , "Command line switch \"-R\" should be followed by a positive floating number.\n" );
                goto usage;
            }
            range = atof( argv[globalUtilOptind] );
            globalUtilOptind++;
            if ( range < 0.0 || range > 1.0 ) goto usage;
            break;
         case 'L':
            if ( globalUtilOptind >= argc ) {
                Abc_Print( -1 , "Command line switch \"-L\" should be followed by a positive integer.\n" );
                goto usage;
            }
            cLimit = atoi( argv[globalUtilOptind] );
            globalUtilOptind++;
            if ( cLimit != -1 && !(cLimit > 0) ) goto usage;
            break;
         case 'a':
            fAll ^= 1;
            break;
         case 'm':
            fMini ^= 1;
            break;
         case 'h':
            goto usage;
         default:
            goto usage;
      }
   }
   if ( argc != globalUtilOptind + 1 ) {
      Abc_Print( -1 , "No ssat file!\n" );
      goto usage;
   }
   in = gzopen( argv[globalUtilOptind] , "rb" );
   pSsat = new SsatSolver;
   pSsat->readSSAT(in);
   gzclose(in);
   clk  = Abc_Clock();
   Abc_Print( -2 , "\n==== SSAT solving process ====\n" );
   pSsat->solveSsat( range , cLimit , fAll , fMini );
   Abc_Print( -2 , "\n  > Upper bound = %f\n" , pSsat->upperBound() );
   Abc_Print( -2 , "  > Lower bound = %f\n" , pSsat->lowerBound() );
   Abc_PrintTime( 1 , "  > Time  " , Abc_Clock() - clk );
   printf("\n");
   delete pSsat;
   return 0;

usage:
   Abc_Print( -2 , "usage: ssat [-R <num>] [-L <num>] [-amh] <file>\n" );
   Abc_Print( -2 , "\t        Solve 2SSAT by Qesto and model counting / bdd signal prob\n" );
   Abc_Print( -2 , "\t-R <num>  : gap between upper and lower bounds, default=%f\n" , 0.01 );
   Abc_Print( -2 , "\t-L <num>  : number of cubes to construct network, default=%d (-1: construct only once)\n" , 1 );
   Abc_Print( -2 , "\t-a        : toggles using All-SAT enumeration solve, default=false\n" );
   Abc_Print( -2 , "\t-m        : toggles using minimal UNSAT core, default=true\n" );
   Abc_Print( -2 , "\t-h        : prints the command summary\n" );
   Abc_Print( -2 , "\tfile      : the sdimacs file\n" );
   return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

