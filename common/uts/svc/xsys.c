/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)uts-comm:svc/xsys.c	1.6.3.3"
#ident	"$Header: $"

/* XENIX source support */

#include <util/types.h>
#include <proc/proc.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/time.h>
#include <svc/timeb.h>
#include <svc/proctl.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/inline.h>
#include <util/spl.h>

extern int Dstflag;	/* daylight savings time flag, defined in kernel.cf */
extern int Timezone;	/* local time zone, defined in kernel.cf */

/*
 *	Nap for the specified number of milliseconds.
 */
struct napa {
	long msec;
};

nap(uap, rvp)
	struct napa *uap;
	rval_t *rvp;
{
	clock_t fst = 0, lst;
	long togo;
	int ospl;

	/* Make sure no clock interrupt between reading time and lbolt.
	 * spl is not sufficient here, since we could be running on
	 * a 'slave' cpu, and the master could take a clock interrupt.
	 * We therefore check lbolt twice and make sure it is the same.
	 */
	while(fst != lbolt) 
		fst = lbolt;

	/* preclude overflow */
	if (uap->msec >= LONG_MAX/timer_resolution || uap->msec < 0)
		return EINVAL;
	/* togo gets time to nap in ticks */
	if ((togo = uap->msec * timer_resolution / 1000) < 0)
		return EINVAL;
	
	lst = fst + togo;

	/* nap, return time napped */
	while(togo > 0) {	/* now handle short part of nap */
		ospl = splhi();
		(void)timeout(wakeup, (caddr_t)u.u_procp, togo);
		(void)sleep((caddr_t)u.u_procp, PSLEP);
		splx(ospl);
		togo = lst - lbolt; 
	}
	rvp->r_time = (lbolt-fst) * 1000L / timer_resolution ;
	return 0;
}

/*
 * Return TOD with milliseconds, timezone, DST flag
 */
struct ftimea {
	struct timeb *tp;
};

/* ARGSUSED */
ftime(uap, rvp)
	struct ftimea *uap;
	rval_t *rvp;
{
	struct timeb t;
	register unsigned ms = 0;

	/* 
	 * The meaning of lticks changed in 5.3. Used
	 * to be the number of ticks until next second; is now
	 * a rescheduling variable. This has been changed to use
	 * lbolt, which is the total time accumulation since startup
	 * in ticks.
	 * Make sure no clock interrupt between reading time and lbolt,
	 * spl is not sufficient here, since we could be running on
	 * a 'slave' cpu, and the master could take a clock interrupt.
	 * We therefore check lbolt twice and make sure it is the same.
	 */
	while(ms != lbolt) {
		ms = lbolt;
		t.time = hrestime.tv_sec;
	}

	/* new calculation using lbolt.	*/
	t.millitm = (unsigned) (ms % timer_resolution)*(1000/timer_resolution);
	t.timezone = Timezone;
	t.dstflag = Dstflag;
	if (copyout((caddr_t) &t, (caddr_t)uap->tp, sizeof(t)) == -1)
		return EFAULT;
	return 0;
}


/*
 * proctl system call (process control)
 */
struct proctla {
	pid_t	pid;
	int	cmd;
	char	*arg;
};

/* ARGSUSED */
proctl(uap, rvp)
	struct proctla *uap;
	rval_t *rvp;
{
	register struct proc *p;
	register pid_t pid;
	int found = 0;

	pid = uap->pid;

	for (p = practive; p != NULL; p = p->p_next) {
		if (pid > 0) {
			if (p->p_pid != pid)
				continue;
		} else if (p == proc_init)
			continue;
		if (pid == 0 && p->p_pgidp != u.u_procp->p_pgidp)
			continue;
		if (pid < -1 && p->p_pgrp != -pid)
			continue;
		if (!hasprocperm(p->p_cred, u.u_cred)) {
			if (pid > 0)
				return EPERM;
			else
				continue;
		}
		found++;
		switch(uap->cmd) {
			case PRHUGEX:
			case PRNORMEX:
				/*
				 * A no-op.  Cannot really maintain backwards
				 * compatibility here without adding an
				 * unused field to the user struct, so
				 * we'll have to punt.
				 */
				break;
			default:
				return EINVAL;
		}
		if (pid > 0)
			break;
	}

	if (found == 0)
		return ESRCH;
	return 0;
}

/* End XENIX Support */
