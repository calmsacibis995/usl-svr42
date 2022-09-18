/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_FC_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_FC_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/class/fc.h	1.2"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * time-sharing dispatcher parameter table entry
 */
typedef struct fcdpent {
	int	fc_globpri;	/* global (class independent) priority */
	long	fc_quantum;	/* time quantum given to procs at this level */

} fcdpent_t;


/*
 * time-sharing class specific proc structure
 */
typedef struct fcproc {
	long	fc_timeleft;	/* time remaining in procs quantum */
	short	fc_cpupri;	/* system controlled component of fc_umdpri */
	short	fc_uprilim;	/* user priority limit */
	short	fc_upri;	/* user priority */
	short	fc_umdpri;	/* user mode priority within fc class */
	char	fc_nice;	/* nice value for compatibility */
	unsigned char fc_flags;	/* flags defined below */
	short	fc_dispwait;	/* number of wall clock seconds since start */
				/*   of quantum (not reset upon preemption) */
	struct proc *fc_procp;	/* pointer to proc table entry */
	char	*fc_pstatp;	/* pointer to p_stat */
	int	*fc_pprip;	/* pointer to p_pri */
	uint	*fc_pflagp;	/* pointer to p_flag */
	struct fcproc *fc_next;	/* link to next fcproc on list */
	struct fcproc *fc_prev;	/* link to previous fcproc on list */
} fcproc_t;


/* flags */
#define	FCKPRI	0x01		/* proc at kernel mode priority */
#define	FCBACKQ	0x02		/* proc goes to back of disp q when preempted */
#define FCFORK	0x04		/* proc has forked, do not restore full quantum */

#endif	/* _PROC_CLASS_FC_H */
