/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/opother.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)opother.c 1.3 89/04/24";
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

#include    	"glbvars.h"
#include    	"error.h"
#include    	"scanners.h"
#include    	"array.h"
#include    	"opother.h"


VOID	f3_MakeArray(dummyp) f3_ArgTYPE *dummyp;
/*
 *  	num:size    ARRAY   array:result
 */
{   	register f3_PairTYPE *p;
    	    	 int32 	      size;

    	p = --SP;
    	DCHECK(S_UFLO,SP<SB);
    	DCHECK(INV_OPERAND,p->func!=f3_Number);
    	size = floorfr(p->arg.f);
    	DCHECK(INV_OPERAND,size < 0);

    	p->func =   f3_Array;
    	p->arg.p =  f3_MakeTempArray(size);
    	SP++;
}

VOID	f3_Index(dummyp) f3_ArgTYPE *dummyp;
/*
 *  	array:base    num:index	    INDEX     num:base[index]
 *  	array:base  array:index	    INDEX   array:base[index[*]]
 */
{   	register f3_PairTYPE	*basep,*indexp,*resultp;
    	register int32 	     	 basesize,indexsize,index;

    	indexp =    --SP;
    	basep =	    --SP;
    	DCHECK(S_UFLO,SP<SB);

    	DCHECK(INV_OPERAND, basep->func!=f3_Array);
    	basep = basep->arg.p;
    	basesize = f3_ArraySize(basep);

    	DCHECK(INV_OPERAND, indexp->func!=f3_Number &&
    	    	    	    indexp->func!=f3_Array);
    	if (indexp->func == f3_Number) {
    	    index = floorfr(indexp->arg.f);
    	    DCHECK(INV_OPERAND,index < 0 || index >= basesize);
    	    *SP =   *(basep + index);
    	} else {
    	    indexp = indexp->arg.p;
    	    indexsize = f3_ArraySize(indexp);
    	    SP->func =	    	    f3_Array;
    	    resultp = SP->arg.p =   f3_MakeTempArray(indexsize);
    	    while (indexsize--) {
    	    	index = floorfr(indexp->arg.f);
    	    	DCHECK(INV_OPERAND,index < 0 || index >= basesize);
    	    	*resultp = *(basep + index);
    	    	indexp++;
    	    	resultp++;
    	    }
    	}

    	SP++;
}

VOID	f3_Cat(dummyp) f3_ArgTYPE *dummyp;
/*
 *  	array:a	    array:b 	CAT 	array:r = a|b
 */
{   	register f3_PairTYPE	*ap,*bp,*rp;
    	register int32 	     	 asize,bsize;

    	bp =	--SP;
    	ap =	--SP;
    	DCHECK(S_UFLO,SP<SB);

    	DCHECK(INV_OPERAND,ap->func!=f3_Array);
    	DCHECK(INV_OPERAND,bp->func!=f3_Array);
    	ap = ap->arg.p;
    	bp = bp->arg.p;
    	asize = f3_ArraySize(ap);
    	bsize = f3_ArraySize(bp);

    	SP->func =  	    f3_Array;
    	rp = SP->arg.p =    f3_MakeTempArray(asize+bsize);
    	while (asize--)	*rp++ = *ap++;
    	while (bsize--)	*rp++ = *bp++;

    	SP++;
}

VOID	f3_SubArray(dummyp) f3_ArgTYPE *dummyp;
/*
 *  	array:a	    num:i0	num:i1	    SUBARRAY	array:b = a[i0]...a[i1]
 */
{   	register f3_PairTYPE	*ap,*i0p,*i1p,*rp;
    	register int32	    	 i0,i1,i;
    	    	 int32 	     	 asize;

    	i1p =	--SP;
    	i0p =	--SP;
    	ap =	--SP;
    	DCHECK(S_UFLO,SP<SB);

    	DCHECK(INV_OPERAND,ap->func!=f3_Array);
    	ap = ap->arg.p;
    	asize = f3_ArraySize(ap);

    	DCHECK(INV_OPERAND,i0p->func!=f3_Number);
    	i0 = floorfr(i0p->arg.f);

    	DCHECK(INV_OPERAND,i1p->func!=f3_Number);
    	i1 = floorfr(i1p->arg.f);

    	DCHECK(INV_OPERAND,i0 < 0 || i0 >= asize);
    	DCHECK(INV_OPERAND,i1 < 0 || i1 >= asize);
    	DCHECK(INV_OPERAND,i1 < i0);

    	SP->func =  	    f3_Array;
    	rp = SP->arg.p =    f3_MakeTempArray(i1 - i0 + 1);
    	SP++;
    	for (i = i0; i <= i1; i++)
	    *rp++ = *(ap + i);
}

