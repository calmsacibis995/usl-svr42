/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/sigcalls.c	1.8"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/statvfs.h>
#include <fs/ustat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/reg.h>
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
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

/*
 * ssig() is the common entry for signal, sigset, sighold, sigrelse,
 * sigignore and sigpause.
 *
 * For implementations that don't require binary compatibility, signal,
 * sigset, sighold, sigrelse, sigignore and sigpause may be made into
 * library routines that call sigaction, sigsuspend and sigprocmask.
 */

struct siga {
	int	signo;
	void	(*fun)();
};

int
ssig(uap, rvp)
	struct siga *uap;
	rval_t *rvp;
{
	register sig;
	register struct proc *pp;
	register void (*func)();
	register flags;

	sig = uap->signo & SIGNO_MASK;

	/*
	 * Validate the signal number specified.  If its value
	 * isn't in the range of valid signal numbers, or the
	 * signal can't be masked, return an error.
	 */
	if (sig <= 0 || sig >= NSIG || sigismember(&cantmask, sig))
		return EINVAL;

	pp = u.u_procp;
	func = uap->fun;

	switch (uap->signo & ~SIGNO_MASK) {

	case SIGHOLD:	/* sighold */
		sigaddset(&pp->p_hold, sig);
		return 0;

	case SIGRELSE:	/* sigrelse */
		sigdelset(&pp->p_hold, sig);
		return 0;

	case SIGPAUSE:	/* sigpause */
		sigdelset(&pp->p_hold, sig);
		pause();
		/* NOTREACHED */

	case SIGIGNORE:	/* signore */
		sigdelset(&pp->p_hold, sig);
		func = SIG_IGN;
		flags = 0;
		break;

	case SIGDEFER:		/* sigset */
		u.u_sigreturn = (void (*)()) u.u_ar0[EDX];
		if (sigismember(&pp->p_hold, sig))
			rvp->r_val1 = (int) SIG_HOLD;
		else
			rvp->r_val1 = (int) u.u_signal[sig-1];
		if (func == SIG_HOLD) {
			sigaddset(&pp->p_hold, sig);
			return 0;
		}
		sigdelset(&pp->p_hold, sig);
		flags = 0;
		break;

	case 0:	/* signal */
		u.u_sigreturn = (void (*)()) u.u_ar0[EDX];
		rvp->r_val1 = (int) u.u_signal[sig-1];
		flags = SA_RESETHAND|SA_NODEFER;
		break;

	default:		/* error */
		return EINVAL;
	}

	if (sigismember(&stopdefault, sig))
		flags |= SA_RESTART;
	else if (sig == SIGCLD) {
		flags |= SA_NOCLDSTOP;
		if (func == SIG_IGN)
			flags |= SA_NOCLDWAIT;
		else if (func != SIG_DFL) {
			register proc_t *cp;
			for (cp = pp->p_child; cp; cp = cp->p_sibling) {
				if (cp->p_stat == SZOMB) {
					sigaddset(&pp->p_sig, SIGCLD);
					break;
				}
			}
		}
	}

	/* set the signal disposition for the specified signal */
	setsigact(sig, func, (k_sigset_t)0, flags);
	sigaddset(&u.u_oldsig, sig);
	
	return 0;
}

/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

struct sigsuspenda {
	sigset_t *set;
};

/* ARGSUSED */
int
sigsuspend(uap, rvp)
	register struct sigsuspenda *uap;
	rval_t *rvp;
{
	sigset_t set;
	k_sigset_t kset;

	if (copyin((caddr_t)uap->set, (caddr_t)&set, sizeof(sigset_t)))
		return EFAULT;

	/* set the kernel signal set equal to the user signal set */
	sigutok(&set, &kset);

	/* set set1 equal to the difference of set1 and set2 */
	sigdiffset(&kset, &cantmask);
	u.u_sigoldmask = u.u_procp->p_hold;
	u.u_procp->p_hold = kset;
	u.u_sigflag |= SOMASK;
	pause();
	/* NOTREACHED */
}

struct sigaltstacka {
	struct sigaltstack *ss;
	struct sigaltstack *oss;
};

