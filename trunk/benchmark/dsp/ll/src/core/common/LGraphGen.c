/* ============================================================================
 *  LGraphGen.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi, Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        January 12, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:52 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#include "LGraphGen.h"
#include "LException.h"
#include "LRandSource.h"
#include "LEdgeMap.h"
#include "LHash.h"
#include "LMath.h"
#include "LFile.h"
#include "LString.h"
#include "LMemory.h"
#include "LEdgeMap.h"
#include "_LGraph.h"
#include <time.h>


/* INTERNAL DEFINES */
#define _LGraphGen_BUF_LEN  2048
#define _LGraphGen_TLEN     256


/* ---------------------------------------------------------------------------------
 *  RandomNM
 * ---------------------------------------------------------------------------------
 * Returns a random directed graph with inN nodes and inM edges */

LGraph* LGraphGen_RandomNM(ui2 inN, ui4 inM, ui4 inSeed)
{
	ui4			   i;
	ui4			   theRand;
	ui4			   theCount;
	ui4			   theSource;
	ui4			   theTarget;
	ui4			   theValue;
	ui4			   theRow;
	ui4			   theCol;
	LHash**		   theHashList   = NULL;
	LGraph*		   theGraph      = NULL;
	LGraph_TNode** theNodesList  = NULL;
	LRandSource*   theRandSource = NULL;
    
	theCount = (ui4)inN*inN;
	
	Try
	{
		theGraph      = LGraph_New(LGraph_DIRECTED);
		theNodesList  = (LGraph_TNode**)LMemory_Malloc(inN*sizeof(LGraph_TNode*));
		theHashList   = (LHash**)LMemory_Malloc(inN*sizeof(LHash*));
		theRandSource = LRandSource_New(inSeed);
	}
	CatchAny
	{/* is something goes wrong, cleanup */
		if (theNodesList) LMemory_Free(&theNodesList);
		if (theHashList) LMemory_Free(&theHashList);
		if (theGraph) LGraph_Delete(&theGraph);
		if (theRandSource) LRandSource_Delete(&theRandSource);
		Rethrow;
	}	

	/* inM cannot be higher than inN*inN */
	if (inM > theCount) inM = theCount;

	/* builds up nodes */
	for (i=0; i < inN; i++)
	{
		theNodesList[i] = LGraph_NewNode(theGraph);
		theHashList[i]  = LHash_New();
	}
		
	theCount--; /* C arrays start from 0 */
	/* builds up edges */
	for (i=0; i < inM; i++)
	{
		theRand = LRandSource_GetRandUI4(theRandSource, 0, theCount);
		theSource = theRand / inN;
		theTarget = theRand % inN;

		if ( LHash_IsInTable(theHashList[theSource], theTarget) )
		{
			theRand = (ui4)LHash_GetItemByKey(theHashList[theSource], theTarget);
			LHash_RemoveItem(theHashList[theSource], theTarget);
		}
		
		/* checks if the location corresponding to theCount has been used */
		theRow = theCount / inN;
		theCol = theCount % inN;
		if ( LHash_IsInTable(theHashList[theRow], theCol) )
		{
			theValue = (ui4)LHash_GetItemByKey(theHashList[theRow], theCol);
			LHash_InsertItem(theHashList[theSource], (void*)theValue, theTarget);
		}
		else
            LHash_InsertItem(theHashList[theSource], (void*)theCount, theTarget);

		theSource = theRand / inN;
		theTarget = theRand % inN;
		LGraph_NewEdge(theGraph, theNodesList[theSource], theNodesList[theTarget]);	
		theCount--;	
	}
	for (i=0; i < inN; i++)
		LHash_Delete(&theHashList[i]);
	LMemory_Free(&theHashList);
	LMemory_Free(&theNodesList);
	LRandSource_Delete(&theRandSource);
	return theGraph;
}


/* ---------------------------------------------------------------------------------
 *  RandomUNM
 * ---------------------------------------------------------------------------------
 * Returns a random undirected graph with inN nodes and inM edges */

