/* ============================================================================
 *  main.c
 * ============================================================================

 *  Author:         (c) 2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        May 9, 2003
 *  Module:         DSP

 *  Last changed:   $Date: 2010/11/08 11:58:22 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.2 $
*/

/* INCLUDES */
#include "LDSP.h"
#include "LSP.h"
#include "LGraph.h"
#include "LGraphGen.h"
#include "LEdgeInfo.h"
#include "LString.h"
#include "LArray.h"
#include "LDebug.h"
#include "LFile.h"
#include "LException.h"
#include "LMemory.h"
#include "LTime.h"
#include "LRandSource.h"
#include "LFile.h"

#include "CAPSP_C.h"
#include "CDAPSP_DE.h"
#include "CDAPSP.h"
#include "CDAPSP_D.h"

#ifdef LEONARDO
#include <syscall.h>
#endif

#define MAIN_INFINITY LType_MAX_UI4

/* TYPEDEFS */
typedef struct _TUpdate {
    LGraph_TEdge* mEdge;
    ui4           mWeight;
} _TUpdate;

/* GLOBAL DATA STRUCTURES */
static struct {
    Bool           mCheckerStatus;                       /* if TRUE, algorithm correctness is checked after each update */
    Bool           mVerboseStatus;                       /* if TRUE, verbose mode is on */
	Bool           mAdjustSeq;                           /* if TRUE min and miax weihts of the sequence are the same of the graph */
    i1             mGraphFile[LFile_MAX_PATHNAME_LEN];   /* pathname of current graph ("" if empty or generated on the fly) */
    i1             mResultsPath[LFile_MAX_PATHNAME_LEN]; /* path of results directory */
    i1             mExpName[LFile_MAX_PATHNAME_LEN];     /* name of current experiment */
    LGraph*        mGraph;                               /* current graph */
    LEdgeInfo*     mEdgeWeights;                         /* weights of edges in mGraph */
    ui4            mGraphSeed;                           /* random graph seed */
    ui4            mMinGraphW;                           /* minimum graph weight */
    ui4            mMaxGraphW;                           /* maximum graph weight */
    LArray*        mSequence;                            /* current update sequence */
    ui4            mSeqSeed;                             /* random sequence seed */
    ui4            mMinSeqW;                             /* minimum sequence weight */
    ui4            mMaxSeqW;                             /* maximum sequence weight */
	ui4            mIncCount;							 /* increase operations in the sequence */
	ui4            mDecCount;						     /* decrease operations in the sequence */
    f8             mTime;                                /* time base for computing elapsed time */
    LDSP_TSetup    mLDSPConfig;                          /* LDSP configuration parameters */
	CDAPSP_TConfig mCDAPSPConfig;                        /* CDAPSP configuration parameters */       
} sData;


/* DEFINES */
#define _LINE_LEN_ 1024


/* PRIVATE FUNCTION PROTOTYPES */
static Bool _Init();
static void _Cleanup();

static void _MainMenuSwitch();
static void _SetupSwitch();
static void _GraphInitSwitch();
static void _StructuredSwitch();

static void _AskRandomGraph();
static void _AskRandomSequence();
static void _AskBottleNeckTestSet();
static void _AskInsDelSequence();
static void _AskClusteredTestSet();

#ifdef LEONARDO
static void _AskLeonardoGraph();
#endif
static void _AskXMLGraph();
static void _AskDIMACSGraph();
static void _AskExpName();
static void _AskLDSPConfig();
static void _AskCDAPSPConfig();

static void _MakeRandomGraph(ui4 inSeed, ui4 inN, ui4 inM, ui4 inMinW, ui4 inMaxW);
static void _MakeRandomSequence(ui4 inSeed, ui4 inLength, ui4 inMinW, ui4 inMaxW);
static void _MakeBottleNeckTestSet(ui2 inN, f8 inDensity, ui4 inNumOp, ui4 inSeed, ui4 inMinW, ui4 inMaxW);
static void _MakeInsDelSequence(ui4 theSeed, ui4 theLength);
static void _MakeClusteredTestSet(ui2 theN, ui2 theC, ui4 theSeed, f4 thePc, f4 thePi, 
                                  ui4 theMinW, ui4 theMaxW, ui4 theLength);
static void _CountIncreaseDecrease();
static void _LoadXMLGraph(i1 inPathName[LFile_MAX_PATHNAME_LEN]);
static void _LoadDIMACSGraph(i1 inPathName[LFile_MAX_PATHNAME_LEN]);
static void _FindMinMaxGraphWeight();

static void _RunCAPSP_C();
static void _RunLDSP();
static void _RunCDAPSP_DE();
static void _RunCDAPSP();
static void _RunCDAPSP_D();
static void _RunLSP();

static void _StartTimer();
static f8   _StopTimer();

static void _LogCAPSP_CResults(f8 inTotUpdTime, f8 inUpdTime /*, ui4 inSeqMem*/);
static void _LogLDSPResults(f8 inInitTime, ui4 inInitMem, 
                            f8 inTotUpdTime, f8 inUpdTime, ui4 inSeqMem, 
                            LDSP* inLDSP);
static void _LogCDAPSP_DEResults(f8 inInitTime, ui4 inInitMem, 
                                 f8 inTotUpdTime, f8 inUpdTime, ui4 inSeqMem);
static void _LogCDAPSPResults(f8 inInitTime, ui4 inInitMem, f8 inTotUpdTime, 
							  f8 inUpdTime, ui4 inSeqMem, f8 inAvgBlcks, f8 inAvgError, 
							  ui4 inInfCnt, f8 inErrorRatio); 
static void _LogCDAPSP_DResults(f8 inInitTime, ui4 inInitMem, 
                              f8 inTotUpdTime, f8 inUpdTime, ui4 inSeqMem);
static void _LogLSPResults(f8 inTotUpdTime, f8 inUpdTime /*, ui4 inSeqMem*/);

static void _LogInputHeader(LFile* inRes);
static void _LogInputData(LFile* inRes);


/* PUBLIC FUNCTION DEFINITIONS */

/* ----------------------------------------------------------------------------
 *  main
 * ----------------------------------------------------------------------------
*/
int main() {

    if (!_Init()) return 1;
    _MainMenuSwitch();
    _Cleanup();

    LDebug_Print("-- Undeallocated blocks count: %u\n", LMemory_GetBlocksCount());

    #if INSTRUMENT_FUNCTIONS == 1
    extern unsigned long num_enter, num_exit;
    LDebug_Print("num_enter = %lu, num_exit=%lu\n", num_enter, num_exit);
    #endif

    return 0;
}


/* PRIVATE FUNCTION DEFINITIONS */

/* ----------------------------------------------------------------------------
 *  _Init
 * ----------------------------------------------------------------------------
*/
static Bool _Init() {
    LDSP* theDummy = NULL;
    Try {
        theDummy = LDSP_NewEmpty(1);
        sData.mLDSPConfig = LDSP_GetConfig(theDummy);
    }
    CatchAny LDebug_Print("** Init error\n");
    if (theDummy!=NULL) LDSP_Delete(&theDummy);
	sData.mCDAPSPConfig.mAlpha = 0.0f;
	sData.mCDAPSPConfig.mBeta  = 1.0f;
	sData.mAdjustSeq = FALSE;
    return TRUE;
}


/* ----------------------------------------------------------------------------
 *  _Cleanup
 * ----------------------------------------------------------------------------
*/
static void _Cleanup(){
    if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
    if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
    if (sData.mSequence!=NULL)    LArray_Delete(&sData.mSequence);
}


/* ----------------------------------------------------------------------------
 *  _MainMenuSwitch
 * ----------------------------------------------------------------------------
*/
static void _MainMenuSwitch() {
    i1 theLine[_LINE_LEN_];

    while(TRUE) {

        LDebug_Print("-- MAIN MENU.\n");
        LDebug_Print("g. Make initial graph ");
        if (sData.mGraph == NULL) 
             LDebug_Print("[current: none]\n"); 
        else LDebug_Print("[current: n=%u | m=%u | name=\"%s\"]\n",
                 LGraph_GetNodesCount(sData.mGraph), 
                 LGraph_GetEdgesCount(sData.mGraph), 
                 sData.mGraphFile);

        LDebug_Print("s. Make update sequence ");
        if (sData.mSequence == NULL) 
             LDebug_Print("[current: none]\n"); 
        else LDebug_Print("[current: length=%u | #inc=%u | #dec=%u]\n", 
                LArray_GetItemsCount(sData.mSequence),
                sData.mIncCount, sData.mDecCount);

		LDebug_Print("l. Make insertion/deletion sequence\n");
        LDebug_Print("k. Make structured test sets\n");
        LDebug_Print("t. Setup\n");
        LDebug_Print("a. Run CAPSP_C   (S-DIJ  - Dijkstra n times)\n");
        LDebug_Print("b. Run LDSP      (D-PUP  - potentially uniform paths)\n");
        LDebug_Print("c. Run CDAPSP_DE (D-RRL  - Ramalingam/Reps variant)\n");
        LDebug_Print("d. Run CDAPSP    (D-KIN  - King)\n");
		LDebug_Print("e. Run CDAPSP_D  (D-KIND - King up to D)\n");
        LDebug_Print("f. Run LSP       (S-UP   - uniform paths)\n");
        LDebug_Print("q. Quit\n");

        LDebug_GetString(theLine, _LINE_LEN_);

        switch(theLine[0]) {
            case '#': 
            case '-': break;
            case 'g': _GraphInitSwitch(); break;
            case 's': _AskRandomSequence(); break;
            case 'k': _StructuredSwitch(); break;
			case 'l': _AskInsDelSequence(); break;
            case 't': _SetupSwitch(); break;
            case 'a': _RunCAPSP_C(); break;
            case 'b': _RunLDSP(); break;
            case 'c': _RunCDAPSP_DE(); break;
            case 'd': _RunCDAPSP(); break;
			case 'e': _RunCDAPSP_D(); break;
			case 'f': _RunLSP(); break;
            case 'q': goto Quit;
            default : LDebug_Print("** Unknown option.\n");
        }
    }

  Quit:
    LDebug_Print("-- Sayonara.\n");
}


