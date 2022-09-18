/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/exit.c	1.12"
#ident	"$Header: $"

/*
 * This file contains process deletion routines,
 * including the exit(), wait(), and waitid() system calls.
 */

#include <acc/audit/audit.h>
#include <fs/vnode.h>
#include <mem/page.h>
#include <proc/class.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/swnotify.h>
#include <proc/user.h>
#include <proc/wait.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/fp.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

/* Enhanced Application Compatibility Support */
#include <io/termios.h>
#include <proc/reg.h>
#include <svc/sco.h>
/* End Enhanced Application Compatibility Support */

#ifdef WEITEK
#include <util/weitek.h>
#endif

#ifdef VPIX
extern int vpixenable;		/* Is VP/ix enabled or disabled */
#endif /* VPIX */
#ifdef MERGE386
extern int merge386enable;	/* Is merge386 enabled or disabled */
#endif /* MERGE386 */

#ifdef KPERF
int kpfexitflg;
#endif /* KPERF */

extern void semexit();
extern void shmexit();
extern void xsdexit();
extern void hdeexit();

/*
 * wstat() is an internal routine that
 * converts a code/data pair into an old style wait status
 */

STATIC int
wstat(code, data)
{
	register stat = (data & 0377);

	switch (code) {
		case CLD_EXITED:
			stat <<= 8;
			break;
		case CLD_DUMPED:
			stat |= WCOREFLG;
			break;
		case CLD_KILLED:
			break;
		case CLD_TRAPPED:
		case CLD_STOPPED:
			stat <<= 8;
			stat |= WSTOPFLG;
			break;
		case CLD_CONTINUED:
			stat = WCONTFLG;
			break;
	}
	return stat;
}
			
/*
 * rexit() is the entry point for the
 * exit(2) system call: pass back caller's arg.
 */

struct exita {
	int	rval;
};

/* ARGSUSED */

void
rexit(uap, rvp)
	register struct exita *uap;
	rval_t *rvp;
{
	/* exit() does all the work */
	exit(CLD_EXITED, uap->rval);
}

/*
 * Release resources.
 * Enter zombie state.
 * Wake up parent and init processes,
 * and dispose of children.
 */
		
