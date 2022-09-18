/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/scalls.c	1.10"
#ident	"$Header: $"

/*
 * This file contains entry points for several system calls found in
 * Section 2 of the "Programmer's Reference Manual". All system calls
 * in this file are related to process attributes.
 *
 * There are several lines of code related to the UID and GID elements
 * in the "proc" structure that are required for compatibility with
 * previous releases.
 */

#include <acc/audit/auditmod.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/statvfs.h>
#include <fs/ustat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/ucontext.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/hrtcntl.h>
#include <svc/resource.h>
#include <svc/sysconfig.h>
#include <svc/systeminfo.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/uadmin.h>
#include <svc/ulimit.h>
#include <svc/utsname.h>
#include <svc/utssys.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
#include <svc/times.h>

/* Enhanced Application Compatibility Support */
#include <io/termios.h>
#include <svc/sco.h>
/* End Enhanced Application Compatibility Support */

/*
 * setuid system call - set the real user ID for the calling process.
 */

struct setuida {
	uid_t	uid;
};

/* ARGSUSED */
int
setuid(uap, rvp)
	register struct setuida *uap;
	rval_t *rvp;
{
	register uid_t uid;
	extern void pm_recalc();
	int error = 0;

	/*
	 * Determine if the argument passed is within the
	 * valid ranges for a user ID.
	 */
	if ((uid = uap->uid) > MAXUID || uid < (uid_t) 0)
		return EINVAL;
	/* XENIX Support */
	if (BADVISE_PRE_SV) {
		/* Perform the old System III and V7 code, for pre-System V
		 * XENIX binaries.
		 */
		if (u.u_cred->cr_ruid == uid || (pm_denied(u.u_cred, P_SETUID) == 0)) {
			u.u_cred = crcopy(u.u_cred);
			u.u_cred->cr_uid = uid;
			u.u_cred->cr_ruid = uid;
			u.u_procp->p_uid = (o_uid_t) uid;		/* XXX */
		}
	}
	/* End XENIX Support */
	else { 

	/*
 	* If this process has the privilege to change the
 	* user ID, do it now without regard to the user's
 	* other attributes.
 	*/
	if ((error = pm_denied(u.u_cred, P_SETUID)) == 0) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_uid = uid;
		u.u_cred->cr_ruid = uid;
		u.u_cred->cr_suid = uid;

		if (uid < USHRT_MAX)
			u.u_uid = (o_uid_t) uid;
		else
			u.u_uid = (o_uid_t) UID_NOBODY;
	
		u.u_procp->p_uid = (o_uid_t) uid;	/* compatibility */
	}
	else if (uid == u.u_cred->cr_ruid || uid == u.u_cred->cr_suid) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_uid = uid;
		if (uid < USHRT_MAX)
			u.u_uid = (o_uid_t) uid;
		else
			u.u_uid = (o_uid_t) UID_NOBODY;

		error = 0;
	}
	/* XENIX Support */
	}
	/* End XENIX Support */
	if (!error) {		/* The uid was modified */
		/*
		 * pm_recalc() is only effective in an
		 * ID-based privilege mechanism.
		 */
		pm_recalc(u.u_cred);
		ADT_CHGBAS(u.u_procp);		/* audit this system call */
	}
	return error;
}


/*
 * getuid system call - get the real user ID for the calling process.
 */

/* ARGSUSED */
int
getuid(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	rvp->r_val1 = (int) u.u_cred->cr_ruid;
	rvp->r_val2 = (int) u.u_cred->cr_uid;
	return 0;
}


/*
 * setgid system call - set the real group ID for the calling process.
 */

struct setgida {
	gid_t	gid;
};