/* ARGSUSED */
int
sigaltstack(uap, rvp)
	struct sigaltstacka *uap;
	rval_t *rvp;
{
	struct sigaltstack ss;

	/*
	 * User's oss and ss might be the same address, so copyin first and
	 * save before copying out.
	 */
	if (uap->ss) {
		if (u.u_sigaltstack.ss_flags & SS_ONSTACK)
			return EPERM;
		if (copyin((caddr_t) uap->ss, (caddr_t) &ss, sizeof(ss)))
			return EFAULT;
		if (ss.ss_flags & ~SS_DISABLE)
			return EINVAL;
		if (!(ss.ss_flags & SS_DISABLE) && ss.ss_size < MINSIGSTKSZ)
			return ENOMEM;
	}

	if (uap->oss) {
		if (copyout((caddr_t) &u.u_sigaltstack, (caddr_t) uap->oss,
		  sizeof(struct sigaltstack)))
			return EFAULT;
	}

	if (uap->ss)
		u.u_sigaltstack = ss;

	return 0;
}

/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

struct sigpendinga {
	int flag;
	sigset_t *set;
};

/* ARGSUSED */
int
sigpending(uap, rvp)
	register struct sigpendinga *uap;
	rval_t *rvp;
{
	sigset_t set;
	k_sigset_t kset;

	switch (uap->flag) {
	case 1: /* sigpending */
		kset = u.u_procp->p_sig;

		/* AND the sets specified as the arguments */
		sigandset(&kset, &u.u_procp->p_hold);
		break;
	case 2: /* sigfillset */
		kset = fillset;
		break;
	default:
		return EINVAL;
	}

	/* set the user signal set equal to the kernel signal set */
	sigktou(&kset, &set);
	if (copyout((caddr_t) &set, (caddr_t) uap->set, sizeof(sigset_t)))
		return EFAULT;
	return 0;
}

/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

struct sigprocmaska {
	int how;
	sigset_t *set;
	sigset_t *oset;
};

/* ARGSUSED */
int
sigprocmask(uap, rvp)
	register struct sigprocmaska *uap;
	rval_t *rvp;
{
	k_sigset_t kset;

	/*
	 * User's oset and set might be the same address, so copyin first and
	 * save before copying out.
	 */
	if (uap->set) {
		sigset_t set;
		if (copyin((caddr_t)uap->set, (caddr_t)&set, sizeof(sigset_t)))
			return EFAULT;
		/* set the kernel signal set equal to the user signal set */
		sigutok(&set, &kset);
	}

	if (uap->oset) {
		sigset_t set;
		/* set the user signal set equal to the kernel signal set */
		sigktou(&u.u_procp->p_hold, &set);
		if (copyout((caddr_t)&set, (caddr_t)uap->oset,
		  sizeof(sigset_t)))
			return EFAULT;
	}
	
	if (uap->set) {
		/* set set1 equal to the difference of set1 and set2 */
		sigdiffset(&kset, &cantmask);

		switch (uap->how) {
		case SIG_BLOCK:
			/* OR the sets of the arguments specified */
			sigorset(&u.u_procp->p_hold, &kset);
			break;
		case SIG_UNBLOCK:
			sigdiffset(&u.u_procp->p_hold, &kset);
			break;
		case SIG_SETMASK:
			u.u_procp->p_hold = kset;
			break;
		default:
			return EINVAL;
		}
	}
	return 0;
}

/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

struct sigactiona {
	int sig;
	struct sigaction *act;
	struct sigaction *oact;
};

