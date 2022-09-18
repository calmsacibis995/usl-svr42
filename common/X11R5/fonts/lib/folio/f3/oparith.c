/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/oparith.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)oparith.c 1.2 89/03/10";
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
#include    	"oparith.h"

/*  	The following diadic operators can be used with several combinations
    	of operands; the first three leave a result in the stack:

    	 i)     <num:a>     <num:b> 	OP  <num:   c = a OP b>
    	 ii)    <array:a>   <num:b> 	OP  <array: c[i] = a[i] OP b>
    	 iii)   <array:a>   <array:b>   OP  <array: c[i] = a[i] OP b[i]>

    	In the latter case, both arrays must have the same size, which will
    	also be the size of the result.
    	The remaining three cases are similar to the previous ones, with the
    	exception that (1) the left operand is an address, containing either
    	a number (i) or an array (ii & iii) and (2) no result is created on
    	the stack but rather it replaces the previous contents of the address.
*/

VOID	      f3_Diadic(operation)
fract	    (*operation)();
{    	register f3_PairTYPE  *lp,*rp;
    	register int 	       size;

    	rp = --SP;
    	lp = --SP;
    	DCHECK(S_UFLO,SP<SB);

    	if        (lp->func==f3_Number) {
    	    SP++;
    	}
    	if        (lp->func==f3_Array) {
    	    /* an array, passed by value; we duplicate it, and will
    	       use it later as destination; it is already in the
    	       right stack position */
    	    lp->arg.p = f3_DupArray(lp->arg.p);
    	    SP++;
    	} else if (lp->func==f3_GlobalAddress) {
    	    /* a global address; set <lp> to point to its content
    	       and check it */
    	    DCHECK(GADDX_LO,lp->arg.p < GLOBALB);
    	    DCHECK(GADDX_HI,lp->arg.p >=GLOBALL);
    	    lp = lp->arg.p;
    	    DCHECK(INV_OPERAND,	lp->func!=f3_Number &&
    	    	    	    	lp->func!=f3_Array
    	    );
    	} else if (lp->func==f3_LocalAddress) {
    	    /* a local address; set <lp> to point to its content
    	       and check it */
    	    lp = lp->arg.p;
    	    DCHECK(INV_OPERAND,	lp->func!=f3_Number &&
    	    	    	    	lp->func!=f3_Array
    	    );
    	}

    	DCHECK(INV_OPERAND,	rp->func!=f3_Number &&
    	    	    	    	rp->func!=f3_Array
    	);
    	DCHECK(INV_OPERAND, 	lp->func!=f3_Array &&
    	    	    	    	rp->func==f3_Array
    	);
    	DCHECK(INV_OPERAND, 	lp->func==f3_Array &&
    	    	    	    	rp->func==f3_Array &&
    	    	    	    	f3_ArraySize(lp->arg.p) != f3_ArraySize(rp->arg.p)
    	);

    	/* by now, both operands have been completely
    	   checked */
    	if (lp->func==f3_Number) {
    	    /* the right operand is also a number */
    	    lp->arg.f = operation(lp->arg.f,rp->arg.f);
    	} else {
    	    /* the left operand is an array; the
    	       right one may be a number or and array */
    	    size = f3_ArraySize(lp->arg.p);
    	    lp = lp->arg.p;
    	    if (rp->func==f3_Number) {
    	    	while(size--) {
    	    	    lp->arg.f = operation(lp->arg.f,rp->arg.f);
    	    	    lp++;
    	    	}
    	    } else {
    	    	rp = rp->arg.p;
    	    	while(size--) {
    	    	    lp->arg.f = operation(lp->arg.f,rp->arg.f);
    	    	    lp++;
    	    	    rp++;
    	    	}
    	    }
    	}
}

fract	f3_AddOp(a,b)
fract a,b;
{ return(fradd(a,b)); }

VOID f3_Add(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_AddOp); }

fract	f3_SubOp(a,b)
fract a,b;
{ return(frsub(a,b)); }
VOID f3_Sub(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_SubOp); }

fract	f3_MulOp(a,b)
fract a,b;
{ return(frmul(a,b)); }
VOID f3_Mul(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_MulOp); }

fract	f3_DivOp(a,b)
fract a,b;
{ if (b==0) return(0); else return(frdiv(a,b)); }
VOID f3_Div(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_DivOp); }

fract	f3_MinOp(a,b)
fract a,b;
{	return(a>b?b:a); }
VOID f3_Min(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_MinOp); }

fract	f3_MaxOp(a,b)
fract a,b;
{	return(b>a?b:a); }
VOID f3_Max(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_MaxOp); }

fract	f3_GTOp(a,b)
fract a,b;
{	return(a>b?FRONE:0); }
VOID f3_GT(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_GTOp); }

fract	f3_GEOp(a,b)
fract a,b;
{	return(a>=b?FRONE:0); }
VOID f3_GE(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_GEOp); }

fract	f3_EQOp(a,b)
fract a,b;
{	return(a==b?FRONE:0); }
VOID f3_EQ(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_EQOp); }

fract	f3_NEOp(a,b)
fract a,b;
{	return(a!=b?FRONE:0); }
VOID f3_NE(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_NEOp); }

