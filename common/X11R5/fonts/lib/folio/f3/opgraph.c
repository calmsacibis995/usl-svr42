/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/opgraph.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)opgraph.c 1.6 89/05/23";
#endif
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



#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include    	<setjmp.h>

#include        "cdefs.h"
#include	"frmath.h"
#include	"f3.h"
#include    	"glbvars.h"
#include    	"error.h"
#include    	"scanners.h"
#include    	"array.h"
#include    	"oparith.h"

#ifdef DEBUG
extern	bool df3_Opgraph;
#endif /* DEBUG */

VOID
f3_TransformOp2( tp, ap, size )
	 f3_PairTYPE	*tp;	/* the displacement vector	*/
register f3_PairTYPE	*ap;	/* the array of pair of numbers	*/
	 int32		 size;
{
	register fract	 dx,dy;

	dx =  tp   ->arg.f;
	dy = (tp+1)->arg.f;

	while ( size ) {
		ap->arg.f = fradd(ap->arg.f,dx);
		ap++;
		ap->arg.f = fradd(ap->arg.f,dy);
		ap++;
		size -= 2;
	}
}

VOID
f3_TransformOp4( tp, ap, size )
register f3_PairTYPE	*tp,	/* the transformation matrix	*/
    	    	    	*ap;	/* the array of pair of numbers	*/
	 int32		size;
{
	register fract		x, y;
	register fract		a, b, c, d;


	a = tp++->arg.f;
	b = tp++->arg.f;
	c = tp++->arg.f;
	d = tp->arg.f;

	while ( size ) {
		x = ap->arg.f;
		y = (ap+1)->arg.f;
		ap++->arg.f = fradd(frmul(a,x),frmul(c,y));
		ap++->arg.f = fradd(frmul(b,x),frmul(d,y));
		size -= 2;
	}
}

/*
 *	i)	<array: a>	<array: t>	TRANSFORM	=>	<array: a'>
 *			leaving the array with the transformed values on the stack.
 *	ii)	<aryRef: a>	<array: t>	TRANSFORM	=>
 *			no result is created on the stack; the contents of <t> are
 *			replaced by the adjusted values.
 *
 *	<a> has to be an even sized array, its contents are taken in pair
 *	to be transformed.
 *	<t> is the transformation matrix [a  b]
 *					 [c  d]
 *	or the displacement [dx dy]
 */
VOID
f3_Transform(dummyp) f3_ArgTYPE *dummyp;
{
	register f3_PairTYPE	*ap,	/* the array of pair of numbers	*/
	    	    	    	*tp;	/* the transformation matrix or
					   the displacement vector */
	register int32		 tsize,
				 asize;

	tp = --SP;
	ap = --SP;
	DCHECK(	S_UFLO, SP < SB );

	DCHECK( INV_OPERAND, tp->func != f3_Array );
	DCHECK( INV_OPERAND, !( ap->func == f3_Array		||
				ap->func == f3_GlobalAddress	||
				ap->func == f3_LocalAddress) );
	if ( ap->func == f3_Array ) {
		/*	an array, passed by value; we duplicate it, and will
			use it later as destination; it is already in the
			right stack position
		 */
		ap->arg.p = f3_DupArray( ap->arg.p );
		SP++;
	}
	else if ( ap->func == f3_GlobalAddress ) {
		/* a global address; set <ap> to point to its content
		   and check it
		 */
		DCHECK( GADDX_LO, ap->arg.p < GLOBALB );
		DCHECK( GADDX_HI, ap->arg.p >=GLOBALL );
		ap = ap->arg.p;
		DCHECK( INV_OPERAND, ap->func != f3_Array );
	}
	else if ( ap->func == f3_LocalAddress ) {
		/* a local address; set <ap> to point to its content
		   and check it
		 */
    	    	ap = ap->arg.p;
		DCHECK( INV_OPERAND, ap->func != f3_Array );
	}

	/* by now, both operands have been completely checked */
	/* They are both array of numbers */
	tsize = f3_ArraySize( tp->arg.p );
	DCHECK( INV_OPERAND, tsize != 4 && tsize != 2 );
	asize = f3_ArraySize( ap->arg.p );
	DCHECK( INV_OPERAND, asize & 0x0001 );

	tp = tp->arg.p;
	ap = ap->arg.p;
	if (tsize == 4)	f3_TransformOp4( tp, ap, asize );
	else		f3_TransformOp2( tp, ap, asize );
}

