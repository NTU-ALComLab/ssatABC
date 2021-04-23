/**CFile****************************************************************

  FileName    [SsatSolver.cc]

  SystemName  [ssatQesto]

  Synopsis    [Implementations of member functions for ssatSolver]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [16, Dec., 2016]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "ssat/core/SsatSolver.h"

#include <stdio.h>

#include <iostream>

#include "ssat/core/Dimacs.h"
#include "ssat/core/Solver.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/utils/ParseUtils.h"

using namespace Minisat;
using namespace std;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Ssat_Timer_t timer;
extern void printTimer(Ssat_Timer_t*);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Constructor/Destructor]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

SsatSolver::~SsatSolver() {
  if (_s1) {
    delete _s1;
    _s1 = NULL;
  }
  if (_s2) {
    delete _s2;
    _s2 = NULL;
  }
  if (_pNtkCube) {
    Abc_NtkDelete(_pNtkCube);
    _pNtkCube = NULL;
  }
  if (_pNtkCnf) {
    if (_dd) {
      Abc_NtkFreeGlobalBdds(_pNtkCnf, 1);
      _dd = NULL;
    }
    Abc_NtkDelete(_pNtkCnf);
    _pNtkCnf = NULL;
  }
  if (_dd) {
    Cudd_Quit(_dd);
    _dd = NULL;
  }
  if (_vMapVars) {
    Vec_PtrFree(_vMapVars);
    _vMapVars = NULL;
  }
}

/**Function*************************************************************

  Synopsis    [Build two solvers for qesto-like SSAT solving]

  Description [Interface function]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::readSSAT(gzFile& input_stream) {
  _s1 = parse_SDIMACS(input_stream);
}

/**Function*************************************************************

  Synopsis    [SSAT formula parser]

  Description [Build quantification and _s1 (root clauses)]

  SideEffects []

  SeeAlso     []

***********************************************************************/

