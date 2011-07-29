/* ============================================================================
 *  LDSP.c
 * ============================================================================

 *  Author:         (c) 2002-2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 30, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:47 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LDSP.h"
#include "LArray.h"
#include "LHeap.h"
#include "LMemory.h"
#include "LException.h"
#include "LDebug.h"

/* SPACE REQUIREMENTS: 
 * S(n) = 24*n^2 + 80*PUP + c, 
 * where PUP = max #potentially uniform paths in the sequence 
 * and c is a constant 
*/

/* SETUP */
#define _LDSP_TRACE_SMOOTHING_                FALSE
#define _LDSP_SMOOTHING_                      TRUE
#define _LDSP_SMOOTHING_THRESHOLD             0.7
#define _LDSP_SMOOTHING_THRESHOLD_CORRECTION  0.6
#define _LDSP_SMOOTHING_STEP                  100
#define _LDSP_SMOOTHING_RATIO                 10

/* TYPEDEFS */
typedef struct TPath {   /* 80 bytes for representing each potentially uniform path pi_xy */
    ui2       mX;        /* start vertex     */
    ui2       mY;        /* end vertex       */
    ui4       mW;        /* path weight      */
    ui4       mTB;       /* tie breaking tag (or next free slot if mZ==_LDSP_FREE) */
    ui4       mZ;        /* _LDSP_SP_ZOMBIE if shortest path or zombie, _LDSP_FREE if free slot, 0 otherwise */
    ui4       ml;        /* index of l(pi_xy) in mPathArena */
    ui4       mr;        /* index of r(pi_xy) in mPathArena */
    ui4       mL;        /* index of first path of L(pi_xy)  in mPathArena */
    ui4       mR;        /* index of first path of R(pi_xy)  in mPathArena */
    ui4       mLS;       /* index of first path of L*(pi_xy) in mPathArena */
    ui4       mRS;       /* index of first path of R*(pi_xy) in mPathArena */
    ui4       mNextLr;   /* index of next path in L(r(pi_xy))  (_LDSP_NIL if none) */
    ui4       mPrevLr;   /* index of prev path in L(r(pi_xy))  (_LDSP_NIL if none) */
    ui4       mNextRl;   /* index of next path in R(l(pi_xy))  (_LDSP_NIL if none) */
    ui4       mPrevRl;   /* index of prev path in R(l(pi_xy))  (_LDSP_NIL if none) */
    ui4       mNextLSr;  /* index of next path in LS(r(pi_xy)) (_LDSP_NIL if none) */
    ui4       mPrevLSr;  /* index of prev path in LS(r(pi_xy)) (_LDSP_NIL if none) */
    ui4       mNextRSl;  /* index of next path in RS(l(pi_xy)) (_LDSP_NIL if none) */
    ui4       mPrevRSl;  /* index of prev path in RS(l(pi_xy)) (_LDSP_NIL if none) */
    ui4       mNextP;    /* index of next path in P(pi_xy)     (_LDSP_NIL if none) */
    ui4       mPrevP;    /* index of prev path in P(pi_xy)     (_LDSP_NIL if none) */
} TPath;

typedef struct TMatrixEntry { /* 24 bytes for each pair of vertices (x,y) */
    ui4       mW;        /* weight of edge (x,y), or LDSP_INFTY if no such path exists  */
    ui4       mT;        /* time of latest update of edge (x,y) */
    ui4       mSP;       /* index in mPathArena of shortest path between x and y */
    ui4       mP;        /* index in mPathList of P_xy */
    ui4       mEdgePath; /* index in mPathArena of path pi_xy corresponding to edge (x,y) */
    ui4       mSHIdx;    /* index of edge (x,y) in mSH (or _LDSP_NIL if not there) */
} TMatrixEntry;

typedef struct TPair {   /* 4 bytes for each pair of vertices in mA */
    ui2 mX;
    ui2 mY;
    ui4 mPad;            /* for supporting 64-bit memory models */
} TPair;

struct LDSP {
    ui4             mTime;          /* number of performed update operations */
    ui2             mN;             /* number of vertices in G */
    LArray*         mPathArena;     /* pool of potentially uniform paths (or empty slots). Slots of type TPath */
    TPath*          mArenaBase;     /* addess of data segment maintained by mPathArena */
    ui4             mFreeSlot;      /* index in mPathArena of free slot */
    TMatrixEntry**  mMatrix;        /* adjacency matrix of G (pointers to rows) */
    TMatrixEntry*   mMatrixRows;    /* rows of the adjacency matrix of G */
    LHeap*          mSH;            /* priority queue for smoothing */

    LHeap*          mH;             /* temp priority queue for supporting update operations */
    LArray*         mQ;             /* temp list of indices of paths in mPathArena removed by _Delete */
    LArray*         mA;             /* temp list of pairs of vertices affected by an update */

    ui4             mAffected;      /* #affected pairs during the last update */
    ui4             mNewPUP;        /* #PUP created during the last update */
    ui4             mDelPUP;        /* #PUP deleted during the last update */
    ui4             mDummyUpdates;  /* total #dummy updates */
    ui4             mSkipped;       /* total #dummy updates skipped by adaptive strategy */
    f8              mSmoothingRate; /* average mNewPUP/mDelPUP per dummy update */

    LDSP_TSetup     mConfig;        /* configuration parameters */
};