/* ARGSUSED */
int
setgid(uap, rvp)
	register struct setgida *uap;
	rval_t *rvp;
{
	register gid_t gid;
	int error = 0;

	/*
	 * Determine if the argument passed is within the
	 * valid ranges for a group ID.
	 */
	if ((gid = uap->gid) > MAXUID || gid < (gid_t) 0)
		return EINVAL;

	/* XENIX Support */
	if (BADVISE_PRE_SV) {
		/* Perform the old System III and V7 code, for pre-System V
		 * XENIX binaries.
		 */
		if (u.u_cred->cr_rgid == gid || (pm_denied(u.u_cred, P_SETUID) == 0)) {
			u.u_cred = crcopy(u.u_cred);
			u.u_cred->cr_gid = gid;
			u.u_cred->cr_rgid = gid;
		}
	}
	else {
	/* End XENIX Support */

	/*
	 * If this process has the privilege to change the
	 * group ID, do it now without regard to the user's
	 * other attributes.
	 */
	if ((error = pm_denied(u.u_cred, P_SETUID)) == 0) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_gid = gid;
		u.u_cred->cr_rgid = gid;
		u.u_cred->cr_sgid = gid;

		if (gid < USHRT_MAX)
			u.u_gid = (o_gid_t) gid;	/* compatibility */
		else
			u.u_gid = (o_gid_t) UID_NOBODY;

		ADT_CHGBAS(u.u_procp);		/* audit this system call */
	}
	else if (gid == u.u_cred->cr_rgid || gid == u.u_cred->cr_sgid) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_gid = gid;

		if (gid < USHRT_MAX)
			u.u_gid = (o_gid_t) gid;	/* compatibility */
		else
			u.u_gid = (o_gid_t) UID_NOBODY;

		error = 0;
		ADT_CHGBAS(u.u_procp);		/* audit this system call */
	}
	/* XENIX Support */
	}
	/* End XENIX Support */
	return error;
}


/*
 * getgid system call - get the real group ID for the calling process.
 */

/* ARGSUSED */
int
getgid(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	rvp->r_val1 = u.u_cred->cr_rgid;
	rvp->r_val2 = u.u_cred->cr_gid;
	return 0;
}


/*
 * getpid system call - get the process ID.
 */

/* ARGSUSED */
int
getpid(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	rvp->r_val1 = u.u_procp->p_pid;
	rvp->r_val2 = u.u_procp->p_ppid;
	return 0;
}


/*
 * seteuid system call - set the effective user ID for the calling process.
 */

struct seteuida {
	int	uid;
};

/* ARGSUSED */
int
seteuid(uap, rvp)
	register struct seteuida *uap;
	rval_t *rvp;
{
	register unsigned uid;
	extern void pm_recalc();
	int error = 0;

	/*
	 * Determine if the argument passed is within the
	 * valid ranges for a user ID.
	 */
	if ((uid = uap->uid) > MAXUID || uid < (uid_t) 0)
		error = EINVAL;

	else if (uid == u.u_cred->cr_ruid 
	  || uid == u.u_cred->cr_uid
	  || uid == u.u_cred->cr_suid
	  || !pm_denied(u.u_cred, P_SETUID)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_uid = uid;

		if (uid < USHRT_MAX)
			u.u_uid = (o_uid_t) uid;
		else
			u.u_uid = (o_uid_t) UID_NOBODY;

		/*
		 * pm_recalc() is only effective in an
		 * ID-based privilege mechanism.
		 */
		pm_recalc(u.u_cred);
		ADT_CHGBAS(u.u_procp);		/* audit this system call */
	} else
		error = EPERM;

	return error;
}


/*
 * setegid system call - set the effective group ID for the calling process.
 */

struct setegida {
	int	gid;
};

/* ARGSUSED */
int
setegid(uap, rvp)
	register struct setegida *uap;
	rval_t *rvp;
{
	register unsigned gid;
	int error = 0;

	/*
	 * Determine if the argument passed is within the
	 * valid ranges for a group ID.
	 */
	if ((gid = uap->gid) > MAXUID || gid < (gid_t) 0)
		error = EINVAL;

	else if (gid == u.u_cred->cr_rgid 
	  || gid == u.u_cred->cr_gid
	  || gid == u.u_cred->cr_sgid
	  || !pm_denied(u.u_cred, P_SETUID)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_gid = gid;


		if (gid < USHRT_MAX)
			u.u_gid = (o_gid_t) gid;	/* compatibility */
		else
			u.u_gid = (o_gid_t) UID_NOBODY;

		ADT_CHGBAS(u.u_procp);		/* audit this system call */
	} else
		error = EPERM;

	return error;
}


/*
 * setgroups system call - set the supplementary group access lists IDs.
 *
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function
 */

struct setgroupsa {
	u_int	gidsetsize;
	gid_t	*gidset;
};

