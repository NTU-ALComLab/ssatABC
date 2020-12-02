/**CFile****************************************************************

  FileName    [ssatBddSolve.cc]

  SystemName  [ssatABC]

  Synopsis    [Solve general SSAT by BDD]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [16, Mar., 2017]

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

extern "C" {
Abc_Ntk_t* Abc_NtkDC2(Abc_Ntk_t*, int, int, int, int, int);
Abc_Ntk_t* Abc_NtkFromGlobalBdds(Abc_Ntk_t*);
void Abc_NodeShowBdd(Abc_Obj_t*);
void Abc_NtkShow(Abc_Ntk_t*, int, int, int);
DdManager* Ssat_NtkPoBuildGlobalBdd(Abc_Ntk_t*, int, int, int);
int Pb_BddShuffleGroup(DdManager*, int, int);
void Pb_BddResetProb(DdManager*, DdNode*);
void BddComputeSsat_rec(Abc_Ntk_t*, DdNode*);
float Ssat_BddComputeProb_rec(Abc_Ntk_t*, DdNode*, int, int);
void Nz_DebugBdd(DdNode*);
}
extern Abc_Obj_t* Ssat_SopAnd2Obj(Abc_Obj_t*, Abc_Obj_t*);
extern void Ssat_DumpCubeNtk(Abc_Ntk_t*);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::bddSolveSsat(bool fGroup, bool fReorder) {
  if (_fVerbose)
    printf("  > grouping = %s , reordering = %s\n", fGroup ? "yes" : "no",
           fReorder ? "yes" : "no");
  initCnfNetwork();
  buildBddFromNtk(fGroup, fReorder);
  computeSsatBdd();
}

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::initCnfNetwork() {
  if (_fVerbose) printf("  > Constructing a circuit from root clauses:\n");
  Abc_Ntk_t* pNtkSop;
  Abc_Obj_t* pObjCla;
  char name[32];
  pNtkSop = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_SOP, 1);
  sprintf(name, "root_clause_ntk");
  pNtkSop->pName = Extra_UtilStrsav(name);
  _varToPi.growTo(_s1->nVars(), -1);
  cnfNtkCreatePi(pNtkSop, _varToPi);
  pObjCla = cnfNtkCreateNode(pNtkSop, _varToPi);
  if (!pObjCla) {
    Abc_Print(
        -1, "Known bug! After unit propagation, there is no clause left...\n");
    exit(1);
  }
  cnfNtkCreatePo(pNtkSop, pObjCla);
  if (!Abc_NtkCheck(pNtkSop)) {
    Abc_Print(-1, "Something wrong with cnf to network ...\n");
    Abc_NtkDelete(pNtkSop);
    exit(1);
  }
  _pNtkCnf = Abc_NtkStrash(pNtkSop, 0, 1, 0);
  Abc_NtkDelete(pNtkSop);
}

void SsatSolver::cnfNtkCreatePi(Abc_Ntk_t* pNtkCnf, vec<int>& varToPi) {
  Abc_Obj_t* pPi;
  char name[1024];
  for (int i = 0; i < _rootVars.size(); ++i) {
    for (int j = 0; j < _rootVars[i].size(); ++j) {
      pPi = Abc_NtkCreatePi(pNtkCnf);
      sprintf(name, "var_%d_%d", _rootVars[i][j], Abc_NtkPiNum(pNtkCnf) - 1);
      Abc_ObjAssignName(pPi, name, "");
      varToPi[_rootVars[i][j]] = Abc_NtkPiNum(pNtkCnf) - 1;
    }
  }
}

Abc_Obj_t* SsatSolver::cnfNtkCreateNode(Abc_Ntk_t* pNtkCnf,
                                        const vec<int>& varToPi) {
  Abc_Obj_t *pObj, *pObjCla = NULL;
  int* pfCompl = new int[_s1->nVars()];
  char name[1024];
  bool sat;

  for (int i = 0; i < _s1->nClauses(); ++i) {
    Clause& c = _s1->ca[_s1->clauses[i]];
    sat = false;
    for (int j = 0; j < c.size(); ++j)
      if (_s1->value(c[j]) == l_True) {
        sat = true;
        break;
      }
    if (sat) continue;
    pObj = Abc_NtkCreateNode(pNtkCnf);
    sprintf(name, "c%d", i);
    Abc_ObjAssignName(pObj, name, "");
    for (int j = 0; j < c.size(); ++j) {
      if (_s1->value(c[j]) != l_False) {
        Abc_ObjAddFanin(pObj, Abc_NtkPi(pNtkCnf, varToPi[var(c[j])]));
        pfCompl[Abc_ObjFaninNum(pObj) - 1] = sign(c[j]) ? 1 : 0;
      }
    }
    Abc_ObjSetData(pObj, Abc_SopCreateOr((Mem_Flex_t*)pNtkCnf->pManFunc,
                                         Abc_ObjFaninNum(pObj), pfCompl));
    pObjCla = Ssat_SopAnd2Obj(pObjCla, pObj);
  }
  delete[] pfCompl;
  return pObjCla;
}

void SsatSolver::cnfNtkCreatePo(Abc_Ntk_t* pNtkCnf, Abc_Obj_t* pObjCla) {
  Abc_ObjAssignName(Abc_NtkCreatePo(pNtkCnf), "clause_output", "");
  Abc_ObjAddFanin(Abc_NtkPo(pNtkCnf, 0), Abc_ObjRegular(pObjCla));
}

/**Function*************************************************************

  Synopsis    [Bdd solves SSAT entrance point.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::buildBddFromNtk(bool fGroup, bool fReorder) {
  if (_fVerbose) printf("  > Building a global bdd for the circuit:\n");
  if (fGroup) {
    if (_rootVars.size() != 2) {
      Abc_Print(-1, "Grouping is supported when Lv=2! (current Lv=%d)",
                _rootVars.size());
      exit(1);
    }
    if (!fReorder)
      Abc_Print(0,
                "Automatically enable reordering when grouping is specified!");
    _dd = Ssat_NtkPoBuildGlobalBdd(_pNtkCnf, 0, _rootVars[0].size(), 1);
  } else
    _dd = fReorder
              ? Ssat_NtkPoBuildGlobalBdd(_pNtkCnf, 0, Abc_NtkPiNum(_pNtkCnf), 0)
              : Ssat_NtkPoBuildGlobalBdd(_pNtkCnf, 0, 0, 0);
  if (!_dd) {
    Abc_Print(-1, "Bdd construction has failed.\n");
    exit(1);
  }
  // check random/exist variables are correctly ordered
  if (fGroup && Cudd_ReadInvPerm(_dd, 0) > _rootVars[0].size() - 1) {
    if (!Pb_BddShuffleGroup(_dd, _rootVars[0].size(), _rootVars[1].size())) {
      Abc_Print(-1, "Bdd Shuffle has failed.\n");
      exit(-1);
    }
  }
}

/**Function*************************************************************

  Synopsis    [Compute Ssat value by traversing bdd.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/

void SsatSolver::computeSsatBdd() {
  if (_fVerbose) printf("  > Compute SSAT value on Bdd:\n");
  DdNode* bFunc;

  for (int i = 0; i < _rootVars.size(); ++i)
    for (int j = 0; j < _rootVars[i].size(); ++j)
      Abc_NtkPi(_pNtkCnf, _varToPi[_rootVars[i][j]])->dTemp =
          (float)_quan[_rootVars[i][j]];
  bFunc = (DdNode*)Abc_ObjGlobalBdd(Abc_NtkPo(_pNtkCnf, 0));
  Pb_BddResetProb(_dd, bFunc);
  BddComputeSsat_rec(_pNtkCnf, bFunc);
  _satPb = Cudd_IsComplement(bFunc) ? 1.0 - Cudd_Regular(bFunc)->pMin
                                    : Cudd_Regular(bFunc)->pMax;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