LGraph* LGraphGen_RandomUNM(ui2 inN, ui4 inM, ui4 inSeed)
{
	ui4			   i;
	ui4			   theRand;
	ui4			   theCount;
	ui4			   theSource;
	ui4			   theTarget;
	ui4			   theValue;
	ui4			   theRow;
	ui4			   theCol;
	LHash**		   theHashList   = NULL;
	LGraph*		   theGraph      = NULL;
	LGraph_TNode** theNodesList  = NULL;
	LRandSource*   theRandSource = NULL;
    
	theCount = ((inN*((ui4)inN-1))/2) + inN;
	
	Try
	{
		theGraph      = LGraph_New(LGraph_UNDIRECTED);
		theNodesList  = (LGraph_TNode**)LMemory_Malloc(inN*sizeof(LGraph_TNode*));
		theHashList   = (LHash**)LMemory_Malloc(inN*sizeof(LHash*));
		theRandSource = LRandSource_New(inSeed);
	}
	CatchAny
	{/* is something goes wrong, cleanup */
		if (theNodesList) LMemory_Free(&theNodesList);
		if (theHashList) LMemory_Free(&theHashList);
		if (theGraph) LGraph_Delete(&theGraph);
		if (theRandSource) LRandSource_Delete(&theRandSource);
		Rethrow;
	}	

	/* inM cannot be higher than ((inN*(inN-1))/2) + inN */
	if (inM > theCount) inM = theCount;

	/* builds up nodes */
	for (i=0; i < inN; i++)
	{
		theNodesList[i] = LGraph_NewNode(theGraph);
		theHashList[i]  = LHash_New();
	}
		
	theCount--; /* C arrays start from 0 */
	/* builds up edges */
	for (i=0; i < inM; i++)
	{
		theRand = LRandSource_GetRandUI4(theRandSource, 0, theCount);
		theSource = (ui4)LMath_Floor(((2*inN+1)-LMath_Sqrt((2*inN+1)*(2*inN+1)-8*theRand))/2);
		theTarget = theRand - theSource*(2*inN-1-theSource)/2;
		if ( LHash_IsInTable(theHashList[theSource], theTarget) )
		{
			theRand = (ui4)LHash_GetItemByKey(theHashList[theSource], theTarget);
			LHash_RemoveItem(theHashList[theSource], theTarget);
		}
		
		/* checks if the location corresponding to theCount has been used */
		theRow = (ui4)LMath_Floor(((2*inN+1)-LMath_Sqrt((2*inN+1)*(2*inN+1)-8*theCount))/2);
		theCol = theCount - theRow*(2*inN-1-theRow)/2;
		if ( LHash_IsInTable(theHashList[theRow], theCol) )
		{
			theValue = (ui4)LHash_GetItemByKey(theHashList[theRow], theCol);
			LHash_InsertItem(theHashList[theSource], (void*)theValue, theTarget);
		}
		else
            LHash_InsertItem(theHashList[theSource], (void*)theCount, theTarget);

		theSource = (ui4)LMath_Floor(((2*inN+1)-LMath_Sqrt((2*inN+1)*(2*inN+1)-8*theRand))/2);
		theTarget = theRand - theSource*(2*inN-1-theSource)/2;
		LGraph_NewEdge(theGraph, theNodesList[theSource], theNodesList[theTarget]);	
		theCount--;	
	}
	for (i=0; i < inN; i++)
		LHash_Delete(&theHashList[i]);
	LMemory_Free(&theHashList);
	LMemory_Free(&theNodesList);
	LRandSource_Delete(&theRandSource);
	return theGraph;
}


/* ---------------------------------------------------------------------------------
 *  LGraphGen_RandomNP
 * ---------------------------------------------------------------------------------
 * Returns a random graph with inN nodes and approx inP*inN^2 edges*/

LGraph* LGraphGen_RandomNP(ui2 inN, f4 inP, ui4 inSeed)
{
	ui4			   i;
	ui4			   j;
	f4			   theRand;
	LException*    theEx;
	LGraph*		   theGraph      = NULL;
	LGraph_TNode** theNodesList  = NULL;
	ui4			   theSeed       = inSeed;
	LRandSource*   theRandSource = NULL;
    
	Try
	{
		theGraph      = LGraph_New(LGraph_DIRECTED);
		theNodesList  = (LGraph_TNode**)LMemory_Calloc(inN*sizeof(LGraph_TNode*));
		theRandSource = LRandSource_New(theSeed);
	}
	Catch(theEx)
	{/* is something goes wrong, cleanup */
		if (theNodesList) LMemory_Free(&theNodesList);
		if (theGraph) LGraph_Delete(&theGraph);
		if (theRandSource) LRandSource_Delete(&theRandSource);
		Rethrow;
	}

	/* builds up nodes */
	for (i=0; i < inN; i++)
		theNodesList[i] = LGraph_NewNode(theGraph);

	for (i=0; i < inN; i++)
		for (j=0; j < inN; j++)
		{
			theRand = (f4)LRandSource_GetRandUI4(theRandSource, 0, LType_MAX_UI4)/LType_MAX_UI4;
			if (theRand < inP) 
				LGraph_NewEdge(theGraph, theNodesList[i], theNodesList[j]);
		}
    LMemory_Free(&theNodesList);
	LRandSource_Delete(&theRandSource);
	return theGraph;
}


/* ---------------------------------------------------------------------------------
 *  LGraphGen_RandomUNP
 * ---------------------------------------------------------------------------------
 * Returns a random graph with inN nodes and approx inP*(inN(inN-1)/2 + inN) edges */

