/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/sig.c	1.14"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/mac/cca.h>
#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/procfs/procfs.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/reg.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/debugreg.h>
#include <util/fp.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef WEITEK
#include <util/weitek.h>
#endif

#ifdef VPIX
#include <vpix/v86.h>
extern	char v86procflag;
#endif

/* Enhanced Application Compatibility Support */
#include <io/termios.h>
#include <svc/isc.h>
#include <svc/sco.h>
/* End Enhanced Application Compatibility Support */

k_sigset_t fillset = 0x7fffffff;	/* MUST be contiguous */

k_sigset_t cantmask = (sigmask(SIGKILL)|sigmask(SIGSTOP));

k_sigset_t cantreset = (sigmask(SIGILL)|sigmask(SIGTRAP)|sigmask(SIGPWR));

k_sigset_t ignoredefault = (sigmask(SIGCONT)|sigmask(SIGCLD)|sigmask(SIGPWR)
			|sigmask(SIGWINCH)|sigmask(SIGURG));

k_sigset_t stopdefault = (sigmask(SIGSTOP)|sigmask(SIGTSTP)
			|sigmask(SIGTTOU)|sigmask(SIGTTIN));

k_sigset_t coredefault = (sigmask(SIGQUIT)|sigmask(SIGILL)|sigmask(SIGTRAP)
			|sigmask(SIGIOT)|sigmask(SIGEMT)|sigmask(SIGFPE)
			|sigmask(SIGBUS)|sigmask(SIGSEGV)|sigmask(SIGSYS)
			|sigmask(SIGXCPU)|sigmask(SIGXFSZ));

k_sigset_t holdvfork = (sigmask(SIGTTOU)|sigmask(SIGTTIN)|sigmask(SIGTSTP));

#define tracing(p, sig) \
	(((p)->p_flag & STRC) || sigismember(&(p)->p_sigmask, (sig)))

int		fsig();
STATIC int	isjobstop();
STATIC int	procxmt();
STATIC void	sigtoproc();

/*
 * Send the specified signal to the specified process.
 */

void
psignal(p, sig)
	register proc_t *p;
	register int sig;
{
	sigtoproc(p, sig, 0);
}

STATIC void
sigtoproc(p, sig, fromuser)
	register proc_t *p;
	register int sig;
	register int fromuser;
{
	if (sig <= 0 || sig >= NSIG)
		return;

	if (sig == SIGCONT) {
		if(p->p_sig & sigmask(SIGSTOP))
			sigdelq(p, SIGSTOP);
		if(p->p_sig & sigmask(SIGTSTP))
			sigdelq(p,SIGTSTP);
		if(p->p_sig & sigmask(SIGTTOU))
			sigdelq(p,SIGTTOU);
		if(p->p_sig & sigmask(SIGTTIN))
			sigdelq(p,SIGTTIN);
		sigdiffset(&p->p_sig, &stopdefault);
		if (p->p_stat == SSTOP && p->p_whystop == PR_JOBCONTROL) {
			p->p_flag |= SXSTART;
			setrun(p);
		} 
	} else if (sigismember(&stopdefault, sig)) {
		sigdelq(p, SIGCONT);
		sigdelset(&p->p_sig, SIGCONT);
	}

	if (!tracing(p, sig) && sigismember(&p->p_ignore, sig))
		return;

	sigaddset(&p->p_sig, sig);

	if (p->p_stat == SSLEEP) {
		if ((p->p_flag & SNWAKE) || sigismember(&p->p_hold, sig))
			return;
		setrun(p);
	} else if (p->p_stat == SSTOP) {
		if (sig == SIGKILL) {
			p->p_flag |= SXSTART|SPSTART;
			setrun(p);
		} else if (p->p_wchan && ((p->p_flag & SNWAKE) == 0))
			/*
			 * If process is in the sleep queue at an
			 * interruptible priority but is stopped,
			 * remove it from the sleep queue but don't
			 * set it running yet. The signal will be
			 * noticed when the process is continued.
			 */
			unsleep(p);
	} else if (p == curproc) {

		/*
		 * If the process is the current process we set the
		 * u_sigevpend flag to ensure that signals posted to the
		 * current process from interrupt level are received by
		 * the process before it returns to user mode.
		 */
		u.u_sigevpend = 1;
	}
}

/*
 * Attempt to stop current process.  Called from issig().
 */