void
exit(why, what)
	int why;	/* reason for the exit */
	int what;	/* return status for the process */
{
	register int rv;
	register struct proc *p, *q;
	struct ufchunk *ufp, *nufp;
	sess_t *sp = u.u_procp->p_sessp;
	swnotify_t *swp;

	p = u.u_procp;
	p->p_flag &= ~STRC;

	if (p->p_alarmid)
		untimeout(p->p_alarmid);

	ASSERT(p->p_curinfo == NULL);
	ASSERT(p->p_cursig == 0);

	sigfillset(&p->p_ignore);	/* ignore all signals */
	sigemptyset(&p->p_sig);		/* clear pending signals */
	sigemptyset(&p->p_sigmask);	/* clear tracing signal mask */

	/*
	 * Unlink and free all siginfo structures
	 * for the process.
	 */
	sigdelq(p, 0);

	/*
	 * Cancel any context-switch notifications.
	 */
	if ((swp = p->p_swtch_in) != NULL) {
		do {
			swp->swn_contextp = NULL;
		} while ((swp = swp->swn_next) != NULL);
		p->p_swtch_in = NULL;
	}

	closeall(1);	/* close all files */

	(void)hrt_cancel_proc();	/* cancel outstanding alarms */

	/*
	 * If the process is the session leader,
	 * free the controlling tty.
	 */
	if (sp->s_sidp == p->p_pidp && sp->s_vp != NULL)
		freectty(sp);

	if (p->p_user_license & PU_LOGIN_PROC)
		enable_user_alloc (EUA_REM_USER);

	u.u_nofiles = 0;	/* zap nofiles so that fuser will work */

	/*
	 * Free memory allocated for file pointers.
	 */
	ufp = u.u_flist.uf_next;
	u.u_flist.uf_next = (struct ufchunk *)NULL;
	while (ufp) {
		nufp = ufp->uf_next;
		kmem_free(ufp, sizeof(struct ufchunk));
		ufp = nufp;
	}

#ifdef ASYNCIO
	/*
	 * Unlock memory that was locked for raw disk async i/o.
	 * Pending/current i/o was flushed above (in closef/closeall)
	 */
	if (u.u_raioaddr != 0)  {
		(void) (as_fault(p->p_as, u.u_raioaddr, u.u_raiosize,
		    F_SOFTUNLOCK, S_WRITE));

		u.u_raioaddr = 0;
		u.u_raiosize = 0;
	}
#endif /* ASYNC IO */

	(void)punlock();	/* unlock the process */

	
	/*
	 * Release the vnode for the process' current directory.
	 */
	VN_RELE(u.u_cdir);
	/*
	 * Now that u.u_cdir is stale, reassign it a safe value in case
	 * someone looks while we sleep.  Don't hold rootdir because it
	 * would need to be released and it won't go away anyway.  
	 */
	u.u_cdir = rootdir;
 	if (u.u_rdir) {
		VN_RELE(u.u_rdir);
		u.u_rdir = NULLVP;
	}

	/*
         * disown 80387 chip
	 */
	if (p == fp_proc) {
		fp_proc = NULL;
	}

#ifdef WEITEK
	u.u_weitek = WEITEK_NO;
	if (p == weitek_proc) {
		weitek_proc = NULL;
	}
#endif

	/*
	 * Insert calls to "exitfunc" functions.
	 */
#ifdef VPIX
	if (vpixenable)
		v86exit(p);
#endif

	/*
	 * Release semaphores and shared memory.
	 */
	semexit();
	if (u.u_nshmseg)
		shmexit(p);
	if (p->p_sdp)
		xsdexit();	/* XENIX shared data exit */

#ifdef MERGE386
	if (merge386enable)
		vm86exit();	/* if we are exiting a vm86 task, clean up */
#endif /* MERGE386 */

	rv = wstat(why, what);	/* format exit status */

	nfc_exit();		/* added for OpenNET NFA */

	acct(rv & 0xff);	/* write a record to the accounting file */

	/* 
	 * Free address space.
	 * relvm can take long; preempt. 
	 */

	PREEMPT();
	relvm(p, ISNOTADAEMON);
	PREEMPT();

	/* 
 	 * If auditing exit, write a record 
 	 */
 	if (audit_on)
 		adt_exit(what);

	adt_freeaproc(p);	/* Free audit process structure */
	crfree(u.u_cred);	/* free the cred structure */

	p->p_utime += p->p_cutime;
	p->p_stime += p->p_cstime;

	/*
	 * Find the next of kin for processes orphaned
	 * by this exit.
	 */
	if ((q = p->p_orphan) != NULL) {

		register proc_t *nokp = p->p_nextofkin;

		for (; ; q = q->p_nextorph) {
			q->p_nextofkin = nokp;
			if (q->p_nextorph == NULL)
				break;
		}

		q->p_nextorph = nokp->p_orphan;
		nokp->p_orphan = p->p_orphan;
		p->p_orphan = NULL;

	}

	/*
	 * Give all of the process' children to init().
	 */
	if ((q = p->p_child) != NULL) {

		pgdetach(p);	/* check for orphaned process groups */

		for (; ; q = q->p_sibling) {
			q->p_ppid = 1;
			q->p_oppid = 1;
			q->p_parent = proc_init;
			if (q->p_flag & STRC)
				psignal(q, SIGKILL);
			sigcld(q);
			if (q->p_sibling == NULL)
				break;
		}

		q->p_sibling = proc_init->p_child;
		proc_init->p_child = p->p_child;
		p->p_child = NULL;

	} else 
		/* If I am the only member in the group or group
	  	 * is already orphaned then we do not
		 * have to do anything.
		 */
		if(!p->p_pgidp->pid_pgorphaned && p->p_pgidp->pid_pglink->p_pglink != NULL)
			pgorphan(p->p_pgidp,p,0);
		

	/*
	 * Hook for /proc.
	 * If process is undergoing /proc I/O, it must be a system process.
	 * Become an ordinary process and perform preempt() in order to keep
	 * the ublock around until the controlling process is done with it.
	 */
	if (p->p_flag & SPROCIO) {
		ASSERT(p->p_flag & SSYS);
		p->p_flag &= ~SSYS;
		availrmem += USIZE;
		pages_pp_kernel -= USIZE;
		preempt();
	}

	/*
	 * Release a.out vnode, if process was exec'ed.
	 */
	if (p->p_exec) {
		VN_RELE(p->p_exec);
		p->p_exec = NULLVP;
	}

	/*
	 * These MUST be set here because parents waiting for children
	 * will only check p_wcode since more conditions than zombie
	 * status can be waited for.
	 */

	p->p_stat = SZOMB;	/* enter zombie state */
	p->p_wcode = (char)why;	/* wait code is reason for exit */
	p->p_wdata = what;	/* wait return value */

	if (p->p_trace)
		wakeprocs((caddr_t)p->p_trace, PRMPT);

	/*
	 * Perform wakeups on this process and its parent.
	 * Send SIGCHLD signal to the parent.
	 */
	sigcld(p);

	/*
	 * Call class-specific function to clean up as necessary.
	 */
	
	CL_EXITCLASS(p, p->p_clproc);

	/* pswtch() frees up stack and ublock */

#ifdef KPERF
	if(kpftraceflg)
		kpfexitflg = 1;
#endif /* KPERF */

	swtch();

	/* NOTREACHED */
}