LGraph* LGraphGen_RandomUNP(ui2 inN, f4 inP, ui4 inSeed)
{
	ui4			   i;
	ui4			   theRand;
	f4			   theProb;
	ui4			   theCount;
	ui4			   theEdgesCount;
	ui4			   theSource;
	ui4			   theTarget;
	ui4			   theRow;
	ui4			   theCol;
	ui4            theValue;
	ui4			   theSeed       = inSeed;
	LGraph*		   theGraph      = NULL;
	LHash**		   theHashList   = NULL;
	LGraph_TNode** theNodesList  = NULL;
	LRandSource*   theRandSource = NULL;

	theEdgesCount = theCount = ((inN*(inN-1))/2) + inN;
	
	Try
	{
		theGraph      = LGraph_New(LGraph_UNDIRECTED);
		theNodesList  = (LGraph_TNode**)LMemory_Malloc(inN*sizeof(LGraph_TNode*));
		theHashList   = (LHash**)LMemory_Malloc(inN*sizeof(LHash*));
		theRandSource = LRandSource_New(theSeed);
	}
	CatchAny
	{/* is something goes wrong, cleanup */
		if (theNodesList) LMemory_Free(&theNodesList);
		if (theGraph) LGraph_Delete(&theGraph);
		if (theHashList) LMemory_Free(&theHashList);
		if (theRandSource) LRandSource_Delete(&theRandSource);
		Rethrow;
	}	

	/* builds up nodes */
	for (i=0; i < inN; i++)
	{
		theNodesList[i] = LGraph_NewNode(theGraph);
		theHashList[i]  = LHash_New();
	}
	
	theCount--; /* C arrays start from 0 */
	/* builds up edges */
	for (i=0; i < theEdgesCount; i++)
	{
		theProb = (f4)LRandSource_GetRandF8(theRandSource);
        /* CD030529 theProb = (f4)LRandSource_GetRandUI4(theRandSource, 0, LType_MAX_UI4)/LType_MAX_UI4; */
		if (theProb <= inP)
		{
			theRand = LRandSource_GetRandUI4(theRandSource, 0, theCount);
			theSource = theRand / inN;
			theTarget = theRand % inN;

			if ( LHash_IsInTable(theHashList[theSource], theTarget) )
			{
				theRand = (ui4)LHash_GetItemByKey(theHashList[theSource], theTarget);
				LHash_RemoveItem(theHashList[theSource], theTarget);
			}
		
			/* checks if the location corresponding to theCount has been used */
			theRow = theCount / inN;
			theCol = theCount % inN;
			if ( LHash_IsInTable(theHashList[theRow], theCol) )
			{
				theValue = (ui4)LHash_GetItemByKey(theHashList[theRow], theCol);
				LHash_InsertItem(theHashList[theSource], (void*)theValue, theTarget);
			}
			else
				LHash_InsertItem(theHashList[theSource], (void*)theCount, theTarget);

			theSource = theRand / inN;
			theTarget = theRand % inN;
			LGraph_NewEdge(theGraph, theNodesList[theSource], theNodesList[theTarget]);	
			theCount--;	
		}
	}

	for (i=0; i < inN; i++)
		LHash_Delete(&theHashList[i]);
	LMemory_Free(&theHashList);
	LMemory_Free(&theNodesList);
	LRandSource_Delete(&theRandSource);
	return theGraph;
}


/* ---------------------------------------------------------------------------------
 *  Grid
 * ---------------------------------------------------------------------------------
 * Returns a graph which represents a n * n grid*/

LGraph* LGraphGen_Grid(ui2 inN, Bool inDirected)
{
	ui2 x;
	ui2 y;
	ui4 theCnt = inN*inN;
	LException* theEx;
	LGraph_TNode** theMatrix;
	LGraph* theGraph = NULL;
  
	Try
	{		
		theGraph = LGraph_New(inDirected);
		theMatrix = (LGraph_TNode**)LMemory_Calloc(theCnt*sizeof(LGraph_TNode*));
	}
		
    Catch(theEx)
	{/* if something goes wrong, do cleanup */
		if (theGraph) LGraph_Delete(&theGraph);
		LMemory_Free(&theMatrix);
		Rethrow;
	}

	for(x=0; x<theCnt; x++)
		theMatrix[x] = LGraph_NewNode(theGraph);
       
	for(x=0; x<inN; x++)
		for(y=0; y<inN; y++)
		{ 
			if (x < inN-1) LGraph_NewEdge(theGraph, theMatrix[x*inN+y], theMatrix[(x+1)*inN+y]);
			if (y < inN-1) LGraph_NewEdge(theGraph, theMatrix[x*inN+y], theMatrix[x*inN+(y+1)]);
		}
	LMemory_Free(&theMatrix);
	return theGraph;
}


/* ---------------------------------------------------------------------------------
 *  RandomClustered
 *  ---------------------------------------------------------------------------------
 *  Returns a random graph with inN nodes and inC strongly connected clusters. 
 *  There is an edge in the same cluster with probability inPc, and there
 *  is an edge between different clusters with probability inPi. To make sure that
 *  clusters are strongly connected, a cycle through all nodes in the cluster
 *  is added.  */

