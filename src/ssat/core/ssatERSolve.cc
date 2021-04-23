/**CFile****************************************************************

  FileName    [SsatERSolver.cc]

  SystemName  [ssatQesto]

  Synopsis    [Implementations of member functions for ssatSolver]

  Author      []

  Affiliation [NTU]

  Date        [14, Jan., 2017]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>

#include <iostream>

#include "ssat/core/Dimacs.h"
#include "ssat/core/Solver.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/core/SsatSolver.h"
#include "ssat/mtl/Sort.h"
#include "ssat/utils/ParseUtils.h"

using namespace Minisat;
using namespace std;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// global variables
extern Ssat_Timer_t timer;
extern abctime gloClk;
// functions
extern void printERParams(Ssat_Params_t*);
static void setDropVec(vec<bool>&, const int&);
static void convertClaCube(const vec<Lit>&, vec<Lit>&);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Solve ER or ERE type of SSAT.]

  Description [Use BDD or Cachet to evaluate weight.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::erSolve2SSAT(Ssat_Params_t* pParams) {
  if (_fVerbose) {
    printf("[INFO] Invoking erSSAT solver with the following configuration:\n");
    printERParams(pParams);
  }
  _s1->simplify();
  checkERParams(pParams);
  _s2 = pParams->fGreedy ? buildQestoSelector() : buildERSelector();
  assertUnitClause();
  _selClaId.capacity(_s1->nClauses());
  if (pParams->fBdd) initERBddCount(pParams);
  if (pParams->fSub) buildSubsumeTable(*_s1);
  if (pParams->fPure) assertPureLit();

  // declare and initialize temp variables
  vec<Lit> eLits(_rootVars[0].size()), sBkCla;
  vec<bool> dropVec(_rootVars[0].size(), false);
  int lenBeforeDrop = 0;
  double subvalue;
  bool sat;
  abctime clk = 0, clk1 = Abc_Clock();
  _erModel.capacity(_rootVars[0].size());
  _erModel.clear();
  printf("[INFO] Starting analysis ...\n");
  // main loop, pseudo code line04-14
  while (true) {
    if (_fTimer) clk = Abc_Clock();
    sat = _s2->solve();
    if (_fTimer) {
      timer.timeS2 += Abc_Clock() - clk;
      ++timer.nS2solve;
    }
    if (!sat) {  // _s2 UNSAT --> main loop terminate
      printf("[INFO] Stopping analysis ...\n");
      _fExactlySolved = true;
      _exactSatProb = _satPb;
      if (_fVerbose) {
        printf("  > Found an optimizing assignment to exist vars:\n\t");
        dumpCla(_erModel);
      }
      break;
    }
    getExistAssignment(eLits);                      // line05
    if (pParams->fGreedy) selectMinClauses(eLits);  // line07
    if (_fTimer) clk = Abc_Clock();
    sat = _s1->solve(eLits);
    if (_fTimer) timer.timeS1 += Abc_Clock() - clk;
    if (!sat) {  // UNSAT case, line12-13
      if (_fTimer) ++timer.nS1_unsat;
      if (pParams->fMini) {
        sBkCla.clear();
        miniUnsatCore(_s1->conflict, sBkCla);
        _s2->addClause(sBkCla);
      } else
        _s2->addClause(_s1->conflict);
    } else {  // SAT case
      if (_fTimer) ++timer.nS1_sat;
      collectBkClaERSub(sBkCla, pParams->fSub);  // clause containment learning
                                                 // + subsumption: line08,10
      if (_fTimer) {
        if (pParams->fSub)
          timer.lenSubsume += sBkCla.size();
        else
          timer.lenBase += sBkCla.size();
        if (pParams->fDynamic) lenBeforeDrop = sBkCla.size();
      }
      if (_fTimer) clk = Abc_Clock();
      subvalue = erSolveWMC(pParams, eLits, dropVec, eLits.size());
      if (_fTimer) {
        timer.timeCt += Abc_Clock() - clk;
        ++timer.nCount;
      }
      if (subvalue == 1) {  // early termination
        _satPb = subvalue;
        timer.timeBest = Abc_Clock() - gloClk;
        eLits.copyTo(_erModel);
        printf("[INFO] Stopping analysis ...\n");
        _fExactlySolved = true;
        _exactSatProb = _satPb;
        if (_fVerbose) {
          printf("  > Found an optimizing assignment to exist vars:\n\t");
          dumpCla(_erModel);
        }
        break;
      }
      if (subvalue >= _satPb) {  // update current solution
        if (_fVerbose) {
          if (subvalue > _satPb) {
            printf("  > Found a better solution , value = %f\n\t", subvalue);
            dumpCla(eLits);
            Abc_PrintTime(1, "  > Time consumed", Abc_Clock() - clk1);
            fflush(stdout);
          }
        }
        if (subvalue > _satPb) timer.timeBest = Abc_Clock() - gloClk;
        _satPb = subvalue;
        eLits.copyTo(_erModel);
      }
      if (pParams->fPart)
        discardLit(pParams, sBkCla);  // partial assignment pruning: line11
      else if (pParams->fPart2)
        discardAllLit(pParams, sBkCla);
      _s2->addClause(sBkCla);
      if (_fTimer) {
        timer.lenPartial += sBkCla.size();
        if (pParams->fDynamic) {  // record average # of dropped lits
          timer.lenDrop += lenBeforeDrop - sBkCla.size();
          if (!timer.avgDone &&
              timer.nS2solve >= 500) {  // 500 is a magic number: tune it!
            timer.avgDone = true;
            timer.avgDrop = (int)(timer.lenDrop / timer.nS1_sat);
            printf("  > average done, avg drop = %d\n", timer.avgDrop);
          }
        }
      }
    }
  }
}

void SsatSolver::checkERParams(Ssat_Params_t* pParams) {
  if (pParams->fSub && _s1->nClauses() < 2) {
    Abc_Print(0, "Disable subsumption as there are less than 2 clauses\n");
    pParams->fSub = false;
  }
  if (pParams->fPart && pParams->fPart2) {
    Abc_Print(0, "Partial assignment pruning ver.1 and ver.2 both enabled\n");
    Abc_Print(0, "Ver.2 will be used\n");
    pParams->fPart = false;
  }
  if (pParams->fPart2 && !pParams->fBdd) {
    Abc_Print(0, "Partial assignment pruning ver.2 only works with BDD\n");
    Abc_Print(0, "Switch the counting engine from Cachet to BDD\n");
    pParams->fBdd = true;
  }
}

/**Function*************************************************************

  Synopsis    [Assert unit clauses in the matrix.]

  Description [For an existential var, assert it in the selector;
               for a random var, record its weight in the multiplier]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::assertUnitClause() {
  for (Var v = 0; v < _s1->assigns.size(); ++v) {
    if (_s1->assigns[v] == l_Undef) continue;
    Lit p = mkLit(v, _s1->assigns[v] == l_False);
    if (_level[v] == 0 && isEVar(v)) {
      _s2->addClause(p);
    }
    if (isRVar(v)) {
      _unitClauseMultiplier = !sign(p) ? _unitClauseMultiplier * _quan[v]
                                       : _unitClauseMultiplier * (1 - _quan[v]);
    }
  }
}

/**Function*************************************************************

  Synopsis    [Assign pure X literals to be true.]

  Description [If some X literal is pure, always deselect.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::assertPureLit() {
  vec<int> phase(_s1->nVars(), -1);  // -1: default, 0:pos, 1:neg, 2: both
  for (int i = 0; i < _s1->nClauses(); ++i) {
    CRef cr = _s1->clauses[i];
    Clause& c = _s1->ca[cr];
    for (int j = 0; j < c.size(); ++j) {
      if (_level[var(c[j])] == 0 && phase[var(c[j])] != 2) {
        if (phase[var(c[j])] == -1)
          phase[var(c[j])] = sign(c[j]) ? 1 : 0;
        else if (((bool)phase[var(c[j])]) ^ sign(c[j]))
          phase[var(c[j])] = 2;
      }
    }
  }
  for (int i = 0; i < _rootVars[0].size(); ++i) {
    if (_s1->assigns[_rootVars[0][i]] != l_Undef) continue;
    if (phase[_rootVars[0][i]] == 0 || phase[_rootVars[0][i]] == 1)
      _s2->addClause(mkLit(_rootVars[0][i], (bool)phase[_rootVars[0][i]]));
  }
}

/**Function*************************************************************

  Synopsis    [Select a minimal set of clauses.]

  Description [Enable by setting fGreedy to true.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::selectMinClauses(vec<Lit>& eLits) {
  abctime clk = 0;
  bool sat = false;
  vec<Lit> block, assump;
  block.capacity(_claLits.size());
  assump.capacity(_claLits.size());
  while (true) {
    block.clear();
    assump.clear();
    for (int i = 0; i < _claLits.size(); ++i) {
      if (_claLits[i] == lit_Undef) continue;
      (_s2->modelValue(_claLits[i]) == l_True) ? block.push(~_claLits[i])
                                               : assump.push(~_claLits[i]);
    }
    _s2->addClause(block);
    if (_fTimer) clk = Abc_Clock();
    sat = _s2->solve(assump);
    if (_fTimer) timer.timeGd += Abc_Clock() - clk;
    if (!sat) break;  // minimal set of clauses obtained
    if (_fTimer) ++timer.nGdsolve;
    getExistAssignment(eLits);  // update exist assignment
  }
}

/**Function*************************************************************

  Synopsis    [Clause subsumption]

  Description [Enable by setting fSub to true]

  SideEffects [Cachet: variables ids must be consecutive!]

  SeeAlso     []

***********************************************************************/

