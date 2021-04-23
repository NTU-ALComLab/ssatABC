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

extern "C" void Ssat_Init(Abc_Frame_t*);
extern "C" void Ssat_End(Abc_Frame_t*);

// commands
static int SsatCommandSSAT(Abc_Frame_t* pAbc, int argc, char** argv);
static int SsatCommandBddSSAT(Abc_Frame_t* pAbc, int argc, char** argv);
static int SsatCommandBranchBound(Abc_Frame_t* pAbc, int argc, char** argv);
static int SsatCommandCktBddsp(Abc_Frame_t* pAbc, int argc, char** argv);
static int SsatCommandTest(Abc_Frame_t* pAbc, int argc, char** argv);
static bool Ssat_NtkReadQuan(char*);

// other helpers
static void sig_handler(int);
void initTimer(Ssat_Timer_t*);
void printTimer(Ssat_Timer_t*);
void initParams(Ssat_Params_t*);
static void printCommonParams(Ssat_Params_t*);
void printREParams(Ssat_Params_t*);
void printERParams(Ssat_Params_t*);

////////////////////////////////////////////////////////////////////////
///                        VARIABLES DECLARATIONS                    ///
////////////////////////////////////////////////////////////////////////

// global variables
SsatSolver* gloSsat;
Ssat_Timer_t timer;
abctime gloClk;
map<string, double> quanMap;  // Pi name -> quan prob , -1 means exist

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start / Stop the ssat package]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void Ssat_Init(Abc_Frame_t* pAbc) {
  gloSsat = NULL;
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  Cmd_CommandAdd(pAbc, "z SSAT", "ssat", SsatCommandSSAT, 0);
  Cmd_CommandAdd(pAbc, "z SSAT", "bddssat", SsatCommandBddSSAT, 0);
  Cmd_CommandAdd(pAbc, "z SSAT", "branchbound", SsatCommandBranchBound, 1);
  Cmd_CommandAdd(pAbc, "z SSAT", "cktbddsp", SsatCommandCktBddsp, 1);
  Cmd_CommandAdd(pAbc, "z SSAT", "ssat_test", SsatCommandTest, 0);
}

void Ssat_End(Abc_Frame_t* pAbc) {
  if (!quanMap.empty()) quanMap.clear();
  if (gloSsat) {
    delete gloSsat;
    gloSsat = NULL;
  }
}

/**Function*************************************************************

  Synopsis    [Paramter structure functions]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void initParams(Ssat_Params_t* pParams) {
  memset(pParams, 0, sizeof(Ssat_Params_t));
  pParams->range = 0.0;
  pParams->upper = -1;
  pParams->lower = -1;
  pParams->fGreedy = true;
  pParams->fSub = true;
  pParams->fPart = true;
  pParams->fDynamic = true;
  pParams->fPart2 = false;
  pParams->fBdd = true;
  pParams->fIncre = true;
  pParams->fIncre2 = false;
  pParams->fCkt = true;
  pParams->fPure = true;
  pParams->fAll = true;
  pParams->fMini = true;
  pParams->fTimer = false;
  pParams->fVerbose = false;
}

void printCommonParams(Ssat_Params_t* pParams) {
  printf("  > Counting engine: %s\n", pParams->fBdd ? "BDD" : "Cachet");
}

void printREParams(Ssat_Params_t* pParams) {
  printCommonParams(pParams);
  printf("  > Tolerable gap between the derived upper and lower bounds: %f\n",
         pParams->range);
  printf("  > Number of UNSAT cubes to update upper bounds: %d\n",
         pParams->upper);
  printf("  > Number of SAT cubes to update lower bounds: %d\n",
         pParams->lower);
  printf("  > SAT/UNSAT minterm generalization (minimal): %s\n",
         pParams->fMini ? "yes" : "no");
}

void printERParams(Ssat_Params_t* pParams) {
  printCommonParams(pParams);
  printf("  > Minimal clause selection (greedy): %s\n",
         pParams->fGreedy ? "yes" : "no");
  printf("  > Clause subsumption (subsume): %s\n",
         pParams->fSub ? "yes" : "no");
  printf("  > Partial assignment pruning (partial): %s\n",
         pParams->fPart ? "yes" : "no");
  // Original printing function is below
  // More options will be added to the above when they become relevant
  /*printf(
      "  > Using %s for counting, greedy=%s, subsume=%s, partial=%s, "
      "dynamic=%s, partial-2=%s, incre=%s, incre-2=%s, circuit=%s, pure=%s\n",
      pParams->fBdd ? "bdd" : "cachet", pParams->fGreedy ? "yes" : "no",
      pParams->fSub ? "yes" : "no", pParams->fPart ? "yes" : "no",
      pParams->fDynamic ? "yes" : "no", pParams->fPart2 ? "yes" : "no",
      pParams->fIncre ? "yes" : "no", pParams->fIncre2 ? "yes" : "no",
      pParams->fCkt ? "yes" : "no", pParams->fPure ? "yes" : "no");*/
}

