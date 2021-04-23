/**HFile****************************************************************

  FileName    [ssatSolver.h]

  SystemName  [ssatQesto]

  PackageName []

  Synopsis    [External declarations]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [16, Dec., 2016]

***********************************************************************/

#ifndef SSAT_SOLVER_H
#define SSAT_SOLVER_H

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include <fstream>
#include <iostream>
#include <map>
#include <set>

// minisat headers
#include "ssat/core/Solver.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/mtl/Alg.h"
#include "ssat/mtl/Heap.h"
#include "ssat/mtl/Vec.h"
#include "ssat/utils/Options.h"

// ABC headers
extern "C" {
#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "prob/prob.h"
#include "proof/fraig/fraig.h"
}

// macros for quantifiers
#define EXIST -1.0
#define FORALL -2.0

namespace Minisat {

typedef vec<std::set<int>> SubTbl;

// runtime profilier
typedef struct Ssat_Timer_t_ {
  // timing info
  abctime timeS1;    // s1 solving
  abctime timeS2;    // s2 solving
  abctime timeGd;    // s2 greedy
  abctime timeCt;    // Counting
  abctime timeCk;    // Cnf to sop ckt
  abctime timeSt;    // Sop ckt strash to aig
  abctime timeBd;    // Bdd construction
  abctime timeBest;  // Time until the current best solution is found
  // timeCt ~ timeCk + timeSt + timeBd
  // iteration info
  int nS1_sat;    // # of s1 solving
  int nS1_unsat;  // # of s1 solving
  int nS2solve;   // # of s2 solving
  int nGdsolve;   // # of s2 successful greedy
  int nCount;     // # of counting
  // learnt clause info
  double lenBase;     // total length of base learnt clause
  double lenSubsume;  // total length of subsume learnt clause
  double lenPartial;  // total length of partial learnt clause
  double lenDrop;     // total length of dropped literals
  // for dynamic dropping
  bool avgDone;  // indicates average done
  int avgDrop;   // average dropped literals
} Ssat_Timer_t;

// parameters for SsatSolver
typedef struct Ssat_ParamsStruct_t_ {
  // reSSAT
  double range;  // acceptable gap between upper and lower bounds
  int upper;     // #UNSAT cubes to compute upper bounds (-1: compute only once)
  int lower;     // #SAT   cubes to compute lower bounds (-1: compute only once)
  // erSSAT
  bool fGreedy;   // using minimal clause selection
  bool fSub;      // using clause subsumption
  bool fPart;     // using partial assignment pruning
  bool fDynamic;  // using dynamic dropping
  bool fPart2;    // using partial pruning ver.2
  bool fBdd;      // using BDD or Cachet to compute weight
  bool fIncre;    // using incremental counting
  bool fIncre2;   // using incremental counting ver.2
  bool fCkt;      // using circuit for counting
  bool fPure;     // using pure literal assertion
  // misc
  bool fAll;      // using All-SAT enumeration to solve
  bool fMini;     // using minimal UNSAT core
  bool fTimer;    // profiling runtime information
  bool fVerbose;  // print verbose information
} Ssat_Params_t;

//=================================================================================================
// SsatSolver -- the main class:

class SsatSolver {
 public:
  // Constructor/Destructor:
  SsatSolver(bool fTimer = false, bool fVerbose = false) {
    _fVerbose = fVerbose;
    _fTimer = fTimer;
    _s1 = NULL;
    _s2 = NULL;
    _pNtkCube = NULL;
    _vMapVars = NULL;
    _fExactlySolved = false;
    _exactSatProb = 0.0;
    _unsatPb = 0.0;
    _satPb = 0.0;
    _pNtkCnf = NULL;
    _dd = NULL;
    _unitClauseMultiplier = 1.0;
  }
  ~SsatSolver();
  // Problem specification:
  void readSSAT(gzFile&);
  // Ssat Solving:
  void solveSsat(Ssat_Params_t*);  // Solve RE/ER-2SSAT
  void bddSolveSsat(bool, bool);   // Solve SSAT by bdd
  void solveBranchBound(
      Abc_Ntk_t*);  // ER-2SSAT solving by branch and bound method
  // Results reporting
  void reportSolvingResults() const;
  // get statistics
  double exactSatProb() const { return _exactSatProb; }
  double upperBound() const { return 1.0 - _unsatPb; }
  double lowerBound() const { return _satPb; }
  int nSatCube() const { return _nSatCube; }
  int nUnsatCube() const { return _nUnsatCube; }
  // Testing interface:
  void test() const;
  // Handling interruption
  void interrupt();