void SsatSolver::collectBkClaERSub(vec<Lit>& sBkCla, bool fSub) {
  sBkCla.clear();
  _selClaId.clear();
  bool select;
  for (int i = 0; i < _s1->nClauses(); ++i) {
    select = true;
    Clause& c = _s1->ca[_s1->clauses[i]];
    for (int j = 0; j < c.size(); ++j) {
      if (isEVar(var(c[j])) && _level[var(c[j])] == 0 &&
          _s1->modelValue(c[j]) == l_True) {
        select = false;
        break;
      }
    }
    if (select) _selClaId.push(i);
  }
  for (int i = 0; i < _selClaId.size(); ++i) {
    if (fSub && checkSubsume(_selClaId, i)) continue;
    Clause& c = _s1->ca[_s1->clauses[_selClaId[i]]];
    for (int j = 0; j < c.size(); ++j)
      if (isEVar(var(c[j])) && _level[var(c[j])] == 0) sBkCla.push(c[j]);
  }
  removeDupLit(sBkCla);
}

bool SsatSolver::checkSubsume(const vec<int>& ClasInd, int i) const {
  for (set<int>::iterator it = _subsumeTable[ClasInd[i]].begin();
       it != _subsumeTable[ClasInd[i]].end(); ++it)
    for (int j = 0; j < ClasInd.size(); ++j)
      if (ClasInd[j] == *it) return true;
  return false;
}