LGraph* LGraphGen_RandomClustered(ui2 inN, ui2 inC, f4 inPc, f4 inPi, ui4 inSeed, Bool inDirected) {

    f4             theP;
    ui4            i, j, theCSize;
    LGraph*        theGraph  = NULL;
    LArray*        theArray  = NULL;
    LRandSource*   theSource = NULL;
    LGraph_TNode** theNodes;

    if (inC == 0)  inC = 1;
    if (inC > inN) inC = inN;
    theCSize = inN / inC;

    Try {
        theGraph  = LGraph_New(inDirected);
        theSource = LRandSource_New(inSeed);

        for (i = 0; i < inN; ++i) LGraph_NewNode(theGraph);
        theArray = LGraph_GetAllNodes(theGraph);
        LArray_InstallSyncPtr(theArray, (void**)&theNodes);

        for (i = 0; i < inN; ++i)
            for (j = 0; j < inN; ++j) {

                /* avoid self-loops */
                if (i == j) continue;

                /* if nodes belong to the same cluster */
                if (i / theCSize == j / theCSize)
                    /* add cycle through nodes in the cluster */
                    if ((i+1) % theCSize == j % theCSize) theP = 1.0; 
                    else theP = inPc;
                else theP = inPi;

                /* add edge (i,j) with probability theP */
                if (LRandSource_GetRandF8(theSource) <= theP) 
                    LGraph_NewEdge(theGraph, theNodes[i], theNodes[j]);
            }
    }

    CatchAny {
        if (theGraph != NULL)  LGraph_Delete(&theGraph);
        if (theArray != NULL)  LArray_Delete(&theArray);
        if (theSource != NULL) LRandSource_Delete(&theSource);
        Rethrow;
    }

    if (theArray != NULL)  LArray_Delete(&theArray);
    if (theSource != NULL) LRandSource_Delete(&theSource);

    return theGraph;
}


/* ---------------------------------------------------------------------------------
 *  RndEdgeInfoUI4
 *  ---------------------------------------------------------------------------------
 *  Returns a LEdgeInfo associated to inGraph with random weights from inA to inB  */
LEdgeInfo* LGraphGen_RndEdgeInfoUI4(LGraph* inGraph, ui4 inA, ui4 inB, ui4 inSeed)
{
	ui4			  i;
	ui4			  theCount;
	ui4			  theRnd;
	LGraph_TEdge* theEdge;
	LArray*       theEdgesList = NULL;
	LRandSource*  theRndSrc    = NULL;
	LEdgeInfo*	  theEdgeInfo  = NULL;

	Try
	{
		theRndSrc    = LRandSource_New(inSeed);
		theEdgeInfo	 = LEdgeInfo_New(inGraph, LType_UI4);
		theEdgesList = LGraph_GetAllEdges(inGraph);
	}
	CatchAny
	{/* cleanup */
		if (theRndSrc) LRandSource_Delete(&theRndSrc);
		if (theEdgeInfo) LEdgeInfo_Delete(&theEdgeInfo);
		if (theEdgesList) LArray_Delete(&theEdgesList);
		Rethrow;
	}
	
	theCount = LArray_GetItemsCount(theEdgesList);
	for (i=0; i < theCount; i++)
	{
		theRnd = LRandSource_GetRandUI4(theRndSrc, inA, inB);
		theEdge = *(LGraph_TEdge**) LArray_ItemAt(theEdgesList, i);
		LEdgeInfo_UI4At(theEdgeInfo, theEdge) = theRnd;
	}
	
	LRandSource_Delete(&theRndSrc);
	LArray_Delete(&theEdgesList);
	return theEdgeInfo;	
}


/* ---------------------------------------------------------------------------------
 *  AddCycle
 *  ---------------------------------------------------------------------------------
 *  Adds the required edges to form a cycle through all nodes in the passed graph. 
 *  No multi-edges are added to the graph. */

void LGraphGen_AddCycle(LGraph** thruGraph)
{
	ui4           i;
	ui4           theCount;
	LGraph_TNode* theSource;
	LGraph_TNode* theTarget;
	LEdgeMap*     theMap      = LEdgeMap_New(*thruGraph);
	LArray*       theNodeList = LGraph_GetAllNodes(*thruGraph);

	theCount = LArray_GetItemsCount(theNodeList);

	for (i=0; i < theCount-1; i++)
	{
		theSource = *(LGraph_TNode**) LArray_ItemAt(theNodeList, i);
		theTarget = *(LGraph_TNode**) LArray_ItemAt(theNodeList, i+1);

		if (!LEdgeMap_EdgeExists(theMap, theSource, theTarget))
			LGraph_NewEdge(*thruGraph, theSource, theTarget);

	}
	/* adds the last edge */
	theSource = *(LGraph_TNode**) LArray_ItemAt(theNodeList, i);
	theTarget = *(LGraph_TNode**) LArray_ItemAt(theNodeList, 0);

	if (!LEdgeMap_EdgeExists(theMap, theSource, theTarget))
		LGraph_NewEdge(*thruGraph, theSource, theTarget);

	LArray_Delete(&theNodeList);
	LEdgeMap_Delete(&theMap);
}


/* ---------------------------------------------------------------------------------
 *  LoadDimacs
 *  ---------------------------------------------------------------------------------
 *  Loads the graph and the edgeinfo from the dimacs format */