 private:
  // member functions
  // parser helpers
  Solver* parse_SDIMACS(gzFile&);
  void readPrefix(StreamBuffer&, Solver&, double, int, int&, int&);
  // solve interface
  void qSolve(Ssat_Params_t*);        // Qesto-like solve
  void aSolve(Ssat_Params_t*);        // All-SAT enumeration solve
  void erSolve2SSAT(Ssat_Params_t*);  // Solve ER/ERE-2SSAT
  // erSolve helpers
  void checkERParams(Ssat_Params_t*);
  void assertUnitClause();
  void assertPureLit();
  void selectMinClauses(vec<Lit>&);
  void collectBkClaERSub(vec<Lit>&, bool);
  void discardLit(Ssat_Params_t*, vec<Lit>&);
  void discardAllLit(Ssat_Params_t*, vec<Lit>&);
  double erSolveWMC(Ssat_Params_t*, const vec<Lit>&, const vec<bool>&, int);
  // branch and bound helpers
  void ntkBuildPrefix(Abc_Ntk_t*);
  Solver* ntkBuildSolver(Abc_Ntk_t*, bool);
  bool binaryIncrement(vec<Lit>&) const;
  // solve helpers
  Solver* buildQestoSelector();
  Solver* buildERSelector();
  Solver* buildAllSelector();
  void addSelectCla(Solver&, const Lit&, const vec<Lit>&);
  void qSolve2QBF();
  void qSolve2SSAT(Ssat_Params_t*);
  void aSolve2QBF();
  void aSolve2SSAT(Ssat_Params_t*);
  void miniUnsatCore(const vec<Lit>&, vec<Lit>&);
  void collectBkCla(vec<Lit>&);
  void miniHitSet(vec<Lit>&, int) const;
  void miniHitOneHotLit(vec<Lit>&, vec<bool>&) const;
  void miniHitCollectLit(vec<Lit>&, vec<Lit>&, vec<bool>&) const;
  void miniHitDropLit(vec<Lit>&, vec<Lit>&, vec<bool>&) const;
  double cachetCount(bool);
  double baseProb() const;
  double countModels(const vec<Lit>&, int);
  void buildSubsumeTable(Solver&);
  bool checkSubsume(const vec<int>&, int) const;
  bool subsume(const Clause&, const Clause&) const;
  void getExistAssignment(vec<Lit>&) const;
  void removeDupLit(vec<Lit>&) const;
  // write file for Model Counting
  void toDimacsWeighted(FILE*, const vec<Lit>&, int);
  void toDimacsWeighted(const char*, const vec<Lit>&, int);
  void toDimacsWeighted(const char*, vec<vec<Lit>>&);
  void toDimacsWeighted(FILE*, vec<double>&, Var&);
  void toDimacs(FILE*, Clause&, vec<Var>&, Var&, int);
  // construct circuits from cubes for Model Counting
  void initCubeNetwork(bool);
  bool unsatCubeListFull() const {
    return (_upperLimit != -1) && (_unsatClause.size() > 0) &&
           (_unsatClause.size() % _upperLimit == 0);
  };
  bool satCubeListFull() const {
    return (_lowerLimit != -1) && (_satClause.size() > 0) &&
           (_satClause.size() % _lowerLimit == 0);
  };
  double cubeToNetwork(bool);
  void ntkCreatePi(Abc_Ntk_t*, Vec_Ptr_t*);
  void ntkCreatePo(Abc_Ntk_t*);
  void ntkCreateSelDef(Abc_Ntk_t*, Vec_Ptr_t*);
  Abc_Obj_t* ntkCreateNode(Abc_Ntk_t*, Vec_Ptr_t*, bool);
  void ntkPatchPoCheck(Abc_Ntk_t*, Abc_Obj_t*, bool);
  void ntkWriteWcnf(Abc_Ntk_t*);
  double ntkBddComputeSp(Abc_Ntk_t*, bool);
  // construct circuits from clauses for ER-2SSAT
  void initERBddCount(Ssat_Params_t*);
  void initClauseNetwork(bool, bool);
  void erNtkCreatePi(Abc_Ntk_t*, Vec_Ptr_t*);
  void erNtkCreatePo(Abc_Ntk_t*);
  double bddCountWeight(Ssat_Params_t*, const vec<Lit>&, const vec<bool>&);
  double clauseToNetwork(const vec<Lit>&, const vec<bool>&, bool, bool);
  Abc_Obj_t* erNtkCreateNode(Abc_Ntk_t*, Vec_Ptr_t*, const vec<Lit>&,
                             const vec<bool>&);
  DdNode* erNtkCreateBdd(DdManager*, Vec_Ptr_t*, const vec<Lit>&,
                         const vec<bool>&, int, int);
  void erNtkPatchPoCheck(Abc_Ntk_t*, Abc_Obj_t*);
  DdManager* erInitCudd(int, int, int);
  double erNtkBddComputeSp(Abc_Ntk_t*, bool);
  // All-Sat enumeration-based model counting
  double allSatModelCount(Solver*, const vec<Lit>&, double);
  // Bdd solving subroutines
  void initCnfNetwork();
  void cnfNtkCreatePi(Abc_Ntk_t*, vec<int>&);
  Abc_Obj_t* cnfNtkCreateNode(Abc_Ntk_t*, const vec<int>&);
  void cnfNtkCreatePo(Abc_Ntk_t*, Abc_Obj_t*);
  void buildBddFromNtk(bool, bool);
  void computeSsatBdd();
  // inline methods
  bool isProblemVar(const Var&) const;
  bool isRVar(const Var&) const;
  bool isEVar(const Var&) const;
  bool isAVar(const Var&) const;
  void initSelLitMark();
  void unmarkSelLit();
  bool isSelLitMarked(const Lit&) const;
  void markSelLit(const Lit&);
  // dump methods
  void dumpCla(Solver&) const;
  void dumpCla(const vec<Lit>&) const;
  void dumpCla(const Clause&) const;
  // data members
  bool _fVerbose;           // toggles verbose information
  bool _fTimer;             // toggles runtime information
  vec<vec<Var>> _rootVars;  // var used in root clauses, levelized
  vec<double> _quan;  // quantification structure, var to prob, "-1" denotes
                      // exist, "-2" denotes forall
  vec<int> _level;    // var to level
  vec<Lit> _claLits;  // lit used to select clauses: cla->lit
  int _numLv;
  Solver* _s1;           // sat solver holding the original matrix
  Solver* _s2;           // sat solver holding the selection formula
  int _selLitGlobalId;   // global mark for selection lits
  vec<int> _markSelLit;  // mark selection lits to avoid repeat
  vec<vec<Lit>>
      _unsatClause;  // added clauses during solving, used in model counting
  vec<vec<Lit>>
      _satClause;        // added clauses during solving, used in model counting
  SubTbl _subsumeTable;  // clause subsumption table
  vec<int> _selClaId;    // ids of selected clauses
  Abc_Ntk_t* _pNtkCube;  // network of SAT/UNSAT cubes
  Vec_Ptr_t* _vMapVars;  // mapping Var to Abc_Obj_t
  Abc_Obj_t* _pConst0;   // network Const0 node
  Abc_Obj_t* _pConst1;   // network Const1 node
  bool _fExactlySolved;  // flag indicating exact solution exists
  double _exactSatProb;  // satisfying probability when input is exactly
                         // solved
  double _unsatPb;       // current UNSAT pb, uppper bound
  double _satPb;         // current SAT pb, lower bound
  int _nSatCube;         // number of collected SAT cubes
  int _nUnsatCube;       // number of collected UNSAT cubes
  int _upperLimit;       // number of UNSAT cubes to invoke network construction
  int _lowerLimit;       // number of SAT cubes to invoke network construction
  vec<Lit> _erModel;     // optimizer for ER-2SSAT
  vec<Var> _control;     // control variables for model counting
  // data members for bdd solving
  Abc_Ntk_t* _pNtkCnf;  // ckt from cnf
  DdManager* _dd;       // bdd from ckt
  vec<int> _varToPi;    // Var --> Pi id
  // data members for incremental ver.2
  vec<DdNode*> _claNodes;        // clause id --> DdNode*
  vec<vec<Lit>> _unitClause;     // unit clause literals
  vec<vec<Lit>> _dupClause;      // copy of s1 clauses
  double _unitClauseMultiplier;  // The multiplier induced by unit clauses with
                                 // random variables
};

// Implementation of inline methods:

inline bool SsatSolver::isProblemVar(const Var& x) const {
  return (x > var_Undef && x < _quan.size());
}
inline bool SsatSolver::isRVar(const Var& x) const {
  return (isProblemVar(x) && _quan[x] >= 0.0 && _quan[x] <= 1.0);
}
inline bool SsatSolver::isEVar(const Var& x) const {
  return (isProblemVar(x) && _quan[x] == EXIST);
}
inline bool SsatSolver::isAVar(const Var& x) const {
  return (isProblemVar(x) && _quan[x] == FORALL);
}
inline void SsatSolver::initSelLitMark() {
  _selLitGlobalId = 0;
  _markSelLit.growTo(_s2->nVars(), _selLitGlobalId);
}
inline void SsatSolver::unmarkSelLit() { ++_selLitGlobalId; }
inline bool SsatSolver::isSelLitMarked(const Lit& x) const {
  return (_markSelLit[var(x)] == _selLitGlobalId);
}
inline void SsatSolver::markSelLit(const Lit& x) {
  _markSelLit[var(x)] = _selLitGlobalId;
}

}  // namespace Minisat

#endif
