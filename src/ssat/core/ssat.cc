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

//ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern "C" void Ssat_Init ( Abc_Frame_t * );
extern "C" void Ssat_End  ( Abc_Frame_t * );

extern void Ssat_CubeToNtk( SsatSolver& );
static int SsatCommandHello        ( Abc_Frame_t * pAbc, int argc, char **argv );

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
   Cmd_CommandAdd( pAbc , "z SSAT" , "hello" , SsatCommandHello , 0 );
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
SsatCommandHello( Abc_Frame_t * pAbc , int argc , char ** argv )
{
   SsatSolver * pSsat = new SsatSolver;
   gzFile in;
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
   in = gzopen( argv[1] , "rb" );
   pSsat->readSSAT(in);
   gzclose(in);
   printf( "  > Answer: %f\n" , pSsat->ssolve() );

  // extern void Ssat_CubeToNtk( SsatSolver& );
   Ssat_CubeToNtk( *pSsat );
   delete pSsat;
   return 0;

usage:
   fprintf( pAbc->Err, "usage: hello [-h]\n" );
   fprintf( pAbc->Err, "\t         Let ABC say hello to everyone\n" );
   fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
   return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


//ABC_NAMESPACE_IMPL_END