Bool LGraphGen_LoadDimacs(i1* inPathName, LGraph** outGraph, 
						  LEdgeInfo** outEdgeInfo, Bool inDirected)
{
	LFile* theFile;
	i1  theBuf[_LGraphGen_BUF_LEN];
	i1* theToken;
	ui4 theNodesNumber;
	LEdgeMap* theEdgeMap;
	Bool theGraphHasBeenCreated = FALSE;
	LGraph_TNode** theNodesList = NULL;
	LGraph_TNode*  theSrc;
	LGraph_TNode*  theDst;
	LGraph_TEdge*  theEdge;

	Try
		theFile = LFile_Open(inPathName, LFile_READ);
	CatchAny
		return FALSE;
	theEdgeMap     = NULL;
	(*outGraph)    = NULL;
	(*outEdgeInfo) = NULL;
	while (LFile_ReadString(theFile, theBuf, _LGraphGen_BUF_LEN, "\n\r"))
	{
		ui4 i,j;
		i1 theTokenList[_LGraphGen_TLEN][_LGraphGen_TLEN];

		i=0;
		theToken = LString_Tokenizer(theBuf, " \n\r");        /* reads first char */
		if (theToken)
		{
			LString_Copy(theToken, theTokenList[i]);
			i++;
			while(theToken = LString_Tokenizer(NULL, " \n\r"))
			{
				LString_Copy(theToken, theTokenList[i]);
				i++;
			}
			for (j=0; j<i; j++)
			{
				if (!LString_Compare(theTokenList[j], "c"))
					break;

				if (!LString_Compare(theTokenList[j], "n"))
				{
					ui4 k;
					if (theGraphHasBeenCreated) 
					{/* the graph has already been created, bad input file */
						/* do cleanup and return FALSE						   */

						if (*outGraph) LGraph_Delete(outGraph);
						if (*outEdgeInfo) LEdgeInfo_Delete(outEdgeInfo);
						if (theNodesList) LMemory_Free(&theNodesList);
						return FALSE;
					}
					theGraphHasBeenCreated = TRUE;
					theToken = theTokenList[j+1];/* reads nodes number */
					theNodesNumber = LString_ToUI4(theToken);
					*outGraph = LGraph_New(inDirected);
					theEdgeMap = LEdgeMap_New(*outGraph);
					*outEdgeInfo = LEdgeInfo_New(*outGraph, LType_UI4);
					theNodesList = (LGraph_TNode**)
									LMemory_Malloc(theNodesNumber*sizeof(LGraph_TNode*));
					for (k=0; k < theNodesNumber; k++)
						theNodesList[k] = LGraph_NewNode(*outGraph);
					break;
				}

				if (!LString_Compare(theTokenList[j], "a"))
				{	
					ui4 k;

					
					theToken = theTokenList[j+1];/* reads src node */
					k = LString_ToUI4(theToken);
					if (k < theNodesNumber)
						theSrc = theNodesList[k];
					else
						break;
					theToken = theTokenList[j+2];/* reads dst node */
					k = LString_ToUI4(theToken);
					if (k < theNodesNumber)
						theDst = theNodesList[k];
					else
						break;
					if (!LEdgeMap_EdgeExists(theEdgeMap, theSrc, theDst))
					{
						theEdge = LGraph_NewEdge(*outGraph, theSrc, theDst);
						theToken = theTokenList[j+3];/* reads edge weight */
						LEdgeInfo_UI4At(*outEdgeInfo, theEdge) = LString_ToUI4(theToken);
					}
					break;
				}
			}
		}
		
	}
	if (theNodesList) LMemory_Free(&theNodesList);
	LEdgeMap_Delete(&theEdgeMap);
	LFile_Close(&theFile);
	return TRUE;
}


/* ---------------------------------------------------------------------------------
 *  SaveDimacs
 *  ---------------------------------------------------------------------------------
 *  Saves the graph and the edgeinfo in the dimacs format */

Bool LGraphGen_SaveDimacs(i1* inPathName, LGraph* inGraph, LEdgeInfo* inEdgeInfo)
{
	LFile* theFile;
	ui4 theNodesCount;
	i1  theBuf[_LGraphGen_BUF_LEN];
	LGraph_TEdge* theEdge;

	Try
		theFile = LFile_Open(inPathName, LFile_WRITE);
	CatchAny
		return FALSE;
	LFile_WriteString(theFile, "c LGraph2Dimacs utility (c)2003 Stefano Emiliozzi");
	LFile_WriteString(theFile, "\n");
	LFile_WriteString(theFile, "c this is part of Leonardo Library www.leonardo-vm.org\n");
	theNodesCount = LGraph_GetNodesCount(inGraph);
	LString_Format(theBuf, "n %u\n", theNodesCount);
	LFile_WriteString(theFile, theBuf);
	LGraph_ForAllEdges(inGraph, theEdge)
	{
		if (inEdgeInfo)
			LString_Format(theBuf, "a %u %u %u\n", theEdge->mSource->mIndex, 
				           theEdge->mTarget->mIndex, 
						   LEdgeInfo_UI4At(inEdgeInfo, theEdge));
		else
			LString_Format(theBuf, "a %u %u\n", theEdge->mSource->mIndex, 
				           theEdge->mTarget->mIndex);

		LFile_WriteString(theFile, theBuf);
	}
	LFile_Close(&theFile);
	return TRUE;
}


/* ---------------------------------------------------------------------------------
 *  LoadXML
 *  ---------------------------------------------------------------------------------
 *  Loads the graph and the edgeinfo from the dimacs format */
