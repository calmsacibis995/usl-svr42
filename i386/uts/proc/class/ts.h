/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_TS_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_TS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/class/ts.h	1.5"
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
typedef struct tsdpent {
	int	ts_globpri;	/* global (class independent) priority */
	long	ts_quantum;	/* time quantum given to procs at this level */
	short	ts_tqexp;	/* ts_umdpri assigned when proc at this level */
				/*   exceeds its time quantum */
	short	ts_slpret;	/* ts_umdpri assigned when proc at this level */
				/*  returns to user mode after sleeping */
	short	ts_maxwait;	/* bumped to ts_lwait if more than ts_maxwait */
				/*  secs elapse before receiving full quantum */
	short	ts_lwait;	/* ts_umdpri assigned if ts_dispwait exceeds  */
				/*  ts_maxwait */				
} tsdpent_t;


/*
 * time-sharing class specific proc structure
 */
typedef struct tsproc {
	long	ts_timeleft;	/* time remaining in procs quantum */
	short	ts_cpupri;	/* system controlled component of ts_umdpri */
	short	ts_uprilim;	/* user priority limit */
	short	ts_upri;	/* user priority */
	short	ts_umdpri;	/* user mode priority within ts class */
	char	ts_nice;	/* nice value for compatibility */
	unsigned char ts_flags;	/* flags defined below */
	short	ts_dispwait;	/* number of wall clock seconds since start */
				/*   of quantum (not reset upon preemption) */
	struct proc *ts_procp;	/* pointer to proc table entry */
	char	*ts_pstatp;	/* pointer to p_stat */
	int	*ts_pprip;	/* pointer to p_pri */
	uint	*ts_pflagp;	/* pointer to p_flag */
	struct tsproc *ts_next;	/* link to next tsproc on list */
	struct tsproc *ts_prev;	/* link to previous tsproc on list */
        int     ts_sleepwait;   /* accumulated sleep time */
} tsproc_t;


/* flags */
#define	TSKPRI	0x01		/* proc at kernel mode priority */
#define	TSBACKQ	0x02		/* proc goes to back of disp q when preempted */
#define TSFORK	0x04		/* proc has forked, do not restore full quantum */

/* 
 * Tuning functions, should eventually be made tunable parameters.
 * SLEEPWAIT of 4 implies you must sleep at least 1/2 second to get
 * priority boost. SLEEPMAX of 60 means we only keep track
 * of sleeptime up to about 7 1/2 seconds.
 */

#define TS_SLEEPWAIT  4  /* reward the proc sleeping at least this amount */
#define TS_SLEEPMAX   60 /* max out credited sleep time at this amount */

#endif	/* _PROC_CLASS_TS_H */
