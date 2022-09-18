/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/cfbrrop.c	1.1"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * $XConsortium: cfbrrop.c,v 1.4 91/01/27 13:03:00 keith Exp $
 * Copyright 1989 Massachusetts Institute of Technology
 * Author:  Keith Packard, MIT X Consortium
 */

/* cfb reduced rasterop computations */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "newfill.h"

/* A description:
 *
 * There are four possible operations on each bit in the destination word,
 *
 *	    1	2   3	4
 *
 *    0	    0	0   1	1
 *    1	    0	1   0	1
 *
 * On examination of the reduced rop equation (dst = (dst & and) ^ xor),
 * these four fall to reduced rops as follows:
 *
 *  and	    0	1   1	0
 *  xor	    0	0   1	1
 *
 * or, (if 'and' is expensive) (dst = (dst | or) ^ xor)
 *
 *  or	    1	0   0	1
 *  xor	    1	0   1	0
 *
 * The trouble with using this later equation is that trivial
 * rasterop reduction is more difficult; some common rasterops
 * use complicated expressions of xor/and instead of the simple
 * ones while other common rasterops are not made any simpler:
 *
 * GXcopy:	*dst = ~xor		instead of  *dst = xor
 * GXand:	*dst = *dst & ~or	instead of  *dst = *dst & and
 * GXor:	*dst = *dst | or	instead of  *dst = *dst | xor
 * GXxor:	*dst = *dst ^ xor	instead of  *dst = *dst ^ xor
 *
 * If you're really set on using this second mechanism, the changes
 * are pretty simple.
 *
 * All that remains is to provide a mechanism for computing and/xor values
 * based on the raster op and foreground value.
 *
 * The 16 rops fall as follows, with the associated reduced
 * rop and/xor and or/xor values.  The values in parenthesis following the
 * reduced values gives an equation using the source value for
 * the reduced value, and is one of {0, src, ~src, 1} as appropriate.
 *
 *	clear		and		andReverse	copy
 *     src  0	1	    0   1	    0	1	    0	1
 *  dst	0   0	0	0   0   0	0   0	1	0   0	1
 *	1   0	0	1   0   1	1   0	0	1   0	1
 *
 *  and	    0	0 (0)	    0   1 (src)	    0	1 (src)	    0	0 (0)
 *  xor	    0	0 (0)	    0   0 (0)	    0	1 (src)	    0	1 (src)
 *
 *  or	    1	1 (1)	    1	0 (~src)    1	0 (~src)    1	1 (1)
 *  xor	    1	1 (1)	    1	0 (~src)    1	1 (1)	    1	0 (~src)
 *
 *	andInverted	noop		xor		or
 *     src  0	1	    0   1	    0	1	    0	1
 *  dst	0   0	0	0   0   0	0   0	1	0   0	1
 *	1   1	0	1   1   1	1   1	0	1   1	1
 *
 *  and	    1	0 (~src)    1   1 (1)	    1	1 (1)	    1	0 (~src)
 *  xor	    0	0 (0)	    0   0 (0)	    0	1 (src)	    0	1 (src)
 *
 *  or	    0	1 (src)	    0	0 (0)	    0	0 (0)	    0	1 (src)
 *  xor	    0	1 (src)	    0	0 (0)	    0	1 (src)	    0	0 (0)
 *
 *	nor		equiv		invert		orReverse
 *     src  0	1	    0   1	    0	1	    0	1
 *  dst	0   1	0	0   1   0	0   1	1	0   1	1
 *	1   0	0	1   0   1	1   0	0	1   0	1
 *
 *  and	    1	0 (~src)    1   1 (1)	    1	1 (1)	    1	0 (~src)
 *  xor	    1	0 (~src)    1   0 (~src)    1	1 (1)	    1	1 (1)
 *
 *  or	    0	1 (src)	    0	0 (0)	    0	0 (0)	    0	1 (src)
 *  xor	    1	1 (1)	    1	0 (~src)    1	1 (1)	    1	0 (~src)
 *
 *	copyInverted	orInverted	nand		set
 *     src  0	1	    0   1	    0	1	    0	1
 *  dst	0   1	0	0   1   0	0   1	1	0   1	1
 *	1   1	0	1   1   1	1   1	0	1   1	1
 *
 *  and	    0	0 (0)	    0   1 (src)	    0	1 (src)	    0	0 (0)
 *  xor	    1	0 (~src)    1   0 (~src)    1	1 (1)	    1	1 (1)
 *
 *  or	    1	1 (1)	    1	0 (~src)    1	0 (~src)    1	1 (1)
 *  xor	    0	1 (src)	    0	0 (0)	    0	1 (src)	    0	0 (0)
 */

int
cfbReduceRasterOp (rop, fg, pm, andp, xorp)
    int		    rop;
    unsigned long   fg, pm;
    unsigned long   *andp, *xorp;
{
    unsigned long   and, xor;
    int		    rrop;

    fg = PFILL (fg);
    pm = PFILL (pm);
    switch (rop)
    {
    case GXclear:
    	and = 0;
    	xor = 0;
	break;
    case GXand:
	and = fg;
	xor = 0;
	break;
    case GXandReverse:
	and = fg;
	xor = fg;
	break;
    case GXcopy:
	and = 0;
	xor = fg;
	break;
    case GXandInverted:
	and = ~fg;
	xor = 0;
	break;
    case GXnoop:
	and = ~0;
	xor = 0;
	break;
    case GXxor:
	and = ~0;
	xor = fg;
	break;
    case GXor:
	and = ~fg;
	xor = fg;
	break;
    case GXnor:
	and = ~fg;
	xor = ~fg;
	break;
    case GXequiv:
	and = ~0;
	xor = ~fg;
	break;
    case GXinvert:
	and = ~0;
	xor = ~0;
	break;
    case GXorReverse:
	and = ~fg;
	xor = ~0;
	break;
    case GXcopyInverted:
	and = 0;
	xor = ~fg;
	break;
    case GXorInverted:
	and = fg;
	xor = ~fg;
	break;
    case GXnand:
	and = fg;
	xor = ~0;
	break;
    case GXset:
	and = 0;
	xor = ~0;
	break;
    }
    and |= ~pm;
    xor &= pm;
    *andp = and;
    *xorp = xor;
    /* check for some special cases to reduce computation */
    if (and == 0)
	rrop = GXcopy;
    else if (and == ~0)
	rrop = GXxor;
    else if (xor == 0)
	rrop = GXand;
    else if (and | xor == ~0)
	rrop = GXor;
    else if (and == ~0 && xor == 0)
	rrop = GXnoop;
    else
	rrop = GXset;   /* rop not reduced */
    return rrop;
}
