/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/lmul.c	1.3.3.2"
#ident	"$Header: $"

#include <util/dl.h>

/*
 * Double long multiplication.
 * It multiplies lop and rop, and 
 * returns the result.
 * 
 * Traverse each of the DL_BITS bits in rop.
 * If current bit of rop is 0, skip that bit.
 * Otherwise, multiply lop by 2 to the power of the current
 * bit position and add the product to ans. Process the next
 * bit as described, until DL_BITS bits have been scanned.
 */

static dl_t dl_zero = { 0 };

dl_t
lmul(lop, rop)
dl_t	lop;
dl_t	rop;
{
	dl_t		ans;
	dl_t		tmp;
	register int	jj;

	ans = dl_zero;

	for (jj = 0; jj <= DL_BITS-1; jj++) {
		if ((lshiftl(rop, -jj).dl_lop & 1) == 0)
			continue;
		tmp = lshiftl(lop, jj);
		tmp.dl_hop &= 0x7fffffff;
		ans = ladd(ans, tmp);
	};

	return ans;
}