/*
 * Format siginfo structure for wait system calls.
 */

void
winfo(pp, ip, waitflag)
	register proc_t *pp;
	register k_siginfo_t *ip;
{
	struct_zero((caddr_t)ip, sizeof(k_siginfo_t));
	ip->si_signo = SIGCLD;
	ip->si_code = pp->p_wcode;
	ip->si_pid = pp->p_pid;
	ip->si_status = pp->p_wdata;
	ip->si_stime = pp->p_stime;
	ip->si_utime = pp->p_utime;
	
	if (waitflag) {
		pp->p_wcode = 0;
		pp->p_wdata = 0;
	}
}

/*
 * Wait system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped children,
 * and pass back status from them.
 */

int
waitid(idtype, id, ip, options)
	idtype_t idtype;	/* type of id for process search */
	id_t id;		/* id of "idtype" type */
	int options;		/* options (defined in wait.h, see waitid(2)) */
	k_siginfo_t *ip;	/* pass back status through siginfo pointer */
{
	int found;
	register proc_t *cp, *pp;

	/*
	 * If options is not set or if options is set with 
	 * invalid values, return EINVAL.
	 */
	if ((options == 0) || (options & ~WOPTMASK))
		return EINVAL;
	
	/*
	 * Validate "idtype" and "id".
	 */
	switch (idtype) {
		case P_PID:
		case P_PGID:
			if (id < 0 || id >= MAXPID)
				return EINVAL;
		case P_ALL:
			break;
		default:
			return EINVAL;
	}
	
	/*
	 * Search children for process that satisfies the criteria
	 * defined by "idtype", "id", and "options".
	 */
	pp = u.u_procp;
	while ((cp = pp->p_child) != NULL) {

		found = 0;

		do {

			if (idtype == P_PID && id != cp->p_pid)
				continue;	/* process not the one we want */

			if (idtype == P_PGID && id != cp->p_pgrp)
				continue;	/* not in desired process group */

			found++;

			/*
			 * The setting of p_wcode due to a process exiting
			 * is only done after the process has entered the
			 * zombie state.
			 */
			switch (cp->p_wcode) {

			case CLD_EXITED:
			case CLD_DUMPED:
			case CLD_KILLED:
				if (!(options & WEXITED)) {
					/*
					 * This process can never satisfy the
					 * specified options, so don't count
					 * it as a candidate.
					 */
					--found;
					break;
				}
				if (options & WNOWAIT)
					winfo(cp, ip, 0);
				else {
					winfo(cp, ip, 1);
					pp->p_cpu += cp->p_cpu;
					if (pp->p_cpu > 80 || pp->p_cpu < 0)
						pp->p_cpu = 80;
					freeproc(cp);
				}
				return 0;

			case CLD_TRAPPED:
				if (!(options & WTRAPPED))
					break;
				winfo(cp, ip, !(options & WNOWAIT));
				return 0;

			case CLD_STOPPED:
				if (!(options & WSTOPPED))
					break;
				winfo(cp, ip, !(options & WNOWAIT));
				return 0;

			case CLD_CONTINUED:
				if (!(options & WCONTINUED))
					break;
				winfo(cp, ip, !(options & WNOWAIT));
				return 0;
			}

			if (idtype == P_PID)
				break;

		} while ((cp = cp->p_sibling) != NULL);

		if (!found)
			break;

		/*
		 * Return immediately.
		 */
		if (options & WNOHANG) {
			ip->si_pid = 0;
			return 0;
		}

		/*
		 * Wait for process to change state.
		 */
		if (idtype == P_PID)
			sleep((caddr_t)cp, PWAIT);
		else
			sleep((caddr_t)pp, PWAIT);

	}
	/*
	 * Did not find child.
	 */
	return ECHILD;
}

/*
 * For implementations that don't require binary compatibility,
 * the wait system call may be made into a library call to the
 * waitid system call.
 */

struct waita {
	int *stat_loc; /* the library function copies this value from r_val2 */
};

/* Enhanced Application Compatibility Support */
struct waita_sco {
	pid_t   pid;
	int     *stat_loc;
	int     opt;
};
/* End Enhanced Application Compatibility Support */

/*
 * Entry point for wait(2) system call.
 */