/* GLOBAL VARIABLE (be careful when using multi-threading...) */
static LDSP* sThis;

/* INTERNAL DEFINES */
#define _LDSP_NIL              0xFFFFFFFF
#define _LDSP_SP_ZOMBIE        1
#define _LDSP_FREE             2

/* SHORTCUTS */
#define _Path                  (This->mArenaBase)

/* MACROS WITH ARGUMENTS */
#define _AddToList(list,path,prev_field,next_field) {    \
    _Path[path].prev_field = _LDSP_NIL;                  \
    _Path[path].next_field = list;                       \
    list = path;                                         \
    if (_Path[path].next_field != _LDSP_NIL)             \
        _Path[_Path[path].next_field].prev_field = path; \
}

#define _DelFromList(list,path,prev_field,next_field) {                     \
    if (_Path[path].next_field != _LDSP_NIL)                                \
         _Path[_Path[path].next_field].prev_field = _Path[path].prev_field; \
    if (_Path[path].prev_field != _LDSP_NIL)                                \
         _Path[_Path[path].prev_field].next_field = _Path[path].next_field; \
    else list = _Path[path].next_field;                                     \
}

/* PRIVATE FUNCTION PROTOTYPES */
static void   _UnSmoothedUpdate  (LDSP* This, ui2 inX, ui2 inY, ui4 inW);
static void   _Delete            (LDSP* This, ui2 inX, ui2 inY);
static void   _Insert            (LDSP* This, ui2 inX, ui2 inY, ui4 inW);
static ui4    _NewPUP            (LDSP* This, ui4 inL, ui4 inR);
static void   _DelPUP            (LDSP* This, ui4 inPath, ui2 inU, ui2 inV);
static ui4    _GetMinPath        (LDSP* This, ui2 inX, ui2 inY);
static Bool   _SHComparator      (ui4 inKeyA, ui4 inKeyB);
static Bool   _HComparator       (ui4 inKeyA, ui4 inKeyB);


/* PUBLIC FUNCTION DEFINITIONS */

/* ----------------------------------------------------------------------------
 *  NewEmpty
 * ----------------------------------------------------------------------------
*/
LDSP* LDSP_NewEmpty(ui2 inN){

    ui4    theIdx;
    TPath* thePath;
    LDSP   theObject = { 0 };
    LDSP*  This = NULL;

    Try {

        /* init object */
        theObject.mTime          = 0;
        theObject.mN             = inN;
        theObject.mPathArena     = LArray_New(sizeof(TPath));
        theObject.mFreeSlot      = _LDSP_NIL;
        theObject.mMatrix        = (TMatrixEntry**)LMemory_Malloc(sizeof(TMatrixEntry*)*inN);
        theObject.mMatrixRows    = (TMatrixEntry*) LMemory_Malloc(sizeof(TMatrixEntry)*inN*inN);
        theObject.mSH            = LHeap_New(_SHComparator);

        theObject.mH             = LHeap_New(_HComparator);
        theObject.mQ             = LArray_New(sizeof(ui4));
        theObject.mA             = LArray_New(sizeof(TPair));
        theObject.mSmoothingRate = 0.0;

        theObject.mConfig.mThreshold  = _LDSP_SMOOTHING_THRESHOLD;
        theObject.mConfig.mCorrection = _LDSP_SMOOTHING_THRESHOLD_CORRECTION;
        theObject.mConfig.mStep       = _LDSP_SMOOTHING_STEP;
        theObject.mConfig.mRatio      = _LDSP_SMOOTHING_RATIO;

        /* create new object */
        This = LMemory_NewObject(LDSP,theObject);

        /* install synchronized data pointers */
        LArray_InstallSyncPtr(This->mPathArena, (void**)&This->mArenaBase);

        /* make paths of the form pi_vv */
        LArray_ResizeBy(This->mPathArena, (i4)inN);
        for (theIdx=0, thePath = _Path; theIdx<inN; ++theIdx, ++thePath) {
            thePath->mX  = thePath->mY = (ui2)theIdx;
            thePath->mW  = 0;
            thePath->mTB = 0;
            thePath->mZ  = _LDSP_SP_ZOMBIE;
            thePath->ml  = thePath->mr = theIdx;
            thePath->mL       = thePath->mR       = _LDSP_NIL;
            thePath->mLS      = thePath->mRS      = _LDSP_NIL;
            thePath->mNextLr  = thePath->mPrevLr  = _LDSP_NIL;
            thePath->mNextRl  = thePath->mPrevRl  = _LDSP_NIL;
            thePath->mNextLSr = thePath->mPrevLSr = _LDSP_NIL;
            thePath->mNextRSl = thePath->mPrevRSl = _LDSP_NIL;
            thePath->mNextP   = thePath->mPrevP   = _LDSP_NIL;
        }

        /* init adjacency matrix */
        LMemory_Set(This->mMatrixRows, 0xFF, sizeof(TMatrixEntry)*inN*inN);
        for (theIdx=0; theIdx<inN; ++theIdx) {
            This->mMatrix[theIdx] = This->mMatrixRows + theIdx*inN;
            This->mMatrix[theIdx][theIdx].mSP = theIdx;
        }
    }

    CatchAny {
        if (theObject.mPathArena!=NULL)  LArray_Delete(&theObject.mPathArena); 
        if (theObject.mMatrixRows!=NULL) LMemory_Free(&theObject.mMatrixRows); 
        if (theObject.mMatrix!=NULL)     LMemory_Free(&theObject.mMatrix); 
        if (theObject.mSH!=NULL)         LHeap_Delete(&theObject.mSH);
        if (theObject.mH!=NULL)          LHeap_Delete(&theObject.mH); 
        if (theObject.mQ!=NULL)          LArray_Delete(&theObject.mQ); 
        if (theObject.mA!=NULL)          LArray_Delete(&theObject.mA);
        if (This != NULL)                LMemory_DeleteObject(&This);
        Rethrow;
    }

    return This;
}