/* ARGSUSED */
int
setgroups(uap, rvp)
	register struct setgroupsa *uap;
	rval_t *rvp;
{
	register u_short i;
	register cred_t *newcr;
	register cred_t *cr = u.u_cred;
	register u_short n = uap->gidsetsize;

	/*
	 * Return an error if the calling process is not
	 * privileged to set the supplementary group IDs.
	 */
	if (pm_denied(cr, P_SETUID))
		return EPERM;

	if (n > (u_short)ngroups_max)
		return EINVAL;

	/*
	 * Make changes to a "working copy" of the credential structure.
	 */
	newcr = crdup(u.u_cred);

	/*
	 * A value other than 0 for "gidsetsize" means that the user supplied
	 * a list of supplemental group IDs. A value of 0 means to remove the
	 * supplemental group IDs for the calling process.
	 */
	if (n != 0
	  && copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    n * sizeof(gid_t))) {
		crfree(newcr);
		return EFAULT;
	}
	for (i = 0; i < n; i++) {
		/*
		 * Check the value for each group specified.  Return an error
		 * if any of the groups is not valid.
		 */
		if (newcr->cr_groups[i] < 0 || newcr->cr_groups[i] > MAXUID) {
			crfree(newcr);
			return EINVAL;
		}
	}

	newcr->cr_ngroups = n;

	/* 
	 * Make the credential structure of the calling process the
	 * same as the "working copy".
	 */
	u.u_cred = newcr;

	crfree(cr);		/* free the "working copy" */

	ADT_CHGGRP(u.u_procp);		/* audit this system call */

	return 0;
}


/*
 * getgroups system call - get the supplementary group access lists IDs.
 *
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function
 */
struct getgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

int
getgroups(uap, rvp)
	register struct getgroupsa *uap;
	rval_t *rvp;
{
	register struct cred *cr = u.u_cred;
	register gid_t n = cr->cr_ngroups;

	/*
	 * Return the list of supplemental group IDs if the "gidsetsize"
	 * is other than 0.
	 */
	if (uap->gidsetsize != 0) {
		if ((gid_t)uap->gidsetsize < n)
			return EINVAL;

		if (copyout((caddr_t)cr->cr_groups, (caddr_t)uap->gidset, 
		  n * sizeof(gid_t)))
			return EFAULT;
	}
	/*
	 * Set the return value to the number of supplemental group IDs
	 * for the calling process.
	 */
	rvp->r_val1 = n;
	return 0;
}


/*
 * setpgrp call - single entry point for process and session IDs system calls.
 */

struct setpgrpa {
	int	flag;
	int	pid;
	int	pgid;
};


/* Enhanced Application Compatibility Support */
struct setpgrpa_sco {
	int	flag;
	int	(*retaddr)();
	int	pid;
	int	pgid;
};
/* End Enhanced Application Compatibility Support */