/* ARGSUSED */
int
sigaction(uap, rvp)
	register struct sigactiona *uap;
	rval_t *rvp;
{
	struct sigaction act;
	k_sigset_t set;
	register int sig;

	sig = uap->sig;

	if (sig <= 0 
	  || sig >= NSIG 
	  || (uap->act != NULL && sigismember(&cantmask, sig)))
		return EINVAL;

	/* act and oact might be the same address, so copyin act first */

	if (uap->act && copyin((caddr_t)uap->act, (caddr_t)&act, sizeof(act)))
		return EFAULT;

	if (uap->oact) {
		struct sigaction oact;
		register flags;
		register void (*disp)();

		disp = u.u_signal[sig - 1];	

		flags = 0;
		if (disp != SIG_DFL && disp != SIG_IGN) {
			set = u.u_sigmask[sig-1];
			if (sigismember(&u.u_procp->p_siginfo, sig))
				flags |= SA_SIGINFO;
			if (sigismember(&u.u_sigrestart, sig))
				flags |= SA_RESTART;
			if (sigismember(&u.u_sigonstack, sig))
				flags |= SA_ONSTACK;
			if (sigismember(&u.u_sigresethand, sig))
				flags |= SA_RESETHAND;
			if (sigismember(&u.u_signodefer, sig))
				flags |= SA_NODEFER;
		} else
			sigemptyset(&set);

		if (sig == SIGCLD) {
			if (u.u_procp->p_flag & SNOWAIT)
				flags |= SA_NOCLDWAIT;
			if (!(u.u_procp->p_flag & SJCTL))
				flags |= SA_NOCLDSTOP;
		}

		oact.sa_handler = disp;
		oact.sa_flags = flags;
		/* set the user signal set equal to the kernel signal set */
		sigktou(&set, &oact.sa_mask);

		if (copyout((caddr_t)&oact, (caddr_t) uap->oact, sizeof(oact)))
			return EFAULT;
	}

	if (uap->act) {
		/* set the kernel signal set equal to the user signal set */
		sigutok(&act.sa_mask, &set);
		/* set the signal disposition for the specified signal */
		setsigact(sig, act.sa_handler, set, act.sa_flags);
		sigdelset(&u.u_oldsig, sig);
	}

	return 0;
}

struct sigsenda {
	procset_t *psp;
	int	   sig;
};

/* ARGSUSED */
int
sigsendsys(uap, rvp)
	register struct sigsenda *uap;
	rval_t *rvp;
{
	procset_t set;

	if (uap->sig < 0 || uap->sig >= NSIG)
		return EINVAL;

	if (copyin((caddr_t)uap->psp, (caddr_t)&set, sizeof(procset_t)))
		return EFAULT;

	if (set.p_op == POP_AND
	&&  set.p_lidtype == P_PID
	&&  set.p_ridtype == P_ALL) {
		/* signaling a single process, so do it the fast way */
		return(killsingle(set.p_lid == P_MYID
			? u.u_procp->p_pid : (pid_t)set.p_lid, uap->sig));
	}

	/*
	 * Return the value received from sending the specified
	 * signal to the set of processes specified by `set'.
	 */
	return sigsendset(&set, uap->sig);
}

/*
 * Alarm clock signal.
 */

STATIC void
alarm_expiry(p)
	register proc_t *p;
{
	psignal(p, SIGALRM);
	p->p_alarmid = 0;
}

struct alarma {
	unsigned int deltat;
};

int
alarm(uap, rvp)
	struct alarma	*uap;
	rval_t		*rvp;
{
	register proc_t	*p;
	register long	t;
	int		s;

	p = u.u_procp;
	s = splhi();

	/*
	 * If we have an unexpired timer from a previous call, cancel it.
	 * The syscall return value in this case is the time remaining (in
	 * seconds) until the timer would have fired.  Boundary conditions
	 * could give us a zero result here, which we can't permit since it
	 * would imply that the timer had indeed fired.
	 */
	if (p->p_alarmid) {
		untimeout(p->p_alarmid);
		p->p_alarmid = 0;
		t = howmany(p->p_alarmtime - lbolt, HZ);
		rvp->r_val1 = (t > 0)? t : 1;
	} else
		rvp->r_val1 = 0;

	if (uap->deltat) {
		/*
		 * Compute time to alarm, in ticks.  We add 1 in timeout so
		 * that the alarm occurs AFTER the requested amount of time
		 * has elapsed; otherwise it could occur up to 1 tick early.
		 * A maximum alarm time is silently enforced here.
		 */
		t = MIN(uap->deltat, (INT_MAX-1)/HZ) * HZ;
		p->p_alarmtime = lbolt + t;
		p->p_alarmid = timeout(alarm_expiry, (caddr_t)p, t + 1);
	}

	splx(s);
	return 0;
}