void SsatSolver::buildSubsumeTable(Solver& S) {
  int numOfClas = S.nClauses();
  assert(numOfClas > 1);
  _subsumeTable.growTo(numOfClas);
  for (int i = 0; i < S.nClauses(); ++i) {
    Clause& c = S.ca[S.clauses[i]];
    for (int j = 0; j < S.nClauses(); ++j) {
      if (j == i) continue;
      if (subsume(c, S.ca[S.clauses[j]])) _subsumeTable[i].insert(j);
    }
  }
}

bool SsatSolver::subsume(const Clause& c1, const Clause& c2) const {
  // return true iff c2 subsumes c1
  bool noRVar = true;
  for (int i = 0; i < c2.size(); ++i) {
    if (isEVar(var(c2[i]))) continue;
    noRVar = false;
    bool find = false;
    for (int j = 0; j < c1.size(); ++j) {
      if (isEVar(var(c1[j]))) continue;
      if (c1[j] == c2[i]) {
        find = true;
        break;
      }
    }
    if (!find) return false;
  }
  return !noRVar;
}

/**Function*************************************************************

  Synopsis    [Partial assignment pruning.]

  Description [Enable by setting fPart to true.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::discardLit(Ssat_Params_t* pParams, vec<Lit>& sBkCla) {
  if (sBkCla.size() == 0) return;
  abctime clk = 0;
  double subvalue = 0.0;
  vec<Lit> eLits;
  convertClaCube(sBkCla, eLits);
  vec<bool> dropVec(eLits.size(), false);
  int dropIndex = eLits.size();
  // dropping
  if (pParams->fDynamic && timer.avgDone)
    dropIndex -= timer.avgDrop;
  else
    dropIndex -= 1;
  setDropVec(dropVec, dropIndex);
  if (_fTimer) clk = Abc_Clock();
  subvalue = erSolveWMC(pParams, eLits, dropVec, dropIndex);
  if (_fTimer) {
    timer.timeCt += Abc_Clock() - clk;
    ++timer.nCount;
  }
  if (subvalue <= _satPb) {  // success, keep dropping 1 by 1
    while (dropIndex > 0) {
      --dropIndex;
      setDropVec(dropVec, dropIndex);
      if (_fTimer) clk = Abc_Clock();
      subvalue = erSolveWMC(pParams, eLits, dropVec, dropIndex);
      if (_fTimer) {
        timer.timeCt += Abc_Clock() - clk;
        ++timer.nCount;
      }
      if (subvalue > _satPb) {
        ++dropIndex;
        break;
      }
    }
  } else {  // fail, undo dropping 1 by 1
    if (!pParams->fDynamic || !timer.avgDone)
      ++dropIndex;
    else {
      while (true) {
        ++dropIndex;
        setDropVec(dropVec, dropIndex);
        if (_fTimer) clk = Abc_Clock();
        subvalue = erSolveWMC(pParams, eLits, dropVec, dropIndex);
        if (_fTimer) {
          timer.timeCt += Abc_Clock() - clk;
          ++timer.nCount;
        }
        if (subvalue <= _satPb || dropIndex == eLits.size()) break;
      }
    }
  }
  sBkCla.clear();
  for (int i = 0; i < dropIndex; ++i) sBkCla.push(~eLits[i]);
}

void setDropVec(vec<bool>& dropVec, const int& dropIndex) {
  assert(dropIndex >= 0 && dropIndex <= dropVec.size());
  for (int i = 0; i < dropIndex; ++i) dropVec[i] = false;
  for (int i = dropIndex; i < dropVec.size(); ++i) dropVec[i] = true;
}

void convertClaCube(const vec<Lit>& cla, vec<Lit>& cube) {
  cla.copyTo(cube);
  for (int i = 0; i < cube.size(); ++i) cube[i] = ~cube[i];
}

void SsatSolver::discardAllLit(Ssat_Params_t* pParams, vec<Lit>& sBkCla) {
  if (sBkCla.size() == 0) return;
  abctime clk = 0;
  double subvalue = 0.0;
  vec<Lit> eLits;
  convertClaCube(sBkCla, eLits);
  vec<bool> dropVec(eLits.size(), false);
  for (int i = 0; i < eLits.size(); ++i) {
    dropVec[i] = true;
    if (_fTimer) clk = Abc_Clock();
    subvalue = erSolveWMC(pParams, eLits, dropVec, eLits.size());
    if (_fTimer) {
      timer.timeCt += Abc_Clock() - clk;
      ++timer.nCount;
    }
    if (subvalue > _satPb) dropVec[i] = false;
  }
  sBkCla.clear();
  for (int i = 0; i < dropVec.size(); ++i)
    if (!dropVec[i]) sBkCla.push(~eLits[i]);
}

/**Function*************************************************************

  Synopsis    [Solve ER or ERE type of SSAT.]

  Description [misc helpers.]

  SideEffects []

  SeeAlso     []

***********************************************************************/

