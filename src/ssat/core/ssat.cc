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
using namespace std;

////////////////////////////////////////////////////////////////////////
///                        FUNCTION DECLARATIONS                     ///
////////////////////////////////////////////////////////////////////////

extern "C" void Ssat_Init ( Abc_Frame_t * );
extern "C" void Ssat_End  ( Abc_Frame_t * );

// commands
static int SsatCommandSSAT         ( Abc_Frame_t * pAbc , int argc , char ** argv );
static int SsatCommandBranchBound  ( Abc_Frame_t * pAbc , int argc , char ** argv );
static int SsatCommandCktBddsp     ( Abc_Frame_t * pAbc , int argc , char ** argv );
static int SsatCommandTest         ( Abc_Frame_t * pAbc , int argc , char ** argv );
static bool Ssat_NtkReadQuan       ( char * );

// other helpers
static void sig_handler  ( int );
static void initTimer    ( SsatTimer* );
void        printTimer   ( SsatTimer* );

////////////////////////////////////////////////////////////////////////
///                        VARIABLES DECLARATIONS                    ///
////////////////////////////////////////////////////////////////////////

SsatSolver         * gloSsat;
SsatTimer            timer;
abctime              gloClk;
map<string,double>   quanMap; // Pi name -> quan prob , -1 means exist

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start / Stop the ssat package]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void 
Ssat_Init( Abc_Frame_t * pAbc )
{
   gloSsat = NULL;
   initTimer( &timer );
   signal( SIGINT  , sig_handler );
   signal( SIGTERM , sig_handler );
   Cmd_CommandAdd( pAbc , "z SSAT" , "ssat"        , SsatCommandSSAT        , 0 );
   Cmd_CommandAdd( pAbc , "z SSAT" , "branchbound" , SsatCommandBranchBound , 1 );
   Cmd_CommandAdd( pAbc , "z SSAT" , "cktbddsp"    , SsatCommandCktBddsp    , 1 );
   Cmd_CommandAdd( pAbc , "z SSAT" , "ssat_test"   , SsatCommandTest        , 0 );
}

void 
Ssat_End( Abc_Frame_t * pAbc )
{
   if ( !quanMap.empty() ) quanMap.clear();
   if ( gloSsat ) {
      delete gloSsat;
      gloSsat = NULL;
   }
}

