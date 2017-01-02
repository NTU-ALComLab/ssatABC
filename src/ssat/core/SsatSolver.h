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
}

// macros for quantifiers
#define EXIST  -1.0
#define FORALL -2.0

namespace Minisat {

//=================================================================================================
// SsatSolver -- the main class:

class SsatSolver {

public:
   // Constructor/Destructor:
   SsatSolver() : _s1(NULL) , _s2(NULL) {}
   ~SsatSolver();
   // Problem specification:
   void      readSSAT( gzFile& );
   // Ssat Solving:
   double    ssolve();
   // Testing interface:
   void      test() const;
private:
   // member functions
   // read helpers
   Solver * parse_SDIMACS      ( gzFile& );
   void     readPrefix         ( StreamBuffer& , Solver& , double , int , int& , int& );
   // solve helpers
   Solver * buildSelectSolver();
   void     addSelectCla       ( Solver& , const Lit& , const vec<Lit>& );
   bool     ssolve2QBF         ();
   double   ssolve2SSAT        ();
   void     collectBkCla       ( vec<Lit>& );
   double   baseProb           () const;
   double   countModels        ( const vec<Lit>& );
   // write file for Model Counting
   void     toDimacsWeighted   ( FILE* , const vec<Lit>& );
   void     toDimacsWeighted   ( const char* , const vec<Lit>& );
   void     toDimacsWeighted   ( FILE* , vec<double>& , Var& );
   void     toDimacs           ( FILE* , Clause& , vec<Var>& , Var& );
   // construct circuits from cubes for Model Counting
   void        cubeToNetwork      () const;
   void        ntkCreatePi        ( Abc_Ntk_t * , Vec_Ptr_t * ) const;
   Abc_Obj_t * ntkCreateSelDef    ( Abc_Ntk_t * , Vec_Ptr_t * ) const;
   Abc_Obj_t * ntkCreateNode      ( Abc_Ntk_t * , Vec_Ptr_t * ) const;
   void        ntkWriteWcnf       () const;
   // inline methods
   bool     isProblemVar       ( const Var& ) const;
   bool     isRVar             ( const Var& ) const;
   bool     isEVar             ( const Var& ) const;
   bool     isAVar             ( const Var& ) const;
   void     initSelLitMark     ();
   void     unmarkSelLit       ();
   bool     isSelLitMarked     ( const Lit& ) const;
   void     markSelLit         ( const Lit& );
   // dump methods
   void     dumpCla            ( Solver& ) const;
   void     dumpCla            ( const vec<Lit>& ) const;
   // data members
   vec< vec<Var> > _rootVars;        // var used in root clauses, levelized
   vec<double>     _quan;            // quantification structure, var to prob, "-1" denotes exist, "-2" denotes forall
   vec<int>        _level;           // var to level
   vec<Lit>        _claLits;         // lit used to select clauses: cla->lit
   int             _numLv;
   Solver        * _s1;              // sat solver holding the original matrix
   Solver        * _s2;              // sat solver holding the selection formula
   double          _satProb;         // 2SSAT sat prob
   int             _selLitGlobalId;  // global mark for selection lits
   vec<int>        _markSelLit;      // mark selection lits to avoid repeat
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