/* ----------------------------------------------------------------------------
 *  New
 * ----------------------------------------------------------------------------
*/
LDSP* LDSP_New(LGraph* inGraph, LEdgeInfo* inEdgeWeights) {

    ui2            i, j, theN;
    LGraph_TEdge*  theEdge;
    LGraph_TEdge** theMap = NULL;
    LDSP*          This   = NULL;

    /* get number of nodes */
    theN = (ui2)LGraph_GetNodesCount(inGraph);

    Try {

        /* create empty LDSP object */
        This = LDSP_NewEmpty(theN);
        
        /* create cycle map */
        theMap = (LGraph_TEdge**)LMemory_Calloc(sizeof(LGraph_TEdge*)*theN);

        /* insert cycle between vertices with zero length to avoid 
         * creation of zombies during insertions of edges */
        for (i = 0; i < theN; ++i) {
            This->mMatrix[i][(i+1) % theN].mT = ++This->mTime;
            _UnSmoothedUpdate(This, i, (i+1) % theN, 0);
        }

        #if 0
        LDSP_Dump(This);
        #endif

        /* set weights of edges without doing smoothing */
        LGraph_ForAllEdges(inGraph, theEdge) {
            i = LGraph_GetNodeIndex(LGraph_GetSource(theEdge));
            j = LGraph_GetNodeIndex(LGraph_GetTarget(theEdge));
            if (i == j) continue;
            if ((i+1) % theN == j) { theMap[i] = theEdge; continue; };
            This->mMatrix[i][j].mT = ++This->mTime;
            _UnSmoothedUpdate(This, i, j, LEdgeInfo_UI4At(inEdgeWeights, theEdge));
        }

        #if 0
        LDSP_Dump(This);
        #endif

        /* remove edges in the cycle that are not in the graph */
        for (i = 0; i < theN; ++i) {
            ui4 theWeight;
            if (theMap[i] != NULL) 
                 theWeight = LEdgeInfo_UI4At(inEdgeWeights, theMap[i]);
            else theWeight = LDSP_INFTY;
            This->mMatrix[i][(i+1) % theN].mT = ++This->mTime;
            _UnSmoothedUpdate(This, i, (i+1) % theN, theWeight);
        }
    }

    CatchAny {
        if (This != NULL)     LDSP_Delete(&This);
        if (theMap != NULL)   LMemory_Free(&theMap);
        Rethrow;
    }

    /* cleanup */
    LMemory_Free(&theMap);

    return This;
}


/* ----------------------------------------------------------------------------
 *  Delete
 * ----------------------------------------------------------------------------
*/
void LDSP_Delete(LDSP** AThis){
    LArray_Delete(&(*AThis)->mPathArena);
    LMemory_Free(&(*AThis)->mMatrixRows); 
    LMemory_Free(&(*AThis)->mMatrix); 
    LHeap_Delete(&(*AThis)->mSH);
    LHeap_Delete(&(*AThis)->mH);
    LArray_Delete(&(*AThis)->mQ);
    LArray_Delete(&(*AThis)->mA);
    LMemory_DeleteObject(AThis);
}


/* ----------------------------------------------------------------------------
 *  GetNumVertices
 * ----------------------------------------------------------------------------
*/
ui2 LDSP_GetNumVertices(LDSP* This){
    return This->mN;
}


/* ----------------------------------------------------------------------------
 *  GetEdgeWeight
 * ----------------------------------------------------------------------------
*/
ui4 LDSP_GetEdgeWeight(LDSP* This, ui2 inU, ui2 inV){
    if (inU>=This->mN || inV>=This->mN || inU==inV) Throw(LDSP_BAD_PARAMETERS);
    return This->mMatrix[inU][inV].mW;
}


