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

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <map>
#include <fstream>
#include <iostream>
#include <zlib.h>

// minisat headers
#include "ssat/mtl/Vec.h"
#include "ssat/mtl/Heap.h"
#include "ssat/mtl/Alg.h"
#include "ssat/utils/Options.h"
#include "ssat/core/SolverTypes.h"
#include "ssat/core/Solver.h"

// ABC headers
extern "C" {
   #include "base/abc/abc.h"
   #include "base/main/mainInt.h"
   #include "proof/fraig/fraig.h"
   #include "prob/prob.h"
}

// macros for quantifiers
#define EXIST  -1.0
#define FORALL -2.0

namespace Minisat {

typedef struct SsatTimer_ {
   abctime timeS1;
   abctime timeS2;
   int nCachet;
   int nSubsume;
} SsatTimer;

//=================================================================================================
// SsatSolver -- the main class:

class SsatSolver {

public:
   // Constructor/Destructor:
   SsatSolver( bool fVerbose = false , bool fTimer = false ) : _s1(NULL) , _s2(NULL) , _pNtkCube(NULL) , _vMapVars(NULL) , _unsatPb(0.0) , _satPb(0.0) { _fVerbose = fVerbose; _fTimer = fTimer;}
   ~SsatSolver();
   // Problem specification:
   void        readSSAT( gzFile& );
   // Ssat Solving:
   void        solveSsat( double , int , int , bool , bool , bool ); // Solve 2SSAT/2QBF
   // ER-2-Ssat solving by branch and bound method
   void        solveBranchBound( Abc_Ntk_t* );
   double      upperBound() const { return 1.0 - _unsatPb; }
   double      lowerBound() const { return _satPb; }
   int         nSatCube()   const { return _nSatCube; }
   int         nUnsatCube() const { return _nUnsatCube; }
   // Testing interface:
   void        test() const;
   // Handling interruption
   void        interrupt();
private:
   // member functions
   // read helpers
   Solver *    parse_SDIMACS      ( gzFile& );
   void        readPrefix         ( StreamBuffer& , Solver& , double , int , int& , int& );
   // solve interface
   void        qSolve             ( double , int , int , bool ); // Qesto-like solve
   void        aSolve             ( double , int , int , bool , bool ); // All-SAT enumeration solve
   void        erSolve2SSAT       ( bool ); // Solve ER/ERE-2SSAT
   // branch and bound helpers
   void        ntkBuildPrefix     ( Abc_Ntk_t * );
   Solver *    ntkBuildSolver     ( Abc_Ntk_t * , bool );
   bool        binaryIncrement    ( vec<Lit> & ) const;
   // solve helpers
   Solver *    buildQestoSelector ();
   Solver *    buildERSelector    ();
   Solver *    buildAllSelector   ();
   void        addSelectCla       ( Solver& , const Lit& , const vec<Lit>& );
   void        qSolve2QBF         ();
   void        qSolve2SSAT        ( double , int , int , bool );
   void        aSolve2QBF         ();
   void        aSolve2SSAT        ( double , int , int , bool , bool );
   void        miniUnsatCore      ( const vec<Lit> & , vec<Lit>& );
   void        collectBkCla       ( vec<Lit>& );
   void        collectBkClaER     ( vec<Lit>& , int );
   void        collectBkClaER     ( vec<Lit>& );
   void        miniHitSet         ( vec<Lit>& , int ) const;
   void        miniHitOneHotLit   ( vec<Lit>& , vec<bool>& ) const;
   void        miniHitCollectLit  ( vec<Lit>& , vec<Lit>& , vec<bool>& ) const;
   void        miniHitDropLit     ( vec<Lit>& , vec<Lit>& , vec<bool>& ) const;
   double      baseProb           () const;
   double      countModels        ( const vec<Lit>& , int );
   double      countModels        ( const vec<Lit>& );
   bool        checkSubsumption   ( Solver& ) const;
   bool        subsume            ( const Clause& , const Clause& ) const;
   int         getLearntClaLen    ( Solver& , const vec<int>& , const vec<bool>& ) const;
   // write file for Model Counting
   void        toDimacsWeighted   ( FILE* , const vec<Lit>& );
   void        toDimacsWeighted   ( const char* , const vec<Lit>& );
   void        toDimacsWeighted   ( FILE* , vec<double>& , Var& );
   void        toDimacs           ( FILE* , Clause& , vec<Var>& , Var& );
   double      cachetCount        ( bool );
   void        toDimacsWeighted   ( const char* , vec< vec<Lit> >& );
   // construct circuits from cubes for Model Counting
   void        initCubeNetwork    ( bool );
   bool        unsatCubeListFull  () const { return (_upperLimit!=-1) && (_unsatClause.size()>0) && (_unsatClause.size()% _upperLimit==0); };
   bool        satCubeListFull    () const { return (_lowerLimit!=-1) && (_satClause.size()>0) && (_satClause.size()% _lowerLimit==0); };
   double      cubeToNetwork      ( bool );
   void        ntkCreatePi        ( Abc_Ntk_t * , Vec_Ptr_t * );
   void        ntkCreatePo        ( Abc_Ntk_t * );
   void        ntkCreateSelDef    ( Abc_Ntk_t * , Vec_Ptr_t * );
   Abc_Obj_t * ntkCreateNode      ( Abc_Ntk_t * , Vec_Ptr_t * , bool );
   void        ntkPatchPoCheck    ( Abc_Ntk_t * , Abc_Obj_t * , bool );
   void        ntkWriteWcnf       ( Abc_Ntk_t * );
   double      ntkBddComputeSp    ( Abc_Ntk_t * , bool );
   // construct circuits from clauses for ER-2SSAT
   void        initClauseNetwork  ();
   void        erNtkCreatePi      ( Abc_Ntk_t * , Vec_Ptr_t * );
   void        erNtkCreatePo      ( Abc_Ntk_t * );
   double      clauseToNetwork    ();
   Abc_Obj_t * erNtkCreateNode    ( Abc_Ntk_t * , Vec_Ptr_t * );
   void        erNtkPatchPoCheck  ( Abc_Ntk_t * , Abc_Obj_t * );
   double      erNtkBddComputeSp  ( Abc_Ntk_t * );
   // All-Sat enumeration-based model counting
   double      allSatModelCount   ( Solver * , const vec<Lit>& , double );
   // inline methods
   bool        isProblemVar       ( const Var& ) const;
   bool        isRVar             ( const Var& ) const;
   bool        isEVar             ( const Var& ) const;
   bool        isAVar             ( const Var& ) const;
   void        initSelLitMark     ();
   void        unmarkSelLit       ();
   bool        isSelLitMarked     ( const Lit& ) const;
   void        markSelLit         ( const Lit& );
   // dump methods
   void        dumpCla            ( Solver& ) const;
   void        dumpCla            ( const vec<Lit>& ) const;
   void        dumpCla            ( const Clause&   ) const;
   // data members
   bool              _fVerbose;        // toggles verbose information
   bool              _fTimer;          // toggles runtime information
   vec< vec<Var> >   _rootVars;        // var used in root clauses, levelized
   vec<double>       _quan;            // quantification structure, var to prob, "-1" denotes exist, "-2" denotes forall
   vec<int>          _level;           // var to level
   vec<Lit>          _claLits;         // lit used to select clauses: cla->lit
   int               _numLv;
   Solver          * _s1;              // sat solver holding the original matrix
   Solver          * _s2;              // sat solver holding the selection formula
   double            _satProb;         // 2SSAT sat prob
   int               _selLitGlobalId;  // global mark for selection lits
   vec<int>          _markSelLit;      // mark selection lits to avoid repeat
   vec< vec<Lit> >   _unsatClause;     // added clauses during solving, used in model counting
   vec< vec<Lit> >   _satClause;       // added clauses during solving, used in model counting
   Abc_Ntk_t       * _pNtkCube;        // network of SAT/UNSAT cubes
   Vec_Ptr_t       * _vMapVars;        // mapping Var to Abc_Obj_t
   Abc_Obj_t       * _pConst0;         // network Const0 node
   Abc_Obj_t       * _pConst1;         // network Const1 node
   double            _unsatPb;         // current UNSAT pb, uppper bound
   double            _satPb;           // current SAT pb, lower bound
   int               _nSatCube;        // number of collected SAT cubes
   int               _nUnsatCube;      // number of collected UNSAT cubes
   int               _upperLimit;      // number of UNSAT cubes to invoke network construction
   int               _lowerLimit;      // number of SAT cubes to invoke network construction
   vec<Lit>          _erModel;         // optimizer for ER-2SSAT
   vec<Var>          _control;         // control variables for model counting
};

// Implementation of inline methods:

inline bool SsatSolver::isProblemVar    ( const Var & x ) const { return ( x > var_Undef && x < _quan.size() ); }
inline bool SsatSolver::isRVar          ( const Var & x ) const { return ( isProblemVar(x) && _quan[x] >= 0.0 && _quan[x] <= 1.0 ); }
inline bool SsatSolver::isEVar          ( const Var & x ) const { return ( isProblemVar(x) && _quan[x] == EXIST  ); }
inline bool SsatSolver::isAVar          ( const Var & x ) const { return ( isProblemVar(x) && _quan[x] == FORALL ); }
inline void SsatSolver::initSelLitMark  () { _selLitGlobalId = 0; _markSelLit.growTo( _s2->nVars() , _selLitGlobalId ); }
inline void SsatSolver::unmarkSelLit    () { ++_selLitGlobalId; }
inline bool SsatSolver::isSelLitMarked  ( const Lit & x ) const { return ( _markSelLit[var(x)] == _selLitGlobalId ); }
inline void SsatSolver::markSelLit      ( const Lit & x ) { _markSelLit[var(x)] = _selLitGlobalId; }

}

#endif
