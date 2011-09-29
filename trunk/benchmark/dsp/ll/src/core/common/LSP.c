/* ============================================================================
 *  LSP.c
 * ============================================================================

 *  Author:         (c) 2001-2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 4, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:57 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LSP.h"
#include "LMemory.h"
#include "LException.h"
#include "LDebug.h"

/* SHORTCUTS */
#define _LSP_Pack(a,b)  (((ui4)(a)<<16)|(ui4)(b))
#define _LSP_Unpack1(c) (ui2)((c)>>16)
#define _LSP_Unpack2(c) (ui2)(c)

/* PRIVATE FUNCTION PROTOTYPES */
static Bool _Comparator(ui4 inA, ui4 inB);
static void _Compute(LSP* This);

typedef struct {
    ui2 mX;
    ui2 mY;
} _TPair;

/* GLOBAL VARIABLE (be careful when using multi-threading...) */
static LSP* sThis;

/* PUBLIC FUNCTION DEFINITIONS */

/* ----------------------------------------------------------------------------
 *  New
 * ----------------------------------------------------------------------------
*/
LSP* LSP_New(LGraph* inGraph, LEdgeInfo* inEdgeInfo){

    ui4           theI, theN, theWeight;
    ui2           theX, theY;
    LGraph_TEdge* theEdge;
    LSP           theObject = { 0 };

    theN = LGraph_GetNodesCount(inGraph);

    Try {

        sThis = &theObject;

        /* init object */
        theObject.mMatrixData = (_LSP_TMatrixEntry*) LMemory_Malloc(sizeof(_LSP_TMatrixEntry)*(ui4)theN*theN);
        theObject.mMatrix     = (_LSP_TMatrixEntry**)LMemory_Malloc(sizeof(_LSP_TMatrixEntry*)*theN);
        theObject.mN          = theN;
        theObject.mM          = LGraph_GetEdgesCount(inGraph);
        theObject.mHeap       = LHeap_New(_Comparator);

        #if LSP_STATISTICS == 1
        theObject.mUP       = 0;
        theObject.mHeapDecr = 0;
        #endif

        /* init matrix */
        #if 0
        LMemory_Set(theObject.mMatrixData, 0xFF, sizeof(_LSP_TMatrixEntry)*(ui4)theN*theN);
        #endif
        for (theI = 0; theI < theN; ++theI) {
            theObject.mMatrix[theI] = theObject.mMatrixData + theI*theN;
            LMemory_Set((void*)theObject.mMatrix[theI], 0xFF, sizeof(_LSP_TMatrixEntry)*theN);
            theObject.mMatrix[theI][theI].mDist = 0;
        }

        /* init heap by loading edges */
        LGraph_ForAllEdges(inGraph, theEdge){
            theWeight  = LEdgeInfo_UI4At(inEdgeInfo, theEdge);
            theX = LGraph_GetNodeIndex(LGraph_GetSource(theEdge));
            theY = LGraph_GetNodeIndex(LGraph_GetTarget(theEdge));
            if (theX == theY || theObject.mMatrix[theX][theY].mW != 0xFFFFFFFF) continue;
            theObject.mMatrix[theX][theY].mW    = theWeight;
            theObject.mMatrix[theX][theY].mDist = theWeight;
            theObject.mMatrix[theX][theY].mTB   = _LSP_Pack(theX,theY);
            theObject.mMatrix[theX][theY].mL    = theY;
            theObject.mMatrix[theX][theY].mR    = theX;
            theObject.mMatrix[theX][theY].mHeapIdx = 
                LHeap_Add(theObject.mHeap, NULL, _LSP_Pack(theX,theY));
        }

        /* compute all-pairs shortest paths that use more than one edge */
        _Compute(&theObject);
    }

    CatchAny {
        if (theObject.mMatrixData!=NULL) LMemory_Free(&theObject.mMatrixData); 
        if (theObject.mMatrix!=NULL)     LMemory_Free(&theObject.mMatrix); 
        if (theObject.mHeap!=NULL)       LHeap_Delete(&theObject.mHeap); 
        Rethrow;
    }

    return LMemory_NewObject(LSP,theObject);
}