/* ----------------------------------------------------------------------------
 *  UpdateEdge
 * ----------------------------------------------------------------------------
*/
void LDSP_UpdateEdge(LDSP* This, ui2 inU, ui2 inV, ui4 inW){

    ui4 theSmoothingTime;
    ui4 theWindow = (This->mTime > This->mN) ? This->mN : This->mTime;

    if (inU >= This->mN || inV >= This->mN || inW < 0 || inU == inV) 
        Throw(LDSP_BAD_PARAMETERS);

    /* update time counter and date edge (inX,inY) */
    This->mMatrix[inU][inV].mT = ++This->mTime;

    /* perform original update \sigma_i */
    _UnSmoothedUpdate(This, inU, inV, inW);

    #if _LDSP_SMOOTHING_

    /* schedule edge (inX,inY) for future smoothing */
    if (inW != LDSP_INFTY) {

        TPair thePair;
        thePair.mX = inU;
        thePair.mY = inV;

        /* compute time at which edge has to be smoothed */
        theSmoothingTime = This->mTime + This->mConfig.mStep;

        if (This->mMatrix[inU][inV].mSHIdx == _LDSP_NIL)
             This->mMatrix[inU][inV].mSHIdx = 
                 LHeap_Add(This->mSH, *(void**)(&thePair), theSmoothingTime);
        else LHeap_Update(This->mSH, *(void**)(&thePair), 
                          theSmoothingTime, This->mMatrix[inU][inV].mSHIdx);

        #if _LDSP_TRACE_SMOOTHING_
        LDebug_Print("[LDSP] Time=%u - Scheduling edge (%hu,%hu) at time %u\n", This->mTime, inU, inV, theSmoothingTime);
        #endif
    }
    else if (This->mMatrix[inU][inV].mSHIdx != _LDSP_NIL) {
             LHeap_Remove(This->mSH, This->mMatrix[inU][inV].mSHIdx);
             This->mMatrix[inU][inV].mSHIdx = _LDSP_NIL;
         }

    /* smoothing */
    while(!LHeap_Empty(This->mSH)) {

        ui4   theKey;
        TPair thePair;

        /* get edge (x,y) with minimum key in This->mSH */
        LHeap_GetMin(This->mSH, (void**)&thePair, &theKey);

        /* quit smoothing operation if edge (x,y) is not scheduled at this time */
        if (theKey > This->mTime) break;

        /* if smoothing rate is below the threshold, then perform dummy update */
        if (This->mSmoothingRate < This->mConfig.mThreshold) {

            #if _LDSP_TRACE_SMOOTHING_
            LDebug_Print("[LDSP] Smoothing edge (%hu,%hu)\n", thePair.mX, thePair.mY);
            #endif

            /* perform dummy update on edge (x,y) */
            _UnSmoothedUpdate(This, thePair.mX, thePair.mY, This->mMatrix[thePair.mX][thePair.mY].mW);
            ++This->mDummyUpdates;

            #if _LDSP_TRACE_SMOOTHING_
            LDebug_Print("[LDSP] NewPUP/DelPUP = %1.3f\n",(f8)(This->mNewPUP+1)/(This->mDelPUP+1));
            #endif

            /* update smoothing rate in a window of <= This->mN operations */
            This->mSmoothingRate = (This->mSmoothingRate*(theWindow-1) + (f8)(This->mNewPUP+1)/(This->mDelPUP+1))/theWindow;
        }

        /* artificially decrease smoothing rate */
        else {
            This->mSmoothingRate = (This->mSmoothingRate*(theWindow-1) + This->mConfig.mCorrection)/theWindow;
            ++This->mSkipped;
            #if _LDSP_TRACE_SMOOTHING_
            LDebug_Print("[LDSP] SMOOTHING SKIPPED\n");
            #endif
        }

        /* reschedule edge for smoothing */
        theSmoothingTime = 
            This->mMatrix[thePair.mX][thePair.mY].mT + 
            (This->mTime - This->mMatrix[thePair.mX][thePair.mY].mT)*This->mConfig.mRatio;
        LHeap_Update(This->mSH, *(void**)(&thePair), 
                     theSmoothingTime, This->mMatrix[thePair.mX][thePair.mY].mSHIdx);

        #if _LDSP_TRACE_SMOOTHING_
        LDebug_Print("[LDSP] Rescheduling edge (%hu,%hu) at time %u\n", thePair.mX, thePair.mY, theSmoothingTime);
        #endif
    }
    #endif
}


/* ----------------------------------------------------------------------------
 *  GetDist
 * ----------------------------------------------------------------------------
*/
ui4 LDSP_GetDist(LDSP* This, ui2 inX, ui2 inY){
    if (inX>=This->mN || inY>=This->mN) Throw(LDSP_BAD_PARAMETERS);
    if (inX==inY) return 0;
    if (This->mMatrix[inX][inY].mSP == _LDSP_NIL) return LDSP_INFTY;
    return _Path[This->mMatrix[inX][inY].mSP].mW;
}


/* ----------------------------------------------------------------------------
 *  GetLWit
 * ----------------------------------------------------------------------------
*/
ui2 LDSP_GetLWit(LDSP* This, ui2 inX, ui2 inY){
    if (inX>=This->mN || inY>=This->mN) Throw(LDSP_BAD_PARAMETERS);
    if (This->mMatrix[inX][inY].mSP == _LDSP_NIL) return LDSP_NIL;
    return _Path[_Path[This->mMatrix[inX][inY].mSP].ml].mX;
}


/* ----------------------------------------------------------------------------
 *  GetRWit
 * ----------------------------------------------------------------------------
*/
ui2 LDSP_GetRWit(LDSP* This, ui2 inX, ui2 inY){
    if (inX>=This->mN || inY>=This->mN) Throw(LDSP_BAD_PARAMETERS);
    if (This->mMatrix[inX][inY].mSP == _LDSP_NIL) return LDSP_NIL;
    return _Path[_Path[This->mMatrix[inX][inY].mSP].mr].mY;
}


/* ----------------------------------------------------------------------------
 *  LDSP_GetConfig
 * ----------------------------------------------------------------------------
*/
LDSP_TSetup LDSP_GetConfig(LDSP* This){
    return This->mConfig;
}


/* ----------------------------------------------------------------------------
 *  LDSP_SetConfig
 * ----------------------------------------------------------------------------
*/
void LDSP_SetConfig(LDSP* This, LDSP_TSetup inConfig){
    This->mConfig = inConfig;
}


