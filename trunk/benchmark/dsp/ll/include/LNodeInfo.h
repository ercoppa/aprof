/* ============================================================================
 *  LNodeInfo.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi, Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        January 12, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:18 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#ifndef __LNodeInfo__
#define __LNodeInfo__

#include "LType.h"
#include "LArray.h"
#include "LGraph.h"

/* COMPONENT ID */
#define LNodeInfo_ID   0x802A

/* EXCEPTIONS */
enum 
{ 
    LNodeInfo_OBJECT_NULL_POINTER = LNodeInfo_ID<<16, 
    LNodeInfo_GRAPH_NULL_POINTER,
    LNodeInfo_NODE_NULL_POINTER,
    LNodeInfo_ITEM_NULL_POINTER,
	LNodeInfo_BASE_TYPE_MISMATCH
};

/* TYPEDEFS */
typedef struct LNodeInfo LNodeInfo;

struct LNodeInfo
{
    LGraph*		 mGraph;
    LArray*		 mData;
	LType_TType  mBaseType;
	void         (*mAlloc)  (LNodeInfo*, LGraph_TNode*);
	void         (*mDealloc)(LNodeInfo*, LGraph_TNode*);
	ui4          mGraphIdx;
};

/* PUBLIC MACROS */
#define LNodeInfo_ItemAt(NodeInfo, Node) LArray_ItemAt((NodeInfo)->mData, (Node)->mIndex)

/* MACROS TO ACCESS BASE TYPES */
#define LNodeInfo_UI1At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_UI1))? _LNodeInfo_Panic(0) : 0, ((ui1*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_UI2At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_UI2))? _LNodeInfo_Panic(0) : 0, ((ui2*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_UI4At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_UI4))? _LNodeInfo_Panic(0) : 0,((ui4*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_I1At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_I1))? _LNodeInfo_Panic(0) : 0, ((i1*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_I2At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_I2))? _LNodeInfo_Panic(0) : 0, ((i2*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_I4At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_I4))? _LNodeInfo_Panic(0) : 0, ((i4*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_F4At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_F4))? _LNodeInfo_Panic(0) : 0, ((f4*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_F8At(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_F8))? _LNodeInfo_Panic(0) : 0, ((f8*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_BoolAt(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_Bool))? _LNodeInfo_Panic(0) : 0, ((Bool*)LNodeInfo_ItemAt(NodeInfo,Node)) ))

#define LNodeInfo_PointerAt(NodeInfo, Node) \
	(*( !(LType_EqualTypes((NodeInfo)->mBaseType, LType_Ptr))? _LNodeInfo_Panic(0) : 0, ((void**)LNodeInfo_ItemAt(NodeInfo,Node)) ))


/* PUBLIC FUNCTIONS */
LNodeInfo*  LNodeInfo_New                   (LGraph* inGraph, const LType_TType inType);
LNodeInfo*  LNodeInfo_NewCustom             (LGraph* inGraph, ui4 inItemSize);
void        LNodeInfo_Delete                (LNodeInfo** ThisA);
void        LNodeInfo_AssignItemAt          (LNodeInfo* This, LGraph_TNode* inNode, const void* inItem);
Bool        LNodeInfo_FetchItemAt           (LNodeInfo* This, LGraph_TNode* inNode, void* outItem);
void        LNodeInfo_InstallDelNodeHandler (LNodeInfo* This, 
                                             void (*inDealloc)(LNodeInfo*, LGraph_TNode*) );
void        LNodeInfo_InstallNewNodeHandler (LNodeInfo* This, 
                                             void (*inAlloc)(LNodeInfo*, LGraph_TNode*) );
LType_TType LNodeInfo_GetBaseType           (LNodeInfo* This);
ui4         LNodeInfo_GetUsedMem            (LNodeInfo* This);

/* PRIVATE FUNCTIONS */
ui4		   _LNodeInfo_Panic	      (ui4 inValue);


#endif


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

