/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/timecalls.c	1.5"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/hrtcntl.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/types.h>

/*
 * time system call - return the system time.
 */

struct gtimea {
	int a;
};

/* ARGSUSED */
int
gtime(uap, rvp)
	struct gtimea *uap;
	rval_t *rvp;
{
	rvp->r_time = hrestime.tv_sec;
	return 0;
}

/*
 * stime system call - set the system time.
 */

struct stimea {
	time_t	time;
};

/* ARGSUSED */
int
stime(uap, rvp)
	register struct stimea *uap;
	rval_t *rvp;
{
	extern int rf_stime();
	extern void settime();

	if (pm_denied(u.u_cred, P_SYSOPS))
		return EPERM;

	settime(uap->time);
	wtodc();
	(void)rf_stime(u.u_cred);	/* RFS */
	return 0;
}

/*
 * adjtime system call - adjust the system time.
 */

struct adjtimea {
	struct timeval *delta;
	struct timeval *olddelta;
};

/* ARGSUSED */
int
adjtime(uap, rvp)
	register struct adjtimea *uap;
	rval_t *rvp;
{
	register long	previous;	/* uncompleted previous adjustment */
	struct timeval	tv;

	if (pm_denied(u.u_cred, P_SYSOPS))
		return EPERM;

	if (copyin((caddr_t)uap->delta, (caddr_t)&tv, sizeof tv))
		return EFAULT;

	previous = clockadj(tv.tv_sec * MICROSEC + tv.tv_usec);

	if (uap->olddelta) {
		tv.tv_sec = previous / MICROSEC;
		tv.tv_usec = previous % MICROSEC;

		if (copyout((caddr_t)&tv, (caddr_t)uap->olddelta, sizeof tv))
			return EFAULT;
	}

	return 0;
}

/*
 * Set the system's idea of the time.
 * For backward compatibility we maintain the old "time"
 * variable in addition to the new high-resolution version.
 */

void
settime(t)
time_t t;
{
	extern time_t time;		/* time in seconds since 1970 */

	hrestime.tv_sec = time = t;
	hrestime.tv_nsec = 0;
}