/* ----------------------------------------------------------------------------
 *  Dump
 * ----------------------------------------------------------------------------
*/
void LDSP_Dump(LDSP* This) {
    ui4 theTotalSize     = LDSP_GetUsedMem(This);
    ui4 theMatrixSize    = sizeof(TMatrixEntry*)*This->mN + sizeof(TMatrixEntry)*This->mN*This->mN;
    ui4 thePathArenaSize = LArray_GetUsedMem(This->mPathArena);
    ui4 theHSize         = LHeap_GetUsedMem(This->mH);
    ui4 theSHSize        = LHeap_GetUsedMem(This->mSH);
    ui4 theQSize         = LArray_GetUsedMem(This->mQ);
    ui4 theASize         = LArray_GetUsedMem(This->mA);
    LDSP_TStat theStat;

    /* memory usage */
    LDebug_Print("[LDSP] Used memory: %u bytes\n", theTotalSize);
    LDebug_Print("[LDSP]    Matrix:     %u bytes (%3.2f%%)\n", theMatrixSize, (f8)theMatrixSize/theTotalSize*100);
    LDebug_Print("[LDSP]    mPathArena: %u bytes (%3.2f%%)\n", thePathArenaSize, (f8)thePathArenaSize/theTotalSize*100);
    LDebug_Print("[LDSP]    mSH:        %u bytes (%3.2f%%)\n", theSHSize, (f8)theSHSize/theTotalSize*100);
    LDebug_Print("[LDSP]    mH:         %u bytes (%3.2f%%)\n", theHSize, (f8)theHSize/theTotalSize*100);
    LDebug_Print("[LDSP]    mQ:         %u bytes (%3.2f%%)\n", theQSize, (f8)theQSize/theTotalSize*100);
    LDebug_Print("[LDSP]    mA:         %u bytes (%3.2f%%)\n", theASize, (f8)theASize/theTotalSize*100);
    LDebug_Print("[LDSP]    LDSP obj:   %u bytes (%3.2f%%)\n", sizeof(LDSP), (f8)sizeof(LDSP)/theTotalSize*100);

    theStat = LDSP_GetStatistics(This);
    LDebug_Print("[LDSP] Path statistics:\n");
    LDebug_Print("[LDSP]    #PUP:       %u\n", theStat.mPUP);
    LDebug_Print("[LDSP]    #UP:        %u\n", theStat.mUP);
    LDebug_Print("[LDSP]    #HSP:       %u\n", theStat.mHSP);
    LDebug_Print("[LDSP]    #SP:        %u\n", theStat.mSP);
    LDebug_Print("[LDSP]    #Zombie:    %u\n", theStat.mZombie);
    LDebug_Print("[LDSP]    #Free:      %u\n", theStat.mFree);
    LDebug_Print("[LDSP]    #Affected:  %u\n", theStat.mAffected);
    LDebug_Print("[LDSP]    #NewPUP:    %u\n", theStat.mNewPUP);
    LDebug_Print("[LDSP]    #DelPUP:    %u\n", theStat.mDelPUP);
    LDebug_Print("[LDSP]    #DummyUpd:  %u\n", theStat.mDummyUpdates);
    LDebug_Print("[LDSP]    #Skipped:   %u\n", theStat.mSkipped);
    LDebug_Print("[LDSP]    #SRate:     %1.3f\n", This->mSmoothingRate);
}


/* ----------------------------------------------------------------------------
 *  GetStatistics
 * ----------------------------------------------------------------------------
*/
LDSP_TStat LDSP_GetStatistics(LDSP* This){
    LDSP_TStat theStat = { 0 };
    ui4 theIdx, thePathArenaCount = LArray_GetItemsCount(This->mPathArena);

    /* gather path statistics */
    for (theIdx=0; theIdx < thePathArenaCount; ++theIdx) {

        /* count free slots */
        if (_Path[theIdx].mZ == _LDSP_FREE) { ++theStat.mFree; continue; }

        /* count historical shortest paths and zombies */
        if (_Path[theIdx].mZ == _LDSP_SP_ZOMBIE) {
            ++theStat.mHSP;
            if (This->mMatrix[_Path[theIdx].mX][_Path[theIdx].mY].mSP != theIdx) {
                ++theStat.mZombie;
                #if 0
                {	ui4 theSub;
                    LDebug_Print("Zombie %hu %hu (TB=%u) - weight %u - edgew %u - spw %u (TB=%u)\n", 
                        _Path[theIdx].mX, _Path[theIdx].mY, 
                        _Path[theIdx].mW, This->mMatrix[_Path[theIdx].mX][_Path[theIdx].mY].mW,
                        _Path[This->mMatrix[_Path[theIdx].mX][_Path[theIdx].mY].mSP].mW,
                        _Path[This->mMatrix[_Path[theIdx].mX][_Path[theIdx].mY].mSP].mTB);
                    LDebug_Print("- SP:     ");
                    theSub = This->mMatrix[_Path[theIdx].mX][_Path[theIdx].mY].mSP;
                    for (;;) {
                        LDebug_Print("%hu ", _Path[theSub].mX);
                        if (_Path[theSub].mX == _Path[theSub].mY) break;
                        theSub = _Path[theSub].mr;
                    };
                    LDebug_Print("\n");
                    LDebug_Print("- Zombie: ");
                    theSub = theIdx;
                    for (;;) {
                        LDebug_Print("%hu ", _Path[theSub].mX);
                        if (_Path[theSub].mX == _Path[theSub].mY) break;
                        theSub = _Path[theSub].mr;
                    };
                    LDebug_Print("\n");
                }
                #endif
            }
        }

        /* count uniform paths */
        if (_Path[theIdx].mX != _Path[theIdx].mY) {
            ui4 theL = _Path[theIdx].ml, theR = _Path[theIdx].mr;
            if (This->mMatrix[_Path[theL].mX][_Path[theL].mY].mSP == theL &&
                This->mMatrix[_Path[theR].mX][_Path[theR].mY].mSP == theR) ++theStat.mUP;
        }
        else ++theStat.mUP;

        /* count potentially uniform paths */
        ++theStat.mPUP;
    }

    /* other fields */
    theStat.mSP           = theStat.mHSP - theStat.mZombie;
    theStat.mAffected     = This->mAffected;
    theStat.mNewPUP       = This->mNewPUP;
    theStat.mDelPUP       = This->mDelPUP;
    theStat.mDummyUpdates = This->mDummyUpdates;
    theStat.mSkipped      = This->mSkipped;

    return theStat;
}