/* ----------------------------------------------------------------------------
 *  Delete
 * ----------------------------------------------------------------------------
*/
void LSP_Delete(LSP** AThis){
    LMemory_Free(&(*AThis)->mMatrixData); 
    LMemory_Free(&(*AThis)->mMatrix); 
    LHeap_Delete(&(*AThis)->mHeap);
    LMemory_DeleteObject(AThis);
}


/* ----------------------------------------------------------------------------
 *  Dump
 * ----------------------------------------------------------------------------
*/
void LSP_Dump(LSP* This){
    #if LSP_STATISTICS == 1
    LDebug_Print("[LSP] m       = %u\n",This->mM);
    LDebug_Print("[LSP] n       = %u\n",This->mN);
    LDebug_Print("[LSP] m*n     = %u\n",This->mN * This->mM);
    LDebug_Print("[LSP] #up     = %u\n",This->mUP);
    LDebug_Print("[LSP] n^2     = %u\n",This->mN * This->mN);
    LDebug_Print("[LSP] m/n     = %f\n",This->mM/(f4)This->mN);
    LDebug_Print("[LSP] #up/n^2 = %f\n",This->mUP/(f4)(This->mN * This->mN));   
    LDebug_Print("[LSP] #decr   = %u\n",This->mHeapDecr);
    LDebug_Print("\n[LSP] tot mem = %u %s\n",
        LSP_GetUsedMem(This) >> ((LSP_GetUsedMem(This)<=(1<<20))? 0:20), 
        (LSP_GetUsedMem(This)<=(1<<20))? "byte":"MB");
    LDebug_Print("[LSP] heap    = %u %s\n",
        LHeap_GetUsedMem(This->mHeap) >> ((LHeap_GetUsedMem(This->mHeap)<=(1<<20))? 0:20), 
        (LHeap_GetUsedMem(This->mHeap)<=(1<<20))? "byte":"MB");
    #else
    LDebug_Print("[LSP] No statistics available: Recompile with LSP_STATISTICS == 1\n");
    #endif  
}


/* ----------------------------------------------------------------------------
 *  GetUsedMem
 * ----------------------------------------------------------------------------
*/
ui4 LSP_GetUsedMem(LSP* This){
    return sizeof(LSP) + 
           (sizeof(_LSP_TMatrixEntry) * (ui4)This->mN * This->mN) + 
           (sizeof(_LSP_TMatrixEntry*) * This->mN) + 
           LHeap_GetUsedMem(This->mHeap);
}


/* PRIVATE FUNCTIONS */

