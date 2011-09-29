/* ============================================================================
 *  LFile.c
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:52 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LConfig.h"
#include "LFile.h"
#include "LException.h"
#include "LMemory.h"
#include "LString.h"


/* ----------------------------------------------------------------------------
 *  WriteXPBlock
 * ----------------------------------------------------------------------------
*/
void LFile_WriteXPBlock(LFile* This, LXPBlock* inBlock){
    LFile_Write(This, LXPBlock_GetData(inBlock), LXPBlock_GetSize(inBlock));
}


/* ----------------------------------------------------------------------------
 *  ReadXPBlock
 * ----------------------------------------------------------------------------
*/
LXPBlock* LFile_ReadXPBlock(LFile* This, ui4 inSize){
    void* theBlock = LMemory_Malloc(inSize);
    if (LFile_Read(This,theBlock,inSize)!=inSize) Throw(LFile_IO_ERROR);
    return LXPBlock_NewFromData(&theBlock,inSize);
}


/* ----------------------------------------------------------------------------
 *  WriteN1
 * ----------------------------------------------------------------------------
*/
void LFile_WriteN1(LFile* This, const void* inVal){
    LFile_Write(This, inVal, 1);
}


/* ----------------------------------------------------------------------------
 *  WriteN2
 * ----------------------------------------------------------------------------
*/
void LFile_WriteN2(LFile* This, const void* inVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui2 theN2 = AtMem_(ui2,inVal);
    theN2 = SwapN2_(theN2);
    LFile_Write(This, (void*)&theN2, 2);
#else
    LFile_Write(This, inVal, 2);
#endif
}


/* ----------------------------------------------------------------------------
 *  WriteN4
 * ----------------------------------------------------------------------------
*/
void LFile_WriteN4(LFile* This, const void* inVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4 = AtMem_(ui4,inVal);
    theN4 = SwapN4_(theN4);
    LFile_Write(This, (void*)&theN4, 4);
#else
    LFile_Write(This, inVal, 4);
#endif
}


/* ----------------------------------------------------------------------------
 *  WriteN8
 * ----------------------------------------------------------------------------
*/
void LFile_WriteN8(LFile* This, const void* inVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4a = AtMem_(ui4,inVal);
    ui4 theN4b = *(Mem_(ui4,inVal)+1);
    theN4a = SwapN4_(theN4a);
    theN4b = SwapN4_(theN4b);
    LFile_Write(This, (void*)&theN4b, 4);
    LFile_Write(This, (void*)&theN4a, 4);
#else
    LFile_Write(This, inVal, 8);
#endif
}


/* ----------------------------------------------------------------------------
 *  ReadN1
 * ----------------------------------------------------------------------------
*/
void LFile_ReadN1(LFile* This, void* outVal){
    if (LFile_Read(This, outVal, 1)!=1) Throw(LFile_IO_ERROR);
}


/* ----------------------------------------------------------------------------
 *  ReadN2
 * ----------------------------------------------------------------------------
*/
void LFile_ReadN2(LFile* This, void* outVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui2 theN2;
    if (LFile_Read(This, (void*)&theN2, 2)!=2) Throw(LFile_IO_ERROR);
    AtMem_(ui2,outVal) = SwapN2_(theN2);
#else
    if (LFile_Read(This, outVal, 2)!=2) Throw(LFile_IO_ERROR);
#endif
}


/* ----------------------------------------------------------------------------
 *  ReadN4
 * ----------------------------------------------------------------------------
*/
void LFile_ReadN4(LFile* This, void* outVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4;
    if (LFile_Read(This, (void*)&theN4, 4)!=4) Throw(LFile_IO_ERROR);
    AtMem_(ui4,outVal) = SwapN4_(theN4);
#else
    if (LFile_Read(This, outVal, 4)!=4) Throw(LFile_IO_ERROR);
#endif
}


