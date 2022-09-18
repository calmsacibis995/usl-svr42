/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/ldivide.c	1.3.3.2"
#ident	"$Header: $"

#include <util/dl.h>

/* 
 * Double long division.
 */

static dl_t dl_zero = { 0 };

dl_t
ldivide(lop, rop)
dl_t	lop;	/* dividend */
dl_t	rop;	/* divisor */
{
	register int	cnt;
	dl_t		ans; 	/* quotient */
	dl_t		tmp;
	dl_t		div;	/* remainder */

	/* if arguments are negative, convert them to positive numbers */
	if (lsign(lop))
		lop = lsub(dl_zero, lop);
	if (lsign(rop))
		rop = lsub(dl_zero, rop);
	
	ans = dl_zero;
	div = dl_zero;

	for (cnt = 0; cnt < DL_BITS-1; cnt++) {
		div = lshiftl(div, 1);
		lop = lshiftl(lop, 1);
		if (lsign(lop))
			div.dl_lop |= 1;
		tmp = lsub(div, rop);
		ans = lshiftl(ans, 1);
		if (lsign(tmp) == 0) {
			ans.dl_lop |= 1;
			div = tmp;
		}
	}

	return ans;
}