/* ----------------------------------------------------------------------------
 *  _Compute
 * ----------------------------------------------------------------------------
*/
static void _Compute(LSP* This) {/** Not SceneRebuildOn; **/

    ui4      thePair;
    register ui4 theEDist, theETB;
    register ui2 theX, theY, theXP, theYP, theL, theR;

    while(!LHeap_Empty(This->mHeap)) {

        /* extract path <x,...,y> with minimum weight */
        LHeap_ExtractMin(This->mHeap, NULL, &thePair);
        theX = _LSP_Unpack1(thePair);
        theY = _LSP_Unpack2(thePair);
        This->mMatrix[theX][theY].mHeapIdx = 0xFFFFFFFF;

        /* cache values */
        theR = This->mMatrix[theX][theY].mR;
        theL = This->mMatrix[theX][theY].mL;

        /* add shortest path to lists */
        This->mMatrix[theX][theY].mNextL = This->mMatrix[theL][theY].mFirstL;
        This->mMatrix[theL][theY].mFirstL = theX;
        This->mMatrix[theX][theY].mNextR = This->mMatrix[theX][theR].mFirstR;
        This->mMatrix[theX][theR].mFirstR = theY;

        /* left uniform extension <x',x,...,r,y> */
        for (theXP = This->mMatrix[theX][theR].mFirstL; 
             theXP != 0xFFFF; 
             theXP = This->mMatrix[theXP][theR].mNextL) {

            /* skip cyclic uniform paths */
            if (theXP == theY) continue;

            #if LSP_STATISTICS == 1
            ++This->mUP;
            #endif

            /* explicit common subexpression elimination */
            theEDist = This->mMatrix[theXP][theX].mW + This->mMatrix[theX][theY].mDist;
            theETB   = This->mMatrix[theXP][theX].mTB > This->mMatrix[theX][theY].mTB ?
                       This->mMatrix[theXP][theX].mTB : This->mMatrix[theX][theY].mTB;

            /* relaxation and tie breaking */
            if (theEDist <  This->mMatrix[theXP][theY].mDist ||
                theEDist == This->mMatrix[theXP][theY].mDist && 
                theETB   <  This->mMatrix[theXP][theY].mTB) {

                if (This->mMatrix[theXP][theY].mHeapIdx != 0xFFFFFFFF) {
                    #if LSP_STATISTICS == 1
                    ++This->mHeapDecr;
                    #endif
                    LHeap_Remove(This->mHeap, This->mMatrix[theXP][theY].mHeapIdx);
//                    LHeap_Update(This->mHeap, (void*)_ui4_(&theEPair), 
//                                 (void*)&This->mMatrix[theXP][theY], 
//                                 This->mMatrix[theXP][theY].mHeapIdx);
                }

                This->mMatrix[theXP][theY].mL    = theX;
                This->mMatrix[theXP][theY].mR    = theR;
                This->mMatrix[theXP][theY].mDist = theEDist;
                This->mMatrix[theXP][theY].mTB   = theETB;

//                else {
                    This->mMatrix[theXP][theY].mHeapIdx = 
                        LHeap_Add(This->mHeap, NULL, _LSP_Pack(theXP,theY));
//                }
           }
        }

        /* right uniform extension <x,l,...,y,y'> */
        for (theYP = This->mMatrix[theL][theY].mFirstR; 
             theYP != 0xFFFF; 
             theYP = This->mMatrix[theL][theYP].mNextR) {

            /* skip cyclic uniform paths */
            if (theX == theYP) continue;

            #if LSP_STATISTICS == 1
            ++This->mUP;
            #endif

            /* explicit common subexpression elimination */
            theEDist = This->mMatrix[theX][theY].mDist + This->mMatrix[theY][theYP].mW;
            theETB   = This->mMatrix[theY][theYP].mTB > This->mMatrix[theX][theY].mTB ?
                       This->mMatrix[theY][theYP].mTB : This->mMatrix[theX][theY].mTB;

            /* relaxation and tie breaking */
            if (theEDist <  This->mMatrix[theX][theYP].mDist ||
                theEDist == This->mMatrix[theX][theYP].mDist &&
                theETB   <  This->mMatrix[theX][theYP].mTB) {

                if (This->mMatrix[theX][theYP].mHeapIdx != 0xFFFFFFFF) {
                    #if LSP_STATISTICS == 1
                    ++This->mHeapDecr;
                    #endif
                    LHeap_Remove(This->mHeap, This->mMatrix[theX][theYP].mHeapIdx);
//                    LHeap_Update(This->mHeap, (void*)_ui4_(&theEPair), 
//                                 (void*)&This->mMatrix[theX][theYP], 
//                                 This->mMatrix[theX][theYP].mHeapIdx);
                }

                This->mMatrix[theX][theYP].mR    = theY;
                This->mMatrix[theX][theYP].mL    = theL;
                This->mMatrix[theX][theYP].mDist = theEDist;
                This->mMatrix[theX][theYP].mTB   = theETB;

//                else {
                    This->mMatrix[theX][theYP].mHeapIdx = 
                        LHeap_Add(This->mHeap, NULL, _LSP_Pack(theX,theYP));
//                }
           }
        }
    }
}


/* ----------------------------------------------------------------------------
 *  _Comparator
 * ----------------------------------------------------------------------------
*/
static Bool _Comparator(ui4 inA, ui4 inB) {
    return sThis->mMatrix[_LSP_Unpack1(inA)][_LSP_Unpack2(inA)].mDist <
           sThis->mMatrix[_LSP_Unpack1(inB)][_LSP_Unpack2(inB)].mDist ||
           sThis->mMatrix[_LSP_Unpack1(inA)][_LSP_Unpack2(inA)].mDist ==
           sThis->mMatrix[_LSP_Unpack1(inB)][_LSP_Unpack2(inB)].mDist &&
           sThis->mMatrix[_LSP_Unpack1(inA)][_LSP_Unpack2(inA)].mTB <
           sThis->mMatrix[_LSP_Unpack1(inB)][_LSP_Unpack2(inB)].mTB;
}


/* Copyright (C) 2001-2003 Camil Demetrescu

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