/* ----------------------------------------------------------------------------
 *  GetUsedMem
 * ----------------------------------------------------------------------------
 * return the total number of bytes used for maintaining the data structure */

ui4 LDSP_GetUsedMem(LDSP* This){
    return LArray_GetUsedMem(This->mPathArena)     +
           sizeof(TMatrixEntry*)*This->mN          +
           sizeof(TMatrixEntry)*This->mN*This->mN  +
           LHeap_GetUsedMem(This->mSH)             + 
           LHeap_GetUsedMem(This->mH)              + 
           LArray_GetUsedMem(This->mQ)             +
           LArray_GetUsedMem(This->mA)             +
           sizeof(LDSP);
}


/* PRIVATE FUNCTIONS */

/* ----------------------------------------------------------------------------
 *  _UnSmoothedUpdate
 * ----------------------------------------------------------------------------
*/
static void _UnSmoothedUpdate(LDSP* This, ui2 inX, ui2 inY, ui4 inW){

    /* operation setup */
    This->mNewPUP = This->mDelPUP = 0;
    sThis = This;

    /* perform update */
    _Delete(This, inX, inY);
    _Insert(This, inX, inY, inW);
}


/* ----------------------------------------------------------------------------
 *  _Delete
 * ----------------------------------------------------------------------------
*/
static void _Delete(LDSP* This, ui2 inU, ui2 inV){

    ui4 thePath = This->mMatrix[inU][inV].mEdgePath;
    ui4 theExt;

    /* skip deletion if the edge is not in the graph G */
    if (thePath == _LDSP_NIL) return;

    Try {

        /* add potentially uniform path p_uv=(u,v) to Q and delete it */
        LArray_AppendItem(This->mQ, (void*)&thePath);
        _DelPUP(This, thePath, inU, inV);

        /* propagation loop */
        while (LArray_GetItemsCount(This->mQ)) {

            /* extract path pi_xy from Q (thePath) */
            thePath = _ui4_(LArray_LastItem(This->mQ));
            LArray_RemoveLastItem(This->mQ);
                
            /* if the removed path pi_xy is a shortest path, then add (u,v) to A */
            if (thePath == This->mMatrix[_Path[thePath].mX][_Path[thePath].mY].mSP) {
                TPair thePair;
                thePair.mX = _Path[thePath].mX; 
                thePair.mY = _Path[thePath].mY;
                LArray_AppendItem(This->mA, (void*)&thePair);
            }

            /* add paths in L(pi_xy) to Q and delete them */
            for (theExt = _Path[thePath].mL; theExt!=_LDSP_NIL; theExt = _Path[theExt].mNextLr) {
                if (_Path[theExt].mZ == _LDSP_FREE) continue;
                LArray_AppendItem(This->mQ, (void*)&theExt);
                _DelPUP(This, theExt, inU, inV);
            }

            /* add paths in R(pi_xy) to Q and delete them */
            for (theExt = _Path[thePath].mR; theExt!=_LDSP_NIL; theExt = _Path[theExt].mNextRl) {
                if (_Path[theExt].mZ == _LDSP_FREE) continue;
                LArray_AppendItem(This->mQ, (void*)&theExt);
                _DelPUP(This, theExt, inU, inV);
            }
        }
    }

    /* clear Q and A in case of exception */
    CatchAny {
        LArray_RemoveAllItems(This->mQ);
        LArray_RemoveAllItems(This->mA);
        Rethrow;
    }
}