/* ARGSUSED */
int
wait(uap, rvp)
	struct waita *uap;
	rval_t *rvp;
{
	register error;
	k_siginfo_t info;
	flags_t *eflgs;

	/* Enhanced Application Compatibility Support */
        eflgs = (flags_t *)&u.u_ar0[EFL];
	if (IS_SCOEXEC && SCO_DOWAITPID(eflgs)) {
		int		opt;
		idtype_t	idtype;
		pid_t		pid;
		id_t		id;
		struct waita_sco *nuap;

		nuap = (struct waita_sco *)uap;
		if (nuap->opt & ~(SCO_WNOHANG | SCO_WUNTRACED))
			return(EINVAL);
		opt = WEXITED|WTRAPPED;
		if (nuap->opt & SCO_WNOHANG)
			opt |= WNOHANG;
		if (nuap->opt & SCO_WUNTRACED)
			opt |= WUNTRACED;

		pid = nuap->pid;
		if (pid > 0) {
			idtype = P_PID;
			id = pid;
		} else if (pid < -1) {
			idtype = P_PGID;
			id = -pid;
		} else if (pid == -1) {
			idtype = P_ALL;
			id = 0;
		} else {
			idtype = P_PGID;
			id = u.u_procp->p_pgrp;
		}

		error =  waitid(idtype, id, &info, opt);
	} else 
		error =  waitid(P_ALL, (id_t)0, &info, WEXITED|WTRAPPED);
	/* End Enhanced Application Compatibility Support */

	if (error)
		return error;
	rvp->r_val1 = info.si_pid;
	rvp->r_val2 = wstat(info.si_code, info.si_status);
	return 0;
}

struct waitida {
	idtype_t idtype;
	id_t	 id;
	siginfo_t *infop;
	int	 options;
};

/*
 * waitsys() is the entry point for the waitid(2)
 * and waitpid(2) system calls.
 */
/* ARGSUSED */
int
waitsys(uap, rvp)
	struct waitida *uap;
	rval_t *rvp;
{
	register error;
	k_siginfo_t info;

	error =  waitid(uap->idtype, uap->id, &info, uap->options);
	if (error)
		return error;
	if (copyout((caddr_t)&info, (caddr_t)uap->infop, sizeof(k_siginfo_t)))
		return EFAULT;
	return 0;
}

/*
 * Wait for child to exec or exit.
 * Called by parent of vfork'ed process.
 */

void
vfwait(pid)
	pid_t pid;
{
	register proc_t *pp = u.u_procp;
	register proc_t *cp = prfind(pid);

	ASSERT(cp != NULL && cp->p_parent == pp);

	/*
	 * Wait for child to exec or exit.
	 */
	while (cp->p_flag & SVFORK)	{
		(void) sleep((caddr_t)cp, PZERO-1);
	}

	/*
	 * Copy back sizes to parent; child may have grown.
	 * This should be the only info outside the
	 * "as" struct that needs to be shared like this!
	 */
	if (pp->p_brkbase == cp->p_brkbase)	{
		pp->p_brksize = cp->p_brksize;
	}
	if (pp->p_stkbase == cp->p_stkbase)	{
		pp->p_stksize = cp->p_stksize;
	}

	/*
	 * Wake up child, send it on its way.
	 */
	cp->p_flag |= SVFDONE;
	wakeprocs((caddr_t)cp, PRMPT);
}

/*
 * Remove zombie children from the process table.
 */

void
freeproc(p)
	proc_t *p;
{
        ASSERT(p->p_stat == SZOMB);

        p->p_nextofkin->p_cutime += p->p_utime;
	p->p_nextofkin->p_cstime += p->p_stime;

        if (p->p_trace)
		prexit(p);

	/*
	 * Locate the correct p_orphan/p_nextorphan and
	 * p_child/p_sibling links and unlink the process.
	 */
	CLEAN_PCS(p);

	pid_exit(p);	/* frees pid and proc structure */
}

/*
 * Clean up common process stuff -- called from newproc()
 * on error in fork() due to no swap space.
 */

void
pexit()
{
	/* Don't include punlock() since not needed for newproc() clean. */

	closeall(0);
	sigdelq(u.u_procp, 0);

	VN_RELE(u.u_cdir);
	if (u.u_rdir)
		VN_RELE(u.u_rdir);

	semexit();
}

/*
 * Release virtual memory.
 * Called by exit and getxfile (via execve).
 *
 * daemon_ind allows certain implementations to
 * not free the address space as a process without
 * an address space may not be valid for them. We
 * (i386) ignore it.
 */

void
relvm(p, daemon_ind)
	register proc_t *p;		/* process exiting or exec'ing */
	register int	daemon_ind;	/* this is a kernel process */
{
	if ((p->p_flag & SVFORK) == 0) {
		if (p->p_as != NULL) {
			as_free(p->p_as);
			p->p_as = NULL;
		}
	} else {
		p->p_flag &= ~SVFORK;	/* no longer a vforked process */
		p->p_as = NULL;		/* no longer using parent's adr space */
		wakeprocs((caddr_t)p, PRMPT);	/* wake up parent */

		while ((p->p_flag & SVFDONE) == 0) {	/* wait for parent */
			(void) sleep((caddr_t)p, PZERO - 1);
		}
		p->p_flag &= ~SVFDONE;	/* continue on to die or exec */
	}
}