STATIC int
isjobstop(sig)
	register int sig;
{
	register proc_t *p = u.u_procp;

	if (u.u_signal[sig-1] == SIG_DFL && sigismember(&stopdefault, sig)) {
		/*
		 * If SIGCONT has been posted since we promoted this signal
		 * from pending to current, then don't do a jobcontrol stop.
		 * Also discard SIGCONT, since it would not have been sent
		 * if a debugger had not been holding the process stopped.
		 */
		if (sigismember(&p->p_sig, SIGCONT)) {
			sigdelset(&p->p_sig, SIGCONT);
			sigdelq(p, SIGCONT);
		} else if (sig == SIGSTOP || p->p_detached == 0) {
			if (stop(p, PR_JOBCONTROL, sig, 0))
				swtch();
			p->p_wcode = CLD_CONTINUED;
			p->p_wdata = SIGCONT;
			sigcld(p);
		}
		return 1;
	}
	return 0;
}


/*
 * Returns true if the current process has a signal to process, and
 * the signal is not held.  The signal to process is put in p_cursig.
 * This is asked at least once each time a process enters the system
 * (though this can usually be done without actually calling issig by
 * checking the pending signal masks).  A signal does not do anything
 * directly to a process; it sets a flag that asks the process to do
 * something to itself.
 *
 * The "why" argument indicates the allowable side-effects of the call:
 *
 * FORREAL:  Extract the next pending signal from p_sig into p_cursig;
 * stop the process if a stop has been requested or if a traced signal
 * is pending.
 *
 * JUSTLOOKING:  Don't stop the process, just indicate whether or not
 * a signal is pending.
 */
