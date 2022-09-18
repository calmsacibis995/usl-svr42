/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:util/fpmdep.c	1.2"
#ident	"$Header: $"

/*
** fp_intr
**      handle a processor extension error interrupt on the AT386
**
**      this comes in on line 5 of the slave PIC at SPL1
*/

#include	<util/fp.h>
#include	<util/weitek.h>

fp_intr()
{
	oem_fclex();		/* Clear NDP BUSY latch */

#ifdef WEITEK
	if (weitek_kind != WEITEK_NO) {
		/*
		 * wtl 1167 and 80387 errors are or'd and the result
		 * is sent to the PIC.  therefore, we
		 * need to check whether this interrupt is from
		 * weitek or 387
		 * we'll do this by looking at the 387 status reg.
		 */
		int stat387;

		if (fp_kind == FP_NO) {
			/* with no 387 support, assume weitek */
			weitek_reset_intr();
			weitintr(0);
			return;
		}
		stat387 = get87();
		if ((stat387 & FPS_ES) == 0) {	/* no 387 error */
			weitek_reset_intr();
			weitintr(0);
			return;
		}
	}
#endif
	fpexterrflt();
}

