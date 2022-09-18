/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:common/fract.c	1.1"

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


/*
 *	Copyright 1988 Sun Microsystems, Inc
 */

#ifndef lint 
static char sccsid[] = "@(#)fract.c	1.2 9/28/89 Copyright 1988 Sun Microsystems";
#endif

/*
 *
 *	@@@@@  @@@@    @@@    @@@   @@@@@
 *	@      @   @  @   @  @        @
 *	@@@    @@@@   @@@@@  @        @
 *	@      @  @   @   @  @        @
 *	@      @   @  @   @   @@@     @
 *
 *	C version of the fract math routines
 *
 */
/*#include "FBconfig.h" */
#include "sh_fract.h"

#define U(e) ((unsigned short)(e))

short fract_overflows = 0;	/* overflow flag */
frsqrt(x)			/* sqrt(x) */
    register fract x;
{
    register    root,
                temp;
    if (x <= 0)
	return 0;
    root = temp = x;		/* Compute initial approximation */
    if (temp > fracti(1)) {
	while (temp > fracti(1))
	    root >>= 1, temp >>= 2;
    }
    else {
	while (temp < fracti(1))
	    root <<= 1, temp <<= 2;
    }
    /* Then three Newton/Raphson iterations */
    root = (root + frdiv(x, root)) >> 1;
    root = (root + frdiv(x, root)) >> 1;
    root = (root + frdiv(x, root)) >> 1;
    /* root = (root + frdiv(x, root)) >> 1; */
    return root;
}

vfradd(a, b)
    register fract a, b;
{
    register    s = a + b;
    if ((a ^ b) < 0		/* different signs, so no overflow */
	    || (s ^ a) >= 0)	/* sign of sum matches sign of argument */
	return s;
    fract_overflows++;
    return 0;
}

vfrsub(a, b)
    register a, b;
{
    register    s = a - b;
    if ((a ^ b) >= 0		/* matching signs, so no overflow */
	    || (s ^ a) >= 0)	/* sign of sum matches sign of argument */
	return s;
    fract_overflows++;
    return 0;
}


vfrmul(a, b)
    register fract a, b;
{
    /*
     * A fract is a fixed point number that is 32 bits wide, the bottom 16
     * bits are the fractional part.  If you multiplied a & b together as
     * integers the product would be the MIDDLE 32 bits of the 64 bit product.
     * Integer multiplication in C returns the low order bits, so cross
     * multiplications of the 16 bit halves need to be done.
     */
    register unsigned short ah,
                bh;
    register    neg = 0;
    register    top;
    if (a < 0)
	a = -a, neg = ~neg;
    if (b < 0)
	b = -b, neg = ~neg;
    ah = a >> 16;
    bh = b >> 16;
    top = ah * bh;
    if (top >= 1 << 15)
	fract_overflows++;
    a = (((U(a) * U(b)) >> 16)
	 + (top << 16)
	 + ah * U(b)
	 + U(a) * bh);
    if (a < 0)
	fract_overflows++;
    return neg ? -a : a;
}

vfrdiv(a, b)
    register fract a, b;
{				/* Uses simple old-fashioned shift & subtract */
    register    neg = 0;
    register    ret,
                bit;
    if (a < 0)
	a = -a, neg = ~neg;
    if (b < 0)
	b = -b, neg = ~neg;
    if (b == 0 || a == 0)
	return 0;
    ret = 0;
    bit = 0x10000;
    while (b < a && (b << 1) > 0)
	b <<= 1, bit <<= 1;
    if (bit <= 0)
	fract_overflows++;
    while (a && b) {
	if (a >= b) {
	    a -= b;
	    ret |= bit;
	}
	b >>= 1;
	bit >>= 1;
    }
    return neg ? -ret : ret;
}


frmul(a, b)			/* Compute a*b */
    register fract a, b;
{
    /*
     * A fract is a fixed point number that is 32 bits wide, the bottom 16
     * bits are the fractional part.  If you multiplied a & b together as
     * integers the product would be the MIDDLE 32 bits of the 64 bit
     * product.  Integer multiplication in C returns the low order bits,
     * so cross multiplications of the 16 bit halves need to be done. 
     */
    register unsigned short ah,
                bh;
    register    neg = 0;
    if (a < 0)
	a = -a, neg = ~neg;
    if (b < 0)
	b = -b, neg = ~neg;
    ah = a >> 16;
    bh = b >> 16;
    a = (((U(a) * U(b)) >> 16)
	 + ((ah * bh) << 16)
	 + ah * U(b)
	 + U(a) * bh);
    return neg ? -a : a;
}

frdiv(a, b)			/* a/b */
    register fract a, b;
{				/* Uses simple old-fashioned shift & subtract */
    register    neg = 0;
    register    ret,
                bit;
    if (a < 0)
	a = -a, neg = ~neg;
    if (b < 0)
	b = -b, neg = ~neg;
    if (b == 0 || a == 0)
	return 0;
    ret = 0;
    bit = 0x10000;
    while (b < a && (b<<1)>0)
	b <<= 1, bit <<= 1;
    while (a && b) {
	if (a >= b) {
	    a -= b;
	    ret |= bit;
	}
	b >>= 1;
	bit >>= 1;
    }
    return neg ? -ret : ret;
}