/* ----------------------------------------------------------------------------
 *  _SetupSwitch
 * ----------------------------------------------------------------------------
*/
static void _SetupSwitch(){
    i1 theLine[_LINE_LEN_];

    while(TRUE) {

        LDebug_Print("-- SETUP MENU.\n");
        LDebug_Print("c. Toggle checker status [current status: %s]\n", sData.mCheckerStatus? "ON":"OFF");
        LDebug_Print("v. Toggle verbose mode status [current status: %s]\n", sData.mVerboseStatus? "ON":"OFF");
		LDebug_Print("s. Toggle sequence adjust mode status [current status: %s]\n", sData.mAdjustSeq? "ON":"OFF");
		LDebug_Print("e. Enter experiment name [current name: \"%s\"]\n", sData.mExpName);
        LDebug_Print("l. Config LDSP (current: T=%4.1f | C=%4.1f | S=%u | R=%u)\n", 
            sData.mLDSPConfig.mThreshold, sData.mLDSPConfig.mCorrection, sData.mLDSPConfig.mStep, sData.mLDSPConfig.mRatio);
		LDebug_Print("d. Config CDAPSP (current: ALPHA=%1.4f | BETA=%1.4f)\n", sData.mCDAPSPConfig.mAlpha,
			sData.mCDAPSPConfig.mBeta);
        LDebug_Print("m. Back to main menu\n");

        LDebug_GetString(theLine, _LINE_LEN_);

        switch(theLine[0]) {
            case '#': 
            case '-': break;
            case 'c': sData.mCheckerStatus = !sData.mCheckerStatus; break;
            case 'v': sData.mVerboseStatus = !sData.mVerboseStatus; break;
			case 's': sData.mAdjustSeq     = !sData.mAdjustSeq;     break;
            case 'e': _AskExpName(); break;
            case 'l': _AskLDSPConfig(); break;
			case 'd': _AskCDAPSPConfig(); break;
            case 'm': return;
            default : LDebug_Print("** Unknown option.\n");
        }
    }
}


/* ----------------------------------------------------------------------------
 *  _GraphInitSwitch
 * ----------------------------------------------------------------------------
*/
static void _GraphInitSwitch(){
    i1 theLine[_LINE_LEN_];

    while(TRUE) {

        LDebug_Print("-- GRAPH INIT MENU.\n");
        LDebug_Print("r. Generate random graph\n");
        LDebug_Print("l. Load graph in XML format\n");
        LDebug_Print("d. Load graph in DIMACS format\n");
        #ifdef LEONARDO
        LDebug_Print("e. Load graph in Leonardo format\n");
        #endif
        LDebug_Print("m. Back to main menu\n");

        LDebug_GetString(theLine, _LINE_LEN_);

        switch(theLine[0]) {
            case '#': 
            case '-': break;
            case 'r': _AskRandomGraph(); break;
            case 'l': _AskXMLGraph(); break;
            case 'd': _AskDIMACSGraph(); break;
            #ifdef LEONARDO
            case 'e': _AskLeonardoGraph(); break;
            #endif
            case 'm': return;
            default : LDebug_Print("** Unknown option.\n");
        }
    }
}


/* ----------------------------------------------------------------------------
 *  _StructuredSwitch
 * ----------------------------------------------------------------------------
*/
static void _StructuredSwitch(){
    i1 theLine[_LINE_LEN_];

    while(TRUE) {

        LDebug_Print("-- STRUCTURED TEST SET MENU.\n");
        LDebug_Print("b. Generate bottleneck test set\n");
        LDebug_Print("c. Generate clustered test set\n");
        LDebug_Print("m. Back to main menu\n");

        LDebug_GetString(theLine, _LINE_LEN_);

        switch(theLine[0]) {
            case '#': 
            case '-': break;
            case 'b': _AskBottleNeckTestSet(); break;
            case 'c': _AskClusteredTestSet(); break;
            case 'm': return;
            default : LDebug_Print("** Unknown option.\n");
        }
    }
}