/* ARGSUSED */
int
setpgrp(uap, rvp)
	register struct setpgrpa *uap;
	rval_t *rvp;
{
	register proc_t *p = u.u_procp;
	struct setpgrpa_sco *scoap;

	/* Enhanced Application Compatibility Support */
	if (IS_SCOEXEC) {
		/* Must be careful to overlay the first argument first */
		/* to avoid clobbering the next arugment in the array*/
		scoap = (struct setpgrpa_sco *)uap;
		uap->pid = scoap->pid;
		uap->pgid = scoap->pgid;

		/* Translate SCO flag to SVR4 flag */
		switch (uap->flag) {
		case	0:	/* SCO_GETPGRP */
		case	1:	/* SCO_SETPGRP */
		case	3:	/* SCO_SETSID */
			break;
		case	2:	/* SCO_SETPGID	*/
			uap->flag = 5;
			break;
		default:
			return (ENOSYS);
		}
	}
	/* End Enhanced Application Compatibility Support */

	switch (uap->flag) {

	case 1: /* setpgrp() */
		if (p->p_sessp->s_sidp != p->p_pidp && !pgmembers(p->p_pid))
			sess_create();

		rvp->r_val1 = p->p_sessp->s_sid;
		ADT_CHGBAS(u.u_procp);		/* audit this system call */
		return 0;

	case 3: /* setsid() */
		if (p->p_pgidp == p->p_pidp || pgmembers(p->p_pid))
			return EPERM;

		sess_create();
		rvp->r_val1 = p->p_sessp->s_sid;
		ADT_CHGBAS(u.u_procp);		/* audit this system call */
		return 0;

	case 5: /* setpgid() */
	{
		register pid_t pgid;
		register pid_t pid;
		register struct pid *opgidp = NULL;

		if ((pid = uap->pid) == 0)
			pid = p->p_pid;

		else if (pid < 0 || pid >= MAXPID)
			return EINVAL;

		else if (pid != p->p_pid) {
			/*
			 * search for specified pid argument
			 */
			for (p = p->p_child; ; p = p->p_sibling) {
				if (p == NULL)
					return ESRCH;

				if (p->p_pid == pid)
					break;	/* found */
			}
			if (p->p_flag & SEXECED)
				return EACCES;

			if (p->p_sessp != u.u_procp->p_sessp)
				return EPERM;	/* user doesn't own pid */

			/*
			 * Verify lid of current process equals lid
			 * of target process.
			 */
			if (MAC_ACCESS(MACEQUAL, u.u_cred->cr_lid,
				p->p_cred->cr_lid) &&
                            	pm_denied(u.u_cred, P_MACWRITE)) {

				if (MAC_ACCESS(MACDOM, u.u_cred->cr_lid,
					p->p_cred->cr_lid))
					return ESRCH;
				else
					return EACCES;
			}
		}
		if (p->p_sessp->s_sid == pid)
			return EPERM;

		if ((pgid = uap->pgid) == 0)
			pgid = p->p_pid;

		else if (pgid < 0 || pgid >= MAXPID)
			return EINVAL;

		/* pgexit() can free the pgidp */

		if (p->p_pgidp->pid_pglink != p)
			opgidp = p->p_pgidp;

		if (p->p_pgrp == pgid)
			break;

		else if (p->p_pid == pgid) {
			pgexit(p);
			/*
			 * Add this process to its parent's process group.
			 */
			pgjoin(p, p->p_pidp);
		} else {
			register proc_t *q;

			if ((q = pgfind(pgid)) == NULL 
			  || q->p_sessp != p->p_sessp)
				return EPERM;

			/*
			 * Verify that current process LID equals that
			 * of the new process group.
			 */

			if (MAC_ACCESS(MACEQUAL, u.u_cred->cr_lid,
				q->p_cred->cr_lid) &&
			     	pm_denied(u.u_cred, P_MACWRITE)) {

				if (MAC_ACCESS(MACDOM, u.u_cred->cr_lid,
				    q->p_cred->cr_lid))
					return ESRCH;
				else
					return EACCES;
			}
			pgexit(p);
			/*
			 * Add this process to the new process group.
			 */
			pgjoin(p, q->p_pgidp);
		}
		/* unorphan the process group */

		if (opgidp && opgidp->pid_pgorphaned == 1)
			unorphan(opgidp);

		/*
		 * wake up all process waiting on p->p_parent.
		 */
		wakeprocs((caddr_t)p->p_parent, PRMPT);
		break;
	}

	case 0: /* getpgrp() */
		rvp->r_val1 = p->p_pgrp;
		break;

	case 2: /* getsid() */
	case 4: /* getpgid() */
		if (uap->pid < 0 || uap->pid >= MAXPID)
			return EINVAL;

		else if (uap->pid != 0 
		  && p->p_pid != uap->pid
		  && (p = prfind(uap->pid)) == NULL)
			return ESRCH;

		if (MAC_ACCESS(MACDOM, u.u_cred->cr_lid, p->p_cred->cr_lid)
			&& pm_denied(u.u_cred, P_MACREAD))
			return ESRCH;

		if (uap->flag == 2)
			rvp->r_val1 = p->p_sessp->s_sid;
		else
			rvp->r_val1 = p->p_pgrp;

		break;
	}
	ADT_CHGBAS(u.u_procp);		/* audit this system call */
	return 0;
}


/*
 * pause system call -	suspend the calling process until it catches a signal.
 */

void
pause()
{
	/*
	 * Indefinite wait.  No one should call wakeup() or
	 * wakeprocs() with a chan of &u.
	 */
	for (;;)
		sleep((caddr_t)&u, PSLEP);

	/* NOTREACHED */
}