/**Function*************************************************************

  Synopsis    [Runtime profiling functions]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
initTimer( SsatTimer * pTimer )
{
   pTimer->timeS1     = 0;
   pTimer->timeS2     = 0;
   pTimer->timeGd     = 0;
   pTimer->timeCa     = 0;
   pTimer->nS1solve   = 0;
   pTimer->nGdsolve   = 0;
   pTimer->nS2solve   = 0;
   pTimer->nCachet    = 0;
   pTimer->lenBase    = 0.0;
   pTimer->lenPartial = 0.0;
   pTimer->lenSubsume = 0.0;
}

void
printTimer( SsatTimer * pTimer )
{
   Abc_Print( -2 , "\n==== Runtime profiling ====\n\n" );
   Abc_PrintTime( 1 , "  > Time consumed on s1 solving" , pTimer->timeS1 );
   Abc_PrintTime( 1 , "  > Time consumed on s2 solving" , pTimer->timeS2 );
   Abc_PrintTime( 1 , "  > Time consumed on s2 greedy " , pTimer->timeGd );
   Abc_PrintTime( 1 , "  > Time consumed on Cachet    " , pTimer->timeCa );
   Abc_PrintTime( 1 , "  > Total elapsed time         " , Abc_Clock()-gloClk );
   Abc_Print( -2 , "\n==== Solving profiling ====\n\n" );
   Abc_Print( -2 , "  > Number of s1 solving                = %10d\n" , pTimer->nS1solve  );
   Abc_Print( -2 , "  > Number of s2 solving                = %10d\n" , pTimer->nS2solve  );
   Abc_Print( -2 , "  > Number of s2 greedy                 = %10d\n" , pTimer->nGdsolve  );
   Abc_Print( -2 , "  > Number of calls to Cachet           = %10d\n" , pTimer->nCachet   );
   Abc_Print( -2 , "  > Average length of learnt (base)     = %10f\n" , pTimer->lenBase    / pTimer->nS2solve );
   Abc_Print( -2 , "  > Average length of learnt (partial)  = %10f\n" , pTimer->lenPartial / pTimer->nS2solve );
   Abc_Print( -2 , "  > Average length of learnt (subsume)  = %10f\n" , pTimer->lenSubsume / pTimer->nS2solve );
}

/**Function*************************************************************

  Synopsis    [Signal handling functions]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void
sig_handler( int sig )
{
   if ( gloSsat ) {
      gloSsat->interrupt();
      delete gloSsat;
      gloSsat = NULL;
   }
   Abc_Stop();
   exit(1);
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
   double range;
   int upper , lower , c;
   bool fAll , fMini , fBdd , fPart , fSub , fGreedy , fVerbose , fTimer;

   range    = 0.0;
   upper    = 16;
   lower    = 65536;
   fAll     = true;
   fMini    = true;
   fBdd     = false;
   fPart    = true;
   fSub     = false;
   fGreedy  = false;
   fVerbose = true;
   fTimer   = true;
   Extra_UtilGetoptReset();
   while ( ( c = Extra_UtilGetopt( argc, argv, "RULambpsgvth" ) ) != EOF )
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
         case 'p':
            fPart ^= 1;
            break;
         case 's':
            fSub ^= 1;
            break;
         case 'g':
            fGreedy ^= 1;
            break;
         case 'v':
            fVerbose ^= 1;
            break;
         case 't':
            fTimer ^= 1;
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
   gloSsat = pSsat = new SsatSolver( fVerbose , fTimer );
   pSsat->readSSAT(in);
   gzclose(in);
   gloClk = Abc_Clock();
   Abc_Print( -2 , "\n==== SSAT solving process ====\n" );
   pSsat->solveSsat( range , upper , lower , fAll , fMini , fBdd , fPart , fSub , fGreedy );
   Abc_Print( -2 , "\n==== SSAT solving result ====\n" );
   Abc_Print( -2 , "\n  > Upper bound = %e\n" , pSsat->upperBound() );
   Abc_Print( -2 , "  > Lower bound = %e\n"   , pSsat->lowerBound() );
   Abc_PrintTime( 1 , "  > Time       " , Abc_Clock() - gloClk );
   delete pSsat;
   gloSsat = NULL;
   if ( fTimer ) printTimer( &timer );
   printf( "\n" );
   return 0;

usage:
   Abc_Print( -2 , "usage: ssat [-R <num>] [-U <num>] [-L <num>] [-ambpsgvth] <file>\n" );
   Abc_Print( -2 , "\t        Solve 2SSAT by Qesto and model counting / bdd signal prob\n" );
   Abc_Print( -2 , "\t-R <num>  : gap between upper and lower bounds, default=%f\n" , range );
   Abc_Print( -2 , "\t-U <num>  : number of UNSAT cubes for upper bound, default=%d (-1: construct only once)\n" , upper );
   Abc_Print( -2 , "\t-L <num>  : number of SAT   cubes for lower bound, default=%d (-1: construct only once)\n" , lower );
   Abc_Print( -2 , "\t-a        : toggles using All-SAT enumeration solve, default=%s\n" , fAll ? "yes" : "no" );
   Abc_Print( -2 , "\t-m        : toggles using minimal UNSAT core, default=%s\n" , fMini ? "yes" : "no" );
   Abc_Print( -2 , "\t-b        : toggles using BDD or Cachet to compute weight, default=%s\n" , fBdd ? "bdd" : "cachet" );
   Abc_Print( -2 , "\t-p        : toggles using partial assignment technique, default=%s\n" , fPart ? "yes" : "no" );
   Abc_Print( -2 , "\t-s        : toggles using subsumption simplify technique, default=%s\n" , fSub ? "yes" : "no" );
   Abc_Print( -2 , "\t-g        : toggles using greedy heuristic, default=%s\n" , fGreedy ? "yes" : "no" );
   Abc_Print( -2 , "\t-v        : toggles verbose information, default=%s\n" , fVerbose ? "yes" : "no" );
   Abc_Print( -2 , "\t-t        : toggles runtime information, default=%s\n" , fTimer ? "yes" : "no" );
   Abc_Print( -2 , "\t-h        : prints the command summary\n" );
   Abc_Print( -2 , "\tfile      : the sdimacs file\n" );
   return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int 
SsatCommandCktBddsp( Abc_Frame_t * pAbc , int argc , char ** argv )
{
   Abc_Ntk_t * pNtk;
   Abc_Obj_t * pObj;
   char * pFileName;
   char sCmd[1000];
   int fResyn , numExist = 0 , i , c;
   gloClk = Abc_Clock();

   fResyn = 1;
   Extra_UtilGetoptReset();
   while ( ( c = Extra_UtilGetopt( argc, argv, "sh" ) ) != EOF )
   {
      switch ( c )
      {
         case 's':
            fResyn ^= 1;
            break;
         case 'h':
            goto usage;
         default:
            goto usage;
      }
   }
   if ( argc != globalUtilOptind + 1 ) {
      Abc_Print( -1 , "Missing probabilistic network file!\n" );
      goto usage;
   }
   pFileName = argv[globalUtilOptind];
   if ( Io_ReadFileType( pFileName ) != IO_FILE_BLIF ) {
      Abc_Print( -1 , "Only support blif format for now!\n" );
      return 1;
   }
   pNtk = Io_Read( pFileName , Io_ReadFileType(pFileName) , 1 , 0 );
   if ( !pNtk ) {
      Abc_Print( -1 , "Reading network %s has failed!\n" , pFileName );
      return 1;
   }
   // read quantification structure and build global map
   if ( !Ssat_NtkReadQuan( pFileName ) ) {
      Abc_Print( -1 , "Reading quantification has failed!\n" );
      Abc_NtkDelete( pNtk );
      return 1;
   }
   Abc_FrameReplaceCurrentNetwork( pAbc , pNtk );
   fResyn ? Cmd_CommandExecute( pAbc , "resyn2" ) : Cmd_CommandExecute( pAbc , "st" );
   Cmd_CommandExecute( pAbc , "sst" );
   pNtk = Abc_FrameReadNtk( pAbc );
   Abc_NtkForEachPi( pNtk , pObj , i )
      if ( pObj->dTemp == -1 ) ++numExist;

   //Abc_ObjForEachFanout( Abc_NtkPi(pNtk,0) , pObj , i )
     // Abc_ObjPatchFanin( pObj , Abc_NtkPi(pNtk,0), Abc_ObjNot(Abc_AigConst1(pNtk)) );
   
  // Cmd_CommandExecute( pAbc , "st" );
  // Cmd_CommandExecute( pAbc , "sst" );
   
   sprintf( sCmd , "bddsp -g -E %d" , numExist );
   Cmd_CommandExecute( pAbc , sCmd );
   
   /*Abc_ObjForEachFanout( Abc_AigConst1(pNtk) , pObj , i )
      Abc_ObjPatchFanin( pObj , Abc_AigConst1(pNtk) , Abc_NtkPi(pNtk,0) );
   
   Abc_ObjForEachFanout( Abc_NtkPi(pNtk,0) , pObj , i )
      Abc_ObjPatchFanin( pObj , Abc_NtkPi(pNtk,0), Abc_AigConst1(pNtk) );
   
   Cmd_CommandExecute( pAbc , sCmd );*/
   Abc_PrintTime( 1 , "  > Time consumed" , Abc_Clock() - gloClk );
   return 0;