int
issig(why)
	int why;
{
	register int sig;
	register proc_t *p = u.u_procp;

	u.u_sigevpend = 0;

	for (;;) {
		/*
		 * Honor requested stop before dealing with the
		 * current signal; a debugger may change it.
		 */
		if (why == FORREAL
		  && (p->p_flag & SPRSTOP)
		  && stop(p, PR_REQUESTED, 0, 0))
			swtch();

		/*
		 * If a debugger wants us to take a signal it will
		 * have left it in p->p_cursig.  If p_cursig has been
		 * cleared or if it's being ignored, we continue on
		 * looking for another signal.  Otherwise we return
		 * the specified signal, provided it's not a signal
		 * that causes a job control stop.
		 *
		 * When stopped on PR_JOBCONTROL, there is no current
		 * signal; we cancel p->p_cursig temporarily before
		 * calling isjobstop().  The current signal may be reset
		 * by a debugger while we are stopped in isjobstop().
		 */
		if ((sig = p->p_cursig) != 0) {
			p->p_cursig = 0;
			if (why == JUSTLOOKING
			  || (p->p_flag & SPTRX)
			  || (!sigismember(&p->p_ignore, sig)
			    && !isjobstop(sig)))
				return p->p_cursig = (char)sig;
			/*
			 * The signal is being ignored or it caused a
			 * job-control stop.  If another current signal
			 * has not been established, return the current
			 * siginfo, if any, to the memory manager.
			 */
			if (p->p_cursig == 0 && p->p_curinfo != NULL) {
				kmem_free((caddr_t)p->p_curinfo,
				  sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
			/*
			 * Loop around again in case we were stopped
			 * on a job control signal and a /proc stop
			 * request was posted or another current signal
			 * was established while we were stopped.
			 */
			continue;
		}

		/*
		 * Loop on the pending signals until we find a 
		 * non-held signal that is traced or not ignored.
		 */
		for (;;) {
			if ((sig = fsig(p)) == 0)
				return 0;
			if (tracing(p, sig)
			  || !sigismember(&p->p_ignore, sig)) {
				if (why == JUSTLOOKING)
					return sig;
				break;
			}
			sigdelset(&p->p_sig, sig);
			sigdelq(p, sig);
		}

		/*
		 * Promote the signal from pending to current.
		 *
		 * Note that sigdeq() will set p->p_curinfo to NULL
		 * if no siginfo_t exists for this signal.
		 */
		sigdelset(&p->p_sig, sig);
		p->p_cursig = (char)sig;
		ASSERT(p->p_curinfo == NULL);
		sigdeq(p, sig, &p->p_curinfo);

		/*
		 * If tracing, stop.  If tracing via ptrace(2), call
		 * procxmt() repeatedly until released by debugger.
		 */
		if (tracing(p, sig)) {
			int firststop = 1;

			do  {
				if ((p->p_flag & STRC) && p->p_ppid == 1) {
					p->p_flag |= SPTRX;
					break;
				}
				if (!stop(p, PR_SIGNALLED, sig, firststop))
					break;
				swtch();
				firststop = 0;
			} while ((p->p_flag & STRC) && !procxmt());

			if ((p->p_flag & SPTRX) && p->p_cursig == 0)
				p->p_cursig = SIGKILL;
		}

		/*
		 * Loop around to check for requested stop before
		 * performing the usual current-signal actions.
		 */
	}
}

/*
 * Put the specified process into the stopped state and notify
 * tracers via wakeprocs().  Returns 0 if process can't be stopped.
 * Returns non-zero in the normal case.
 */
int
stop(p, why, what, firststop)
	register proc_t *p;
	register why, what;
	int firststop;		/* only for SIGNALLED stop */
{
	register int	oldlvl;

        if (p->p_flag & STRC) {
		/*
                 * save the floating point register
                 */
                if (p == fp_proc)
                        fpsave();
#ifdef WEITEK
             	if (p == weitek_proc) {
                        weitek_save();
		}
#endif
	}

	/*
	 * Don't stop a process with SIGKILL pending and
	 * don't stop one undergoing a ptrace(2) exit.
	 */
	if (p->p_cursig == SIGKILL
	  || sigismember(&p->p_sig, SIGKILL)
	  || (p->p_flag & SPTRX))
		return 0;

	/*
	 * Since dispdeq() disables interrupts for a long time, it is
	 * possible to get a timer interrupt before the SSTOP flag is
	 * set.  The spls prevent this.
	 */
	oldlvl = splhi();
	(void)dispdeq(p);
	p->p_stat = SSTOP;
	p->p_flag &= ~(SPSTART|SXSTART);
	splx(oldlvl);

	p->p_whystop = (short)why;
	p->p_whatstop = (short)what;
	CL_STOP(p, p->p_clproc, why, what);

	switch (why) {
	case PR_JOBCONTROL:
		p->p_flag |= SPSTART;
		p->p_wcode = CLD_STOPPED;
		p->p_wdata = what;
		sigcld(p);
		break;
	case PR_SIGNALLED:
		if (p->p_flag & STRC) {		/* ptrace() */
			p->p_wcode = CLD_TRAPPED;
			p->p_wdata = what;
			wakeprocs((caddr_t)p->p_parent, PRMPT);
			wakeprocs((caddr_t)p, PRMPT);
			if (!firststop || !sigismember(&p->p_sigmask, what)) {
				p->p_flag |= SPSTART;
				break;
			}
		} else
			p->p_flag |= SXSTART;
		/* fall through */
	default:
		if (why != PR_SIGNALLED)
			p->p_flag |= SXSTART;
		p->p_flag &= ~SPRSTOP;
		if (p->p_trace)			/* /proc */
			wakeprocs((caddr_t)p->p_trace, PRMPT);
		break;
	}

	return 1;
}

/*
 * Perform the action specified by the current signal.
 * The usual sequence is:
 * 	if (issig())
 * 		psig();
 * The signal bit has already been cleared by issig(),
 * the current signal number has been stored in p->p_cursig,
 * and the current siginfo is now referenced by p->p_curinfo.
 */

void
psig()
{
	register proc_t *p = u.u_procp;
	register int sig = p->p_cursig;
	void (*func)();
	int rc, code;

	code = CLD_KILLED;

	/*
	 * A pending SIGKILL overrides the current signal.
	 */
	if (sigismember(&p->p_sig, SIGKILL))
		sig = p->p_cursig = SIGKILL;

/* Enhanced Application Compatibility Support */
        if (VIRTUAL_XOUT && sig == SIGPOLL)
                sig = XENIX_SIGPOLL;

/* End Enhanced Application Compatibility Support */

	/*
	 * Exit immediately on a ptrace exit request.
	 */
	if (p->p_flag & SPTRX) {
		p->p_flag &= ~SPTRX;
		if (sig == 0)
			sig = p->p_cursig = SIGKILL;
	} else {
		ASSERT(sig);
		ASSERT(!sigismember(&p->p_ignore, sig));
		if ((func = u.u_signal[sig-1]) != SIG_DFL) {
			k_siginfo_t *sip;

                        /*
                         * save siginfo pointer here, in case the
                         * the signal's reset bit is on
                         */

			if (p->p_curinfo && sigismember(&p->p_siginfo, sig))
                                sip = &p->p_curinfo->sq_info;
			else
                                sip = NULL;

			if (u.u_sigflag & SOMASK) 
				u.u_sigflag &= ~SOMASK;
			else 
				u.u_sigoldmask = p->p_hold;
			sigorset(&p->p_hold, &u.u_sigmask[sig-1]);
			if (!sigismember(&u.u_signodefer, sig))
				sigaddset(&p->p_hold, sig);
			if (sigismember(&u.u_sigresethand, sig))
				setsigact(sig, SIG_DFL, 0, 0);
			rc = sendsig(sig, sip, func);
			p->p_cursig = 0;
			if (p->p_curinfo) {
				kmem_free((caddr_t)p->p_curinfo,
				  sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
			if (rc)
				return;
			sig = p->p_cursig = SIGSEGV;
		}
		if (sigismember(&coredefault, sig)) {
			if (core("core", u.u_procp, u.u_cred,
			   u.u_rlimit[RLIMIT_CORE].rlim_cur, sig) == 0)
				code = CLD_DUMPED;
		}
	}
	p->p_cursig = 0;
	if (p->p_curinfo) {
		kmem_free((caddr_t)p->p_curinfo, sizeof(*p->p_curinfo));
		p->p_curinfo = NULL;
	}

        if (ISC_USES_POSIX)
                switch (sig) {
                case SIGCONT:
                        sig = ISC_SIGCONT;
                        break;
                case SIGSTOP:
                        sig = ISC_SIGSTOP;
                        break;
                case SIGTSTP:
                        sig = ISC_SIGTSTP;
                        break;
                }

	exit(code, sig);
}

/*
 * Find the next unheld signal in bit-position representation in p_sig.
 */

int
fsig(p)
	register proc_t *p;
{
	register i;
	k_sigset_t temp;

	temp = p->p_sig;
	sigdiffset(&temp, &p->p_hold);
	if (p->p_flag & SVFORK)
		sigdiffset(&temp, &holdvfork);
	if (!sigisempty(&temp)) {
		if (sigismember(&temp, SIGKILL))
			return SIGKILL;
		for (i = 1; i < NSIG; i++) {
			if (sigismember(&temp, i))
				return i;
		}
	}
	return 0;
}

/*
 * sys-trace system call.
 */
struct ptracea {
	int	req;
	int	pid;
	int	*addr;
	int	data;
};

/*
 * Tracing variables.  Used to pass trace command from parent
 * to child being traced.  This data base cannot be shared and
 * is locked per user.
 */
STATIC struct ipstruct {
	struct ipstruct	*ip_next;
	int		ip_lock;
	int		ip_req;
	int		*ip_addr;
	int		ip_data;
} *Ipcp = (struct ipstruct *)NULL;

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO


int
ptrace(uap, rvp)
	register struct ptracea *uap;
	rval_t *rvp;
{
	register struct proc *p;
	register struct ipstruct *ipcp, *ipcq;
	int error = 0;

	if (uap->req <= 0) {
		u.u_procp->p_flag |= STRC;
		return 0;
	}

    again:
	for (p = u.u_procp->p_child; p; p = p->p_sibling)
		if (p->p_pid == uap->pid
		  && p->p_stat == SSTOP
		  && (p->p_flag & (STRC|SXSTART)) == STRC
		  && p->p_whystop == PR_SIGNALLED)
			goto found;
	return ESRCH;

    found:
	/*
	 * Calling process must be at the level of the process to
	 * be traced, or have either the P_COMPAT or P_MACREAD privilege.
	 */
	if (MAC_ACCESS(MACEQUAL, p->p_cred->cr_lid, u.u_cred->cr_lid)
	   && pm_denied(u.u_cred, P_COMPAT)
	   && pm_denied(u.u_cred, P_MACREAD))
		return EPERM;

	/*
	 * Wait until /proc has started the process.  The SPSTART bit is
	 * always set if /proc is not controlling the process.
	 */
	while ((p->p_flag & SPSTART) == 0)
		sleep((caddr_t)u.u_procp, PZERO+1);
	
	/*
	 * Search through singly link list of ipc structures to
	 * find an unused one.  If one cannot be found, allocate
	 * a new one and add to list.  There is hardly ever going
	 * to be contention for ipc structures, so don't bother
	 * deallocating.
	 * It is important to note that no sleeps are allowed
	 * from search until the ipc structure is marked for use
	 * (ip_lock is set).
	 */
	for (ipcp = Ipcp;
	     ipcp && ipcp->ip_lock;
	     ipcq = ipcp, ipcp = ipcp->ip_next)
		;

	if (ipcp == (struct ipstruct *)NULL) {
		ipcp = (struct ipstruct *)
			kmem_zalloc(sizeof(struct ipstruct), KM_NOSLEEP);
		if (ipcp == (struct ipstruct *)NULL)
			return ENOMEM;
		if (Ipcp == (struct ipstruct *)NULL)
			Ipcp = ipcp;
		else
			ipcq->ip_next = ipcp;
	}

	ipcp->ip_lock = p->p_pid;
	ipcp->ip_data = uap->data;
	ipcp->ip_addr = uap->addr;
	ipcp->ip_req = uap->req;
	p->p_flag |= SXSTART;
	setrun(p);
	while (ipcp->ip_req > 0)
		sleep((caddr_t)ipcp, IPCPRI);
	if (ipcp->ip_req < 0)
		error = EIO;
	else
		rvp->r_val1 = ipcp->ip_data;
	ipcp->ip_lock = 0;
	return error;
}
/*
 * Code that the child process executes to implement the command
 * of the parent process in tracing.
 */

STATIC int ipcreg[] = {
                        EAX, EDX, ECX, EBX, ESI, EDI, EFL,
                        EIP, EBP, UESP, GS, FS, ES, DS, CS,
                        SS, ERR, TRAPNO
};

STATIC int
procxmt()
{
	register proc_t *p = u.u_procp;
	register int i;
	register *ip;
	register struct as *as = p->p_as;
	register int *addr;
	register struct ipstruct *ipcp;
	register int usize = u.u_procp->p_usize;

	/*
	 * If there is no ipc for this process, return now.
	 * It was set running either because SIGKILL was sent
	 * or because its parent died.
	 */
	for (ipcp = Ipcp; ipcp && ipcp->ip_lock != p->p_pid; ipcp = ipcp->ip_next)
		;
	if (ipcp == (struct ipstruct *)NULL)
		return 0;

	i = ipcp->ip_req;
	ipcp->ip_req = 0;
	addr = ipcp->ip_addr;
	switch (i) {

	case 1: /* read user I */
	case 2: /* read user D */

                if ((ipcp->ip_data = fuword((int *)addr)) == -1 &&
                        fubyte((caddr_t)addr) == -1)
			goto error;
		break;


	case 3: /* read u */

		ASSERT(MINUSIZE <= usize && usize <= MAXUSIZE);

		i = (int)addr;
		if (i >= 0  &&  i < ctob(usize)) {
			ipcp->ip_data = ((physadr)&u)->r[i>>2];
			break;
		}

		ip = (int *)i;
		for (i = 0; i < sizeof(ipcreg)/sizeof(ipcreg[0]); i++) {
			if (ip == &u.u_ar0[ipcreg[i]]) {
				ipcp->ip_data = *ip;
				goto ok3;
			}
		}
		goto error;
	ok3:
		break;

	case 4: /* write user I */
	case 5: /* write user D */
	{
		int prot, protchanged, naddr;

		/* check whether address is valid before writing */
		if (as_segat(as, addr) == NULL)
			goto error;
		if ((prot = as_getprot(as, addr, &naddr)) == PROT_NONE)
			goto error;

		if ((prot & PROT_WRITE) == 0)
		{
			protchanged = 1;
			if (as_setprot(as, addr, NBPW, prot|PROT_WRITE))
				goto error;
		}
		else
			protchanged = 0;

		i = suword(addr, ipcp->ip_data);

		if (protchanged)
			(void) as_setprot(as, addr, NBPW, prot);

		if (i < 0)
			goto error;
		break;
	}

	case 6: /* write u */

                ASSERT(MINUSIZE <= usize && usize <= MAXUSIZE);

		if ((i = (int)addr) >= 0 && i < ctob(usize))
			/* LINTED */
                        ip = &((physadr)&u)->r[i>>2];
		else
			ip = addr;
		ip = (int *)((int)ip & ~(NBPW-1));
		for (i = 0; i < sizeof(ipcreg)/sizeof(ipcreg[0]); i++)
			if (ip == &u.u_ar0[ipcreg[i]])
				goto ok6;
		if (ip >= &u.u_debugreg[DR_FIRSTADDR] &&
                    ip <= &u.u_debugreg[DR_LASTADDR]) {
			*ip = ipcp->ip_data;
			break;
		}
		if (ip == &u.u_debugreg[DR_CONTROL]) {
			*ip = ipcp->ip_data &
                                ~(DR_GLOBAL_SLOWDOWN | DR_CONTROL_RESERVED |
                                  DR_GLOBAL_ENABLE_MASK);
			if (u.u_debugreg[DR_CONTROL] &
                                (DR_LOCAL_SLOWDOWN|DR_LOCAL_ENABLE_MASK))
                                u.u_debugon = 1;
			else
                                u.u_debugon = 0;
			break;
		}

		goto error;
	ok6:
		if (ipcreg[i] == EFL) {
                        ipcp->ip_data = (u.u_ar0[EFL] & PS_SYSMASK) |
                                        (ipcp->ip_data & ~PS_SYSMASK);
		} else if (ipcreg[i] == ESP)
                        ipcp->ip_data &= ~(NBPW - 1);
		*ip = ipcp->ip_data;
		break;

	case 9:	/* set signal with trace trap and continue. */

		u.u_ar0[EFL] |= PS_T;

		/* FALLTHROUGH */

	case 7: /* set signal and continue */

		if ((int)addr != 1)
			u.u_ar0[EIP] = (int)addr;
		if ((unsigned int)ipcp->ip_data >= NSIG)
			goto error;
		/*
		 * If we're changing or clearing the current signal,
		 * the old current siginfo is invalid.  Discard it.
		 * We don't have a mechanism in ptrace(2), as we do
		 * in /proc, for redefining the current siginfo.
		 * Also, don't clobber SIGKILL.
		 */
		if (p->p_cursig != ipcp->ip_data
		  && p->p_cursig != SIGKILL) {
			p->p_cursig = (char)ipcp->ip_data;
			if (p->p_curinfo) {
				kmem_free((caddr_t)p->p_curinfo,
				  sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
		}
		/* Clear pending signals */
		sigemptyset(&p->p_sig);
		wakeprocs((caddr_t)ipcp, PRMPT);
		return 1;

	case 8: /* force exit */

		wakeprocs((caddr_t)ipcp, PRMPT);
		p->p_flag |= SPTRX;
		return 1;

	default:
	error:
		ipcp->ip_req = -1;
	}
	wakeprocs((caddr_t)ipcp, PRMPT);
	return 0;
}

/*
 * Set the signal disposition for the specified signal.
 */
void
setsigact(sig, disp, mask, flags)
	register int sig;
	register void (*disp)();
	k_sigset_t mask;
	register int flags;
{
	register proc_t *pp = u.u_procp;

	u.u_signal[sig - 1] = disp;

	if (disp != SIG_DFL && disp != SIG_IGN) {
		sigdelset(&pp->p_ignore, sig);
		sigdiffset(&mask, &cantmask);
		u.u_sigmask[sig - 1] = mask;
		if (!sigismember(&cantreset, sig)) {
			if (flags & SA_RESETHAND)
				sigaddset(&u.u_sigresethand, sig);
			else
				sigdelset(&u.u_sigresethand, sig);
		}
		if (flags & SA_NODEFER)
			sigaddset(&u.u_signodefer, sig);
		else
			sigdelset(&u.u_signodefer, sig);
		if (flags & SA_RESTART)
			sigaddset(&u.u_sigrestart, sig);
		else
			sigdelset(&u.u_sigrestart, sig);
		if (flags & SA_ONSTACK)
			sigaddset(&u.u_sigonstack, sig);
		else
			sigdelset(&u.u_sigonstack, sig);
		if (flags & SA_SIGINFO)
			sigaddset(&pp->p_siginfo, sig);
		else if (sigismember(&pp->p_siginfo, sig))
			sigdelset(&pp->p_siginfo, sig);
	} else if (disp == SIG_IGN
	  || (disp == SIG_DFL && sigismember(&ignoredefault, sig))) {
		sigaddset(&pp->p_ignore, sig);
		sigdelset(&pp->p_sig, sig);
		sigdelq(pp, sig);
	} else
		sigdelset(&pp->p_ignore, sig);

	if (sig == SIGCLD) {
		if (flags & SA_NOCLDWAIT) {
			register proc_t *cp;
			pp->p_flag |= SNOWAIT;
			cp = pp->p_child;
			while (cp) {
				if (cp->p_stat == SZOMB) {
					register proc_t *sp;
					sp = cp->p_sibling;
					freeproc(cp);
					cp = sp;
				} else 
					cp = cp->p_sibling;
			}
		}
		else
			pp->p_flag &= ~SNOWAIT;
		if (flags & SA_NOCLDSTOP)
			pp->p_flag &= ~SJCTL;
		else
			pp->p_flag |= SJCTL;
	}
}

/*
 * Perform wakeups on the specified process and its parent.
 */
void
sigcld(cp)
	register proc_t *cp;
{
	register proc_t *pp = cp->p_parent;
	k_siginfo_t info;

	switch (cp->p_wcode) {

		case CLD_EXITED:
		case CLD_DUMPED:
		case CLD_KILLED:
			wakeprocs((caddr_t)cp, PRMPT);
			wakeprocs((caddr_t)pp, PRMPT);
			break;

		case CLD_STOPPED:
		case CLD_CONTINUED:
			wakeprocs((caddr_t)cp, PRMPT);
			wakeprocs((caddr_t)pp, PRMPT);
			if (pp->p_flag & SJCTL)
				break;

			/* fall through */
		default:
			return;
	}

	winfo(cp, &info, 0);
	sigaddq(pp, &info, KM_SLEEP);
}

/*
 * Sends signal information to the process specified.
 */
int
sigsendproc(p, pv)
	register proc_t *p;
	register sigsend_t *pv;
{
	if (p->p_pid == 1 && sigismember(&cantmask, pv->sig))
		return EPERM;

	if (MAC_ACCESS(MACEQUAL, u.u_cred->cr_lid, p->p_cred->cr_lid)
	&&  pm_denied(u.u_cred, P_MACWRITE)
	&&  pm_denied(u.u_cred, P_COMPAT)
	&&  pm_denied(p->p_cred, P_COMPAT)) {
		if (MAC_ACCESS(MACDOM, u.u_cred->cr_lid, p->p_cred->cr_lid))
			return ESRCH;	/* don't make process visible */
		else
			return 0;	/* normal error, pv->perm not set */
	}

        MAC_ASSERT(p, MAC_SAME);

	if (pv->checkperm == 0 || hasprocperm(p->p_cred, u.u_procp->p_cred)
	  || (pv->sig == SIGCONT && p->p_sessp == u.u_procp->p_sessp)) {
		pv->perm++;
		if (pv->sig) {
			k_siginfo_t info;
			struct_zero((caddr_t)&info, sizeof(info));
			info.si_signo = pv->sig;
			info.si_code = SI_USER;
			info.si_pid = u.u_procp->p_pid;
			info.si_uid = u.u_procp->p_cred->cr_ruid;
			sigaddq(p, &info, KM_SLEEP);
		}
	}

	return 0;
}

/*
 * Send the specified signal to the set of processes specified.
 */
int
sigsendset(psp, sig)
	register procset_t *psp;
	register sig;
{
	sigsend_t v;
	register int error;

	v.sig = sig;
	v.perm = 0;
	v.checkperm = 1;

	error = dotoprocs(psp, sigsendproc, (char *)&v);
	if (error == 0 && v.perm == 0)
		return EPERM;

	return error;
}

/*
 * Add the specified signal to the pending set of signals
 * for the process specified if a siginfo structure exists
 * for the specified signal.
 */
void
sigdeq(p, sig, qpp)
	register proc_t *p;
	int sig;
	sigqueue_t **qpp;
{
	register sigqueue_t **psqp, *sqp;

	*qpp = NULL;

	for (psqp = &p->p_sigqueue; ;) {
		if ((sqp = *psqp) == NULL)
			return;
		if (sqp->sq_info.si_signo == sig)
			break;
		else
			psqp = &sqp->sq_next;
	}

	*qpp = sqp;
	*psqp = sqp->sq_next;
	for (sqp = *psqp; sqp; sqp = sqp->sq_next) {
		if (sqp->sq_info.si_signo == sig) {
			sigaddset(&p->p_sig, sig);
			break;
		}
	}
}

/*
 * Remove all siginfo structures from the siginfo queue of
 * the specified process for the specified signal.
 */
void
sigdelq(p, sig)
	proc_t *p;
	int sig;
{
	register sigqueue_t **psqp, *sqp;

	for (psqp = &p->p_sigqueue; *psqp; ) {
		sqp = *psqp;
		if (sig == 0 || sqp->sq_info.si_signo == sig) {
			*psqp = sqp->sq_next;
			kmem_free(sqp, sizeof(sigqueue_t));
		} else
			psqp = &sqp->sq_next;
	}
}

/*
 * Queue signal information on the siginfo queue of the
 * specified process for the signal specified.
 */
void
sigaddq(p, infop, km_flags)
	proc_t *p;
	k_siginfo_t *infop;
	int km_flags;
{
	register sigqueue_t *sqp, **psqp;
	register int sig = infop->si_signo;

	ASSERT(sig >= 1 && sig < NSIG);

	sqp = (sigqueue_t *)kmem_alloc(sizeof(sigqueue_t), km_flags);

	if (sqp == NULL)
		goto postsig;

	if (!tracing(p, sig) && sigismember(&p->p_ignore, sig)) {
		kmem_free(sqp, sizeof(sigqueue_t));
		goto postsig;
	}


	for (psqp = &p->p_sigqueue; *psqp != NULL; psqp = &(*psqp)->sq_next)
		if ((*psqp)->sq_info.si_signo == sig) {
			kmem_free(sqp, sizeof(sigqueue_t));
			return;
		}

	*psqp = sqp;
	sqp->sq_next = NULL;

	bcopy((caddr_t)infop, (caddr_t)&sqp->sq_info, sizeof(k_siginfo_t));

postsig:
	sigtoproc(p, sig, infop->si_code <= 0);
}

/*
 * Append a siginfo queue to another siginfo queue and return
 * a pointer to the siginfo queue appended to.
 */
sigqueue_t	*
sigappend(toks, toqueue, fromks, fromqueue)
	register k_sigset_t	*toks;
	register sigqueue_t	*toqueue;
	register k_sigset_t	*fromks;
	register sigqueue_t	*fromqueue;
{
	register sigqueue_t	**endsq;

	sigorset(toks, fromks);
	if (fromqueue != NULL) {
		if (toqueue == NULL)
			toqueue = fromqueue;
		else {
			for (endsq = &toqueue->sq_next;
			  *endsq != NULL;
			  endsq = &(*endsq)->sq_next)
				;
			*endsq = fromqueue;
		}
	}
	return toqueue;
}

/*
 * Append a siginfo queue on to another siginfo queue and return
 * a pointer to the queue that was appended.
 */
sigqueue_t	*
sigprepend(toks, toqueue, fromks, fromqueue)
	register k_sigset_t	*toks;
	register sigqueue_t	*toqueue;
	register k_sigset_t	*fromks;
	register sigqueue_t	*fromqueue;
{
	register sigqueue_t	*endqueue;

	sigorset(toks, fromks);
	if (fromqueue != NULL) {
		for (endqueue = fromqueue; endqueue->sq_next;
		  endqueue = endqueue->sq_next)
			;
		endqueue->sq_next = toqueue;
		toqueue = fromqueue;
	}
	return toqueue;
}

struct killa {
	pid_t	pid;
	int	sig;
};

/*
 * for implementations that don't require binary compatibility,
 * the kill system call may be made into a library call to the
 * sigsend system call
 */

/* ARGSUSED */
int
kill(uap, rvp)
	register struct killa *uap;
	rval_t *rvp;
{
	register id_t id;
	register idtype_t idtype;
	procset_t set;

/* Enhanced Application Compatibility Support */
        if (VIRTUAL_XOUT && uap->sig == XENIX_SIGPOLL)
                uap->sig = SIGPOLL;

        if (ISC_USES_POSIX)
                switch (uap->sig) {
                case ISC_SIGCONT:
                        uap->sig = SIGCONT;
                        break;
                case ISC_SIGSTOP:
                        uap->sig = SIGSTOP;
                        break;
                case ISC_SIGTSTP:
                        uap->sig = SIGTSTP;
                        break;
                }
/* End Enhanced Application Compatibility Support */

	if (uap->sig < 0 || uap->sig >= NSIG)
		return EINVAL;

	if (uap->pid > 0) {
		if (audit_on)
                        /* audit recording function */
                        (void)adt_kill(uap->sig, &(uap->pid), 1);

		/* signaling a single process, so do it the fast way */
		return(killsingle(uap->pid, uap->sig));
	} else if (uap->pid == 0) {
		idtype = P_PGID;
		id = (id_t)u.u_procp->p_pgrp;
	} else if (uap->pid == -1) {
		idtype = P_ALL;
		id = P_MYID;
	} else { /* uap->pid < -1 */
		idtype = P_PGID;
		id = (id_t)(-uap->pid);
	}

	setprocset(&set, POP_AND, idtype, id, P_ALL, P_MYID);
	return sigsendset(&set, uap->sig);
}

/*
 * killsingle(pid, sig)
 *
 * sends a signal to just the specified process
 */
int
killsingle(pid, sig)
	pid_t pid;
	int sig;
{
	register proc_t *p;
	sigsend_t v;
	register int error;

	if ((p = prfind(pid)) == NULL)
		return ESRCH;
	
	/*
	 * As in the slow dotoprocs() interface, processes
	 * in the following states are skipped.
	 */
	if (p->p_stat == SIDL || p->p_stat == SZOMB)
		return ESRCH;

	/*
	 * If process is in the sys class return ESRCH.
	 */
	if (p->p_cid == 0)
		return ESRCH;

	v.sig = sig;
	v.perm = 0;
	v.checkperm = 1;

	error = sigsendproc(p, &v);

	if (error == 0 && v.perm == 0)
		return EPERM;

	return error;
}
