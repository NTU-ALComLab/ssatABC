/**CFile****************************************************************

  FileName    [ssatAllSolve.cc]

  SystemName  [ssatQesto]

  Synopsis    [All-SAT enumeration solve]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [10, Jan., 2017]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include <iostream>

#include "ssat/core/Dimacs.h"
#include "ssat/core/Solver.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/core/SsatSolver.h"
#include "ssat/utils/ParseUtils.h"

using namespace Minisat;
using namespace std;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Ssat_Timer_t timer;
extern void printREParams(Ssat_Params_t*);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Solving process entrance]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::aSolve(Ssat_Params_t* pParams) {
  _s2 = buildAllSelector();
  if (isAVar(_rootVars[0][0]))
    aSolve2QBF();
  else
    aSolve2SSAT(pParams);
}

/**Function*************************************************************

  Synopsis    [Build _s2 (All-Sat Lv.1 vars selector)]

  Description [forall vars have exactly the same IDs as _s1]

  SideEffects [Initialize as tautology (no clause)]

  SeeAlso     []

***********************************************************************/

Solver* SsatSolver::buildAllSelector() {
  Solver* S = new Solver;
  for (int i = 0; i < _rootVars[0].size(); ++i)
    while (_rootVars[0][i] >= S->nVars()) S->newVar();
  return S;
}