Solver* SsatSolver::buildERSelector() { return buildAllSelector(); }

void SsatSolver::getExistAssignment(vec<Lit>& eLits) const {
  assert(_s2->okay());
  for (int i = 0; i < _rootVars[0].size(); ++i)
    eLits[i] = (_s2->modelValue(_rootVars[0][i]) == l_True)
                   ? mkLit(_rootVars[0][i])
                   : ~mkLit(_rootVars[0][i]);
}

void SsatSolver::removeDupLit(vec<Lit>& c) const {
  Lit p;
  int i, j;

  sort(c);
  for (i = j = 0, p = lit_Undef; i < c.size(); i++)
    if (_s1->value(c[i]) != l_False && c[i] != p) c[j++] = p = c[i];
  c.shrink(i - j);
}

/**Function*************************************************************

  Synopsis    [Counting models under an exist assignment]

  Description []

  SideEffects [Cachet: variables ids must be consecutive!]

  SeeAlso     []

***********************************************************************/

double SsatSolver::erSolveWMC(Ssat_Params_t* pParams, const vec<Lit>& eLits,
                              const vec<bool>& dropVec, int dropIndex) {
  double satisfyProb = pParams->fBdd ? bddCountWeight(pParams, eLits, dropVec)
                                     : countModels(eLits, dropIndex);
  return _unitClauseMultiplier * satisfyProb;
}

