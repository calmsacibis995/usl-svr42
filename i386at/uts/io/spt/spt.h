/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_SPT_SPT_H	/* wrapper symbol for kernel use */
#define _IO_SPT_SPT_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/spt/spt.h	1.3"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */
/*
 *	Copyright (C) The Santa Cruz Operation, 1984-1989.
 *	Copyright (C) Microsoft Corporation, 1984-1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*
 *	defines used by spt.c and vtpsrv.c to control blocking extention
 *	to the tty standard ioctl cmds, and to define the flags for the
 *	mptflags structure.
 * 
 */

/*
 * flags
 */
#define MPT_OPEN	0x0001
#define MPT_BLK_FLAG	0x0002

/*
 * added ioctl cmd
 */
#define MPT_BLK		( 0xff00 | 'x' )		/* S000 */

struct pt_ioctl {					/* S001 vvv */
	int	pt_flags;
	struct proc *pt_selr, *pt_selw;
};

#define PF_RCOLL	0x1
#define PF_WCOLL	0x2				/* S001 ^^^ */

							/* BEGIN SCO_INTL */
/*
 *	xmap structure - provides a per-tty structure for recording
 *	state information by emap and nmap routines.  Also used by
 *	select to avoid cluttering up tty structure.
 */

struct xmap {
	/*
	 *	select fields
	 */
	struct proc *	xm_selrd;	/* Process waiting on selwait (read) */
	struct proc *	xm_selwr;	/* Process waiting on selwait (write)*/
};
							/* END SCO_INTL */

/* End Enhanced Application Compatibility Support */

#endif /* _IO_SPT_SPT_H */