/**Function*************************************************************

  Synopsis    [Runtime profiling functions]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void initTimer(Ssat_Timer_t* pTimer) {
  pTimer->timeS1 = 0;
  pTimer->timeS2 = 0;
  pTimer->timeGd = 0;
  pTimer->timeCt = 0;
  pTimer->timeCk = 0;
  pTimer->timeSt = 0;
  pTimer->timeBd = 0;
  pTimer->timeBest = 0;
  pTimer->nS1_sat = 0;
  pTimer->nS1_unsat = 0;
  pTimer->nS2solve = 0;
  pTimer->nGdsolve = 0;
  pTimer->nCount = 0;
  pTimer->lenBase = 0.0;
  pTimer->lenSubsume = 0.0;
  pTimer->lenPartial = 0.0;
  pTimer->lenDrop = 0.0;
  pTimer->avgDone = false;
  pTimer->avgDrop = 0;
}

void printTimer(Ssat_Timer_t* pTimer) {
  Abc_Print(-2, "\n==== Runtime profiling ====\n\n");
  Abc_PrintTime(1, "  > Time consumed on s1 solving ", pTimer->timeS1);
  Abc_PrintTime(1, "  > Time consumed on s2 solving ", pTimer->timeS2);
  Abc_PrintTime(1, "  > Time consumed on s2 greedy  ", pTimer->timeGd);
  Abc_PrintTime(1, "  > Time consumed on counting   ", pTimer->timeCt);
  Abc_PrintTime(1, "  > Time consumed on build  ckt ", pTimer->timeCk);
  Abc_PrintTime(1, "  > Time consumed on strash ckt ", pTimer->timeSt);
  Abc_PrintTime(1, "  > Time consumed on build  bdd ", pTimer->timeBd);
  Abc_Print(-2, "\n==== Solving profiling ====\n\n");
  Abc_Print(-2, "  > Number of s1 solving (SAT)          = %10d\n",
            pTimer->nS1_sat);
  Abc_Print(-2, "  > Number of s1 solving (UNSAT)        = %10d\n",
            pTimer->nS1_unsat);
  Abc_Print(-2, "  > Number of s2 solving                = %10d\n",
            pTimer->nS2solve);
  Abc_Print(-2, "  > Number of s2 successful greedy      = %10d\n",
            pTimer->nGdsolve);
  Abc_Print(-2, "  > Number of calls to model counting   = %10d\n",
            pTimer->nCount);
  Abc_Print(-2, "  > Average length of learnt (base)     = %10f\n",
            pTimer->lenBase / pTimer->nS1_sat);
  Abc_Print(-2, "  > Average length of learnt (subsume)  = %10f\n",
            pTimer->lenSubsume / pTimer->nS1_sat);
  Abc_Print(-2, "  > Average length of learnt (partial)  = %10f\n",
            pTimer->lenPartial / pTimer->nS1_sat);
  Abc_Print(-2, "  > Average number of dropped literals  = %10f\n",
            pTimer->lenDrop / pTimer->nS1_sat);
  printf("\n");
}

/**Function*************************************************************

  Synopsis    [Signal handling functions]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void sig_handler(int sig) {
  if (gloSsat) {
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

int SsatCommandSSAT(Abc_Frame_t* pAbc, int argc, char** argv) {
  SsatSolver* pSsat;
  gzFile in;
  int c;
  Ssat_Params_t Params, *pParams = &Params;
  initParams(pParams);
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "RULgspdqbijcramtvh")) != EOF) {
    switch (c) {
      case 'R':
        if (globalUtilOptind >= argc) {
          Abc_Print(-1,
                    "Command line switch \"-R\" should be followed by a "
                    "positive floating number.\n");
          goto usage;
        }
        pParams->range = atof(argv[globalUtilOptind]);
        globalUtilOptind++;
        if (pParams->range < 0.0 || pParams->range > 1.0) goto usage;
        break;
      case 'U':
        if (globalUtilOptind >= argc) {
          Abc_Print(-1,
                    "Command line switch \"-U\" should be followed by a "
                    "positive integer.\n");
          goto usage;
        }
        pParams->upper = atoi(argv[globalUtilOptind]);
        globalUtilOptind++;
        if ((pParams->upper < 0) && (pParams->upper != -1)) goto usage;
        break;
      case 'L':
        if (globalUtilOptind >= argc) {
          Abc_Print(-1,
                    "Command line switch \"-L\" should be followed by a "
                    "positive integer.\n");
          goto usage;
        }
        pParams->lower = atoi(argv[globalUtilOptind]);
        globalUtilOptind++;
        if ((pParams->lower < 0) && (pParams->lower != -1)) goto usage;
        break;
      case 'g':
        pParams->fGreedy ^= 1;
        break;
      case 's':
        pParams->fSub ^= 1;
        break;
      case 'p':
        pParams->fPart ^= 1;
        break;
      case 'd':
        pParams->fDynamic ^= 1;
        break;
      case 'q':
        pParams->fPart2 ^= 1;
        break;
      case 'b':
        pParams->fBdd ^= 1;
        break;
      case 'i':
        pParams->fIncre ^= 1;
        break;
      case 'j':
        pParams->fIncre2 ^= 1;
        break;
      case 'c':
        pParams->fCkt ^= 1;
        break;
      case 'r':
        pParams->fPure ^= 1;
        break;
      case 'a':
        pParams->fAll ^= 1;
        break;
      case 'm':
        pParams->fMini ^= 1;
        break;
      case 't':
        pParams->fTimer ^= 1;
        break;
      case 'v':
        pParams->fVerbose ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (pParams->fIncre2) {
    Abc_Print(0, "option \"j\" Incre-2 is currently under construction ...\n");
    goto usage;
  }
  if (argc != globalUtilOptind + 1) {
    Abc_Print(-1, "Missing ssat file!\n");
    goto usage;
  }
  gloClk = Abc_Clock();
  Abc_Print(-2, "[INFO] Input sdimacs file: %s\n", argv[globalUtilOptind]);
  in = gzopen(argv[globalUtilOptind], "rb");
  if (in == Z_NULL) {
    Abc_Print(-1, "There is no ssat file %s\n", argv[globalUtilOptind]);
    return 1;
  }
  gloSsat = pSsat = new SsatSolver(pParams->fTimer, pParams->fVerbose);
  initTimer(&timer);
  pSsat->readSSAT(in);
  gzclose(in);
  pSsat->solveSsat(pParams);
  pSsat->reportSolvingResults();
  Abc_PrintTime(1, "  > Time", Abc_Clock() - gloClk);
  delete pSsat;
  gloSsat = NULL;
  if (pParams->fTimer) printTimer(&timer);
  return 0;

usage:
  Abc_Print(-2,
            "usage: ssat [-R <num>] [-U <num>] [-L <num>] [-gspdqbijcramtvh] "
            "<file>\n");
  Abc_Print(
      -2,
      "\t        Solve 2SSAT by Qesto and model counting / bdd signal prob\n");
  Abc_Print(-2,
            "\t-R <num>  : gap between upper and lower bounds, default=%f\n",
            pParams->range);
  Abc_Print(-2,
            "\t-U <num>  : number of UNSAT cubes for upper bound, default=%d "
            "(-1: construct only once)\n",
            pParams->upper);
  Abc_Print(-2,
            "\t-L <num>  : number of SAT   cubes for lower bound, default=%d "
            "(-1: construct only once)\n",
            pParams->lower);
  Abc_Print(-2, "\t-g        : toggles using greedy heuristic, default=%s\n",
            pParams->fGreedy ? "yes" : "no");
  Abc_Print(-2,
            "\t-s        : toggles using subsumption simplify technique, "
            "default=%s\n",
            pParams->fSub ? "yes" : "no");
  Abc_Print(
      -2,
      "\t-p        : toggles using partial assignment technique, default=%s\n",
      pParams->fPart ? "yes" : "no");
  Abc_Print(-2, "\t-d        : toggles using dynamic dropping, default=%s\n",
            pParams->fDynamic ? "yes" : "no");
  Abc_Print(-2,
            "\t-q        : toggles using partial pruning ver.2, default=%s\n",
            pParams->fPart2 ? "yes" : "no");
  Abc_Print(-2,
            "\t-b        : toggles using BDD or Cachet to compute weight, "
            "default=%s\n",
            pParams->fBdd ? "bdd" : "cachet");
  Abc_Print(-2,
            "\t-i        : toggles using incremental counting, default=%s\n",
            pParams->fIncre ? "yes" : "no");
  Abc_Print(-2,
            "\t-j        : toggles using incremental counting ver.2, "
            "default=%s (WARNING: under construction, do NOT use)\n",
            pParams->fIncre2 ? "yes" : "no");
  Abc_Print(-2,
            "\t-c        : toggles using circuit for counting, default=%s\n",
            pParams->fCkt ? "yes" : "no");
  Abc_Print(-2, "\t-r        : toggles using pure literal, default=%s\n",
            pParams->fPure ? "yes" : "no");
  Abc_Print(
      -2, "\t-a        : toggles using All-SAT enumeration solve, default=%s\n",
      pParams->fAll ? "yes" : "no");
  Abc_Print(-2, "\t-m        : toggles using minimal UNSAT core, default=%s\n",
            pParams->fMini ? "yes" : "no");
  Abc_Print(-2,
            "\t-t        : toggles profiling of runtime and solving "
            "statistics, default=%s\n",
            pParams->fTimer ? "yes" : "no");
  Abc_Print(-2, "\t-v        : toggles verbose information, default=%s\n",
            pParams->fVerbose ? "yes" : "no");
  Abc_Print(-2, "\t-h        : prints the command summary\n");
  Abc_Print(-2, "\tfile      : the sdimacs file\n");
  return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

int SsatCommandBddSSAT(Abc_Frame_t* pAbc, int argc, char** argv) {
  SsatSolver* pSsat;
  gzFile in;
  int c;
  bool fGroup, fReorder, fVerbose, fTimer;

  fGroup = false;
  fReorder = false;
  fVerbose = true;
  fTimer = true;

  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "grvth")) != EOF) {
    switch (c) {
      case 'g':
        fGroup ^= 1;
        break;
      case 'r':
        fReorder ^= 1;
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
  if (argc != globalUtilOptind + 1) {
    Abc_Print(-1, "Missing ssat file!\n");
    goto usage;
  }
  in = gzopen(argv[globalUtilOptind], "rb");
  if (in == Z_NULL) {
    Abc_Print(-1, "There is no ssat file %s\n", argv[globalUtilOptind]);
    return 1;
  }
  gloSsat = pSsat = new SsatSolver(fVerbose, fTimer);
  pSsat->readSSAT(in);
  gzclose(in);
  gloClk = Abc_Clock();
  Abc_Print(-2, "\n==== Bdd SSAT solving process ====\n");
  pSsat->bddSolveSsat(fGroup, fReorder);
  Abc_Print(-2, "\n==== SSAT solving result ====\n");
  Abc_Print(-2, "\n  > Upper bound = %e\n", pSsat->upperBound());
  Abc_Print(-2, "  > Lower bound = %e\n", pSsat->lowerBound());
  Abc_PrintTime(1, "  > Time       ", Abc_Clock() - gloClk);
  printf("\n");
  delete pSsat;
  gloSsat = NULL;
  if (fTimer) printTimer(&timer);
  printf("\n");
  return 0;

usage:
  Abc_Print(-2, "usage: bddssat [-grh] <file>\n");
  Abc_Print(-2, "\t        Solve SSAT by BDD\n");
  Abc_Print(
      -2, "\t-g        : toggles using grouping (only for 2SSAT), default=%s\n",
      fGroup ? "yes" : "no");
  Abc_Print(-2, "\t-r        : toggles using reordering, default=%s\n",
            fReorder ? "yes" : "no");
  Abc_Print(-2, "\t-v        : toggles verbose information, default=%s\n",
            fVerbose ? "yes" : "no");
  Abc_Print(-2, "\t-t        : toggles runtime information, default=%s\n",
            fTimer ? "yes" : "no");
  Abc_Print(-2, "\t-h        : prints the command summary\n");
  Abc_Print(-2, "\tfile      : the sdimacs file\n");
  return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

int SsatCommandCktBddsp(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk;
  Abc_Obj_t* pObj;
  char* pFileName;
  char sCmd[1000];
  int fResyn, numExist = 0, i, c;
  gloClk = Abc_Clock();

  fResyn = 1;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "sh")) != EOF) {
    switch (c) {
      case 's':
        fResyn ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (argc != globalUtilOptind + 1) {
    Abc_Print(-1, "Missing probabilistic network file!\n");
    goto usage;
  }
  pFileName = argv[globalUtilOptind];
  if (Io_ReadFileType(pFileName) != IO_FILE_BLIF) {
    Abc_Print(-1, "Only support blif format for now!\n");
    return 1;
  }
  pNtk = Io_Read(pFileName, Io_ReadFileType(pFileName), 1, 0);
  if (!pNtk) {
    Abc_Print(-1, "Reading network %s has failed!\n", pFileName);
    return 1;
  }
  // read quantification structure and build global map
  if (!Ssat_NtkReadQuan(pFileName)) {
    Abc_Print(-1, "Reading quantification has failed!\n");
    Abc_NtkDelete(pNtk);
    return 1;
  }
  Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
  fResyn ? Cmd_CommandExecute(pAbc, "resyn2") : Cmd_CommandExecute(pAbc, "st");
  Cmd_CommandExecute(pAbc, "sst");
  pNtk = Abc_FrameReadNtk(pAbc);
  Abc_NtkForEachPi(pNtk, pObj, i) if (pObj->dTemp == -1)++ numExist;

  // Abc_ObjForEachFanout( Abc_NtkPi(pNtk,0) , pObj , i )
  // Abc_ObjPatchFanin( pObj , Abc_NtkPi(pNtk,0),
  // Abc_ObjNot(Abc_AigConst1(pNtk)) );

  // Cmd_CommandExecute( pAbc , "st" );
  // Cmd_CommandExecute( pAbc , "sst" );

  sprintf(sCmd, "bddsp -g -E %d", numExist);
  Cmd_CommandExecute(pAbc, sCmd);

  /*Abc_ObjForEachFanout( Abc_AigConst1(pNtk) , pObj , i )
     Abc_ObjPatchFanin( pObj , Abc_AigConst1(pNtk) , Abc_NtkPi(pNtk,0) );

  Abc_ObjForEachFanout( Abc_NtkPi(pNtk,0) , pObj , i )
     Abc_ObjPatchFanin( pObj , Abc_NtkPi(pNtk,0), Abc_AigConst1(pNtk) );

  Cmd_CommandExecute( pAbc , sCmd );*/
  Abc_PrintTime(1, "  > Time consumed", Abc_Clock() - gloClk);
  return 0;