/* ----------------------------------------------------------------------------
 *  _AskRandomGraph
 * ----------------------------------------------------------------------------
*/
static void _AskRandomGraph(){
    ui4 theSeed, theN, theM, theMinW, theMaxW;
    i1 theLine[_LINE_LEN_];

    LDebug_Print("Enter random generator seed: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theSeed = LString_ToUI4(theLine);

    LDebug_Print("Enter number of nodes: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theN = LString_ToUI4(theLine);

    LDebug_Print("Enter number of edges: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theM = LString_ToUI4(theLine);

    LDebug_Print("Enter minimum weight: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theMinW = LString_ToUI4(theLine);

    LDebug_Print("Enter maximum weight: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theMaxW = LString_ToUI4(theLine);

    _MakeRandomGraph(theSeed, theN, theM, theMinW, theMaxW);
}


/* ----------------------------------------------------------------------------
 *  _MakeRandomGraph
 * ----------------------------------------------------------------------------
*/
static void _MakeRandomGraph(ui4 inSeed, ui4 inN, ui4 inM, ui4 inMinW, ui4 inMaxW){

    if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
    if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
    if (sData.mSequence!=NULL)    LArray_Delete(&sData.mSequence);

    Try {
        sData.mGraph       = LGraphGen_RandomNM((ui2)inN, inM, inSeed);
		LGraphGen_RemoveSelfLoops(&(sData.mGraph));
        sData.mEdgeWeights = LGraphGen_RndEdgeInfoUI4(sData.mGraph, inMinW, inMaxW, inSeed);
        LString_Copy("random", sData.mGraphFile);
        sData.mGraphSeed = inSeed;
        sData.mMinGraphW = inMinW;
        sData.mMaxGraphW = inMaxW;
    }

    CatchAny {
        if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
        if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
        LDebug_Print("** Couldn't generate graph.\n");
    }

    if (sData.mVerboseStatus) {
        LGraph_SetDebug(sData.mGraph,TRUE);
        LGraph_Dump(sData.mGraph);
    }
}


/* ----------------------------------------------------------------------------
 *  _AskRandomSequence
 * ----------------------------------------------------------------------------
*/
static void _AskRandomSequence(){

    ui4          theSeed, theLength, theMinW, theMaxW;
    i1           theLine[_LINE_LEN_];

    if (sData.mGraph == NULL) {
        LDebug_Print("** Can't generate sequence without a graph.\n");
        return;
    }

    LDebug_Print("Enter random generator seed: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theSeed = LString_ToUI4(theLine);

    do {
        LDebug_Print("Enter length of the sequence: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        theLength = LString_ToUI4(theLine);
    } while (theLength==0);
	
	if (!sData.mAdjustSeq)
	{
		LDebug_Print("Enter minimum weight (initial graph min=%u): ", sData.mMinGraphW);
		LDebug_GetString(theLine, _LINE_LEN_);
		theMinW = LString_ToUI4(theLine);
	}
	else
	{
		LDebug_Print("Using graph value (graph min=%u)\n", sData.mMinGraphW);
		theMinW = sData.mMinGraphW;
	}


    do {
		if (!sData.mAdjustSeq)
		{
			LDebug_Print("Enter maximum weight (initial graph max=%u): ", sData.mMaxGraphW);
			LDebug_GetString(theLine, _LINE_LEN_);
			theMaxW = LString_ToUI4(theLine);
		}
		else
		{
			LDebug_Print("Using graph value (graph max=%u)\n", sData.mMaxGraphW);
			theMaxW = sData.mMaxGraphW;
		}
    } while(theMaxW<theMinW);

    _MakeRandomSequence(theSeed, theLength, theMinW, theMaxW);
}


/* ----------------------------------------------------------------------------
 *  _AskInsDelSequence
 * ----------------------------------------------------------------------------
*/
static void _AskInsDelSequence(){

    ui4          theLength, theSeed;
    i1           theLine[_LINE_LEN_];

    if (sData.mGraph == NULL) {
        LDebug_Print("** Can't generate sequence without a graph.\n");
        return;
    }

	LDebug_Print("Enter random generator seed: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theSeed = LString_ToUI4(theLine);

    do {
        LDebug_Print("Enter length of the sequence: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        theLength = LString_ToUI4(theLine);
    } while (theLength==0);

    _MakeInsDelSequence(theSeed, theLength);
}


/* ----------------------------------------------------------------------------
 *  _MakeRandomSequence
 * ----------------------------------------------------------------------------
*/
static void _MakeRandomSequence(ui4 inSeed, ui4 inLength, ui4 inMinW, ui4 inMaxW) {
    LRandSource* theSource       = NULL;
    LArray*      theEdgeList     = NULL;
    LEdgeInfo*   theEdgeInfoCopy = NULL;
    f8           theUpdType;

    if (sData.mSequence!=NULL) LArray_Delete(&sData.mSequence);
    sData.mSequence = LArray_New(sizeof(_TUpdate));

    /* save initial edge weights */
    theEdgeInfoCopy = LGraphGen_CopyEdgeInfo(sData.mGraph, sData.mEdgeWeights);

    Try {
        _TUpdate theUpdate;
        ui4      theEdgeIdx, theWeight, i;

        theSource   = LRandSource_New((i4)inSeed);
        theEdgeList = LGraph_GetAllEdges(sData.mGraph);

        for (i=0; i<inLength; ++i) {
            theUpdType = LRandSource_GetRandF8(theSource);
            theEdgeIdx = LRandSource_GetRandUI4(theSource, 0, LArray_GetItemsCount(theEdgeList)-1);
            theUpdate.mEdge = *(LGraph_TEdge**)LArray_ItemAt(theEdgeList, theEdgeIdx);
            theWeight = LEdgeInfo_UI4At(sData.mEdgeWeights, theUpdate.mEdge);
            if (theWeight > inMaxW) theWeight = inMaxW;
            if (theWeight < inMinW) theWeight = inMinW;
            if (theUpdType < 0.5) theWeight = LRandSource_GetRandUI4(theSource, theWeight, inMaxW);
            else                  theWeight = LRandSource_GetRandUI4(theSource, inMinW, theWeight);
            theUpdate.mWeight = theWeight;
            LEdgeInfo_UI4At(sData.mEdgeWeights, theUpdate.mEdge) = theWeight;
            if (theUpdate.mEdge->mSource->mIndex != theUpdate.mEdge->mTarget->mIndex) {
                LArray_AppendItem(sData.mSequence, (void*)&theUpdate);
                if (sData.mVerboseStatus)
                    LDebug_Print("Update %u: (%u,%u,%u) (edge #%u)\n", i, 
                        theUpdate.mEdge->mSource->mIndex, theUpdate.mEdge->mTarget->mIndex, 
                        theWeight, theEdgeIdx);
            }
            else ++inLength;
        }

        sData.mSeqSeed = inSeed;
        sData.mMinSeqW = inMinW;
        sData.mMaxSeqW = inMaxW;
    }

    CatchAny {
        if (sData.mSequence!=NULL) LArray_Delete(&sData.mSequence);
        LDebug_Print("** Couldn't generate sequence.\n");
    }

    if (theEdgeList!=NULL) LArray_Delete(&theEdgeList);
    if (theSource!=NULL)   LRandSource_Delete(&theSource);

    /* restore initial edge weights */
    LEdgeInfo_Delete(&sData.mEdgeWeights);
    sData.mEdgeWeights = theEdgeInfoCopy;

    if (sData.mSequence!=NULL) _CountIncreaseDecrease();
}


/* ----------------------------------------------------------------------------
 *  _MakeInsDelSequence
 * ----------------------------------------------------------------------------
*/
static void _MakeInsDelSequence(ui4 inSeed, ui4 inLength) {
    LRandSource* theSource   = NULL;
    LArray*      theEdgeList = NULL;

    if (sData.mSequence!=NULL) LArray_Delete(&sData.mSequence);
    sData.mSequence = LArray_New(sizeof(_TUpdate));

    Try {
        _TUpdate theUpdate;
        ui4      theEdgeIdx, theWeight, i;

        theSource   = LRandSource_New((i4)inSeed);
        theEdgeList = LGraph_GetAllEdges(sData.mGraph);

        for (i=0; i<inLength; ++i) {
			f8 theProb = LRandSource_GetRandF8(theSource);

            theEdgeIdx = LRandSource_GetRandUI4(theSource, 0, LArray_GetItemsCount(theEdgeList)-1);
			if (theProb <= 0.5f)
				theWeight = MAIN_INFINITY;
			else
				theWeight = 1;
            theUpdate.mEdge   = *(LGraph_TEdge**)LArray_ItemAt(theEdgeList, theEdgeIdx);
            theUpdate.mWeight = theWeight;
            if (theUpdate.mEdge->mSource->mIndex != theUpdate.mEdge->mTarget->mIndex) {
                LArray_AppendItem(sData.mSequence, (void*)&theUpdate);
                if (sData.mVerboseStatus)
                    LDebug_Print("Update %u: (%u,%u,%u) (edge #%u)\n", i, 
                        theUpdate.mEdge->mSource->mIndex, theUpdate.mEdge->mTarget->mIndex, 
                        theWeight, theEdgeIdx);
            }
            else ++inLength;
        }

        sData.mSeqSeed = inSeed;
        sData.mMinSeqW = 1;
        sData.mMaxSeqW = 1;
    }

    CatchAny {
        if (sData.mSequence!=NULL) LArray_Delete(&sData.mSequence);
        LDebug_Print("** Couldn't generate sequence.\n");
    }

    if (theEdgeList!=NULL) LArray_Delete(&theEdgeList);
    if (theSource!=NULL)   LRandSource_Delete(&theSource);

    if (sData.mSequence!=NULL) _CountIncreaseDecrease();
}


/* ----------------------------------------------------------------------------
 *  _AskBottleNeckTestSet
 * ----------------------------------------------------------------------------
*/
static void _AskBottleNeckTestSet(){

    ui4  theSeed, theLength, theMinW, theMaxW;
    ui2  theN;
    f8   theDensity;
    i1   theLine[_LINE_LEN_];

    LDebug_Print("Enter number of nodes: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theN = (ui2)LString_ToUI4(theLine);

    LDebug_Print("Enter random generator seed: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theSeed = LString_ToUI4(theLine);

    do {
        LDebug_Print("Enter edge density: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        theDensity = LString_ToF8(theLine);
    } while (theDensity < 0.0 || theDensity > 1.0);


    LDebug_Print("Enter minimum weight: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theMinW = LString_ToUI4(theLine);

    do {
        LDebug_Print("Enter maximum weight: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        theMaxW = LString_ToUI4(theLine);
    } while(theMaxW<theMinW);

    do {
        LDebug_Print("Enter length of the sequence: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        theLength = LString_ToUI4(theLine);
    } while (theLength==0);

    _MakeBottleNeckTestSet(theN, theDensity, theLength, theSeed, theMinW, theMaxW);
}


/* ----------------------------------------------------------------------------
 *  _MakeBottleNeckTestSet
 * ----------------------------------------------------------------------------
*/
static void _MakeBottleNeckTestSet(ui2 inN, f8 inDensity, ui4 inNumOp, ui4 inSeed, ui4 inMinW, ui4 inMaxW){

    ui2             theI, theJ, theL;
    _TUpdate        theUpdate;
    LArray*         theArray = NULL;
    LGraph_TNode**  theNodes;
    LRandSource*    theSource = NULL;

    if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
    if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
    if (sData.mSequence!=NULL)    LArray_Delete(&sData.mSequence);

    /* adjust inN... */
    if (inN < 6) inN = 6;
    if ((inN-2) & 0xFFFC) inN = (inN-2) & 0xFFFC + 2;
    theL = (inN-2)/4;

    Try {

        /* create pseudo-random source */
        theSource = LRandSource_New(inSeed);

        /* build graph */
        sData.mGraph = LGraph_New(LGraph_DIRECTED);
        for (theI=0; theI<inN; ++theI) LGraph_NewNode(sData.mGraph);
        theArray = LGraph_GetAllNodes(sData.mGraph);
        LArray_InstallSyncPtr(theArray, (void**)&theNodes);

        /* create bipartite graph L0*L1 */
        for (theI = 0; theI < theL; ++theI)
            for (theJ = theL; theJ < 2*theL; ++theJ)
                if (LRandSource_GetRandF8(theSource) < inDensity)
                    LGraph_NewEdge(sData.mGraph, theNodes[theI], theNodes[theJ]);

        /* create bipartite graph L2*L3 */
        for (theI = 2*theL; theI < 3*theL; ++theI)
            for (theJ = 3*theL; theJ < 4*theL; ++theJ)
                if (LRandSource_GetRandF8(theSource) < inDensity)
                    LGraph_NewEdge(sData.mGraph, theNodes[theI], theNodes[theJ]);

        /* connect L1 with start of bottleneck edge */
        for (theI = theL; theI < 2*theL; ++theI)
            LGraph_NewEdge(sData.mGraph, theNodes[theI], theNodes[inN-2]);

        /* connect end of bottleneck edge with L2 */
        for (theI = 2*theL; theI < 3*theL; ++theI)
            LGraph_NewEdge(sData.mGraph, theNodes[inN-1], theNodes[theI]);

        /* create bottleneck edge */
        theUpdate.mEdge = LGraph_NewEdge(sData.mGraph, theNodes[inN-2], theNodes[inN-1]);

        /* build edge weights */
        sData.mEdgeWeights = LGraphGen_RndEdgeInfoUI4(sData.mGraph, inMinW, inMaxW, inSeed);

        /* build sequence */
        sData.mSequence = LArray_New(sizeof(_TUpdate));
        for (theI = 0; theI < inNumOp; ++theI) {
            theUpdate.mWeight = LRandSource_GetRandUI4(theSource, inMinW, inMaxW);            
            LArray_AppendItem(sData.mSequence, (void*)&theUpdate);
            if (sData.mVerboseStatus)
                LDebug_Print("Update %u: (%u,%u,%u)\n", theI, 
                    theUpdate.mEdge->mSource->mIndex, theUpdate.mEdge->mTarget->mIndex, 
                    theUpdate.mWeight);
        }

        LString_Copy("bneck", sData.mGraphFile);
        sData.mGraphSeed = inSeed;
        sData.mMinGraphW = inMinW;
        sData.mMaxGraphW = inMaxW;
        sData.mSeqSeed   = inSeed;
        sData.mMinSeqW   = inMinW;
        sData.mMaxSeqW   = inMaxW;
    }

    CatchAny {
        if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
        if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
        LDebug_Print("** Couldn't generate test set.\n");
    }

    if (theArray!=NULL)  LArray_Delete(&theArray);
    if (theSource!=NULL) LRandSource_Delete(&theSource);

    if (sData.mVerboseStatus && sData.mGraph) {
        LGraph_SetDebug(sData.mGraph,TRUE);
        LGraph_Dump(sData.mGraph);
    }

	if (sData.mGraph) _CountIncreaseDecrease();
}


/* ----------------------------------------------------------------------------
 *  _AskClusteredTestSet
 * ----------------------------------------------------------------------------
*/
static void _AskClusteredTestSet(){
    
    ui4  theSeed, theLength, theMinW, theMaxW;
    ui2  theN, theC;
    f4   thePc, thePi;
    i1   theLine[_LINE_LEN_];

    LDebug_Print("Enter number of nodes: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theN = (ui2)LString_ToUI4(theLine);

    LDebug_Print("Enter number of clusters: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theC = (ui2)LString_ToUI4(theLine);

    LDebug_Print("Enter random generator seed: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theSeed = LString_ToUI4(theLine);

    do {
        LDebug_Print("Enter intra-cluster edge density: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        thePc = (f4)LString_ToF8(theLine);
    } while (thePc < 0.0 || thePc > 1.0);

    do {
        LDebug_Print("Enter inter-cluster edge density: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        thePi = (f4)LString_ToF8(theLine);
    } while (thePi < 0.0 || thePi > 1.0);

    LDebug_Print("Enter minimum weight: ");
    LDebug_GetString(theLine, _LINE_LEN_);
    theMinW = LString_ToUI4(theLine);

    do {
        LDebug_Print("Enter maximum weight: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        theMaxW = LString_ToUI4(theLine);
    } while(theMaxW<theMinW);

    do {
        LDebug_Print("Enter length of the sequence: ");
        LDebug_GetString(theLine, _LINE_LEN_);
        theLength = LString_ToUI4(theLine);
    } while (theLength==0);

    _MakeClusteredTestSet(theN, theC, theSeed, thePc, thePi, theMinW, theMaxW, theLength);
}


/* ----------------------------------------------------------------------------
 *  _MakeClusteredTestSet
 * ----------------------------------------------------------------------------
*/
static void _MakeClusteredTestSet(ui2 inN, ui2 inC, ui4 inSeed, f4 inPc, f4 inPi, 
                                  ui4 inMinW, ui4 inMaxW, ui4 inLength){

    LRandSource*  theSource = NULL;
    LArray*       theIEdges = NULL;
    LGraph_TEdge* theEdge;
    ui4           theI, theCSize;
    _TUpdate      theUpdate;

    if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
    if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
    if (sData.mSequence!=NULL)    LArray_Delete(&sData.mSequence);

    Try {

        /* make graph & weights */
        sData.mGraph = LGraphGen_RandomClustered(inN, inC, inPc, inPi, inSeed, TRUE);
        sData.mEdgeWeights = LGraphGen_RndEdgeInfoUI4(sData.mGraph, inMinW, inMaxW, inSeed);

        /* filter out inter-cluster edges */
        theCSize = inN / inC;
        theIEdges = LArray_New(sizeof(LGraph_TEdge*));
        LGraph_ForAllEdges(sData.mGraph, theEdge) {
            ui2 i, j;
            i = LGraph_GetNodeIndex(LGraph_GetSource(theEdge));
            j = LGraph_GetNodeIndex(LGraph_GetTarget(theEdge));
            if (i / theCSize != j / theCSize)
                LArray_AppendItem(theIEdges, (void*)&theEdge);
        }

        LDebug_Print(">> Generated %u inter-cluster edges\n", LArray_GetItemsCount(theIEdges));

        /* build sequence */
        theSource = LRandSource_New(inSeed);
        sData.mSequence = LArray_New(sizeof(_TUpdate));
        for (theI = 0; theI < inLength; ++theI) {
            ui4 theEdgeIdx = LRandSource_GetRandUI4(theSource, 0, LArray_GetItemsCount(theIEdges)-1);
            theUpdate.mEdge   = *(LGraph_TEdge**)LArray_ItemAt(theIEdges, theEdgeIdx);
            theUpdate.mWeight = LRandSource_GetRandUI4(theSource, inMinW, inMaxW);            
            LArray_AppendItem(sData.mSequence, (void*)&theUpdate);
            if (sData.mVerboseStatus)
                LDebug_Print("Update %u: (%u,%u,%u)\n", theI, 
                    theUpdate.mEdge->mSource->mIndex, theUpdate.mEdge->mTarget->mIndex, 
                    theUpdate.mWeight);
        }

        LString_Format(sData.mGraphFile, "c_%hu_%3.1f_%3.1f", inC, inPc, inPi);
        sData.mGraphSeed = inSeed;
        sData.mMinGraphW = inMinW;
        sData.mMaxGraphW = inMaxW;
        sData.mSeqSeed   = inSeed;
        sData.mMinSeqW   = inMinW;
        sData.mMaxSeqW   = inMaxW;
    }

    CatchAny { /* cleanup */
        if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
        if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
        LDebug_Print("** Couldn't generate test set.\n");
    }

    if (theIEdges != NULL) LArray_Delete(&theIEdges);
    if (theSource != NULL) LRandSource_Delete(&theSource);

    if (sData.mVerboseStatus && sData.mGraph) {
        LGraph_SetDebug(sData.mGraph, TRUE);
        LGraph_Dump(sData.mGraph);
    }

	if (sData.mGraph) _CountIncreaseDecrease();
}


/* ----------------------------------------------------------------------------
 *  _CountIncreaseDecrease
 * ----------------------------------------------------------------------------
*/
static void _CountIncreaseDecrease()
{
	ui4 i;
	LGraph_TEdge* theEdge;
	LEdgeInfo* theCopy = LEdgeInfo_New(sData.mGraph, LType_UI4);
	ui4 theCount = LArray_GetItemsCount(sData.mSequence);

	/* copies all edges weights in theCopy */
	LGraph_ForAllEdges(sData.mGraph, theEdge)
		LEdgeInfo_UI4At(theCopy, theEdge) = LEdgeInfo_UI4At(sData.mEdgeWeights, theEdge);

	sData.mIncCount = sData.mDecCount = 0;
	for(i=0; i < theCount; i++)
	{
		_TUpdate theUpdate = *(_TUpdate*)LArray_ItemAt(sData.mSequence, i);
		ui4 theOldWeight = LEdgeInfo_UI4At(theCopy, theUpdate.mEdge);

		if (theUpdate.mWeight > theOldWeight)
			sData.mIncCount++;
		else
			if (theUpdate.mWeight < theOldWeight)
				sData.mDecCount++;
		LEdgeInfo_UI4At(theCopy, theUpdate.mEdge) = theUpdate.mWeight;
	}
	LEdgeInfo_Delete(&theCopy);
}


/* ----------------------------------------------------------------------------
 *  _AskXMLGraph
 * ----------------------------------------------------------------------------
*/
static void _AskXMLGraph(){
    i1 theLine[LFile_MAX_PATHNAME_LEN];

    #ifndef LEONARDO
    LDebug_Print("Enter pathname: ");
    LDebug_GetString(theLine, LFile_MAX_PATHNAME_LEN);
    #else
    GetFileDialog(theLine);
    LDebug_Print(">> Pathname: %s\n", theLine);
    #endif
    
    _LoadXMLGraph(theLine);
}


/* ----------------------------------------------------------------------------
 *  _AskDIMACSGraph
 * ----------------------------------------------------------------------------
*/
static void _AskDIMACSGraph(){
    i1 theLine[LFile_MAX_PATHNAME_LEN];

    #ifndef LEONARDO
    LDebug_Print("Enter pathname: ");
    LDebug_GetString(theLine, LFile_MAX_PATHNAME_LEN);
    #else
    GetFileDialog(theLine);
    LDebug_Print(">> Pathname: %s\n", theLine);
    #endif
    
    _LoadDIMACSGraph(theLine);
}


/* ----------------------------------------------------------------------------
 *  _AskLeonardoGraph
 * ----------------------------------------------------------------------------
*/
#ifdef LEONARDO
static void _AskLeonardoGraph(){

    ui4            theI, theN, theM;
    LGraph_TNode** theNodes;
    LArray*        theArray;
    Bool           theDir;

    if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
    if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
    if (sData.mSequence!=NULL)    LArray_Delete(&sData.mSequence);

    Try {

        if (!OpenGraph()) Throw(LDebug_INTERNAL_ERROR);
        sData.mGraph       = LGraph_New(LGraph_DIRECTED);
        sData.mEdgeWeights = LEdgeInfo_New(sData.mGraph, LType_UI4);
        theN   = GetNodesCount();
        theM   = GetArcsCount();
        theDir = GetDirected();
        for (theI=0; theI<theN; ++theI) LGraph_NewNode(sData.mGraph);
        theArray = LGraph_GetAllNodes(sData.mGraph);
        LArray_InstallSyncPtr(theArray, (void**)&theNodes);
        for (theI=0; theI<theM; ++theI) {
            i4 theStartIdx, theEndIdx;
            LGraph_TEdge* theEdge;
            GetArc(theI, &theStartIdx, &theEndIdx);
            theEdge = LGraph_NewEdge(sData.mGraph, theNodes[theStartIdx], theNodes[theEndIdx]);
            LEdgeInfo_UI4At(sData.mEdgeWeights, theEdge) = GetArcLabel(theI);
            if (1&&!theDir) {
                theEdge = LGraph_NewEdge(sData.mGraph, theNodes[theEndIdx], theNodes[theStartIdx]);
                LEdgeInfo_UI4At(sData.mEdgeWeights, theEdge) = GetArcLabel(theI);
            }
        }

        CloseGraph();

        LString_Copy("leo", sData.mGraphFile);
        sData.mGraphSeed = 0;
        _FindMinMaxGraphWeight();
    }

    CatchAny {
        if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
        if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
        LDebug_Print("** Couldn't generate graph.\n");
    }

    if (theArray!=NULL) LArray_Delete(&theArray);

    if (sData.mGraph!=NULL && sData.mVerboseStatus) {
        LGraph_SetDebug(sData.mGraph,TRUE);
        LGraph_Dump(sData.mGraph);
    }
}
#endif

/* ----------------------------------------------------------------------------
 *  _AskExpName
 * ----------------------------------------------------------------------------
*/
static void _AskExpName(){
    LDebug_Print("Enter experiment name: ");
    LDebug_GetString(sData.mExpName, LFile_MAX_PATHNAME_LEN);
}


/* ----------------------------------------------------------------------------
 *  _AskLDSPConfig
 * ----------------------------------------------------------------------------
*/
static void _AskLDSPConfig(){
    i1 theLine[_LINE_LEN_];

    LDebug_Print("Enter threshold (T): ");
    LDebug_GetString(theLine, _LINE_LEN_);
    sData.mLDSPConfig.mThreshold = LString_ToF8(theLine);

    LDebug_Print("Enter correction (C): ");
    LDebug_GetString(theLine, _LINE_LEN_);
    sData.mLDSPConfig.mCorrection = LString_ToF8(theLine);

    LDebug_Print("Enter step (S): ");
    LDebug_GetString(theLine, _LINE_LEN_);
    sData.mLDSPConfig.mStep = LString_ToUI4(theLine);

    LDebug_Print("Enter ratio (R): ");
    LDebug_GetString(theLine, _LINE_LEN_);
    sData.mLDSPConfig.mRatio = LString_ToUI4(theLine);
}

/* ----------------------------------------------------------------------------
 *  _AskCDAPSPConfig
 * ----------------------------------------------------------------------------
*/
static void _AskCDAPSPConfig()
{
	i1 theLine[_LINE_LEN_];

	do
	{
		LDebug_Print("Enter alpha[0..1]: ");
		LDebug_GetString(theLine, _LINE_LEN_);
		sData.mCDAPSPConfig.mAlpha = (f4)LString_ToF8(theLine);
	}
	while( (sData.mCDAPSPConfig.mAlpha < 0.0) || (sData.mCDAPSPConfig.mAlpha > 1.0) );

	do
	{
		LDebug_Print("Enter beta[0..1]: ");
		LDebug_GetString(theLine, _LINE_LEN_);
		sData.mCDAPSPConfig.mBeta = (f4)LString_ToF8(theLine);
	}
	while( (sData.mCDAPSPConfig.mBeta < 0.0) || (sData.mCDAPSPConfig.mBeta > 1.0) );
}


/* ----------------------------------------------------------------------------
 *  _LoadXMLGraph
 * ----------------------------------------------------------------------------
*/
static void _LoadXMLGraph(i1 inPathName[LFile_MAX_PATHNAME_LEN]){

    if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
    if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
    if (sData.mSequence!=NULL)    LArray_Delete(&sData.mSequence);

    Try {
	if (!LGraphGen_LoadXML(inPathName, &sData.mGraph, &sData.mEdgeWeights, TRUE))
            Throw(LDebug_INTERNAL_ERROR);
        LString_Copy(inPathName, sData.mGraphFile);
        sData.mGraphSeed = 0;
        _FindMinMaxGraphWeight();
    }

    CatchAny {
        if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
        if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
        LDebug_Print("** Couldn't generate graph.\n");
    }

    if (sData.mGraph!=NULL && sData.mVerboseStatus) {
        LGraph_SetDebug(sData.mGraph,TRUE);
        LGraph_Dump(sData.mGraph);
    }
}


/* ----------------------------------------------------------------------------
 *  _LoadDIMACSGraph
 * ----------------------------------------------------------------------------
*/
static void _LoadDIMACSGraph(i1 inPathName[LFile_MAX_PATHNAME_LEN]){

    if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
    if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
    if (sData.mSequence!=NULL)    LArray_Delete(&sData.mSequence);

    Try {
        if (!LGraphGen_LoadDimacs(inPathName, &sData.mGraph, &sData.mEdgeWeights, TRUE))
            Throw(LDebug_INTERNAL_ERROR);
        LString_Copy(inPathName, sData.mGraphFile);
        sData.mGraphSeed = 0;
        _FindMinMaxGraphWeight();
    }

    CatchAny {
        if (sData.mGraph!=NULL)       LGraph_Delete(&sData.mGraph);
        if (sData.mEdgeWeights!=NULL) LEdgeInfo_Delete(&sData.mEdgeWeights);
        LDebug_Print("** Couldn't generate graph.\n");
    }

    if (sData.mVerboseStatus) {
        LGraph_SetDebug(sData.mGraph,TRUE);
        LGraph_Dump(sData.mGraph);
    }
}


/* ----------------------------------------------------------------------------
 *  _FindMinMaxGraphWeight
 * ----------------------------------------------------------------------------
*/
static void _FindMinMaxGraphWeight(){
    LGraph_TEdge* theEdge;

    if (sData.mGraph == NULL || sData.mEdgeWeights == NULL) return;

    sData.mMinGraphW = LType_MAX_UI4;
    sData.mMaxGraphW = 0;

    LGraph_ForAllEdges(sData.mGraph, theEdge) {
        ui4 theW = LEdgeInfo_UI4At(sData.mEdgeWeights, theEdge);
        if (theW<sData.mMinGraphW) sData.mMinGraphW = theW;
        if (theW>sData.mMaxGraphW) sData.mMaxGraphW = theW;
    }
}


/* ----------------------------------------------------------------------------
 *  _RunCAPSP_C
 * ----------------------------------------------------------------------------
*/
static void _RunCAPSP_C(){

    LException*   theException;
    LEdgeInfo*    theEdgeInfoCopy;

    if (sData.mGraph == NULL || sData.mSequence == NULL) {
        LDebug_Print("** Can't run algorithm without both a graph and a sequence.\n");
        return;
    }

    /* save initial edge weights */
    theEdgeInfoCopy = LGraphGen_CopyEdgeInfo(sData.mGraph, sData.mEdgeWeights);

    LDebug_Print("-- Running CAPSP_C\n");

    Try {

        ui4  theSeqLength = LArray_GetItemsCount(sData.mSequence), i;
        f8   theSequenceTime;
        ui4* theDist;

        /* run sequence of updates */
        _StartTimer();
        for (i=0; i<theSeqLength; ++i) {
            _TUpdate theUpdate;
            theUpdate = *(_TUpdate*)LArray_ItemAt(sData.mSequence, i);
            LEdgeInfo_UI4At(sData.mEdgeWeights, theUpdate.mEdge) = theUpdate.mWeight;
            theDist = CAPSP_C_UI4(sData.mGraph, sData.mEdgeWeights);
            LMemory_Free(&theDist);
			if (i%50==0)
				LDebug_Print("%u\n", i);
			else
				LDebug_Print(".");
        }
        theSequenceTime = _StopTimer();
		LDebug_Print("\n");
        LDebug_Print(">> Total sequence time: %4.3f sec.\n", theSequenceTime);
        LDebug_Print(">> Time per operation: %1.6f sec.\n", theSequenceTime/theSeqLength);
        _LogCAPSP_CResults(theSequenceTime, theSequenceTime/theSeqLength/*, theSequenceMem*/);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't run algorithm\n");
        LException_Dump(theException);
    }

    /* restore initial edge weights */
    LEdgeInfo_Delete(&sData.mEdgeWeights);
    sData.mEdgeWeights = theEdgeInfoCopy;
}


/* ----------------------------------------------------------------------------
 *  _RunLDSP
 * ----------------------------------------------------------------------------
*/
static void _RunLDSP(){
    
    LDSP*         theLDSP   = NULL;
    ui2           theN;
    ui4           i, theSeqLength;
    ui4           theInitMem, theSequenceMem;
    f8	          theInitTime, theSequenceTime;
    LException*   theException;
    LEdgeInfo*    theEdgeInfoCopy;

    if (sData.mGraph == NULL || sData.mSequence == NULL) {
        LDebug_Print("** Can't run algorithm without both a graph and a sequence.\n");
        return;
    }

    /* save initial edge weights */
    theEdgeInfoCopy = LGraphGen_CopyEdgeInfo(sData.mGraph, sData.mEdgeWeights);

    LDebug_Print("-- Running LDSP\n");

    Try {

        theN = (ui2)LGraph_GetNodesCount(sData.mGraph);
        theSeqLength = LArray_GetItemsCount(sData.mSequence);

        /* init LDSP data structure */
        _StartTimer();
        #if 1
        theLDSP = LDSP_New(sData.mGraph, sData.mEdgeWeights);
        LDSP_SetConfig(theLDSP, sData.mLDSPConfig);
        #else
        {
            LGraph_TEdge* theEdge;
            ui4 theCount, theStep;
            theStep = LGraph_GetEdgesCount(sData.mGraph)/10;
            if (theStep<1) theStep = 1;
            theLDSP = LDSP_NewEmpty(theN);
            LDSP_SetConfig(theLDSP, sData.mLDSPConfig);
            theCount = 0;
            LGraph_ForAllEdges(sData.mGraph, theEdge)
                if (theEdge->mSource->mIndex != theEdge->mTarget->mIndex) {
                    LDSP_UpdateEdge(theLDSP, 
                        (ui2)theEdge->mSource->mIndex,
                        (ui2)theEdge->mTarget->mIndex,
                        LEdgeInfo_UI4At(sData.mEdgeWeights, theEdge));
                    if (!(++theCount % theStep)) LDebug_Print("-- Inserting edge # %u\n", theCount);
                }
            }
        #endif
        theInitTime = _StopTimer();
        theInitMem  = LDSP_GetUsedMem(theLDSP);
        LDebug_Print(">> Init time: %3.3f sec.\n", theInitTime);
        LDebug_Print(">> Used memory: %u bytes\n", theInitMem);
        LDSP_Dump(theLDSP);

        if (sData.mCheckerStatus)
        {
            ui4 k,j;
            ui4 N = sData.mGraph->mNodesCount;
            ui4* theDistMatrix;
            
            LDebug_Print("Checking correctness of Initial distances...\n",i);
            theDistMatrix = CAPSP_C_UI4(sData.mGraph, sData.mEdgeWeights);
            for (k=0; k < N; k++)
                for (j=0; j < N; j++)
                {
                    ui4 theDist = LDSP_GetDist(theLDSP, (ui2)k, (ui2)j);
                    if (theDistMatrix[k*N+j] != theDist)
                    {
                        i1 theLine[_LINE_LEN_];
                        LDebug_Print("::Seq[%u] Incorrect distance in [%u][%u], Correct: %u, Actual: %u\n",
                            i,k,j,theDistMatrix[k*N+j],theDist);
                        LDSP_Dump(theLDSP);
                        LDebug_GetString(theLine, _LINE_LEN_);
                    }
                }
            LMemory_Free(&theDistMatrix);
        }

        /* run sequence of updates */
        _StartTimer();
        for (i=0; i<theSeqLength; ++i) {
            _TUpdate theUpdate;
            theUpdate = *(_TUpdate*)LArray_ItemAt(sData.mSequence, i);
            LDSP_UpdateEdge(theLDSP, 
                (ui2)theUpdate.mEdge->mSource->mIndex, 
                (ui2)theUpdate.mEdge->mTarget->mIndex,
                theUpdate.mWeight);

			if (sData.mCheckerStatus)
			{
				ui4 k,j;
				ui4 N = sData.mGraph->mNodesCount;
				ui4* theDistMatrix;
				
				LDebug_Print("Checking correctness of Seq[%u]...\n",i);
				LEdgeInfo_UI4At(sData.mEdgeWeights, theUpdate.mEdge) = theUpdate.mWeight;
				theDistMatrix = CAPSP_C_UI4(sData.mGraph, sData.mEdgeWeights);
				for (k=0; k < N; k++)
					for (j=0; j < N; j++)
					{
						ui4 theDist = LDSP_GetDist(theLDSP, (ui2)k, (ui2)j);
						if (theDistMatrix[k*N+j] != theDist)
						{
							i1 theLine[_LINE_LEN_];
							LDebug_Print("::Seq[%u] Incorrect distance in [%u][%u], Correct: %u, Actual: %u\n",
								i,k,j,theDistMatrix[k*N+j],theDist);
                            LDSP_Dump(theLDSP);
							LDebug_GetString(theLine, _LINE_LEN_);
						}
					}
				LMemory_Free(&theDistMatrix);
			}
			if (i%50==0)
				LDebug_Print("%u\n", i);
			else
				LDebug_Print(".");
        }
        theSequenceTime = _StopTimer();
		LDebug_Print("\n");
        theSequenceMem  = LDSP_GetUsedMem(theLDSP);
        LDebug_Print(">> Total sequence time: %4.3f sec.\n", theSequenceTime);
        LDebug_Print(">> Time per operation: %1.6f sec.\n", theSequenceTime/theSeqLength);
        LDebug_Print(">> Used memory: %u bytes\n", theSequenceMem);
        LDSP_Dump(theLDSP);
        _LogLDSPResults(theInitTime, theInitMem, 
                        theSequenceTime, theSequenceTime/theSeqLength, 
                        theSequenceMem, theLDSP);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't run algorithm\n");
        LException_Dump(theException);
    }

    if (theLDSP != NULL) LDSP_Delete(&theLDSP);

    #if 0
    if (theMatrix != NULL) {
        for (i=0; i<theN; ++i) 
            if (theMatrix[i]) LMemory_Free(&theMatrix[i]);
        LMemory_Free(&theMatrix);
    }
    #endif

    /* restore initial edge weights */
    LEdgeInfo_Delete(&sData.mEdgeWeights);
    sData.mEdgeWeights = theEdgeInfoCopy;
}


/* ----------------------------------------------------------------------------
 *  _RunCDAPSP_DE
 * ----------------------------------------------------------------------------
*/
static void _RunCDAPSP_DE(){

    CDAPSP_DE*    theCDAPSP_DE   = NULL;
    ui4           i, theSeqLength;
    ui4           theInitMem, theSequenceMem;
    f8	          theInitTime, theSequenceTime;
    LException*   theException;
    LEdgeInfo*    theEdgeInfoCopy;

    if (sData.mGraph == NULL || sData.mSequence == NULL) {
        LDebug_Print("** Can't run algorithm without both a graph and a sequence.\n");
        return;
    }

    /* save initial edge weights */
    theEdgeInfoCopy = LGraphGen_CopyEdgeInfo(sData.mGraph, sData.mEdgeWeights);

    LDebug_Print("-- Running CDAPSP_DE\n");

    Try {

        theSeqLength = LArray_GetItemsCount(sData.mSequence);

        /* init CDAPSP_DE data structure */
        _StartTimer();
        theCDAPSP_DE = CDAPSP_DE_New(sData.mGraph, sData.mEdgeWeights);
        theInitTime = _StopTimer();
        theInitMem  = CDAPSP_DE_GetUsedMem(theCDAPSP_DE);
        LDebug_Print(">> Init time: %3.3f sec.\n", theInitTime);
        LDebug_Print(">> Used memory: %u bytes\n", theInitMem);

        /* run sequence of updates */
        _StartTimer();
        for (i=0; i<theSeqLength; ++i) {
            _TUpdate theUpdate    = *(_TUpdate*)LArray_ItemAt(sData.mSequence, i);
            CDAPSP_DE_UpdateEdge(theCDAPSP_DE, theUpdate.mEdge, theUpdate.mWeight);
			if (sData.mCheckerStatus)
			{
				ui4 k,j;
				ui4 N = sData.mGraph->mNodesCount;
				ui4* theDistMatrix = CAPSP_C_UI4(sData.mGraph, sData.mEdgeWeights);
				LArray* theNodesList = LGraph_GetAllNodes(sData.mGraph);
				LGraph_TNode** theNodes;

				LArray_InstallSyncPtr(theNodesList, (void**)&theNodes);
				LDebug_Print("Checking correctness of Seq[%u]...\n",i);
				for (k=0; k < N; k++)
					for (j=0; j < N; j++)
					{
						ui4 theDist = CDAPSP_DE_GetDistance(theCDAPSP_DE, theNodes[k], theNodes[j]);
						if (theDistMatrix[k*N+j] != theDist)
						{
							i1 theLine[_LINE_LEN_];
							LDebug_Print("::Seq[%u] Incorrect distance in [%u][%u], Correct: %u, Actual: %u\n",
								i,k,j,theDistMatrix[k*N+j],theDist);
							LDebug_GetString(theLine, _LINE_LEN_);
						}
					}
				LMemory_Free(&theDistMatrix);
				LArray_Delete(&theNodesList);
			}
			if (i%50==0)
				LDebug_Print("%u\n", i);
			else
				LDebug_Print(".");
        }
        theSequenceTime = _StopTimer();
		LDebug_Print("\n");
        theSequenceMem  = CDAPSP_DE_GetUsedMem(theCDAPSP_DE);
        LDebug_Print(">> Total sequence time: %4.3f sec.\n", theSequenceTime);
        LDebug_Print(">> Time per operation: %1.6f sec.\n", theSequenceTime/theSeqLength);
        LDebug_Print(">> Used memory: %u bytes\n", theSequenceMem);

        _LogCDAPSP_DEResults(theInitTime, theInitMem, 
                             theSequenceTime, theSequenceTime/theSeqLength, theSequenceMem);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't run algorithm\n");
        LException_Dump(theException);
    }

    if (theCDAPSP_DE != NULL) CDAPSP_DE_Delete(&theCDAPSP_DE);

    /* restore initial edge weights */
    LEdgeInfo_Delete(&sData.mEdgeWeights);
    sData.mEdgeWeights = theEdgeInfoCopy;
}


/* ----------------------------------------------------------------------------
 *  _RunCDAPSP
 * ----------------------------------------------------------------------------
*/
static void _RunCDAPSP(){

    CDAPSP*       theCDAPSP = NULL;
    ui4           i, theSeqLength;
    ui4           theInitMem, theSequenceMem;
    f8	          theInitTime, theSequenceTime;
    LException*   theException;
    LEdgeInfo*    theEdgeInfoCopy;

    if (sData.mGraph == NULL || sData.mSequence == NULL) {
        LDebug_Print("** Can't run algorithm without both a graph and a sequence.\n");
        return;
    }

    /* save initial edge weights */
    theEdgeInfoCopy = LGraphGen_CopyEdgeInfo(sData.mGraph, sData.mEdgeWeights);

    LDebug_Print("-- Running CDAPSP\n");

    Try {
	
		CDAPSP_TInfo theInfo;
		f8 theAverage = 0.0f;
		f8 j = 1.0f;
		ui4 theError;
		f8 theAverageError = 0.0f;
		ui4 theErrorsCnt = 0;
		ui4 theInfinityCnt = 0;
		f8 theTotalCnt = 1.0f;

        theSeqLength = LArray_GetItemsCount(sData.mSequence);
		/* init CDAPSP data structure */
        _StartTimer();
		theCDAPSP = CDAPSP_New(sData.mGraph, sData.mEdgeWeights, sData.mMaxSeqW,
			sData.mCDAPSPConfig.mAlpha, sData.mCDAPSPConfig.mBeta);
        theInitTime = _StopTimer();
        theInitMem  = CDAPSP_GetUsedMem(theCDAPSP);
        LDebug_Print(">> Init time: %3.3f sec.\n", theInitTime);
        LDebug_Print(">> Used memory: %u bytes\n", theInitMem);
		theInfo = CDAPSP_GetInfo(theCDAPSP);
		LDebug_Print("[CDAPSP] D = %u, S = %u, BlockersGraphEdges = %u\n", 
			theInfo.mD, theInfo.mBlockersCount, theInfo.mBlockersGraphEdgesCount);


        /* run sequence of updates */
        _StartTimer();
        for (i=0; i<theSeqLength; ++i) {
            _TUpdate theUpdate    = *(_TUpdate*)LArray_ItemAt(sData.mSequence, i);
            CDAPSP_UpdateEdge(theCDAPSP, theUpdate.mEdge, theUpdate.mWeight);
			theAverage += ((f8)theCDAPSP->mBlockGraph->mEdgesCount - theAverage)/j;
			j++;
			if (sData.mCheckerStatus)
			{
				ui4 k,j;
				ui4 N = sData.mGraph->mNodesCount;
				LSP* theLSP=LSP_New(sData.mGraph, sData.mEdgeWeights);
				LArray* theNodesList = LGraph_GetAllNodes(sData.mGraph);
				LGraph_TNode** theNodes;

				LArray_InstallSyncPtr(theNodesList, (void**)&theNodes);
				LDebug_Print("Checking correctness of Seq[%u]...\n",i);
				for (k=0; k < N; k++)
					for (j=0; j < N; j++)
					{
						ui4 theDist = CDAPSP_GetDistance(theCDAPSP, theNodes[k], theNodes[j]);
						if (LSP_GetDist(theLSP, theNodes[k], theNodes[j]) != theDist)
						{
							//i1 theLine[_LINE_LEN_];
							//LDebug_Print("::Seq[%u] Incorrect distance in [%u][%u], Correct: %u, Actual: %u\n",
							//	i,k,j,theDistMatrix[k*N+j],theDist);
							//LDebug_GetString(theLine, _LINE_LEN_);
							if (theDist < LType_MAX_UI4)
							{
								theError = theDist - LSP_GetDist(theLSP, theNodes[k], theNodes[j]);
								theErrorsCnt++;
								theAverageError += ((f8)theError - theAverageError)/(f8)theErrorsCnt;
							}
							else
								theInfinityCnt++;
						}
					}
				LSP_Delete(&theLSP);
				LArray_Delete(&theNodesList);
			}
			if (i%50==0)
				LDebug_Print("%u\n", i);
			else
				LDebug_Print(".");
        }
        theSequenceTime = _StopTimer();
		LDebug_Print("\n");
        theSequenceMem  = CDAPSP_GetUsedMem(theCDAPSP);
        LDebug_Print(">> Total sequence time: %4.3f sec.\n", theSequenceTime);
        LDebug_Print(">> Time per operation: %1.6f sec.\n", theSequenceTime/theSeqLength);
        LDebug_Print(">> Used memory: %u bytes\n", theSequenceMem);
		LDebug_Print(">> Average Blockers Graph Edges: %7.3f\n", theAverage);
        if (sData.mCheckerStatus)
		{
			theTotalCnt = 
				sData.mGraph->mNodesCount*sData.mGraph->mNodesCount*theSeqLength;

			LDebug_Print(">> Average Error: %7.3f\n", theAverageError);
			LDebug_Print(">> Wrong Infinity Distances : %u\n", theInfinityCnt);
			LDebug_Print(">> Error Ratio : %2.2f\n", (f8)theErrorsCnt/theTotalCnt);
		}
        _LogCDAPSPResults(theInitTime, theInitMem, theSequenceTime, 
			theSequenceTime/theSeqLength, theSequenceMem, theAverage, 
			theAverageError, theInfinityCnt, (f8)theErrorsCnt/theTotalCnt);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't run algorithm\n");
        LException_Dump(theException);
    }

    if (theCDAPSP != NULL) CDAPSP_Delete(&theCDAPSP);

    /* restore initial edge weights */
    LEdgeInfo_Delete(&sData.mEdgeWeights);
    sData.mEdgeWeights = theEdgeInfoCopy;
}


/* ----------------------------------------------------------------------------
 *  _RunCDAPSP_D
 * ----------------------------------------------------------------------------
*/
static void _RunCDAPSP_D(){

    CDAPSP_D*     theCDAPSP_D = NULL;
    ui4           i, theSeqLength;
    ui4           theInitMem, theSequenceMem;
    f8	          theInitTime, theSequenceTime;
    LException*   theException;
    LEdgeInfo*    theEdgeInfoCopy;

    if (sData.mGraph == NULL || sData.mSequence == NULL) {
        LDebug_Print("** Can't run algorithm without both a graph and a sequence.\n");
        return;
    }

    /* save initial edge weights */
    theEdgeInfoCopy = LGraphGen_CopyEdgeInfo(sData.mGraph, sData.mEdgeWeights);

    LDebug_Print("-- Running CDAPSP_D\n");

    Try {

        theSeqLength = LArray_GetItemsCount(sData.mSequence);

        /* init CDAPSP_D data structure */
        _StartTimer();
		LDebug_Print("[CDAPSP_D] Max path length is %u\n", sData.mMaxSeqW*(sData.mGraph->mNodesCount-1));
		theCDAPSP_D = CDAPSP_D_New(sData.mGraph, sData.mEdgeWeights, 
			sData.mMaxGraphW*(sData.mGraph->mNodesCount-1));
		
        theInitTime = _StopTimer();
        theInitMem  = CDAPSP_D_GetUsedMem(theCDAPSP_D);
        LDebug_Print(">> Init time: %3.3f sec.\n", theInitTime);
        LDebug_Print(">> Used memory: %u bytes\n", theInitMem);


        /* run sequence of updates */
        _StartTimer();
        for (i=0; i<theSeqLength; ++i) {
            _TUpdate theUpdate    = *(_TUpdate*)LArray_ItemAt(sData.mSequence, i);
            ui4      theOldWeight = LEdgeInfo_UI4At(sData.mEdgeWeights, theUpdate.mEdge);

			/* ******************DEBUG**********************************
			if ( i == 55)
			{
				LGraph_TEdge* theEdge;

				LGraph_SetDebug(sData.mGraph, TRUE);
				LGraph_Dump(sData.mGraph);
				LDebug_Print("::EdgeWeights::\n");
				LGraph_ForAllEdges(sData.mGraph, theEdge)
					LDebug_Print("(%u, %u) W:%u\n", theEdge->mSource->mIndex, theEdge->mTarget->mIndex,
						LEdgeInfo_UI4At(sData.mEdgeWeights, theEdge));
			}
			/* ******************DEBUG********************************* */
			
			LEdgeInfo_UI4At(sData.mEdgeWeights, theUpdate.mEdge) = theUpdate.mWeight;
			if (theUpdate.mWeight == MAIN_INFINITY)
				CDAPSP_D_Increase(theCDAPSP_D, theUpdate.mEdge, MAIN_INFINITY);
			else
			{
				if (theOldWeight > theUpdate.mWeight) 
            		CDAPSP_D_Decrease(theCDAPSP_D, theUpdate.mEdge, theOldWeight - theUpdate.mWeight);
				else if (theOldWeight < theUpdate.mWeight) 
            		CDAPSP_D_Increase(theCDAPSP_D, theUpdate.mEdge, theUpdate.mWeight - theOldWeight);
			}

			if (sData.mCheckerStatus)
			{
				ui4 k,j;
				ui4 N = sData.mGraph->mNodesCount;
				ui4* theDistMatrix = CAPSP_C_UI4(sData.mGraph, sData.mEdgeWeights);
				LArray* theNodesList = LGraph_GetAllNodes(sData.mGraph);
				LGraph_TNode** theNodes;

				LArray_InstallSyncPtr(theNodesList, (void**)&theNodes);
				LDebug_Print("Checking correctness of Seq[%u]...\n",i);
				for (k=0; k < N; k++)
					for (j=0; j < N; j++)
					{
						ui4 theDist = CDAPSP_D_GetDistance(theCDAPSP_D, theNodes[k], theNodes[j]);
						if (theDistMatrix[k*N+j] != theDist)
						{
							i1 theLine[_LINE_LEN_];
							LDebug_Print("::Seq[%u] Incorrect distance in [%u][%u], Correct: %u, Actual: %u\n",
								i,k,j,theDistMatrix[k*N+j],theDist);
							LDebug_GetString(theLine, _LINE_LEN_);
						}
					}
				LMemory_Free(&theDistMatrix);
				LArray_Delete(&theNodesList);
			}

			if (i%50==0)
				LDebug_Print("%u\n", i);
			else
				LDebug_Print(".");
        }
        theSequenceTime = _StopTimer();
		LDebug_Print("\n");
        theSequenceMem  = CDAPSP_D_GetUsedMem(theCDAPSP_D);
        LDebug_Print(">> Total sequence time: %4.3f sec.\n", theSequenceTime);
        LDebug_Print(">> Time per operation: %1.6f sec.\n", theSequenceTime/theSeqLength);
        LDebug_Print(">> Used memory: %u bytes\n", theSequenceMem);

        _LogCDAPSP_DResults(theInitTime, theInitMem, 
                          theSequenceTime, theSequenceTime/theSeqLength, theSequenceMem);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't run algorithm\n");
        LException_Dump(theException);
    }

    if (theCDAPSP_D != NULL) CDAPSP_D_Delete(&theCDAPSP_D);

    /* restore initial edge weights */
    LEdgeInfo_Delete(&sData.mEdgeWeights);
    sData.mEdgeWeights = theEdgeInfoCopy;
}


/* ----------------------------------------------------------------------------
 *  _RunLSP
 * ----------------------------------------------------------------------------
*/
static void _RunLSP(){
    ui4           theSeqLength, i;
    f8            theSequenceTime;
    LSP*          theLSP = NULL;
    LException*   theException;
    LEdgeInfo*    theEdgeInfoCopy;

    if (sData.mGraph == NULL || sData.mSequence == NULL) {
        LDebug_Print("** Can't run algorithm without both a graph and a sequence.\n");
        return;
    }

    /* save initial edge weights */
    theEdgeInfoCopy = LGraphGen_CopyEdgeInfo(sData.mGraph, sData.mEdgeWeights);

    LDebug_Print("-- Running LSP\n");

    Try {

        theSeqLength = LArray_GetItemsCount(sData.mSequence);

        /* run sequence of updates */
        _StartTimer();
        for (i=0; i<theSeqLength; ++i) {
            _TUpdate theUpdate;
            theUpdate = *(_TUpdate*)LArray_ItemAt(sData.mSequence, i);
            LEdgeInfo_UI4At(sData.mEdgeWeights, theUpdate.mEdge) = theUpdate.mWeight;
            theLSP = LSP_New(sData.mGraph, sData.mEdgeWeights);

			if (sData.mCheckerStatus)
			{
				ui4 k,j;
				ui4 N = sData.mGraph->mNodesCount;
				ui4* theDistMatrix = CAPSP_C_UI4(sData.mGraph, sData.mEdgeWeights);
				LArray* theNodesList = LGraph_GetAllNodes(sData.mGraph);
				LGraph_TNode** theNodes;

				LArray_InstallSyncPtr(theNodesList, (void**)&theNodes);
				LDebug_Print("Checking correctness of Seq[%u]...\n",i);
				for (k=0; k < N; k++)
					for (j=0; j < N; j++)
					{
						ui4 theDist = LSP_GetDist(theLSP, theNodes[k], theNodes[j]);
						if (theDistMatrix[k*N+j] != theDist)
						{
							i1 theLine[_LINE_LEN_];
							LDebug_Print("::Seq[%u] Incorrect distance in [%u][%u], Correct: %u, Actual: %u\n",
								i,k,j,theDistMatrix[k*N+j],theDist);
							LDebug_GetString(theLine, _LINE_LEN_);
						}
					}
				LMemory_Free(&theDistMatrix);
				LArray_Delete(&theNodesList);
			}
			if (i%50==0)
				LDebug_Print("%u\n", i);
			else
				LDebug_Print(".");
            if (i==theSeqLength-1) {
                LDebug_Print("\n");
                LSP_Dump(theLSP);
            }
            LSP_Delete(&theLSP);
        }
        theSequenceTime = _StopTimer();
		LDebug_Print("\n");
        LDebug_Print(">> Total sequence time: %4.3f sec.\n", theSequenceTime);
        LDebug_Print(">> Time per operation: %1.6f sec.\n", theSequenceTime/theSeqLength);
        _LogLSPResults(theSequenceTime, theSequenceTime/theSeqLength/*, theSequenceMem*/);
    }

    Catch(theException) {
        if (theLSP != NULL) LSP_Delete(&theLSP);
        LDebug_Print("** Couldn't run algorithm\n");
        LException_Dump(theException);
    }

    /* restore initial edge weights */
    LEdgeInfo_Delete(&sData.mEdgeWeights);
    sData.mEdgeWeights = theEdgeInfoCopy;
}


/* ----------------------------------------------------------------------------
 *  _StartTimer
 * ----------------------------------------------------------------------------
*/
static void _StartTimer(){
    sData.mTime = LTime_GetUserTime();
}


/* ----------------------------------------------------------------------------
 *  _StopTimer
 * ----------------------------------------------------------------------------
*/
static f8 _StopTimer(){
    return LTime_GetUserTime() - sData.mTime;
}


/* ----------------------------------------------------------------------------
 *  _LogCAPSP_CResults
 * ----------------------------------------------------------------------------
*/
static void _LogCAPSP_CResults(f8 inTotUpdTime, f8 inUpdTime /*, ui4 inSeqMem*/){
    LFile*      theLogFile = NULL;
    i1          theLogFileName[LFile_MAX_PATHNAME_LEN];
    i1          theLine[_LINE_LEN_];
    LException* theException;

    Try {
        LString_Copy(sData.mResultsPath, theLogFileName);
        LString_Append("CAPSP_C_",theLogFileName);
        LString_Append(sData.mExpName,theLogFileName);
        LString_Append(".res",theLogFileName);
        if (!LFile_Exists(theLogFileName)) {
            theLogFile = LFile_Open(theLogFileName, LFile_WRITE);
            _LogInputHeader(theLogFile);
            LString_Format(theLine, "%-9s %-9s %-10s\n", 
                "seq-sec", "upd-sec", "seq-mem");
            LFile_WriteString(theLogFile, theLine);
        }
        else {
            theLogFile = LFile_Open(theLogFileName, LFile_READ_WRITE);
            LFile_Seek(theLogFile, 0, LFile_END);
        }

        _LogInputData(theLogFile);
        LString_Format(theLine, "%-9.3f %-9.5f %-10s\n", 
            inTotUpdTime, inUpdTime, "-" /*inSeqMem*/);
        LFile_WriteString(theLogFile, theLine);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't log results.\n");
        LException_Dump(theException);
    }

    if (theLogFile!=NULL) LFile_Close(&theLogFile);
}


/* ----------------------------------------------------------------------------
 *  _LogLDSPResults
 * ----------------------------------------------------------------------------
*/
static void _LogLDSPResults(f8 inInitTime, ui4 inInitMem, 
                            f8 inTotUpdTime, f8 inUpdTime, ui4 inSeqMem, 
                            LDSP* inLDSP) {

    LFile*      theLogFile = NULL;
    i1          theLogFileName[LFile_MAX_PATHNAME_LEN];
    i1          theLine[_LINE_LEN_];
    LException* theException;

    Try {
        LString_Copy(sData.mResultsPath, theLogFileName);
        LString_Append("LDSP_",theLogFileName);
        LString_Append(sData.mExpName,theLogFileName);
        LString_Append(".res",theLogFileName);
        if (!LFile_Exists(theLogFileName)) {
            theLogFile = LFile_Open(theLogFileName, LFile_WRITE);
            _LogInputHeader(theLogFile);
            LString_Format(theLine, "%-9s %-10s %-9s %-9s %-10s %-3s %-3s %-4s %-4s\n", 
                "ini-sec", "ini-mem", "seq-sec", "upd-sec", "seq-mem", "T", "C", "S", "R");
            LFile_WriteString(theLogFile, theLine);
        }
        else {
            theLogFile = LFile_Open(theLogFileName, LFile_READ_WRITE);
            LFile_Seek(theLogFile, 0, LFile_END);
        }

        _LogInputData(theLogFile);
        LString_Format(theLine, "%-9.3f %-10lu %-9.3f %-9.5f %-10lu %-3.1f %-3.1f %-4lu %-4lu\n", 
            inInitTime, inInitMem, inTotUpdTime, inUpdTime, inSeqMem,
            sData.mLDSPConfig.mThreshold, sData.mLDSPConfig.mCorrection, 
            sData.mLDSPConfig.mStep, sData.mLDSPConfig.mRatio);
        LFile_WriteString(theLogFile, theLine);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't log results.\n");
        LException_Dump(theException);
    }

    if (theLogFile!=NULL) LFile_Close(&theLogFile);
}


/* ----------------------------------------------------------------------------
 *  _LogCDAPSP_DEResults
 * ----------------------------------------------------------------------------
*/
static void _LogCDAPSP_DEResults(f8 inInitTime, ui4 inInitMem, 
                                 f8 inTotUpdTime, f8 inUpdTime, ui4 inSeqMem) {

    LFile*      theLogFile = NULL;
    i1          theLogFileName[LFile_MAX_PATHNAME_LEN];
    i1          theLine[_LINE_LEN_];
    LException* theException;

    Try {
        LString_Copy(sData.mResultsPath, theLogFileName);
        LString_Append("CDAPSP_DE_",theLogFileName);
        LString_Append(sData.mExpName,theLogFileName);
        LString_Append(".res",theLogFileName);
        if (!LFile_Exists(theLogFileName)) {
            theLogFile = LFile_Open(theLogFileName, LFile_WRITE);
            _LogInputHeader(theLogFile);
            LString_Format(theLine, "%-9s %-10s %-9s %-9s %-10s\n", 
                "ini-sec", "ini-mem", "seq-sec", "upd-sec", "seq-mem");
            LFile_WriteString(theLogFile, theLine);
        }
        else {
            theLogFile = LFile_Open(theLogFileName, LFile_READ_WRITE);
            LFile_Seek(theLogFile, 0, LFile_END);
        }

        _LogInputData(theLogFile);
        LString_Format(theLine, "%-9.3f %-10lu %-9.3f %-9.5f %-10lu\n", 
            inInitTime, inInitMem, inTotUpdTime, inUpdTime, inSeqMem);
        LFile_WriteString(theLogFile, theLine);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't log results.\n");
        LException_Dump(theException);
    }

    if (theLogFile!=NULL) LFile_Close(&theLogFile);
}


/* ----------------------------------------------------------------------------
 *  _LogCDAPSPResults
 * ----------------------------------------------------------------------------
*/
static void _LogCDAPSPResults(f8 inInitTime, ui4 inInitMem, f8 inTotUpdTime, 
							  f8 inUpdTime, ui4 inSeqMem, f8 inAvgBlcks, f8 inAvgError, 
							  ui4 inInfCnt, f8 inErrorRatio) 
{

    LFile*      theLogFile = NULL;
    i1          theLogFileName[LFile_MAX_PATHNAME_LEN];
    i1          theLine[_LINE_LEN_];
    LException* theException;

    Try {
        LString_Copy(sData.mResultsPath, theLogFileName);
        LString_Append("CDAPSP_",theLogFileName);
        LString_Append(sData.mExpName,theLogFileName);
        LString_Append(".res",theLogFileName);
        if (!LFile_Exists(theLogFileName)) {
            theLogFile = LFile_Open(theLogFileName, LFile_WRITE);
            _LogInputHeader(theLogFile);
            LString_Format(theLine, "%-9s %-10s %-9s %-9s %-10s %-10s %-9s %-6s %-10s %-6s %-6s\n", 
                "ini-sec", "ini-mem", "seq-sec", "upd-sec", "seq-mem", "blck-edges",
				"avg-error", "error%", "wrong-inf", "alpha", "beta");
            LFile_WriteString(theLogFile, theLine);
        }
        else {
            theLogFile = LFile_Open(theLogFileName, LFile_READ_WRITE);
            LFile_Seek(theLogFile, 0, LFile_END);
        }

        _LogInputData(theLogFile);
		if (sData.mCheckerStatus)
			LString_Format(theLine, 
				"%-9.3f %-10lu %-9.3f %-9.5f %-10lu %-10.1f %-9.3f %-6.2f %-10lu %-6.3f %-6.3f\n", 
				inInitTime, inInitMem, inTotUpdTime, inUpdTime, inSeqMem, inAvgBlcks, inAvgError, 
				inErrorRatio, inInfCnt, sData.mCDAPSPConfig.mAlpha, sData.mCDAPSPConfig.mBeta);
		else
			LString_Format(theLine, 
				"%-9.3f %-10lu %-9.3f %-9.5f %-10lu %-10.1f %-9s %-6s %-10s %-6.3f %-6.3f\n", 
				inInitTime, inInitMem, inTotUpdTime, inUpdTime, inSeqMem, inAvgBlcks, "-", 
				"-", "-", sData.mCDAPSPConfig.mAlpha, sData.mCDAPSPConfig.mBeta);

        LFile_WriteString(theLogFile, theLine);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't log results.\n");
        LException_Dump(theException);
    }

    if (theLogFile!=NULL) LFile_Close(&theLogFile);
}


/* ----------------------------------------------------------------------------
 *  _LogCDAPSP_DResults
 * ----------------------------------------------------------------------------
*/
static void _LogCDAPSP_DResults(f8 inInitTime, ui4 inInitMem, 
                              f8 inTotUpdTime, f8 inUpdTime, ui4 inSeqMem) {

    LFile*      theLogFile = NULL;
    i1          theLogFileName[LFile_MAX_PATHNAME_LEN];
    i1          theLine[_LINE_LEN_];
    LException* theException;

    Try {
        LString_Copy(sData.mResultsPath, theLogFileName);
        LString_Append("CDAPSP_D_",theLogFileName);
        LString_Append(sData.mExpName,theLogFileName);
        LString_Append(".res",theLogFileName);
        if (!LFile_Exists(theLogFileName)) {
            theLogFile = LFile_Open(theLogFileName, LFile_WRITE);
            _LogInputHeader(theLogFile);
            LString_Format(theLine, "%-9s %-10s %-9s %-9s %-10s\n", 
                "ini-sec", "ini-mem", "seq-sec", "upd-sec", "seq-mem");
            LFile_WriteString(theLogFile, theLine);
        }
        else {
            theLogFile = LFile_Open(theLogFileName, LFile_READ_WRITE);
            LFile_Seek(theLogFile, 0, LFile_END);
        }

        _LogInputData(theLogFile);
        LString_Format(theLine, "%-9.3f %-10lu %-9.3f %-9.5f %-10lu\n", 
            inInitTime, inInitMem, inTotUpdTime, inUpdTime, inSeqMem);
        LFile_WriteString(theLogFile, theLine);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't log results.\n");
        LException_Dump(theException);
    }

    if (theLogFile!=NULL) LFile_Close(&theLogFile);
}


/* ----------------------------------------------------------------------------
 *  _LogLSPResults
 * ----------------------------------------------------------------------------
*/
static void _LogLSPResults(f8 inTotUpdTime, f8 inUpdTime /*, ui4 inSeqMem*/){
    LFile*      theLogFile = NULL;
    i1          theLogFileName[LFile_MAX_PATHNAME_LEN];
    i1          theLine[_LINE_LEN_];
    LException* theException;

    Try {
        LString_Copy(sData.mResultsPath, theLogFileName);
        LString_Append("LSP_",theLogFileName);
        LString_Append(sData.mExpName,theLogFileName);
        LString_Append(".res",theLogFileName);
        if (!LFile_Exists(theLogFileName)) {
            theLogFile = LFile_Open(theLogFileName, LFile_WRITE);
            _LogInputHeader(theLogFile);
            LString_Format(theLine, "%-9s %-9s %-10s\n", 
                "seq-sec", "upd-sec", "seq-mem");
            LFile_WriteString(theLogFile, theLine);
        }
        else {
            theLogFile = LFile_Open(theLogFileName, LFile_READ_WRITE);
            LFile_Seek(theLogFile, 0, LFile_END);
        }

        _LogInputData(theLogFile);
        LString_Format(theLine, "%-9.3f %-9.5f %-10s\n", 
            inTotUpdTime, inUpdTime, "-" /*inSeqMem*/);
        LFile_WriteString(theLogFile, theLine);
    }

    Catch(theException) {
        LDebug_Print("** Couldn't log results.\n");
        LException_Dump(theException);
    }

    if (theLogFile!=NULL) LFile_Close(&theLogFile);
}


/* ----------------------------------------------------------------------------
 *  _LogInputHeader
 * ----------------------------------------------------------------------------
*/
static void _LogInputHeader(LFile* inLogFile){
    i1 theLine[_LINE_LEN_];
    LString_Format(theLine, "%-13s %-5s %-5s %-7s %-10s %-10s %-7s %-7s %-7s %-5s %-10s %-10s ", 
        "graph", "seed", "n", "m", "wmin", "wmax", "#upd", "#inc", "#dec", "seed", "wmin", "wmax");
    LFile_WriteString(inLogFile, theLine);
}


/* ----------------------------------------------------------------------------
 *  _LogInputData
 * ----------------------------------------------------------------------------
*/
static void _LogInputData(LFile* inLogFile){
    i1 theLine[_LINE_LEN_];
    i1 theGraph[LFile_MAX_PATHNAME_LEN];
    LString_Copy(sData.mGraphFile, theGraph);
	LFile_CutPath(theGraph);
    LString_Format(theLine, "%-13s %-5lu %-5lu %-7lu %-10lu %-10lu %-7lu %-7lu %-7lu %-5lu %-10lu %-10lu ", 
        theGraph, sData.mGraphSeed, LGraph_GetNodesCount(sData.mGraph), LGraph_GetEdgesCount(sData.mGraph),
        sData.mMinGraphW, sData.mMaxGraphW,	LArray_GetItemsCount(sData.mSequence), sData.mIncCount, 
		sData.mDecCount, sData.mSeqSeed, sData.mMinSeqW, sData.mMaxSeqW);
    LFile_WriteString(inLogFile, theLine);
}