usage:
   Abc_Print( -2 , "usage: cktbddsp [-sh] <file>\n" );
   Abc_Print( -2 , "\t     read probabilistic network in blif format and apply bddsp\n" );
   Abc_Print( -2 , "\t-s        : toggles using resyn2 , default = yes\n" );
   Abc_Print( -2 , "\t-h        : prints the command summary\n" );
   Abc_Print( -2 , "\tfile      : the blif file\n" );
   return 1;
}

bool
Ssat_NtkReadQuan( char * pFileName )
{
   if ( !quanMap.empty() ) quanMap.clear();
   string tmpStr;
   char tmp[1024];
   ifstream in;
   char * pch , * name , * quan;
   double prob;

   in.open( pFileName );
   while ( getline( in , tmpStr ) ) {
      strcpy( tmp , tmpStr.c_str() );
      pch = strtok( tmp , " " );
      if ( !pch || strcmp( pch , "c" ) ) continue;
      pch = strtok( NULL , " " );
      if ( !pch || strcmp( pch , "input" ) ) continue;
      name = strtok( NULL , " " );
      if ( !name ) {
         cout << "[Error] Missing Pi name in line: " << tmp << endl;
         return false;
      }
      quan = strtok( NULL , " " );
      if ( !quan || strcmp( quan , "E" ) && strcmp( quan , "R" ) ) {
         cout << "[Error] Unknown quantifier in line: " << tmp << endl;
         return false;
      }
      if ( !strcmp( quan , "E" ) ) prob = -1;
      else {
         pch = strtok( NULL , " " );
         if ( !pch ) {
            cout << "[Error] Missing Prob after R in line: " << tmp << endl;
            return false;
         }
         prob = atof( pch );
      }
      map<string,double>::iterator it = quanMap.find(string(name));
      if ( it != quanMap.end() ) {
         cout << "[Error] Pi " << name << " quantifier redefined in line: " << tmpStr << endl;
         cout << "first define :  " << it->first << " --> " << it->second << endl;
         it = quanMap.begin();
         while ( it != quanMap.end() ) {
            cout << it->first << " --> " << it->second << endl;
            ++it;
         }
         return false;
      }
      quanMap.insert( pair<string,double>(string(name),prob) );
   }
   in.close();
   return true;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int 
SsatCommandBranchBound( Abc_Frame_t * pAbc , int argc , char ** argv )
{
   Abc_Ntk_t * pNtk;
   SsatSolver * pSsat;
   char * pFileName;
   int fResyn , c;
   gloClk = Abc_Clock();

   fResyn = 1;
   Extra_UtilGetoptReset();
   while ( ( c = Extra_UtilGetopt( argc, argv, "sh" ) ) != EOF )
   {
      switch ( c )
      {
         case 's':
            fResyn ^= 1;
            break;
         case 'h':
            goto usage;
         default:
            goto usage;
      }
   }
   if ( argc != globalUtilOptind + 1 ) {
      Abc_Print( -1 , "Missing probabilistic network file!\n" );
      goto usage;
   }
   pFileName = argv[globalUtilOptind];
   if ( Io_ReadFileType( pFileName ) != IO_FILE_BLIF ) {
      Abc_Print( -1 , "Only support blif format for now!\n" );
      return 1;
   }
   pNtk = Io_Read( pFileName , Io_ReadFileType(pFileName) , 1 , 0 );
   if ( !pNtk ) {
      Abc_Print( -1 , "Reading network %s has failed!\n" , pFileName );
      return 1;
   }
   // read quantification structure and build global map
   if ( !Ssat_NtkReadQuan( pFileName ) ) {
      Abc_Print( -1 , "Reading quantification has failed!\n" );
      Abc_NtkDelete( pNtk );
      return 1;
   }
   Abc_FrameReplaceCurrentNetwork( pAbc , pNtk );
   fResyn ? Cmd_CommandExecute( pAbc , "resyn2" ) : Cmd_CommandExecute( pAbc , "st" );
   Cmd_CommandExecute( pAbc , "sst" );
   pNtk = Abc_FrameReadNtk( pAbc );
   pSsat = new SsatSolver;
   pSsat->solveBranchBound( pNtk );
   delete pSsat;
   Abc_PrintTime( 1 , "  > Time consumed" , Abc_Clock() - gloClk );
   return 0;

usage:
   Abc_Print( -2 , "usage: branchbound [-sh] <file>\n" );
   Abc_Print( -2 , "\t     read prob ckt and solve by branch and bound method\n" );
   Abc_Print( -2 , "\t-s        : toggles using resyn2 , default = yes\n" );
   Abc_Print( -2 , "\t-h        : prints the command summary\n" );
   Abc_Print( -2 , "\tfile      : the blif file\n" );
   return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int 
SsatCommandTest( Abc_Frame_t * pAbc , int argc , char ** argv )
{
   map<string,double>::iterator it;
   Abc_Ntk_t * pNtk = Abc_FrameReadNtk( pAbc );
   Abc_Obj_t * pObj;
   int i;

   if ( !pNtk ) return 1;
   Abc_NtkForEachPi( pNtk , pObj , i )
   {
      it = quanMap.find( string(Abc_ObjName(pObj)) );
      if ( it == quanMap.end() ) {
         Abc_Print( -1 , "Pi %s has no quantifier!\n" , Abc_ObjName(pObj) );
         Abc_NtkDelete( pNtk );
         return 1;
      }
      pObj->dTemp = (float)it->second;
   }
   return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