fract	f3_LTOp(a,b)
fract a,b;
{	return(a<b?FRONE:0); }
VOID f3_LT(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_LTOp); }

fract	f3_LEOp(a,b)
fract a,b;
{	return(a<=b?FRONE:0); }
VOID f3_LE(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_LEOp); }

fract	f3_AndOp(a,b)
fract a,b;
{	return(a&&b?FRONE:0); }
VOID f3_And(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_AndOp); }

fract	f3_OrOp(a,b)
fract a,b;
{	return(a||b?FRONE:0); }
VOID f3_Or(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_OrOp); }

fract	f3_ATan2Op(a,b)
fract a,b;
{	return(fratan2(a,b)); }
VOID f3_ATan2(ap)
f3_ArgTYPE *ap;
{ f3_Diadic(f3_ATan2Op); }


/*  	The following monadic operators can be used with several combinations
    	of operands; the first two leave a result in the stack:

    	 i)     <num:a>  	OP  <num:   b =	    OP a>
    	 ii)    <array:a> 	OP  <array: b[i] =  OP a[i]>

    	The remaining two cases are similar to the previous ones, with the
    	exception that (1) the operand is an address, containing either
    	a number (i) or an array (ii) and (2) no result is created on
    	the stack but rather it replaces the previous contents of the address.
*/

VOID	      f3_Monadic(operation)
fract	    (*operation)();
{    	register f3_PairTYPE  *operandp;
    	    	 int 	       size;

    	operandp = --SP;
    	DCHECK(S_UFLO,SP<SB);

    	if        (operandp->func==f3_Number) {
    	    SP++;
    	}
    	if        (operandp->func==f3_Array) {
    	    /* an array, passed by value; we duplicate it, and will
    	       use it later as destination; it is already in the
    	       right stack position */
    	    operandp->arg.p = f3_DupArray(operandp->arg.p);
    	    SP++;
    	} else if (operandp->func==f3_GlobalAddress) {
    	    /* a global address; set <operandp> to point to its content
    	       and check it */
    	    DCHECK(GADDX_LO,operandp->arg.p < GLOBALB);
    	    DCHECK(GADDX_HI,operandp->arg.p >=GLOBALL);
    	    operandp = operandp->arg.p;
    	    DCHECK(INV_OPERAND,	operandp->func!=f3_Number &&
    	    	    	    	operandp->func!=f3_Array
    	    );
    	} else if (operandp->func==f3_LocalAddress) {
    	    /* a local address; set <operandp> to point to its content
    	       and check it */
    	    operandp = operandp->arg.p;
    	    DCHECK(INV_OPERAND,	operandp->func!=f3_Number &&
    	    	    	    	operandp->func!=f3_Array
    	    );
    	}

    	/* by now, both operands have been completely checked */
    	if (operandp->func==f3_Number) {
    	    operandp->arg.f = operation(operandp->arg.f);
    	} else {
    	    size = f3_ArraySize(operandp->arg.p);
    	    operandp = operandp->arg.p;
    	    while(size--) {
    	    	operandp->arg.f = operation(operandp->arg.f);
    	    	operandp++;
    	    }
    	}
}

fract	f3_SqrtOp(a)
fract	a;
{   	return(frsqrt(a)); }
VOID	f3_Sqrt(ap)
f3_ArgTYPE *ap;
{   	f3_Monadic( f3_SqrtOp); }

fract   f3_NegOp(a)
fract   a;
{   return( -a ); }
VOID    f3_Neg(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_NegOp ); }

fract   f3_AbsOp(a)
fract   a;
{   return( (a>=0) ? a : -a ); }
VOID    f3_Abs(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_AbsOp ); }

fract	f3_CeilOp(a)
fract   a;
{   return( ceilingfr(a) ); }
VOID f3_Ceil(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_CeilOp ); }

fract	f3_FloorOp(a)
fract   a;
{   return( frfloor(a) ); }
VOID    f3_Floor(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_FloorOp ); }

fract	f3_TruncOp(a)
fract   a;
{   return(a>=0? frfloor(a) : frceiling(a) ); }
VOID    f3_Trunc(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_TruncOp ); }

fract	f3_RoundOp(a)
fract   a;
{   return( frround(a) ); }
VOID    f3_Round(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_RoundOp ); }

fract	f3_SinOp(a)
fract   a;
{   return( frsind(a) ); }
VOID    f3_Sin(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_SinOp ); }

fract	f3_CosOp(a)
fract   a;
{   return( frcosd(a) ); }
VOID    f3_Cos(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_CosOp ); }

fract	f3_TanOp(a)		/* should dissappear or be fixed */
fract   a;
{   return( 0 ); }
VOID    f3_Tan(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_TanOp ); }

fract	f3_ASinOp(a)		/* should dissappear or be fixed */
fract   a;
{   return( 0 ); }
VOID    f3_ASin(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_ASinOp ); }

fract	f3_ACosOp(a)		/* should dissappear or be fixed */
fract   a;
{   return( 0 ); }
VOID    f3_ACos(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_ACosOp ); }

fract	f3_NotOp(a)
fract   a;
{   return(!a); }
VOID    f3_Not(ap)
f3_ArgTYPE *ap;
{   f3_Monadic( f3_NotOp ); }