Bool LGraphGen_LoadXML(i1* inPathName, LGraph** outGraph, 
					   LEdgeInfo** outEdgeInfo, Bool inDirected)
{
	LFile* theFile;
	i1  theBuf[_LGraphGen_BUF_LEN];
	i1* theToken;
	ui4 theNodesCount, theWeight;
	LEdgeMap*      theEdgeMap   = NULL;
	LGraph_TNode** theNodesList = NULL;
	LGraph_TNode*  theSrc;
	LGraph_TNode*  theDst;
	LGraph_TEdge*  theEdge;

	Try
		theFile = LFile_Open(inPathName, LFile_READ);
	CatchAny
		return FALSE;
	(*outGraph) = NULL;
	while (LFile_ReadString(theFile, theBuf, _LGraphGen_BUF_LEN, "\n\r"))
	{
		theToken = LString_Tokenizer(theBuf, "\t ");/* reads first token */
		if (theToken)
		{
			if (!LString_Compare(theToken, "<GRAPH"))
			{
				i1 theTokenList[_LGraphGen_TLEN][_LGraphGen_TLEN];
				ui1 i = 0;
				ui1 j;

				while (theToken = LString_Tokenizer(NULL, " =\""))
				{/* copies all the token */
					LString_Copy(theToken, theTokenList[i]);
					i++;
				}
				for (j=0; j < i; j++)
					if (!LString_Compare(theTokenList[j], "nodes"))
					{/* now we get the nodesCount */
						ui4 k;

						theNodesCount = LString_ToUI4(theTokenList[j+1]);
						*outGraph = LGraph_New(inDirected);
						theEdgeMap = LEdgeMap_New(*outGraph);
						theNodesList = (LGraph_TNode**)
							LMemory_Malloc(theNodesCount*sizeof(LGraph_TNode*));
						for (k=0; k < theNodesCount; k++)
							theNodesList[k] = LGraph_NewNode(*outGraph);
						if (outEdgeInfo) 
							*outEdgeInfo = LEdgeInfo_New(*outGraph, LType_UI4);
						break;
					}
			}
			else
			if (!LString_Compare(theToken, "<EDGE"))
			{
				i1 theTokenList[_LGraphGen_TLEN][_LGraphGen_TLEN];
				ui1 i = 0;
				ui1 j;

				while (theToken = LString_Tokenizer(NULL, " =\""))
				{/* copies all the tokens */
					LString_Copy(theToken, theTokenList[i]);
					i++;
				}
				for (j=0; j < i; j++)
				{
					if (!LString_Compare(theTokenList[j], "source"))
						theSrc = theNodesList[LString_ToUI4(theTokenList[j+1])];
					else
					if (!LString_Compare(theTokenList[j], "dest"))
						theDst = theNodesList[LString_ToUI4(theTokenList[j+1])];
					else
					if (!LString_Compare(theTokenList[j], "weight"))
					{
						theWeight = LString_ToUI4(theTokenList[j+1]);
						break;
					}
				}
				/* avoid to create a multigraph */
				if (!LEdgeMap_EdgeExists(theEdgeMap, theSrc, theDst))
				{
					theEdge = LGraph_NewEdge(*outGraph, theSrc, theDst);
					LEdgeInfo_UI4At(*outEdgeInfo, theEdge) = theWeight;
				}
			}
		}
	}
	LMemory_Free(&theNodesList);
	LEdgeMap_Delete(&theEdgeMap);
	LFile_Close(&theFile);
	return TRUE;
}


/* ---------------------------------------------------------------------------------
 *  SaveXML
 *  ---------------------------------------------------------------------------------
 *  Saves the graph and the edgeinfo in the XML format */
Bool LGraphGen_SaveXML(i1* inPathName, LGraph* inGraph, LEdgeInfo* inEdgeInfo)
{
	LFile* theFile;
	ui4 theNodesCount, theEdgesCount;
	i1  theBuf[_LGraphGen_BUF_LEN];
	LGraph_TEdge* theEdge;

	Try
		theFile = LFile_Open(inPathName, LFile_WRITE);
	CatchAny
		return FALSE;

	LFile_WriteString(theFile, "<?xml version=\"1.0\"?>\n");
	LFile_WriteString(theFile, "<!DOCTYPE GRAPH [\n");
	LFile_WriteString(theFile, "<!ELEMENT GRAPH (EDGE*)>\n");
	LFile_WriteString(theFile, "<!ATTLIST GRAPH\n");
	LFile_WriteString(theFile, "          label CDATA #IMPLIED\n");
	LFile_WriteString(theFile, "          nodes CDATA #REQUIRED\n");
	LFile_WriteString(theFile, "          edges CDATA #REQUIRED>\n");
	LFile_WriteString(theFile, "<!ELEMENT EDGE EMPTY>\n");
	LFile_WriteString(theFile, "<!ATTLIST EDGE\n");
	LFile_WriteString(theFile, "          label  CDATA #IMPLIED\n");
	LFile_WriteString(theFile, "          weight CDATA #IMPLIED\n");
	LFile_WriteString(theFile, "          source CDATA #REQUIRED\n");
	LFile_WriteString(theFile, "          dest   CDATA #REQUIRED>\n");
	LFile_WriteString(theFile, "]>\n");
	LFile_WriteString(theFile, "<!--\n");
	LFile_WriteString(theFile, "LGraph2XML utility (c) 2003 Stefano Emiliozzi\n");
	LFile_WriteString(theFile, "this is part of the Leonardo Library http://www.leonardo-vm.org\n");
	LFile_WriteString(theFile, "-->\n");
	theNodesCount = LGraph_GetNodesCount(inGraph);
	theEdgesCount = LGraph_GetEdgesCount(inGraph);
	LString_Format(theBuf, 
		"<GRAPH label=\"%s\" nodes=\"%u\" edges=\"%u\" >\n", 
		inPathName, theNodesCount, theEdgesCount);
	LFile_WriteString(theFile, theBuf);
	LGraph_ForAllEdges(inGraph, theEdge)
	{
		if (inEdgeInfo)
			LString_Format(theBuf, "\t<EDGE source=\"%u\" dest=\"%u\" weight=\"%u\" />\n", 
				theEdge->mSource->mIndex, theEdge->mTarget->mIndex, 
				LEdgeInfo_UI4At(inEdgeInfo, theEdge));
		else
			LString_Format(theBuf, "\t<EDGE source=\"%u\" dest=\"%u\" />\n", 
				theEdge->mSource->mIndex, theEdge->mTarget->mIndex);
		LFile_WriteString(theFile, theBuf);
	}
	LFile_WriteString(theFile, "</GRAPH>\n");		
	LFile_Close(&theFile);
	return TRUE;
}


