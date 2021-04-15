/**CFile****************************************************************

  FileName    [prob.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [probabilistic design operation.]

  Synopsis    [External declarations.]

  Author      [Nian-Ze Lee]

  Affiliation [NTU]

  Date        [May 14, 2016.]

***********************************************************************/

#ifndef ABC__prob__prob_h
#define ABC__prob__prob_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "bdd/extrab/extraBdd.h"
#include "misc/extra/extra.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== probIo.c ==========================================================*/
extern void Pb_GenProbNtk(Abc_Ntk_t*, float, float);
extern void Pb_WritePBN(Abc_Ntk_t*, char*, int, int);
/*=== probDistill.c ==========================================================*/
extern Abc_Ntk_t* Pb_DistillNtk(Abc_Ntk_t*);
extern void Pb_PrintDistNtk(Abc_Ntk_t*);
/*=== probWmc.c ==========================================================*/
extern void Pb_WriteWMC(Abc_Ntk_t*, char*, int, int);
/*=== probPmc.c ==========================================================*/
extern void Pb_WritePMC(Abc_Ntk_t*, char*, int);
/*=== probSsat.c ==========================================================*/
extern void Pb_WriteSSAT(Abc_Ntk_t*, char*, int, int, int);
/*=== probBddSp.c ==========================================================*/
extern float Pb_BddComputeSp(Abc_Ntk_t*, int, int, int, int);
extern float Pb_BddComputeRESp(Abc_Ntk_t*, int, int, int, int);
extern void Pb_BddComputeAllSp(Abc_Ntk_t*, int, int, int);
extern float Ssat_BddComputeRESp(Abc_Ntk_t*, DdManager*, int, int, int);
/*=== probMiter.c ==========================================================*/
extern Abc_Ntk_t* Pb_ProbMiter(Abc_Ntk_t*, Abc_Ntk_t*, int);

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