usage:
  Abc_Print(-2, "usage: cktbddsp [-sh] <file>\n");
  Abc_Print(
      -2, "\t     read probabilistic network in blif format and apply bddsp\n");
  Abc_Print(-2, "\t-s        : toggles using resyn2 , default = yes\n");
  Abc_Print(-2, "\t-h        : prints the command summary\n");
  Abc_Print(-2, "\tfile      : the blif file\n");
  return 1;
}

bool Ssat_NtkReadQuan(char* pFileName) {
  if (!quanMap.empty()) quanMap.clear();
  string tmpStr;
  char tmp[1024];
  ifstream in;
  char *pch, *name, *quan;
  double prob;

  in.open(pFileName);
  while (getline(in, tmpStr)) {
    strcpy(tmp, tmpStr.c_str());
    pch = strtok(tmp, " ");
    if (!pch || strcmp(pch, "c")) continue;
    pch = strtok(NULL, " ");
    if (!pch || strcmp(pch, "input")) continue;
    name = strtok(NULL, " ");
    if (!name) {
      cout << "[Error] Missing Pi name in line: " << tmp << endl;
      return false;
    }
    quan = strtok(NULL, " ");
    if (!quan || strcmp(quan, "E") && strcmp(quan, "R")) {
      cout << "[Error] Unknown quantifier in line: " << tmp << endl;
      return false;
    }
    if (!strcmp(quan, "E"))
      prob = -1;
    else {
      pch = strtok(NULL, " ");
      if (!pch) {
        cout << "[Error] Missing Prob after R in line: " << tmp << endl;
        return false;
      }
      prob = atof(pch);
    }
    map<string, double>::iterator it = quanMap.find(string(name));
    if (it != quanMap.end()) {
      cout << "[Error] Pi " << name
           << " quantifier redefined in line: " << tmpStr << endl;
      cout << "first define :  " << it->first << " --> " << it->second << endl;
      it = quanMap.begin();
      while (it != quanMap.end()) {
        cout << it->first << " --> " << it->second << endl;
        ++it;
      }
      return false;
    }
    quanMap.insert(pair<string, double>(string(name), prob));
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

int SsatCommandBranchBound(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk;
  SsatSolver* pSsat;
  char* pFileName;
  int fResyn, c;
  gloClk = Abc_Clock();

  fResyn = 1;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "sh")) != EOF) {
    switch (c) {
      case 's':
        fResyn ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (argc != globalUtilOptind + 1) {
    Abc_Print(-1, "Missing probabilistic network file!\n");
    goto usage;
  }
  pFileName = argv[globalUtilOptind];
  if (Io_ReadFileType(pFileName) != IO_FILE_BLIF) {
    Abc_Print(-1, "Only support blif format for now!\n");
    return 1;
  }
  pNtk = Io_Read(pFileName, Io_ReadFileType(pFileName), 1, 0);
  if (!pNtk) {
    Abc_Print(-1, "Reading network %s has failed!\n", pFileName);
    return 1;
  }
  // read quantification structure and build global map
  if (!Ssat_NtkReadQuan(pFileName)) {
    Abc_Print(-1, "Reading quantification has failed!\n");
    Abc_NtkDelete(pNtk);
    return 1;
  }
  Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
  fResyn ? Cmd_CommandExecute(pAbc, "resyn2") : Cmd_CommandExecute(pAbc, "st");
  Cmd_CommandExecute(pAbc, "sst");
  pNtk = Abc_FrameReadNtk(pAbc);
  pSsat = new SsatSolver;
  pSsat->solveBranchBound(pNtk);
  delete pSsat;
  Abc_PrintTime(1, "  > Time consumed", Abc_Clock() - gloClk);
  return 0;

usage:
  Abc_Print(-2, "usage: branchbound [-sh] <file>\n");
  Abc_Print(-2, "\t     read prob ckt and solve by branch and bound method\n");
  Abc_Print(-2, "\t-s        : toggles using resyn2 , default = yes\n");
  Abc_Print(-2, "\t-h        : prints the command summary\n");
  Abc_Print(-2, "\tfile      : the blif file\n");
  return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

int SsatCommandTest(Abc_Frame_t* pAbc, int argc, char** argv) {
  map<string, double>::iterator it;
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  Abc_Obj_t* pObj;
  int i;

  if (!pNtk) return 1;
  Abc_NtkForEachPi(pNtk, pObj, i) {
    it = quanMap.find(string(Abc_ObjName(pObj)));
    if (it == quanMap.end()) {
      Abc_Print(-1, "Pi %s has no quantifier!\n", Abc_ObjName(pObj));
      Abc_NtkDelete(pNtk);
      return 1;
    }
    pObj->dTemp = (float)it->second;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
