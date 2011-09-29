/* ============================================================================
 *  LRandSource.c
 * ============================================================================

 *  Author:         (c) 2001-2003 Camil Demetrescu, Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:56 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

/* This is an implementation of Steve Park's & Dave Geyer's work, based on: *
 *       "Random Number Generators: Good Ones Are Hard To Find"
 *                   Steve Park and Keith Miller
 *              Communications of the ACM, October 1988                 */



#include "LRandSource.h"
#include "LMemory.h"

#define MODULUS    2147483647 /* DON'T CHANGE THIS VALUE                  */
#define MULTIPLIER 48271      /* DON'T CHANGE THIS VALUE                  */


/* MEMBER VARIABLES */
struct LRandSource 
{
	i4 mSeed;             /* seed */
};


/* ----------------------------------------------------------------------------
 *  New
 * ----------------------------------------------------------------------------
*/
LRandSource* LRandSource_New(ui4 inSeed) 
{
    LRandSource theObject;
	theObject.mSeed = (i4)inSeed;
	return LMemory_NewObject(LRandSource, theObject);
}


/* ----------------------------------------------------------------------------
 *  Delete
 * ----------------------------------------------------------------------------
*/
void LRandSource_Delete(LRandSource** ThisA) 
{
    LMemory_DeleteObject(ThisA);
}

/* ----------------------------------------------------------------
 * GetRandF8
 * ----------------------------------------------------------------
 * Random returns a pseudo-random real number uniformly distributed between 0.0 and 1.0 */
f8 LRandSource_GetRandF8(LRandSource* This)
{
	const i4 theQ = MODULUS / MULTIPLIER;
	const i4 theR = MODULUS % MULTIPLIER;
    i4 theT;

	theT = MULTIPLIER * (This->mSeed % theQ) - theR * (This->mSeed / theQ);
	if (theT > 0) 
		This->mSeed = theT;
	else 
		This->mSeed = theT + MODULUS;
	return ((f8) This->mSeed / MODULUS);
}

/* ----------------------------------------------------------------------------
 *  GetRand
 * ----------------------------------------------------------------------------
*/
ui4 LRandSource_GetRandUI4(LRandSource* This, ui4 inMin, ui4 inMax) 
{
    return inMin+(ui4)(((inMax-inMin)*LRandSource_GetRandF8(This))+0.5);
}

/* ---------------------------------------------------------------
 * GetSeed   
 * ---------------------------------------------------------------
 * Use this function to get the state of the random number generator */
ui4 LRandSource_GetSeed(LRandSource* This)
{
	return (ui4)This->mSeed;
}


/* Copyright (C) 2001-2003 Camil Demetrescu, Stefano Emiliozzi

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
