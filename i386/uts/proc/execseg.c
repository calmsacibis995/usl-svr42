/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/execseg.c	1.1"
#ident	"$Header: $"

/* XENIX source support */

#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/systm.h>

/*
 * execseg system call
 *
 *	This system call returns a code selector pointing to the 
 *	memory region mapped by the user's data selector. This allows
 *	data segments to be executed.
 */

int
execseg(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	struct dscr *dscrp;

	/* Setup alias for 386 Data Selector */
	dscrp = (struct dscr *)(u.u_procp->p_ldt) + seltoi(CSALIAS_SEL);
	*dscrp = *((struct dscr *)(u.u_procp->p_ldt) + seltoi(USER_DS));
	dscrp->a_acc0007 = (unsigned char) UTEXT_ACC1;

	/* flag the process as having a modified LDT */
	u.u_ldtmodified = 1;
	
	/*
	 * Argument is Returned as a Far Pointer 
	 */
	rvp->r_val1 = 0;
	rvp->r_val2 = CSALIAS_SEL;
	return 0;
}

/*
 * unexecseg system call
 *
 * An alias selector is invalidated.
 */

int
unexecseg(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	struct dscr *dscrp;

	dscrp = (struct dscr *)(u.u_procp->p_ldt) + seltoi(CSALIAS_SEL);
	((unsigned int *)dscrp)[0] = 0;
	((unsigned int *)dscrp)[1] = 0;

	/* flag the process as having a modified LDT */
	u.u_ldtmodified = 1;
	return 0;
}