/* ---------------------------------------------------------------------------------
 *  BuildShortestPathTree
 * ---------------------------------------------------------------------------------
 * Builds shortest path tree and optionally gives mapping tree nodes <-> graph nodes */
LGraph* LGraphGen_BuildShortestPathTree(LGraph* inGraph, LNodeInfo* inPredArray, 
								  LNodeInfo** outTreeToGraph, LNodeInfo** outGraphToTree)
{
	LArray* theNodesList = NULL;
	LGraph* theTree = NULL;
	LGraph_TNode** theTreeNodesList = NULL;
	
	if ((!inGraph) || (!inPredArray))
			return NULL;
	Try
	{
		ui4     theNodesCount;
		ui4     i;
		
		theNodesList     = LGraph_GetAllNodes(inGraph);
		theNodesCount    = LGraph_GetNodesCount(inGraph);
		theTree          = LGraph_New(TRUE);
		theTreeNodesList = (LGraph_TNode**)
						    LMemory_Malloc(theNodesCount*sizeof(LGraph_TNode*));
		for (i=0; i < theNodesCount; i++)
			theTreeNodesList[i] = LGraph_NewNode(theTree);
		if (outTreeToGraph)
			*outTreeToGraph = LNodeInfo_New(theTree, LType_Ptr);
		if (outGraphToTree)
			*outGraphToTree = LNodeInfo_New(inGraph, LType_Ptr);
		/* builds shortest path tree */
		for (i=0; i < theNodesCount; i++)
		{
			LGraph_TNode* theNode0;
			LGraph_TNode* theNode1;

			theNode0 = *(LGraph_TNode**) LArray_ItemAt(theNodesList, i);
			theNode1 = (LGraph_TNode*)LNodeInfo_PointerAt(inPredArray, theNode0);
			if (theNode1)/* source of the tree doesn't have any parent */
				LGraph_NewEdge(theTree, theTreeNodesList[theNode1->mIndex],
							   theTreeNodesList[theNode0->mIndex]);
			if (outTreeToGraph)/* tree->graph nodes mapping */
				LNodeInfo_PointerAt(*outTreeToGraph, 
									theTreeNodesList[theNode0->mIndex]) = theNode0;
			if (outGraphToTree)/* graph->tree nodes mapping */
				LNodeInfo_PointerAt(*outGraphToTree, theNode0) = 
					theTreeNodesList[theNode0->mIndex];
		}
	}
	CatchAny
	{
		/* clean up */
		if (theNodesList)	  LArray_Delete(&theNodesList);
		if (theTree)		  LGraph_Delete(&theTree);
		if (theTreeNodesList) LMemory_Free(&theTreeNodesList);
		Rethrow;
	}
	LArray_Delete(&theNodesList);
	LMemory_Free(&theTreeNodesList);
	return theTree;
}

/* ---------------------------------------------------------------------------------
 *  CopyGraph
 * ---------------------------------------------------------------------------------
 * Returns a topological copy of inGraph */
LGraph* LGraphGen_CopyGraph(LGraph* inGraph)
{
	LGraph_TNode** theNodesList = NULL; 
	LGraph*        theCopy      = NULL;
	LGraph_TEdge*  theEdge;
	ui4            i; 
	
	if (!inGraph)
		return NULL;

    Try {
        theCopy = LGraph_New(inGraph->mDirected);
        theNodesList = (LGraph_TNode**)LMemory_Malloc(inGraph->mNodesCount*sizeof(LGraph_TNode*));
        for (i=0; i < inGraph->mNodesCount; i++)
            theNodesList[i] = LGraph_NewNode(theCopy);
        LGraph_ForAllEdges(inGraph, theEdge)
        {
            ui4 theSrcIdx = theEdge->mSource->mIndex;
            ui4 theDstIdx = theEdge->mTarget->mIndex;
            LGraph_NewEdge(theCopy, theNodesList[theSrcIdx], theNodesList[theDstIdx]);
        }
    }

    CatchAny {
        if (theCopy!=NULL)      LGraph_Delete(&theCopy);
        if (theNodesList!=NULL) LMemory_Free(&theNodesList);
        Rethrow
    }

	LMemory_Free(&theNodesList);

	return theCopy;
}