VOID
f3_PixRatio(dummyp) f3_ArgTYPE *dummyp;
{
    	f3_ArgTYPE  arg;

	arg.f = PSP->pixratio;
	f3_Number( &arg );
}

VOID
f3_Box(dummyp) f3_ArgTYPE *dummyp;
{
	register f3_PairTYPE	*ap;	/* the array of pair of numbers	*/
	register int32		 asize;
	register fract		 x,y,xlo,ylo,xhi,yhi;

	DCHECK(	S_UFLO, SP <= SB );
	ap = SP-1;
	DCHECK( INV_OPERAND, ap->func != f3_Array );
	asize = f3_ArraySize( ap->arg.p );
	DCHECK( INV_OPERAND, asize & 0x0001 );

	ap = ap->arg.p;
	xlo = ylo = FRHUGE;
	xhi = yhi = FRHUGENEG;
	while (asize) {
	    x = ap++->arg.f;
	    y = ap++->arg.f;

	    if (x<xlo)	xlo = x;
	    if (y<ylo)	ylo = y;
	    if (x>xhi)	xhi = x;
	    if (y>yhi)	yhi = y;

	    asize -= 2;
	}
	(SP-1)->arg.p = ap = f3_MakeTempArray((int32)4);
	ap++->arg.f = xlo;
	ap++->arg.f = ylo;
	ap++->arg.f = xhi;
	ap++->arg.f = yhi;
}

VOID
f3_Slope(dummyp) f3_ArgTYPE *dummyp;
{
}

VOID
f3_Invert(dummyp) f3_ArgTYPE *dummyp;
/*
 *	<array:points>  <array:curves>  INVERT  <array:points'>  <array:curves'>
 */
{
	register f3_PairTYPE	*crvp,*newcrvp,
				*plop,*phip;
	register int		 crvsize,
				 pntused;

	DCHECK(	S_UFLO, SP < (SB + 2));

	crvp = --SP;
	DCHECK( INV_OPERAND, crvp->func != f3_Array );
	crvsize = f3_ArraySize(crvp->arg.p);
	newcrvp = f3_MakeTempArray(crvsize);
	crvp = crvp->arg.p;
	SP->arg.p = newcrvp;
	SP++;

	/* first we invert curve types */
	newcrvp += crvsize;
	pntused = 1;	/* the 1st one, consumed implicitly */
	while (crvsize) {
	    register int	crvtype,n,i;

	    crvtype = floorfr(crvp->arg.f);
	    if        (crvtype==f3_LINETOKEN) {
		n = 1; pntused += 1;
	    } else if (crvtype==f3_CONICTOKEN) {
		n = 3; pntused += 2;
	    } else if (crvtype==f3_BEZIERTOKEN) {
		n = 1; pntused += 3;
	    } else {
		DCHECK( INV_OPERAND, TRUE);
	    }
	    newcrvp -= n;
	    for (i=0; i<n; i++) {
		(newcrvp+i)->arg.f = (crvp++)->arg.f;
	    }
	    crvsize -= n;
	}

	/* now we invert the points; the situation is
	   P0  P1  ...  Pused-2  (Pused-1 = P0, either explicitly or implicitly)
	   and will be changed to
	   P0  Pused-2  ...  P1  (Pused-1 = P0, ...)
	 */
	plop = SP-2;
	DCHECK( INV_OPERAND, plop->func != f3_Array );
	plop = plop->arg.p + 2;		/* point to P1 */
	phip = plop + ((pntused-3)<<1);	/* point to Pused-2 */
	while (plop < phip) {
	    register fract	temp;

	    temp = plop->arg.f;		/* exchange x's */
	    plop->arg.f = phip->arg.f;
	    phip->arg.f = temp;

	    plop++; phip++;		/* exchange y's */
	    temp = plop->arg.f;
	    plop->arg.f = phip->arg.f;
	    phip->arg.f = temp;

	    plop++; phip -= 3;		/* net change: lo+2 hi-2 */
	}
}