VOID	f3_Sizeof(dummyp) f3_ArgTYPE *dummyp;
{   	register f3_PairTYPE *p;
    	    	 int32 	      size;

    	p = --SP;
    	DCHECK(S_UFLO,SP<SB);
    	DCHECK(INV_OPERAND,p->func!=f3_Array);
    	size = f3_ArraySize(p->arg.p);

    	SP->func = f3_Number;
    	SP->arg.f = fracti(size);
    	SP++;
}

VOID	f3_Pop(dummyp) f3_ArgTYPE *dummyp;
{
    	SP--;
    	DCHECK(S_UFLO,SP<SB);
}

VOID	f3_Exchange(dummyp) f3_ArgTYPE *dummyp;
/*
 *	any:a  any:b  EXCHANGE  any:b  any:a
 */
{		 f3_PairTYPE	 aux;
	register f3_PairTYPE	*ap,*bp;

	bp = SP-1;
	ap = SP-2;

    	DCHECK(S_UFLO,ap<SB);

	aux = *bp;
	*bp = *ap;
	*ap = aux;
}

VOID	f3_StackDepth(dummyp) f3_ArgTYPE *dummyp;
/*
 *	-   STACKDEPTH   num:depth (before pushing the result)
 */
{
    	DCHECK(S_OFLO,SP>=SL);
	SP->func = f3_Number;
	SP->arg.f = fracti(SP-SB);
	SP++;
}

VOID	f3_Roll(dummyp) f3_ArgTYPE *dummyp;
/*
 *	num:depth   num:count   ROLL   -
 *
 *	Pops <count> and <depth>; the (now) topmost <depth> entries
 *	in the stack are rolled <count % depth> positions; ie, if
 *	e[0] is the top of the stack, e[1] the entry immediately
 *	below, etc, and <count = count % depth, count > 0>, then
 *	e[0]...e[count-1] are moved down to positions [depth-count]...
 *	...[depth-1], and e[count]...e[depth-1] are moved up to
 *	positions [0]...[depth-count-1]. If <count < 0> then the
 *	effect is identical to <count = depth + count>. Yes, all this
 *	could be explained in the usual 'before' and 'after' notation,
 *	but it becomes messy.
 */
{		 f3_PairTYPE	*countp,
				*depthp;
	register int32	 	 count,
				 depth;

	countp = --SP;
	depthp = --SP;
	DCHECK(S_UFLO,SP<SB);
	DCHECK(INV_OPERAND,countp->func!=f3_Number);
	DCHECK(INV_OPERAND,depthp->func!=f3_Number);
	count = floorfr(countp->arg.f);
	depth = floorfr(depthp->arg.f);
	DCHECK(S_UFLO,(SP-SB)<depth);

	if (depth < 1)	return;

	count %= depth;
	if (count < 0)	count += depth;

	/* simple and slow, but then <count> and <depth> are usually
	   small... */
	while (count--) {
	    register f3_PairTYPE	*auxsp,
					 aux;
	    register int32		 auxdepth;

	    auxsp = SP-1;
	    aux = *auxsp;
	    for (auxdepth = depth-1; auxdepth > 0; auxdepth--) {
		*auxsp = *(auxsp-1);
		 auxsp--;
	    }
	    *auxsp = aux;
	}
}