double SsatSolver::countModels(const vec<Lit>& sBkCla, int dropIndex) {
  // return _satPb;
  FILE* file;
  int length = 1024;
  char prob_str[length], cmdModelCount[length], cmdCleanTempFiles[length];

  // vec<Lit> assump( sBkCla.size() );
  // for ( int i = 0 ; i < sBkCla.size() ; ++i ) assump[i] = ~sBkCla[i];

  // toDimacsWeighted( "temp.wcnf" , assump );
  // toDimacsWeighted( "temp.wcnf" , sBkCla );
  // cout << "dropIndex: " << dropIndex << " , clause: ";
  // dumpCla(sBkCla);
  toDimacsWeighted("temp.wcnf", sBkCla, dropIndex);
  sprintf(cmdModelCount, "bin/cachet temp.wcnf > tmp.log");
  if (system(cmdModelCount)) {
    fprintf(stderr, "Error! Problems with cachet execution...\n");
    goto clean;
  }

  sprintf(cmdModelCount,
          "cat tmp.log | grep \"Satisfying\" | awk '{print $3}' > satProb.log");
  system(cmdModelCount);

  file = fopen("satProb.log", "r");
  if (file == NULL) {
    fprintf(stderr,
            "Error! Problems with reading probability from \"satProb.log\"\n");
    goto clean;
  }
  fgets(prob_str, length, file);
  fclose(file);
  sprintf(cmdCleanTempFiles, "rm -f temp.wcnf tmp.log satProb.log");
  system(cmdCleanTempFiles);
  return atof(prob_str);
clean:
  sprintf(cmdCleanTempFiles, "rm -f temp.wcnf tmp.log satProb.log");
  system(cmdCleanTempFiles);
  exit(1);
}

/**Function*************************************************************

  Synopsis    [SsatSolve write to cnf file with weighted variables]

  Description [For Weighted Model Counting use.]

  SideEffects [Using S to switch which solver to write.]

  SeeAlso     []

***********************************************************************/

static Var mapVar(Var x, vec<Var>& map, Var& max) {
  if (map.size() <= x || map[x] == -1) {
    map.growTo(x + 1, -1);
    map[x] = max++;
  }
  return map[x];
}