/* ----------------------------------------------------------------------------
 *  ReadN8
 * ----------------------------------------------------------------------------
*/
void LFile_ReadN8(LFile* This, void* outVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4a;
    ui4 theN4b;
    if (LFile_Read(This, (void*)&theN4a, 4)!=4) Throw(LFile_IO_ERROR);
    if (LFile_Read(This, (void*)&theN4b, 4)!=4) Throw(LFile_IO_ERROR);
    AtMem_(ui4,outVal)    = SwapN4_(theN4b);
    *(Mem_(ui4,outVal)+1) = SwapN4_(theN4a);
#else
    if (LFile_Read(This, outVal, 8)!=8) Throw(LFile_IO_ERROR);
#endif
}


/* ----------------------------------------------------------------------------
 *  WriteString
 * ----------------------------------------------------------------------------
*/
void LFile_WriteString(LFile* This, i1* inBuf){
    LFile_Write(This, inBuf, LString_Len(inBuf));
}


/* ----------------------------------------------------------------------------
 *  ReadString
 * ----------------------------------------------------------------------------
*/
ui4 LFile_ReadString(LFile* This, i1* outBuf, ui4 inBufSize, i1* inSeparators){
    ui4 theReadLen, theLineLen;

    /* read block of inBufSize-1 bytes (leave space for string zero-terminator) */
    theReadLen = LFile_Read(This, outBuf, inBufSize-1);

    /* zero-terminate string */
    outBuf[theReadLen] = '\0';

    /* isolate line in buffer */
    LString_Tokenizer(outBuf, inSeparators);

    /* get token length */
    theLineLen = LString_Len(outBuf);

    /* possibly rollback file position */
    if (theLineLen + 1 < theReadLen) 
        LFile_Seek(This, (i4)theLineLen - (i4)theReadLen + 1, LFile_CURR);

    return theLineLen;
}

#if 0
ui4 LFile_ReadString(LFile* This, i1* outBuf, ui4 inBufSize, i1* inTerminator){
    ui4 theLen, theTermPos, theTermLen;

    /* get length of terminator string */
    theTermLen = LString_Len(inTerminator);

    /* read block of inBufSize-1 bytes (leave space for string zero-terminator) */
    theLen = LFile_Read(This, outBuf, inBufSize-1);

    /* look for terminator string */
    if (theTermLen == 1)
         for (theTermPos = 0; theTermPos < theLen - theTermLen; ++theTermPos) 
            if (outBuf[theTermPos] == *inTerminator) break;
    else for (theTermPos = 0; theTermPos < theLen - theTermLen; ++theTermPos)
            if (!LMemory_Compare(outBuf + theTermPos, inTerminator, theTermLen)) break;

    /* rollback file position */
    if (theTermPos < theLen) 
        LFile_Seek(This, LFile_CURR, (i4)theTermPos - (i4)theLen);

    /* zero-terminate string */
    outBuf[theTermPos] = '\0';

    return theTermPos;
}
#endif

/* ----------------------------------------------------------------------------
 *  CutPath
 * ----------------------------------------------------------------------------
*/
void LFile_CutPath(i1 thruPathName[LFile_MAX_PATHNAME_LEN]){
    ui4 i = LString_Len(thruPathName);
    ui4 j=i;
    for (;i>0;i--) 
        if (thruPathName[i-1]==__LSL_DIR_SEP__)  break;
    if (i<j) LMemory_Move(thruPathName+i,thruPathName,j-i);
    thruPathName[j-i]='\0';
}


/* ----------------------------------------------------------------------------
 *  CutName
 * ----------------------------------------------------------------------------
*/
void LFile_CutName(i1 thruPathName[LFile_MAX_PATHNAME_LEN]){
    ui4 i = LString_Len(thruPathName);
    for (;i>0;i--) 
        if (thruPathName[i-1]==__LSL_DIR_SEP__)  break;
    thruPathName[i]='\0';
}


/* ----------------------------------------------------------------------------
 *  CutPathNameExt
 * ----------------------------------------------------------------------------
*/
void LFile_CutPathNameExt(i1 thruPathName[LFile_MAX_PATHNAME_LEN]){
    i4 i = (i4)LString_Len(thruPathName)-1;
    for (;i>0;i--) {
        if (thruPathName[i]==__LSL_DIR_SEP__) break;
        if (thruPathName[i]=='.') {
            thruPathName[i]='\0';
            break;
        }
    }
}


/* Copyright (C) 2001 Camil Demetrescu

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