/*
 * times system call -	return system and user times.
 */

struct timesa {
	struct tms *times;
};

int
times(uap, rvp)
	register struct timesa *uap;
	rval_t *rvp;
{
	struct tms tms_tmp;

	tms_tmp.tms_utime=u.u_procp->p_utime;
	tms_tmp.tms_stime=u.u_procp->p_stime;
	tms_tmp.tms_cutime=u.u_procp->p_cutime;
	tms_tmp.tms_cstime=u.u_procp->p_cstime;

	if (copyout((caddr_t)&tms_tmp, (caddr_t) uap->times, sizeof(struct tms)))
		return EFAULT;

	spl7();
	/*
	 * the elapsed time in clock ticks per second from an
	 * arbitrary point in the past.
	 */
	rvp->r_time = lbolt;
	spl0();
	return 0;
}


/*
 * profil system call - provide execution time profile.
 */

struct profila {
	ushort	*bufbase;
	unsigned bufsize;
	unsigned pcoffset;
	unsigned pcscale;
};

/* ARGSUSED */
int
profil(uap, rvp)
	register struct profila *uap;
	rval_t *rvp;
{
	u.u_prof.pr_base = uap->bufbase;
	u.u_prof.pr_size = uap->bufsize;
	u.u_prof.pr_off = uap->pcoffset;
	u.u_prof.pr_scale = uap->pcscale;

	/*
 	 * for systrap performance improvements, u_systrap now is set
	 * to show profiling is set.
	 */
	u.u_systrap |= P_PROF;
	return 0;
}


/*
 * umask system call - get and set mode mask for creation of files.
 */

struct umaska {
	int	mask;
};

int
umask(uap, rvp)
	register struct umaska *uap;
	rval_t *rvp;
{
	register mode_t t;

	t = u.u_cmask;

	u.u_cmask = (mode_t) (uap->mask & PERMMASK);

	rvp->r_val1 = (int) t;
	return 0;
}


/*
 * rlimit - internal routine called by ulimit() and setrlimit() system calls.
 */
STATIC	int
rlimit(resource, softlimit, hardlimit)
	int resource;
	rlim_t softlimit, hardlimit;
{
	if (softlimit > hardlimit)
		return EINVAL;

	/*
	 * return an error if the user is trying to raise the maximum limit
	 * for the named resource and the calling process doesn't have the
	 * necessary privilege.
	 */
	if (hardlimit > u.u_rlimit[resource].rlim_max &&
	    pm_denied(u.u_cred, P_SYSOPS))
		return EPERM;

	u.u_rlimit[resource].rlim_cur = softlimit;
	u.u_rlimit[resource].rlim_max = hardlimit;

	return 0;
}

/* XENIX Support */
#define NIND		((long) (512 / sizeof(daddr_t)))
#define MAXBLOCKS	(10 + NIND + NIND*NIND + NIND*NIND*NIND)
/* End XENIX Support */

/*
 * ulimit system call - get and set user limits.
 *
 * ulimit could be moved into a user library, as calls to getrlimit
 * and setrlimit, were it not for binary compatibility restrictions.
 */

struct ulimita {
	int	cmd;
	long	arg;
};