static double mapWeight(Var x, vec<double>& weights, double weight) {
  if (weights.size() <= x || weights[x] != weight) {
    weights.growTo(x + 1, -1);
    weights[x] = weight;
  }
  return weights[x];
}

void SsatSolver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max,
                          int dropIndex) {
  // Solver * S = _s1;
  // if ( S->satisfied(c) ) return;

  for (int i = 0; i < c.size(); ++i)
    fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max) + 1);
  fprintf(f, "0\n");
}

void SsatSolver::toDimacsWeighted(FILE* f, vec<double>& weights, Var& max) {
  for (int i = 0; i < weights.size(); ++i)
    fprintf(f, "w %d %f\n", i + 1, weights[i]);
}

void SsatSolver::toDimacsWeighted(const char* file, const vec<Lit>& assumps,
                                  int dropIndex) {
  FILE* f = fopen(file, "wr");
  if (f == NULL) fprintf(stderr, "could not open file %s\n", file), exit(1);
  toDimacsWeighted(f, assumps, dropIndex);
  fclose(f);
}

void SsatSolver::toDimacsWeighted(FILE* f, const vec<Lit>& assumps,
                                  int dropIndex) {
  Solver* S = _s1;

  vec<bool> drop(_s1->nVars(), false);
  for (int i = dropIndex; i < assumps.size(); ++i) drop[var(assumps[i])] = true;

  // Handle case when solver is in contradictory state:
  if (!S->ok) {
    fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
    return;
  }

  Var max = 0, tmpVar;
  int cnt = 0;
  vec<Var> map;
  vec<double> weights;
  bool select = true;

  // map var to 0 ~ max
  // map 0 ~ max to original weight
  for (int i = 0; i < S->clauses.size(); ++i) {
    Clause& c = S->ca[S->clauses[i]];
    for (int j = 0; j < c.size(); ++j) {
      // cout << "dropIndex : " << dropIndex << endl;
      // cout << "var : " << var(c[j]) << endl;
      if (drop[var(c[j])] || isEVar(var(c[j])) && _level[var(c[j])] == 0 &&
                                 _s1->modelValue(c[j]) == l_True) {
        select = false;
      }
    }
    if (select == true) {
      cnt++;
      Clause& c = S->ca[S->clauses[i]];
      for (int j = 0; j < c.size(); ++j) {
        tmpVar = mapVar(var(c[j]), map, max);
        mapWeight(tmpVar, weights, (isRVar(var(c[j])) ? _quan[var(c[j])] : -1));
      }
    }
    select = true;
  }

  // Assumptions are added as unit clauses:
  cnt += dropIndex;
  for (int i = 0; i < dropIndex; ++i) {
    tmpVar = mapVar(var(assumps[i]), map, max);
    mapWeight(tmpVar, weights,
              (isRVar(var(assumps[i])) ? _quan[var(assumps[i])] : -1));
  }

  fprintf(f, "p cnf %d %d\n", max, cnt);

  // clause_average = (clause_average * clause_number + cnt) /
  // (clause_number+1);
  // ++clause_number;

  for (int i = 0; i < dropIndex; ++i) {
    fprintf(f, "%s%d 0\n", sign(assumps[i]) ? "-" : "",
            mapVar(var(assumps[i]), map, max) + 1);
  }

  // for ( int i = 0 ; i < S->clauses.size() ; ++i )
  // toDimacs( f , S->ca[S->clauses[i]] , map , max , dropIndex );
  select = true;
  for (int i = 0; i < S->clauses.size(); ++i) {
    Clause& c = S->ca[S->clauses[i]];
    for (int j = 0; j < c.size(); ++j)
      if (drop[var(c[j])] || isEVar(var(c[j])) && _level[var(c[j])] == 0 &&
                                 _s1->modelValue(c[j]) == l_True) {
        select = false;
      }
    if (select == true) {
      toDimacs(f, S->ca[S->clauses[i]], map, max, dropIndex);
    }
    select = true;
  }

  toDimacsWeighted(f, weights, max);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