/* ---------------------------------------------------------------------------------
 *  CopyEdgeInfo
 * ---------------------------------------------------------------------------------
 * Returns a copy of the information inEdgeInfo associated to edges of inGraph */

LEdgeInfo* LGraphGen_CopyEdgeInfo(LGraph* inGraph, LEdgeInfo* inEdgeInfo) {

    LEdgeInfo*    theCopy = NULL;
    i1*           theBuf  = NULL;
    LGraph_TEdge* theEdge;

    Try {
        theCopy = LEdgeInfo_New(inGraph, LEdgeInfo_GetBaseType(inEdgeInfo));
        theBuf  = (i1*)LMemory_Malloc(LEdgeInfo_GetBaseType(inEdgeInfo).mSize);
        LGraph_ForAllEdges(inGraph, theEdge) {
            LEdgeInfo_FetchItemAt(inEdgeInfo, theEdge, theBuf);
            LEdgeInfo_AssignItemAt(theCopy, theEdge, theBuf);
        }
    }

    CatchAny {
        if (theCopy != NULL) LEdgeInfo_Delete(&theCopy);
        if (theBuf != NULL)  LMemory_Free(&theBuf);
        Rethrow;
    }

    if (theBuf != NULL)  LMemory_Free(&theBuf);

    return theCopy;
}


/* ---------------------------------------------------------------------------------
 *  ReverseGraph
 * ---------------------------------------------------------------------------------
 * Returns the reversed graph of inGraph */
LGraph* LGraphGen_ReverseGraph(LGraph* inGraph)
{
	LGraph_TNode** theNodesList = NULL; 
	LGraph*        theCopy      = NULL;
	LGraph_TEdge*  theEdge;
	ui4     i; 
	
	if ((!inGraph) || (!inGraph->mDirected))
		return NULL;/* this function has meaning only for undirected graphs */
	
	theCopy = LGraph_New(inGraph->mDirected);
	theNodesList = (LGraph_TNode**)LMemory_Malloc(inGraph->mNodesCount*sizeof(LGraph_TNode*));
	for (i=0; i < inGraph->mNodesCount; i++)
		theNodesList[i] = LGraph_NewNode(theCopy);
	LGraph_ForAllEdges(inGraph, theEdge)
	{
		ui4 theSrcIdx = theEdge->mSource->mIndex;
		ui4 theDstIdx = theEdge->mTarget->mIndex;
		LGraph_NewEdge(theCopy, theNodesList[theDstIdx], theNodesList[theSrcIdx]);
	}
	LMemory_Free(&theNodesList);
	return theCopy;
}


/* ---------------------------------------------------------------------------------
 *  RemoveSelfLoops
 * ---------------------------------------------------------------------------------
 * remove self loops from the passed graph */
void LGraphGen_RemoveSelfLoops(LGraph** thruGraph)
{
	ui4 i;
	ui4 theCnt = (*thruGraph)->mEdgesCount;
	LGraph_TEdge** theEdges;
	LArray*        theEdgesList = LGraph_GetAllEdges(*thruGraph);

	LArray_InstallSyncPtr(theEdgesList, (void**)&theEdges);
	for (i=0; i < theCnt; i++)
		if (theEdges[i]->mSource == theEdges[i]->mTarget)
			LGraph_DelEdge(*thruGraph, theEdges[i]);
	LArray_Delete(&theEdgesList);
}

/* ---------------------------------------------------------------------------------
 *  RemoveRandomNodes
 * ---------------------------------------------------------------------------------
 * removes randomly inNodeNumber nodes from the graph */
void LGraphGen_RemoveRandomNodes(ui4 inNodeNumber, LGraph* thruGraph)
{
	ui4 theCnt, i;
	LGraph_TNode** theNodes;
	LRandSource* theRand;
	LArray* theNodesArray;

	if (inNodeNumber >= thruGraph->mNodesCount)
		return;

	theRand = LRandSource_New((ui2)time(NULL));
	theNodesArray = LGraph_GetAllNodes(thruGraph);
	LArray_InstallSyncPtr(theNodesArray, (void**)&theNodes);
	theCnt = thruGraph->mNodesCount;

	for(i=0; i < inNodeNumber; i++)
	{
		ui4 theIndex = LRandSource_GetRandUI4(theRand, 0, theCnt-1);
		LGraph_DelNode(thruGraph, theNodes[theIndex]);
		LArray_RemoveItemAt(theNodesArray, theIndex);
		theCnt--;
	}

	LRandSource_Delete(&theRand);
	LArray_Delete(&theNodesArray);
}

/* Copyright (C) 2003 Stefano Emiliozzi, Camil Demetrescu

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
