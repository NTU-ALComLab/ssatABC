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
   int upper , lower , c;
   bool fAll , fMini , fBdd;

   range  = 0.01;
   upper  = 1;
   lower  = 65536;
   fAll   = true;
   fMini  = true;
   fBdd   = false;
   Extra_UtilGetoptReset();
   while ( ( c = Extra_UtilGetopt( argc, argv, "RULambh" ) ) != EOF )
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
         case 'U':
            if ( globalUtilOptind >= argc ) {
                Abc_Print( -1 , "Command line switch \"-U\" should be followed by a positive integer.\n" );
                goto usage;
            }
            upper = atoi( argv[globalUtilOptind] );
            globalUtilOptind++;
            if ( upper != -1 && !(upper > 0) ) goto usage;
            break;
         case 'L':
            if ( globalUtilOptind >= argc ) {
                Abc_Print( -1 , "Command line switch \"-L\" should be followed by a positive integer.\n" );
                goto usage;
            }
            lower = atoi( argv[globalUtilOptind] );
            globalUtilOptind++;
            if ( lower != -1 && !(lower > 0) ) goto usage;
            break;
         case 'a':
            fAll ^= 1;
            break;
         case 'm':
            fMini ^= 1;
            break;
         case 'b':
            fBdd ^= 1;
            break;
         case 'h':
            goto usage;
         default:
            goto usage;
      }
   }
   if ( argc != globalUtilOptind + 1 ) {
      Abc_Print( -1 , "Missing ssat file!\n" );
      goto usage;
   }
   in = gzopen( argv[globalUtilOptind] , "rb" );
   if ( in == Z_NULL ) {
      Abc_Print( -1 , "There is no ssat file %s\n" , argv[globalUtilOptind] );
      return 1;
   }
   pSsat = new SsatSolver;
   pSsat->readSSAT(in);
   gzclose(in);
   clk  = Abc_Clock();
   Abc_Print( -2 , "\n==== SSAT solving process ====\n" );
   pSsat->solveSsat( range , upper , lower , fAll , fMini , fBdd );
   Abc_Print( -2 , "\n  > Upper bound = %f\n" , pSsat->upperBound() );
   Abc_Print( -2 , "  > Lower bound = %f\n"   , pSsat->lowerBound() );
   Abc_PrintTime( 1 , "  > Time  " , Abc_Clock() - clk );
   printf("\n");
   delete pSsat;
   return 0;

usage:
   Abc_Print( -2 , "usage: ssat [-R <num>] [-U <num>] [-L <num>] [-ambh] <file>\n" );
   Abc_Print( -2 , "\t        Solve 2SSAT by Qesto and model counting / bdd signal prob\n" );
   Abc_Print( -2 , "\t-R <num>  : gap between upper and lower bounds, default=%f\n" , 0.01 );
   Abc_Print( -2 , "\t-U <num>  : number of UNSAT cubes for upper bound, default=%d (-1: construct only once)\n" , upper );
   Abc_Print( -2 , "\t-L <num>  : number of SAT   cubes for lower bound, default=%d (-1: construct only once)\n" , lower );
   Abc_Print( -2 , "\t-a        : toggles using All-SAT enumeration solve, default=true\n" );
   Abc_Print( -2 , "\t-m        : toggles using minimal UNSAT core, default=true\n" );
   Abc_Print( -2 , "\t-b        : toggles using BDD or Cachet to compute weight, default=Cachet\n" );
   Abc_Print( -2 , "\t-h        : prints the command summary\n" );
   Abc_Print( -2 , "\tfile      : the sdimacs file\n" );
   return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

