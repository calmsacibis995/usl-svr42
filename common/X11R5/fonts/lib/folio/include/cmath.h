/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/cmath.h	1.1"
/*
 * @(#)cmath.h 1.3 89/05/24
 *
 */
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


#define	FIXED16		((int32)16)	/* used for 16/16 fixed point manipulation */
#define	FIXED16FACTOR	((double)(1<<FIXED16))

/*----------------------------------------------------------*/
/*  
   Convert double number <*p> to FIXEDPOINT number <*r>
 */

#define	math_dToFixedPoint(r,p,f) {(*r) = math_iRound((*p)*(double)(1<<f)); }


/*----------------------------------------------------------*/
/*
	Convert a 16/16 fixed point number <*p> into a double <*r>.
*/

#define math_Convert16FixedToDouble(r,p) { (*r) = ( ((double)(*p))/(FIXED16FACTOR)); }

/*----------------------------------------------------------*/

/* 
    rounding of a fixed point number <n> with <f> bits of fraction 
    returns result.
*/

extern int32	math_RoundFixedPoint(/*n, f*/);
/*  int32	n, f;   */

extern int32	math_RoundFixedPointShort(/*n, f */);
/*  int32	n, f;   */

extern int32	math_FloorFixedPoint(/*n, f*/);
/*  int32	n, f;   */

extern int32 math_CeilingFixedPoint(/*n, f*/);
/*  int32	n, f;   */