int
ulimit(uap, rvp)
	register struct ulimita *uap;
	rval_t *rvp;
{
	switch (uap->cmd) {

	case UL_SFILLIM: /* Set new file size limit. */
	{
		register long	arg = uap->arg;
		register rlim_t	lim;
		register int	error;

		/* XENIX Support */
		/* For XENIX compatibility, artificially limit the 
		 * maximum number of 512-byte blocks in a file to the 
		 * maximum number of blocks that are representable in 
		 * a 512-byte block file system.  MAXBLOCKS is in 
		 * units of 512 bytes.
 		 */
		if (VIRTUAL_XOUT && arg > MAXBLOCKS)
			return EINVAL;
		/* End XENIX Support */

		if (arg < 0)
			return EINVAL;
		else if (arg >= (RLIM_INFINITY >> SCTRSHFT))
			lim = RLIM_INFINITY;
		else
			lim = arg << SCTRSHFT;

		if (error = rlimit(RLIMIT_FSIZE, lim, lim))
			return error;

		/* FALLTHROUGH */
	}

	case UL_GFILLIM: /* Return current file size limit. */
		rvp->r_off = (u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT);
		break;

	case UL_GMEMLIM: /* Return maximum possible break value. */
	{
		register struct seg *seg, *sseg;
		register struct seg *nextseg;
		register struct proc *p = u.u_procp;
		register struct as *as = p->p_as;
		register caddr_t brkend;
		uint size;

		brkend = (caddr_t) ((int)
			(p->p_brkbase + p->p_brksize + PAGEOFFSET) & PAGEMASK);
		/*
		 * Find the segment with a virtual address
		 * greater than the end of the current break.
		 */
		nextseg = NULL;
		sseg = seg = as->a_segs;
		if (seg != NULL) {
			do {
				if (seg->s_base >= brkend) {
					nextseg = seg;
					break;
				}
				seg = seg->s_next;
			} while (seg != sseg);
		}

		/*
		 * We reduce the max break value (base+rlimit[DATA]
		 * if we run into another segment, the ublock or
		 * the end of memory.  We also are limited by
		 * rlimit[VMEM].
		 */
		rvp->r_off = (off_t)
			(p->p_brkbase + u.u_rlimit[RLIMIT_DATA].rlim_cur);
		if (nextseg != NULL)
			rvp->r_off = min(rvp->r_off, (off_t)nextseg->s_base);

		if (brkend <= (caddr_t)UVUBLK)
			rvp->r_off = min(rvp->r_off, (off_t)UVUBLK);
		else
			rvp->r_off = min(rvp->r_off, (off_t)UVEND);

		/*
		 * Also handle case where rlimit[VMEM] has been 
		 * lowered below the current address space size.
		 */

		size = u.u_rlimit[RLIMIT_VMEM].rlim_cur & PAGEMASK;
		if (as->a_size < size)
			size -= as->a_size;
		else
			size = 0;

		rvp->r_off = min(rvp->r_off, (off_t)(brkend + size));
		break;
	}

	case UL_GDESLIM: /* Return approximate number of open files */
		rvp->r_off = u.u_rlimit[RLIMIT_NOFILE].rlim_cur;
		break;

	default:
		return EINVAL;

	}

	return 0;
}


/*
 * getrlimit system call - get system resource consumption limits.
 */

struct rlimita {
	int	resource;
	struct rlimit *rlp;
};

/* ARGSUSED */
int
getrlimit(uap, rvp)
	register struct rlimita *uap;
	rval_t *rvp;
{
	/*
	 * Determine if the argument passed is within the
	 * valid ranges for the requested resource.
	 */
	if (uap->resource < 0 || uap->resource >= RLIM_NLIMITS)
		return EINVAL;

	if (copyout((caddr_t)&u.u_rlimit[uap->resource], 
	  (caddr_t)uap->rlp, sizeof(struct rlimit)))
		return EFAULT;

	return 0;
}


/*
 * setrlimit system call - set system resource consumption limits.
 */

/* ARGSUSED */
int
setrlimit(uap, rvp)
	register struct rlimita *uap;
	rval_t *rvp;
{
	struct rlimit rlim;

	/*
	 * Determine if the argument passed is within the
	 * valid ranges for the requested resource.
	 */
	if (uap->resource < 0 || uap->resource >= RLIM_NLIMITS)
		return EINVAL;

	if (copyin((caddr_t)uap->rlp, (caddr_t)&rlim, sizeof(rlim)))
		return EFAULT;

	return rlimit(uap->resource,rlim.rlim_cur,rlim.rlim_max);
}


/*
 * procpriv system call - set, clear, retrieve, or count the privileges of
 * the calling process.
 */

struct	procpriva {
	int	cmd;
	priv_t	*privp;
	int	count;
};

int
procpriv(uap, rvp)
	register struct procpriva *uap;
	rval_t	*rvp;
{
	cred_t	*crp;
	extern	int	pm_process();
	register	int	error = 0;

	error = pm_process(uap->cmd, rvp, u.u_cred, uap->privp, uap->count, &crp);

	if (!error) {
		/*
		 * If the value of "crp" is non-NULL then it was copied
		 * in pm_process().
		 */
		if (crp != NULL) {
			crfree(u.u_cred);
			u.u_cred = crp;
		}
	}
	return error;

}	/* end of procpriv syscall */