VOID	f3_Assign(dummyp) f3_ArgTYPE *dummyp;
/*
 *  	any:value   addx:dest	            ASSIGN  	-	(1)
 *  	num:value   addx:dest	num:index   ASSIGN  	-	(2)
 *    array:value   addx:dest array:index   ASSIGN  	-	(3)
 *	num:value   addx:dest array:index   ASSIGN  	-	(4)
 */
{   	register f3_PairTYPE	*valuep,*destp,*indexp;
    	    	 int32 	     	 valuesize,
    	    	     	    	 destsize;
    	register int32	    	 indexsize,
    	    	    	    	 index;
    	    	 bool	     	 isglobal;

    	indexp =    --SP;   /* assuming 2nd, 3rd or 4th case */
    	if (	indexp->func != f3_Number	&&
    	    	indexp->func != f3_Array ) {
    	    /* must be 1st case */
    	    destp = 	indexp;
    	    valuep =  --SP;
    	    indexp = NULL;
    	} else {
    	    /* 2nd, 3rd or 4th cases */
    	    destp =   --SP;
    	    valuep =  --SP;
    	}
    	DCHECK(S_UFLO,SP<SB);

    	DCHECK(INV_OPERAND,	indexp != NULL	    	    &&
    	    	    	    	indexp->func != f3_Number   &&
    	    	    	    	indexp->func != f3_Array );
    	DCHECK(INV_OPERAND,	destp->func != f3_GlobalAddress	&&
    	    	    	    	destp->func != f3_LocalAddress);

    	if (destp->func == f3_GlobalAddress) {
    	    isglobal = TRUE;
    	    destp = destp->arg.p;
    	    DCHECK(GADDX_LO,destp < GLOBALB);
    	    DCHECK(GADDX_HI,destp >=GLOBALL);
    	} else {
    	    isglobal = FALSE;
    	    destp = destp->arg.p;
    	}

    	if        (!indexp) {	    	    	/* 1st case */
    	    if (isglobal && valuep->func==f3_Array)
    	    	valuep->arg.p = f3_MakeArrayPerm(valuep->arg.p);
    	    *destp = *valuep;
    	} else if (indexp->func == f3_Number) {	/* 2nd case */
    	    index = floorfr(indexp->arg.f);

    	    DCHECK(INV_OPERAND, destp->func!=f3_Array);
    	    destp = destp->arg.p;
    	    destsize =	f3_ArraySize(destp);

    	    DCHECK(INV_OPERAND,index < 0 || index >= destsize);

    	    DCHECK(INV_OPERAND,valuep->func!=f3_Number);

    	    (destp + index)->func  = valuep->func;
    	    (destp + index)->arg.f = valuep->arg.f;
    	} else if (indexp->func == f3_Array) {	/* 3rd or 4th case */
    	    indexp = indexp->arg.p;
    	    indexsize = f3_ArraySize(indexp);

    	    DCHECK(INV_OPERAND, destp->func!=f3_Array);
    	    destp = destp->arg.p;
    	    destsize =	f3_ArraySize(destp);

    	    DCHECK(INV_OPERAND,valuep->func!=f3_Array && valuep->func!=f3_Number);
	    if (valuep->func == f3_Array) {	/* 3rd case */
    		valuep = valuep->arg.p;
    		valuesize =	f3_ArraySize(valuep);

		DCHECK(INV_OPERAND,indexsize != valuesize);

		while (indexsize--) {
    	    	    index = floorfr(indexp->arg.f);
    	    	    DCHECK(INV_OPERAND,index < 0 || index >= destsize);

		    *(destp + index) = *valuep++;
		    indexp++;
    	        }
    	    }
	    else {				/* 4th case */
		while (indexsize--) {
    	    	    index = floorfr(indexp->arg.f);
    	    	    DCHECK(INV_OPERAND,index < 0 || index >= destsize);

		    *(destp + index) = *valuep;
		    indexp++;
    	        }
	    }
	}
}

VOID	f3_ValueOf(dummyp) f3_ArgTYPE *dummyp;
/*
 *  	addx:source 	VALUEOF	    any:contents of <addx>
 */
{   	register f3_PairTYPE *p;

    	p = SP-1;
    	DCHECK(S_UFLO,SP<=SB);
    	DCHECK(INV_OPERAND,(p->func!=f3_GlobalAddress) && (p->func!=f3_LocalAddress));
    	if (p->func==f3_GlobalAddress) {
    	    DCHECK(GADDX_LO,p->arg.p < GLOBALB);
    	    DCHECK(GADDX_HI,p->arg.p >=GLOBALL);
    	}
    	p = p->arg.p;
    	*(SP-1) = *p;
}

VOID	f3_Constant(dummyp) f3_ArgTYPE *dummyp;
{
}

VOID	f3_Dup(dummyp) f3_ArgTYPE *dummyp;
{   	register f3_PairTYPE	*p;

    	p = --SP;
    	DCHECK(S_UFLO,SP<SB);
	 SP++;
    	DCHECK(S_OFLO,SP>=SL);
        *SP++ = *p;
}





