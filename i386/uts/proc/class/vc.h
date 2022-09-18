/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_VC_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_VC_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/class/vc.h	1.2"
#ident	"$Header: $"

/*
 * VP/ix process dispatcher parameter table entry
 */
typedef struct vcdpent {
	int	vc_globpri;	/* global (class independent) priority */
	long	vc_quantum;	/* time quantum given to procs at this level */
	short	vc_tqexp;	/* vc_umdpri assigned when proc at this level */
				/*   exceeds its time quantum */
	short	vc_slpret;	/* vc_umdpri assigned when proc at this level */
				/*  returns to user mode after sleeping */
	short	vc_maxwait;	/* bumped to vc_lwait if more than vc_maxwait */
				/*  secs elapse before receiving full quantum */
	short	vc_lwait;	/* vc_umdpri assigned if vc_dispwait exceeds  */
				/*  vc_maxwait */				
} vcdpent_t;


/*
 * VP/ix class specific proc structure
 */
typedef struct vcproc {
	long	vc_timeleft;	/* time remaining in procs quantum */
	short	vc_cpupri;	/* system controlled component of vc_umdpri */
	short	vc_uprilim;	/* user priority limit */
	short	vc_upri;	/* user priority */
	short	vc_umdpri;	/* user mode priority within vc class */
	char	vc_nice;	/* nice value for compatibility */
	unsigned char vc_flags;	/* flags defined below */
	short	vc_dispwait;	/* number of wall clock seconds since start */
				/*   of quantum (not reset upon preemption) */
	struct proc *vc_procp;	/* pointer to proc table entry */
	char	*vc_pstatp;	/* pointer to p_stat */
	int	*vc_pprip;	/* pointer to p_pri */
	uint	*vc_pflagp;	/* pointer to p_flag */
	struct vcproc *vc_next;	/* link to next vcproc on list */
	struct vcproc *vc_prev;	/* link to previous vcproc on list */
} vcproc_t;


/* flags */
#define	VCKPRI	0x01		/* proc at kernel mode priority */
#define	VCBACKQ	0x02		/* proc goes to back of disp q when preempted */
#define	VCFORK	0x04		/* proc has forked, so don't reset full quantum */
#define VCBOOST	0x08		/* Process priority was boosted */
#define VCBSYWT 0x10		/* Process is busywaiting */

#endif	/* _PROC_CLASS_VC_H */
