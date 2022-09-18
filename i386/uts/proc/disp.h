/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_DISP_H	/* wrapper symbol for kernel use */
#define _PROC_DISP_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/disp.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _PROC_PRIOCNTL_H
#include <proc/priocntl.h> /* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/priocntl.h>	/* SVR4.0COMPAT */

#else

#include <sys/priocntl.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * The following is the format of a dispatcher queue entry.
 */
typedef struct dispq {
	struct proc	*dq_first;	/* first proc on queue or NULL */
	struct proc	*dq_last;	/* last proc on queue or NULL */
	int		dq_sruncnt;	/* no. of loaded, runnable procs on queue */
} dispq_t;


#ifdef _KERNEL

/*
 * Global scheduling variables.
 */
extern int	runrun;		/* preemption flag */
extern int	kprunrun;	/* kernel preemption flag */
extern int	npwakecnt;	/* count of non-preemptive wakeups */
extern struct proc *curproc;	/* currently running process */
extern int	curpri;		/* priority of current process */
extern int	maxrunpri;	/* priority of highest priority active queue */


/*
 * Public scheduling functions.
 */
#if defined(__STDC__)

#ifdef _KERNEL_HEADERS

#ifndef _PROC_PROC_H
#include <proc/proc.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/proc.h>		/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

extern boolean_t	dispdeq(proc_t *pp);
extern int	getcid(char *clname, id_t *cidp);
extern int	parmsin(pcparms_t *parmsp, proc_t *reqpp, proc_t *targpp);
extern int	parmsout(pcparms_t *parmsp, proc_t *reqpp, proc_t *targpp);
extern int	parmsset(pcparms_t *parmsp, proc_t *reqpp, proc_t *targpp);
extern void	dispinit();
extern void	getglobpri(pcparms_t *parmsp, int *globprip);
extern void	parmsget(proc_t *pp, pcparms_t *parmsp);
extern void	preempt();
extern void	setbackdq(proc_t *pp);
extern void	setfrontdq(proc_t *pp);
#ifndef KPERF
extern void	swtch();
#endif /* KPERF */
extern void	dq_sruninc(int pri);
extern void	dq_srundec(int pri);

#else

extern boolean_t	dispdeq();
extern int	getcid();
extern int	parmsin();
extern int	parmsout();
extern int	parmsset();
extern void	dispinit();
extern void	getglobpri();
extern void	parmsget();
extern void	preempt();
extern void	setbackdq();
extern void	setfrontdq();
#ifndef KPERF
extern void	swtch();
#endif /* KPERF */
extern void	dq_sruninc();
extern void	dq_srundec();

#endif	/* __STDC__ */


#ifdef	KPERF

#define PREEMPT() \
{ \
	if (kprunrun != 0) { \
		preempt(); \
	} \
	else if (kpftraceflg) { \
		PC_TO_KPC \
		kperf_write(KPT_PREEMPT,Kpc,curproc); \
	} \
}

#else	/* !KPERF */

#define	PREEMPT()	if (kprunrun != 0){ \
				preempt(); \
			}

#endif	/* KPERF */

#endif	/* _KERNEL */

/*
 * Parameter in number of pages.
 * This is a threshold of as->a_rss used in sched to keep
 * a process out a little longer when swapped out.
 */
#define SWPTHRESH (0x100000 / MMU_PAGESIZE)

#endif	/* _PROC_DISP_H */