Solver* SsatSolver::parse_SDIMACS(gzFile& input_stream) {
  StreamBuffer in(input_stream);
  vec<Lit> lits;
  Solver* ptrS = new Solver;
  Solver& S = *ptrS;
  double parsed_prob;
  int vars = 0;
  int clauses = 0;
  int cnt = 0;
  int level = 0;
  int quan = 0;  // 1: random, 2:exist, 3: forall, 0:none

  _rootVars.push();
  for (;;) {
    skipWhitespace(in);
    if (*in == EOF)
      break;
    else if (*in == 'p') {  // p cnf #var #clause
      if (eagerMatch(in, "p cnf")) {
        vars = parseInt(in);
        clauses = parseInt(in);
      } else
        printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    } else if (*in ==
               'r') {  // r p x_i  <-- randomly quantified variable x_i w.p. p
      ++in;
      parsed_prob = parseFloat(in);
      if (parsed_prob > 1.0) {
        fprintf(stderr,
                "PARSE ERROR! Probability for a random quantifier not in range "
                "of [0, 1], got %lf\n",
                parsed_prob),
            exit(3);
      }
      readPrefix(in, S, parsed_prob, 1, quan, level);
    } else if (*in ==
               'e') {  // e y_j    <-- existentially quantified variable y_j
      ++in;
      readPrefix(in, S, EXIST, 2, quan, level);
    } else if (*in ==
               'a') {  // a z_k    <-- universally quantified variable z_k
      ++in;
      readPrefix(in, S, FORALL, 3, quan, level);
    } else {
      if (*in == 'c' || *in == 'p')
        skipLine(in);
      else {
        cnt++;
        readClause(in, S, lits);
        S.addClause_(lits);

        // save unit clauses
        if (lits.size() == 1) {
          _unitClause.push();
          lits.copyTo(_unitClause.last());
        }
      }
    }
  }
  _numLv = level + 1;
  if (vars != S.nVars())
    fprintf(stderr,
            "WARNING! DIMACS header mismatch: wrong number of variables.\n");
  if (cnt != clauses)
    fprintf(stderr,
            "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
  return ptrS;
}

void SsatSolver::readPrefix(StreamBuffer& in, Solver& S, double prob, int cur,
                            int& quan, int& level) {
  vec<int> parsed;
  int parsed_int, var, max = -1;

  if (quan != 0 && cur != quan) {
    _rootVars.push();
    ++level;
  }
  quan = cur;
  for (;;) {
    parsed_int = parseInt(in);
    if (parsed_int == 0) break;
    parsed.push(parsed_int);
    if (parsed_int > max) max = parsed_int;
    var = parsed_int - 1;
    while (var >= S.nVars()) S.newVar();
  }
  _quan.growTo(max, -1);
  _level.growTo(max, -1);
  for (int i = 0; i < parsed.size(); ++i) {
    (_rootVars.last()).push(parsed[i] - 1);
    _quan[parsed[i] - 1] = prob;
    _level[parsed[i] - 1] = level;
  }
}

/**Function*************************************************************

  Synopsis    [Solving process entrance]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::solveSsat(Ssat_Params_t* pParams) {
  if (_numLv > 3 || _numLv == 1)
    fprintf(stderr, "[ERROR] Currently only support \"{RE,ER}-2SSAT\"!\n");
  else if (isEVar(_rootVars[0][0]) && isRVar(_rootVars[1][0]))
    erSolve2SSAT(pParams);
  else if (pParams->fAll)
    aSolve(pParams);
  else
    qSolve(pParams);
}

void SsatSolver::qSolve(Ssat_Params_t* pParams) {
  _s1->simplify();  // avoid clause deletion after solving
  _s2 = buildQestoSelector();
  initSelLitMark();  // avoid repeat selection vars in blocking clause
  if (isAVar(_rootVars[0][0]))
    qSolve2QBF();
  else
    qSolve2SSAT(pParams);
}

/**Function*************************************************************

  Synopsis    [Build _s2 (Qesto clause selector)]

  Description [Initialize forall (lv.0) vars and build selection clauses]

  SideEffects [forall (lv.0) vars have exactly the same IDs as _s1]

  SeeAlso     []

***********************************************************************/

Solver* SsatSolver::buildQestoSelector() {
  Solver* S = new Solver;
  vec<Lit> sLits;

  for (int i = 0; i < _rootVars[0].size(); ++i)
    while (_rootVars[0][i] >= S->nVars()) S->newVar();
  _claLits.growTo(_s1->nClauses(), lit_Undef);
  for (int i = 0; i < _s1->nClauses(); ++i) {
    Clause& c = _s1->ca[_s1->clauses[i]];
    sLits.clear();
    for (int j = 0; j < c.size(); ++j)
      if (_level[var(c[j])] == 0) sLits.push(c[j]);
    if (sLits.size() > 1) {  // allocate new selection var
      _claLits[i] = mkLit(S->newVar());
      addSelectCla(*S, _claLits[i], sLits);
    } else if (sLits.size() == 1)  // reuse the lv.0 var as selection var
      _claLits[i] = ~sLits[0];
  }
  return S;
}

void SsatSolver::addSelectCla(Solver& S, const Lit& x, const vec<Lit>& uLits) {
  vec<Lit> cla(1 + uLits.size());
  for (int i = 0; i < uLits.size(); ++i) S.addClause(~x, ~uLits[i]);
  cla[0] = x;
  for (int i = 0; i < uLits.size(); ++i) cla[i + 1] = uLits[i];
  S.addClause(cla);
}

/**Function*************************************************************

  Synopsis    [Qesto 2QBF solving internal function]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::qSolve2QBF() {
  vec<Lit> uLits(_rootVars[0].size()), sBkCla;
  for (;;) {
    if (!_s2->solve()) _satPb = 1.0;
    for (int i = 0; i < _rootVars[0].size(); ++i)
      uLits[i] = (_s2->modelValue(_rootVars[0][i]) == l_True)
                     ? mkLit(_rootVars[0][i])
                     : ~mkLit(_rootVars[0][i]);
    if (!_s1->solve(uLits)) _satPb = 0.0;
    sBkCla.clear();
    collectBkCla(sBkCla);
    _s2->addClause(sBkCla);
  }
}

/**Function*************************************************************

  Synopsis    [Qesto-like 2SSAT solving internal function]

  Description [Invoking cachet via system call]

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::qSolve2SSAT(Ssat_Params_t* pParams) {
  vec<Lit> rLits(_rootVars[0].size()), sBkCla;
  abctime clk = Abc_Clock();
  _upperLimit = pParams->upper;
  _lowerLimit = pParams->lower;
  (_upperLimit > 0) ? _unsatClause.capacity(_upperLimit)
                    : _unsatClause.capacity(1000000);
  (_lowerLimit > 0) ? _satClause.capacity(_lowerLimit)
                    : _satClause.capacity(1000000);
  initCubeNetwork(false);
  while (1.0 - _unsatPb - _satPb > pParams->range) {
    if (!_s2->solve()) {
      _unsatPb = cubeToNetwork(false);
      _satPb = cubeToNetwork(true);
      return;
    }
    for (int i = 0; i < _rootVars[0].size(); ++i)
      rLits[i] = (_s2->modelValue(_rootVars[0][i]) == l_True)
                     ? mkLit(_rootVars[0][i])
                     : ~mkLit(_rootVars[0][i]);
    if (!_s1->solve(rLits)) {  // UNSAT case
      if (pParams->fMini) {
        sBkCla.clear();
        miniUnsatCore(_s1->conflict, sBkCla);
        _unsatClause.push();
        sBkCla.copyTo(_unsatClause.last());
        _s2->addClause(sBkCla);
      } else {
        _unsatClause.push();
        _s1->conflict.copyTo(_unsatClause.last());
        _s2->addClause(_s1->conflict);
      }
      if (unsatCubeListFull()) {
        printf("  > Collect %d UNSAT cubes, convert to network\n", _upperLimit);
        _unsatPb = cubeToNetwork(false);
        printf("  > current unsat prob = %f\n", _unsatPb);
        Abc_PrintTime(1, "  > current unsat time", Abc_Clock() - clk);
        fflush(stdout);
      }
    } else {  // SAT case
      sBkCla.clear();
      collectBkCla(sBkCla);
      _satClause.push();
      sBkCla.copyTo(_satClause.last());
      _s2->addClause(sBkCla);
      if (satCubeListFull()) {
        printf("  > Collect %d SAT cubes, convert to network\n", _lowerLimit);
        _satPb = cubeToNetwork(true);
        printf("  > current sat prob = %f\n", _satPb);
        Abc_PrintTime(1, "  > current sat time", Abc_Clock() - clk);
        fflush(stdout);
      }
    }
  }
}

void SsatSolver::miniUnsatCore(const vec<Lit>& conflt, vec<Lit>& sBkCla) {
  vec<Lit> assump;
  vec<Lit> conflict;
  vec<bool> drop(conflt.size(), false);
  conflt.copyTo(conflict);
  assump.capacity(conflict.size());
  for (int i = 0; i < conflict.size(); ++i) {
    assump.clear();
    for (int j = 0; j < drop.size(); ++j)
      if (j != i && !drop[j]) assump.push(~conflict[j]);
    if (!_s1->solve(assump)) {
      // printf( "  > %d-th lit can be dropped!\n" , i );
      drop[i] = true;
    } else
      sBkCla.push(conflict[i]);
  }
  // FIXME: turn off mini, no bug?
  vec<Lit> check(sBkCla.size());
  for (int i = 0; i < sBkCla.size(); ++i) check[i] = ~sBkCla[i];
  assert(!_s1->solve(check));
}

void SsatSolver::collectBkCla(vec<Lit>& sBkCla) {
  unmarkSelLit();
  bool block;
  for (int i = 0; i < _s1->nClauses(); ++i) {
    if (_claLits[i] == lit_Undef) continue;
    Clause& c = _s1->ca[_s1->clauses[i]];
    block = true;
    for (int j = 0; j < c.size(); ++j) {
      if (isEVar(var(c[j])) && _s1->modelValue(c[j]) == l_True) {
        block = false;
        break;
      }
    }
    // if ( block && _s2->value(_claLits[i]) != l_False &&
    // !isSelLitMarked(_claLits[i]) ) {
    if (block && !isSelLitMarked(_claLits[i])) {
      sBkCla.push(_claLits[i]);
      markSelLit(_claLits[i]);
    }
  }
}

double SsatSolver::baseProb() const {
  double subspace_base = 1.0;
  for (int i = 0; i < _rootVars[0].size(); ++i) {
    if (_s2->value(_rootVars[0][i]) == l_True)
      subspace_base *= _quan[_rootVars[0][i]];
    else if (_s2->value(_rootVars[0][i]) == l_False)
      subspace_base *= 1 - _quan[_rootVars[0][i]];
  }
  return subspace_base;
}

/**Function*************************************************************

  Synopsis    [Dump content for debugging]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::dumpCla(Solver& S) const {
  for (int i = 0; i < S.nClauses(); ++i) {
    CRef cr = S.clauses[i];
    Clause& c = S.ca[cr];
    for (int j = 0; j < c.size(); ++j) {
      if (S.value(c[j]) != l_False)
        cout << (sign(c[j]) ? "-" : "") << var(c[j]) + 1 << " ";
    }
    cout << "0\n";
  }
  cout << endl;
}

void SsatSolver::dumpCla(const vec<Lit>& c) const {
  for (int i = 0; i < c.size(); ++i)
    cout << (sign(c[i]) ? "-" : "") << var(c[i]) + 1 << " ";
  cout << "0\n";
}

void SsatSolver::dumpCla(const Clause& c) const {
  for (int i = 0; i < c.size(); ++i)
    cout << (sign(c[i]) ? "-" : "") << var(c[i]) + 1 << " ";
  cout << "0\n";
}

/**Function*************************************************************

  Synopsis    [SsatSolve reporting solving results]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::reportSolvingResults() const {
  Abc_Print(-2, "\n==== Solving results ====\n\n");
  if (_fExactlySolved) {
    Abc_Print(-2, "  > Satisfying probability: %e\n", _exactSatProb);
  } else {
    Abc_Print(-2, "  > Best upper bound: %e\n", upperBound());
    Abc_Print(-2, "  > Best lower bound: %e\n", lowerBound());
    Abc_PrintTime(1, "  > Time elapsed for best lower bound ", timer.timeBest);
  }
}

/**Function*************************************************************

  Synopsis    [SsatSolve testing interface]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::test() const {
  printf("\nPrefix structure , numLv = %d\n\n", _numLv);
  for (int i = 0; i < _rootVars.size(); ++i) {
    printf("Lv%d vars:\n", i);
    for (int j = 0; j < _rootVars[i].size(); ++j) {
      if (isRVar(_rootVars[i][j]))
        printf("r %f %d\n", _quan[_rootVars[i][j]], _rootVars[i][j] + 1);
      else if (isEVar(_rootVars[i][j]))
        printf("e %d\n", _rootVars[i][j] + 1);
      else if (isAVar(_rootVars[i][j]))
        printf("a %d\n", _rootVars[i][j] + 1);
      else {
        fprintf(stderr, "Error! Unknown quantifier for var %d\n",
                _rootVars[i][j] + 1);
        assert(0);
      }
    }
    printf("\n");
  }
  if (_s1) {
    printf("  > _s1 clauses:\n\n");
    dumpCla(*_s1);
  }
  if (_s2) {
    printf("  > _s2 clauses:\n\n");
    dumpCla(*_s2);
  }
}

/**Function*************************************************************

  Synopsis    [Handling interruption.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::interrupt() {
  printf("\n");
  Abc_Print(0, "Interruption occurs!\n");
  Abc_Print(0, "Reporting solving results before termination ...\n");
  if (_unsatClause.size() == 0 && _satClause.size() == 0) {
    reportSolvingResults();
  } else {
    abctime clk = 0;
    int numCubeLimits = 60000;
    if (_unsatClause.size()) {
      printf("[INFO] # of UNSAT cubes: %d\n", _unsatClause.size());
      if (_pNtkCube && _unsatClause.size() > numCubeLimits) {
        Abc_Print(0, "Cube number exceeds the limit (%d)!\n", numCubeLimits);
        Abc_Print(0,
                  "Do not compute the upper bound because BDD may fail ...\n");
      } else {
        clk = Abc_Clock();
        _unsatPb = _pNtkCube ? cubeToNetwork(false) : cachetCount(false);
        Abc_Print(-2, "  > Best upper bound: %e\n", upperBound());
        Abc_PrintTime(1, "Time elapsed for upper bound", Abc_Clock() - clk);
      }
      fflush(stdout);
    }
    if (_satClause.size()) {
      printf("[INFO] # of   SAT cubes: %d\n", _satClause.size());
      if (_pNtkCube && _satClause.size() > numCubeLimits) {
        Abc_Print(0, "Cube number exceeds the limit (%d)!\n", numCubeLimits);
        Abc_Print(0,
                  "Do not compute the lower bound because BDD may fail ...\n");
      } else {
        clk = Abc_Clock();
        _satPb = _pNtkCube ? cubeToNetwork(true) : cachetCount(true);
        Abc_Print(-2, "  > Best lower bound: %e\n", lowerBound());
        Abc_PrintTime(1, "Time elapsed for lower bound", Abc_Clock() - clk);
      }
      fflush(stdout);
    }
  }
  if (_fTimer) printTimer(&timer);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