/**Function*************************************************************

  Synopsis    [All-Sat 2QBF solving internal function]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::aSolve2QBF() {
  // TODO
  printf("  > Under construction...\n");
}

/**Function*************************************************************

  Synopsis    [All-Sat 2SSAT solving internal function]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::aSolve2SSAT(Ssat_Params_t* pParams) {
  if (_fVerbose) {
    printf("[INFO] Invoking reSSAT solver with the following configuration:\n");
    printREParams(pParams);
  }
  vec<Lit> rLits(_rootVars[0].size()), sBkCla;
  abctime clk = 0;
  if (pParams->upper > 0 && pParams->lower > 0) {
    _fVerbose = true;
  }
  _upperLimit = pParams->upper;
  _lowerLimit = pParams->lower;
  (_upperLimit > 0) ? _unsatClause.capacity(_upperLimit)
                    : _unsatClause.capacity(1000000);
  (_lowerLimit > 0) ? _satClause.capacity(_lowerLimit)
                    : _satClause.capacity(1000000);
  if (pParams->fBdd) {
    initCubeNetwork(true);
  }
  sBkCla.capacity(_rootVars[0].size());
  printf("[INFO] Starting analysis ...\n");

  // Learn the unit clause of randomized variable first
  for (int i = 0; i < _unitClause.size(); i++) {
    Lit p = _unitClause[i][0];
    if (isRVar(var(p))) {
      _unsatClause.push();
      _unitClause[i].copyTo(_unsatClause.last());
      _s2->addClause(_unitClause[i]);
    }
  }

  // copy the original clauses
  vec<Lit> cl;
  for (int i = 0; i < _s1->nClauses(); ++i) {
    Clause& c = _s1->ca[_s1->clauses[i]];
    cl.clear();
    for (int j = 0; j < c.size(); j++) {
      cl.push(c[j]);
    }
    _dupClause.push();
    cl.copyTo(_dupClause.last());
  }

  while (true) {
    if (1.0 - _unsatPb - _satPb <= pParams->range) {
      printf("[INFO] Stopping analysis ...\n");
      printf("[INFO] # of UNSAT cubes: %d\n", _unsatClause.size());
      printf("[INFO] # of   SAT cubes: %d\n", _satClause.size());
      if (pParams->range == 0.0) {
        _fExactlySolved = true;
        _exactSatProb = upperBound();
      }
      return;
    }
    if (_fTimer) clk = Abc_Clock();
    if (!_s2->solve()) {
      if (_fTimer) {
        timer.timeS2 += Abc_Clock() - clk;
        ++timer.nS2solve;
        clk = Abc_Clock();
      }
      printf("[INFO] Stopping analysis ...\n");
      printf("[INFO] # of UNSAT cubes: %d\n", _unsatClause.size());
      printf("[INFO] # of   SAT cubes: %d\n", _satClause.size());
      // if ( _unsatClause.size() < _satClause.size() )
      _fExactlySolved = true;
      _exactSatProb =
          pParams->fBdd ? 1.0 - cubeToNetwork(false) : 1.0 - cachetCount(false);
      // else
      //_exactSatProb = pParams->fBdd ? cubeToNetwork(true)  :
      // cachetCount(true);
      if (_fTimer) {
        timer.timeCt += Abc_Clock() - clk;
        ++timer.nCount;
      }
      return;
    }
    if (_fTimer) {
      timer.timeS2 += Abc_Clock() - clk;
      ++timer.nS2solve;
    }
    for (int i = 0; i < _rootVars[0].size(); ++i) {
      rLits[i] = (_s2->modelValue(_rootVars[0][i]) == l_True)
                     ? mkLit(_rootVars[0][i])
                     : ~mkLit(_rootVars[0][i]);
    }
    if (_fTimer) clk = Abc_Clock();
    if (!_s1->solve(rLits)) {  // UNSAT case
      if (_fTimer) {
        timer.timeS1 += Abc_Clock() - clk;
        ++timer.nS1_unsat;
      }
      _unsatClause.push();
      if (pParams->fMini) {
        sBkCla.clear();
        if (_fTimer) clk = Abc_Clock();
        miniUnsatCore(_s1->conflict, sBkCla);
        if (_fTimer) timer.timeGd += Abc_Clock() - clk;
        sBkCla.copyTo(_unsatClause.last());
        if (!sBkCla.size()) {  // FIXME: temp sol for UNSAT matrix
          _unsatPb = 1.0;
          _satPb = 0.0;
          return;
        }
        _s2->addClause(sBkCla);
      } else {
        _s1->conflict.copyTo(_unsatClause.last());
        _s2->addClause(_s1->conflict);
      }
      if (unsatCubeListFull()) {
        if (_fTimer) clk = Abc_Clock();
        _unsatPb = pParams->fBdd ? cubeToNetwork(false) : cachetCount(false);
        if (_fTimer) {
          timer.timeCt += Abc_Clock() - clk;
          ++timer.nCount;
        }
        if (_fVerbose) {
          printf("  > current Upper bound = %e\n", 1 - _unsatPb);
          fflush(stdout);
        }
      }
    } else {  // SAT case
      if (_fTimer) {
        timer.timeS1 += Abc_Clock() - clk;
        ++timer.nS1_sat;
      }
      _satClause.push();
      if (pParams->fMini) {
        sBkCla.clear();
        if (_fTimer) clk = Abc_Clock();
        miniHitSet(sBkCla, 0);  // random var at Lv.0
        if (_fTimer) timer.timeCk += Abc_Clock() - clk;
        sBkCla.copyTo(_satClause.last());
        _s2->addClause(sBkCla);
      } else {
        vec<Lit> rCla(_rootVars[0].size());
        for (int i = 0; i < rLits.size(); ++i) rCla[i] = ~rLits[i];
        rLits.copyTo(_satClause.last());
        _s2->addClause(rCla);
      }
      if (satCubeListFull()) {
        if (_fTimer) clk = Abc_Clock();
        _satPb = pParams->fBdd ? cubeToNetwork(true) : cachetCount(true);
        if (_fTimer) {
          timer.timeCt += Abc_Clock() - clk;
          ++timer.nCount;
        }
        if (_fVerbose) {
          printf("\t\t\t\t\t\t  > current Lower bound = %e\n", _satPb);
          fflush(stdout);
        }
      }
    }
  }
}

double SsatSolver::cachetCount(bool sat) {
  FILE* file;
  int length = 256;
  char prob_str[length], cmdModelCount[length], cmdCleanTempFiles[length];

  vec<vec<Lit> >& learntClause = sat ? _satClause : _unsatClause;
  if (!learntClause.size()) return (sat ? _satPb : _unsatPb);

  toDimacsWeighted("temp.wcnf", learntClause);
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
  return 1 - atof(prob_str);  // Since it is the weight of cube list.
clean:
  sprintf(cmdCleanTempFiles, "rm -f temp.wcnf tmp.log satProb.log");
  system(cmdCleanTempFiles);
  exit(1);
}

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

void SsatSolver::toDimacsWeighted(const char* filename, vec<vec<Lit> >& lClas) {
  FILE* f = fopen(filename, "wr");
  if (f == NULL) fprintf(stderr, "could not open file %s\n", filename), exit(1);

  // Start to weighted dimacs
  Var max = 0, tmpVar;
  int clauses = lClas.size();
  vec<Var> map;
  vec<double> weights;

  for (int i = 0; i < lClas.size(); ++i) {
    for (int j = 0; j < lClas[i].size(); ++j) {
      tmpVar = mapVar(var(lClas[i][j]), map, max);
      mapWeight(tmpVar, weights, _quan[var(lClas[i][j])]);
    }
  }

  fprintf(f, "p cnf %d %d\n", max, clauses);

  for (int i = 0; i < lClas.size(); ++i) {
    for (int j = 0; j < lClas[i].size(); ++j) {
      fprintf(f, "%s%d ", sign(lClas[i][j]) ? "-" : "",
              mapVar(var(lClas[i][j]), map, max) + 1);
    }
    fprintf(f, "0\n");
  }

  for (int i = 0; i < weights.size(); ++i)
    fprintf(f, "w %d %f\n", i + 1, weights[i]);

  fclose(f);
}

/**Function*************************************************************

  Synopsis    [Minimum hitting set to generalize SAT solutions.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::miniHitSet(vec<Lit>& sBkCla, int randLv) const {
  vec<Lit> minterm;
  minterm.capacity(_rootVars[0].size());
  vec<bool> pick(_s1->nVars(), false);
  miniHitOneHotLit(sBkCla, pick);
  miniHitCollectLit(sBkCla, minterm, pick);
  if (minterm.size()) miniHitDropLit(sBkCla, minterm, pick);
  // sanity check: avoid duplicated lits --> invalid write!
  if (sBkCla.size() > _rootVars[randLv].size()) {
    Abc_Print(-1, "Wrong hitting set!!!\n");
    dumpCla(sBkCla);
    exit(1);
  }
}

void SsatSolver::miniHitOneHotLit(vec<Lit>& sBkCla, vec<bool>& pick) const {
  Lit lit;
  for (int i = 0; i < _dupClause.size(); ++i) {
    // Clause & c  = _s1->ca[_s1->clauses[i]];
    short find = 0;
    bool claSat = false;
    for (int j = 0; j < _dupClause[i].size(); ++j) {
      if (_s1->modelValue(_dupClause[i][j]) == l_True) {
        if (pick[var(_dupClause[i][j])]) {
          claSat = true;
          break;
        }
        ++find;
        if (find == 1)
          lit = _dupClause[i][j];
        else
          break;
      }
    }
    if (claSat) continue;
    if (find == 1) {
      pick[var(lit)] = true;
      if (isRVar(var(lit))) sBkCla.push(~lit);
    }
  }
}

void SsatSolver::miniHitCollectLit(vec<Lit>& sBkCla, vec<Lit>& minterm,
                                   vec<bool>& pick) const {
  vec<Lit> rLits, eLits;
  rLits.capacity(8), eLits.capacity(8);
  for (int i = 0; i < _dupClause.size(); ++i) {
    // Clause & c  = _s1->ca[_s1->clauses[i]];
    bool claSat = false;
    rLits.clear(), eLits.clear();
    for (int j = 0; j < _dupClause[i].size(); ++j) {
      if (_s1->modelValue(_dupClause[i][j]) == l_True) {
        if (pick[var(_dupClause[i][j])]) {
          claSat = true;
          break;
        }
        isRVar(var(_dupClause[i][j])) ? rLits.push(_dupClause[i][j])
                                      : eLits.push(_dupClause[i][j]);
      }
    }
    if (!claSat) {
      if (eLits.size()) {
        pick[var(eLits[0])] = true;
        continue;
      }
      assert(rLits.size() > 1);
      for (int j = 0; j < rLits.size(); ++j) {
        assert(!pick[var(rLits[j])]);
        pick[var(rLits[j])] = true;
        minterm.push(rLits[j]);
      }
    }
  }
}

void SsatSolver::miniHitDropLit(vec<Lit>& sBkCla, vec<Lit>& minterm,
                                vec<bool>& pick) const {
  for (int i = 0; i < minterm.size(); ++i) {
    pick[var(minterm[i])] = false;
    bool cnfSat = true;
    for (int j = 0; j < _dupClause.size(); ++j) {
      // Clause & c  = _s1->ca[_s1->clauses[j]];
      bool claSat = false;
      for (int k = 0; k < _dupClause[j].size(); ++k) {
        if (pick[var(_dupClause[j][k])] &&
            _s1->modelValue(_dupClause[j][k]) == l_True) {
          claSat = true;
          break;
        }
      }
      if (!claSat) {
        cnfSat = false;
        break;
      }
    }
    if (!cnfSat) {
      pick[var(minterm[i])] = true;
      sBkCla.push(~minterm[i]);
    }
  }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