/* ----------------------------------------------------------------------------
 *  _Insert
 * ----------------------------------------------------------------------------
*/
static void _Insert(LDSP* This, ui2 inU, ui2 inV, ui4 inW){

    ui4 thePath, theNewPath, theExt;

    Try {

        /* PHASE 1 */

        /* handle edge (inU,inV) */
        if (inW < LDSP_INFTY) {

            /* possibly schedule edge (inU,inV) for update. Observe that, if
             * This->mMatrix[inU][inV].mSP == This->mMatrix[inU][inV].mEdgePath, then
             * edge (inU,inV) has already been scheduled for update in _Delete */
            if (This->mMatrix[inU][inV].mSP == _LDSP_NIL ||
                This->mMatrix[inU][inV].mSP != This->mMatrix[inU][inV].mEdgePath) {
                ui4 theMinPath = _GetMinPath(This, inU, inV);
                if (theMinPath == _LDSP_NIL || inW < _Path[theMinPath].mW) {
                    TPair thePair;
                    thePair.mX = inU; 
                    thePair.mY = inV;
                    LArray_AppendItem(This->mA, (void*)&thePair);
                }
            }

            /* update weight of edge (inU,inV) */
            This->mMatrix[inU][inV].mW = inW;

            /* make new potentially uniform path */
            This->mMatrix[inU][inV].mEdgePath = _NewPUP(This, inU, inV);

            /* add path to P_xy */
            _AddToList(This->mMatrix[inU][inV].mP, This->mMatrix[inU][inV].mEdgePath, mPrevP, mNextP);
        }
        else {
            This->mMatrix[inU][inV].mW = inW;
            This->mMatrix[inU][inV].mEdgePath = _LDSP_NIL;
        }

        /* for profiling purposes only... */
        This->mAffected = LArray_GetItemsCount(This->mA);

        /* PHASE 2 */

        /* scan pairs in A (pairs affected by the update) */
        while (LArray_GetItemsCount(This->mA)) {

            /* extract pair (x,y) from A */
            TPair thePair = *(TPair*)LArray_LastItem(This->mA);
            LArray_RemoveLastItem(This->mA);

            /* flag a shortest path (if any) has to be found for pair (x,y) */
            This->mMatrix[thePair.mX][thePair.mY].mSP = _LDSP_NIL;

            /* find pointer to minimum weight path pi_xy in P_xy, if any */
            thePath = _GetMinPath(This, thePair.mX, thePair.mY);

            /* add path to priority queue H */
            if (thePath!=_LDSP_NIL) 
                LHeap_Add(This->mH, NULL, thePath);
        }

        /* PHASE 3 */

        /* propagation loop */
        while (!LHeap_Empty(This->mH)) {

            /* extract pi_xy (thePath) with minimum weight from H */
            LHeap_ExtractMin(This->mH, NULL, &thePath);

            /* skip path pi_xy if it is not better than current shortest path for pair (x,y) */
            if (thePath != _GetMinPath(This, _Path[thePath].mX, _Path[thePath].mY)) continue;

            /* let pi_xy be the new shortest path between x and y */
            This->mMatrix[_Path[thePath].mX][_Path[thePath].mY].mSP = thePath;

            /* skip path pi_xy if it was already a shortest path or a zombie before the update */
            if (_Path[thePath].mZ == _LDSP_SP_ZOMBIE) continue;

            /* make path pi_xy a shortest path */
            _Path[thePath].mZ  = _LDSP_SP_ZOMBIE;
            _AddToList(_Path[_Path[thePath].ml].mRS, thePath, mPrevRSl, mNextRSl);
            _AddToList(_Path[_Path[thePath].mr].mLS, thePath, mPrevLSr, mNextLSr);

            /* scan L*(l(pi_xy)): potentially uniform left extension */
            for (theExt = _Path[_Path[thePath].ml].mLS; theExt!=_LDSP_NIL; theExt = _Path[theExt].mNextLSr) {

                /* skip potentially uniform path of the form pi_vv */
                if (_Path[theExt].mX == _Path[thePath].mY) continue;

                /* make new potentially uniform path */
                theNewPath = _NewPUP(This, theExt, thePath);

                /* add path to P_xy, R(l(pi_xy)) and L(r(pi_xy)) */
                _AddToList(This->mMatrix[_Path[theExt].mX][_Path[thePath].mY].mP, theNewPath, mPrevP, mNextP);
                _AddToList(_Path[theExt].mR, theNewPath, mPrevRl, mNextRl);
                _AddToList(_Path[thePath].mL, theNewPath, mPrevLr, mNextLr);

                /* add new path to priority queue H */
                LHeap_Add(This->mH, NULL, theNewPath);
            }

            /* scan R*(r(pi_xy)): potentially uniform left extension */
            for (theExt = _Path[_Path[thePath].mr].mRS; theExt!=_LDSP_NIL; theExt = _Path[theExt].mNextRSl) {

                /* skip potentially uniform path of the form pi_vv */
                if (_Path[thePath].mX == _Path[theExt].mY) continue;

                /* make new potentially uniform path */
                theNewPath = _NewPUP(This, thePath, theExt);

                /* add path to P_xy, R(l(pi_xy)) and L(r(pi_xy)) */
                _AddToList(This->mMatrix[_Path[thePath].mX][_Path[theExt].mY].mP, theNewPath, mPrevP, mNextP);
                _AddToList(_Path[thePath].mR, theNewPath, mPrevRl, mNextRl);
                _AddToList(_Path[theExt].mL, theNewPath, mPrevLr, mNextLr);

                /* add new path to priority queue H */
                LHeap_Add(This->mH, NULL, theNewPath);
            }
        }
    }

    /* clear A and H in case of exception */
    CatchAny {
        LArray_RemoveAllItems(This->mA);
        LHeap_Delete(&This->mH);
        This->mH = LHeap_New(_HComparator);
        Rethrow;
    }
}


/* ----------------------------------------------------------------------------
 *  _NewPUP
 * ----------------------------------------------------------------------------
*/
static ui4 _NewPUP(LDSP* This, ui4 inL, ui4 inR){ /** Not Inference; **/

    register TPath* thePath;
    register TPath* theL;
    register TPath* theR;

    /* reuse slot or create new slot in mPathArena */
    if (This->mFreeSlot != _LDSP_NIL) {
        thePath = _Path + This->mFreeSlot;
        This->mFreeSlot = thePath->mTB;
    }
    else {
        LArray_ResizeBy(This->mPathArena, 1);
        thePath = (TPath*)LArray_LastItem(This->mPathArena);
    }

    /* cache pointers */
    theL = _Path + inL;
    theR = _Path + inR;

    /* init path pi_xy */
    thePath->mX  = theL->mX;
    thePath->mY  = theR->mY;
    thePath->mW  = theL->mW + This->mMatrix[theL->mY][theR->mY].mW;
    thePath->mZ  = 0;
    thePath->ml  = inL; 
    thePath->mr  = inR;
    thePath->mL       = thePath->mR       = _LDSP_NIL;
    thePath->mLS      = thePath->mRS      = _LDSP_NIL;
    thePath->mNextLr  = thePath->mPrevLr  = _LDSP_NIL;
    thePath->mNextRl  = thePath->mPrevRl  = _LDSP_NIL;
    thePath->mNextLSr = thePath->mPrevLSr = _LDSP_NIL;
    thePath->mNextRSl = thePath->mPrevRSl = _LDSP_NIL;
    thePath->mNextP   = thePath->mPrevP   = _LDSP_NIL;

    /* init tie breaking tag for path pi_xy */
    if (theL->mX != theL->mY) 

         /* if pi_xy is made of more than 2 edges, then take the max of theL->mTB and theR->mTB  */
         thePath->mTB = (theL->mTB > theR->mTB) ? theL->mTB : theR->mTB;

         /* if pi_xy is made of one edge, then build unique tag depending on the endpoints  */
    else thePath->mTB = This->mMatrix[thePath->mX][thePath->mY].mT;

    /* for statistics purposes only... */
    This->mNewPUP++;

    return thePath - _Path;
}


/* ----------------------------------------------------------------------------
 *  _DelPUP
 * ----------------------------------------------------------------------------
*/
static void _DelPUP(LDSP* This, ui4 inPath, ui2 inU, ui2 inV){ /** Not Inference; **/

    /* remove path from lists */
    _DelFromList(This->mMatrix[_Path[inPath].mX][_Path[inPath].mY].mP,inPath,mPrevP,mNextP);
    if (_Path[inPath].mX == inU) {
	     _DelFromList(_Path[_Path[inPath].mr].mL,inPath,mPrevLr,mNextLr);
	     if (_Path[inPath].mZ == _LDSP_SP_ZOMBIE)
	         _DelFromList(_Path[_Path[inPath].mr].mLS,inPath,mPrevLSr,mNextLSr);
    }
    if (_Path[inPath].mY == inV) {
	     _DelFromList(_Path[_Path[inPath].ml].mR,inPath,mPrevRl,mNextRl);
	     if (_Path[inPath].mZ == _LDSP_SP_ZOMBIE)
	         _DelFromList(_Path[_Path[inPath].ml].mRS,inPath,mPrevRSl,mNextRSl);
    }

    /* free slot */
    _Path[inPath].mTB = This->mFreeSlot;
    This->mFreeSlot   = inPath;
    _Path[inPath].mZ  = _LDSP_FREE; /* for profiling purposes only... */

    /* for statistics purposes only... */
    This->mDelPUP++;
}


/* ----------------------------------------------------------------------------
 *  _GetMinPath
 * ----------------------------------------------------------------------------
 * theoretically inefficient solution (linear search) 
 * however, this might be good in practice, especially for sparse graphs
*/

static ui4 _GetMinPath(LDSP* This, ui2 inX, ui2 inY){

    register ui4 theMinPath = _LDSP_NIL;
    register ui4 thePath;

    /* scan paths in P_xy (thePath) */
    for (thePath = This->mMatrix[inX][inY].mP; thePath!=_LDSP_NIL; thePath = _Path[thePath].mNextP) {

        /* extract minimum weight path */
        if (theMinPath == _LDSP_NIL || 
            _Path[thePath].mW < _Path[theMinPath].mW || 
            _Path[thePath].mW == _Path[theMinPath].mW && _Path[thePath].mTB < _Path[theMinPath].mTB) 
            theMinPath = thePath;
    }

    return theMinPath;
}


/* ----------------------------------------------------------------------------
 *  _HComparator
 * ----------------------------------------------------------------------------
 * comparator for priority queue mH */
static Bool _HComparator(ui4 inKeyA,  ui4 inKeyB){
    return sThis->mArenaBase[inKeyA].mW  <  sThis->mArenaBase[inKeyB].mW  ||
           sThis->mArenaBase[inKeyA].mW  == sThis->mArenaBase[inKeyB].mW  &&
           sThis->mArenaBase[inKeyA].mTB <  sThis->mArenaBase[inKeyB].mTB;
}


/* ----------------------------------------------------------------------------
 *  _SHComparator
 * ----------------------------------------------------------------------------
 * comparator for priority queue mSH */
static Bool _SHComparator(ui4 inKeyA,  ui4 inKeyB){
    return inKeyA < inKeyB;
}


/* Copyright (C) 2002-2003 Camil Demetrescu

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
