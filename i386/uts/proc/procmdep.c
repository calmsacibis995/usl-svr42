/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/procmdep.c	1.7"
#ident	"$Header: $"

/*
 * This file contains machine dependent proc routines.
 */

#include <util/sysmacros.h>
#include <util/param.h>
#include <util/types.h>
#include <proc/tss.h>
#include <mem/immu.h>
#include <fs/buf.h>
#include <util/cmn_err.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/siginfo.h>
#include <util/map.h>
#include <proc/reg.h>
#include <svc/utsname.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <proc/user.h>
#include <util/debug.h>
#include <util/var.h>
#include <util/inline.h>
#include <io/conf.h>
#include <proc/ucontext.h>
#include <fs/procfs/prsystm.h>
#include <proc/exec.h>
#include <mem/hat.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <util/fp.h>
#include <util/weitek.h>
#include <svc/isc.h>

#ifdef VPIX
extern int vpixenable;          /* Is VP/ix enabled or disabled */
#endif /* VPIX */

/*
 * Restore the current user context from the given context structure.
 */
void
restorecontext(ucp)
	register ucontext_t *ucp;
{
	register proc_t	*pp = u.u_procp;

	u.u_oldcontext = ucp->uc_link;

	/*
	 * Restore stack state.
	 */
	if (ucp->uc_flags & UC_STACK) {
		if (pp->p_stkbase != (caddr_t)ucp->uc_stack.ss_sp +
					ucp->uc_stack.ss_size - sizeof(int)) {
			pp->p_stksize = ucp->uc_stack.ss_size;
			u.u_sub = (u_long)ucp->uc_stack.ss_sp;
			u.u_userstack = (u_long)(u.u_sub + pp->p_stksize) -
						sizeof(int);
			pp->p_stkbase = (caddr_t)u.u_userstack;
		}
		if (ucp->uc_stack.ss_flags & SS_ONSTACK)
			bcopy((caddr_t)&ucp->uc_stack,
			      (caddr_t)&u.u_sigaltstack,
			      sizeof(struct sigaltstack));
		else
			u.u_sigaltstack.ss_flags &= ~SS_ONSTACK;
	}

	/*
	 * Restore general registers.
	 */
	if (ucp->uc_flags & UC_CPU)
		prsetregs(PTOU(pp), ucp->uc_mcontext.gregs);

	/*
	 * Restore floating point registers.
	 */
#ifdef WEITEK
	if (ucp->uc_flags & (UC_FP|UC_WEITEK)) 
#else
	if (ucp->uc_flags & UC_FP) 
#endif
	{
		prsetfpregs(pp, &ucp->uc_mcontext.fpregs);
#ifdef WEITEK
		if (ucp->uc_flags & UC_FP)
#endif
			u.u_fpvalid = 1;
	}
 	else
 		u.u_fpvalid = 0;
 
 	/* If this process owns the floating point unit,                */
 	/* give up ownership,                                           */
 	/* and set the TS bit so that the saved floating point state    */
 	/* will be restored if this process uses it again.              */
 	if (fp_proc == u.u_procp) {
 		fp_proc = (proc_t *)0;
 		setts();
	}

#ifdef WEITEK
	/* Were we using the Weitek before the signal? */
	if (ucp->uc_flags & UC_WEITEK) {
 		init_weitek();
 		/* clear AE byte of context register */
 		clear_weitek_ae ();
 		weitek_restore(u.u_weitek_reg);
 	} 
#endif

	/*
	 * Restore signal mask.
	 */
	if (ucp->uc_flags & UC_SIGMASK) {
		sigutok(&ucp->uc_sigmask,&u.u_procp->p_hold);
		sigdiffset(&u.u_procp->p_hold,&cantmask);
	}
}

/*
 * Save the current user context.
 */
void
savecontext(ucp, mask)
register ucontext_t *ucp;
k_sigset_t mask;
{
	register proc_t	*pp = u.u_procp;

	ucp->uc_flags = UC_ALL;
	ucp->uc_link = u.u_oldcontext;

	/*
	 * Save current stack state 
	 */
	ucp->uc_stack.ss_sp = (char *)
		(pp->p_stkbase + sizeof(int) - pp->p_stksize);
	ucp->uc_stack.ss_size = pp->p_stksize;
	ucp->uc_stack.ss_flags = 0;
	if (ucp->uc_stack.ss_sp == u.u_sigaltstack.ss_sp)
		ucp->uc_stack.ss_flags |= SS_ONSTACK;

 	/* If this process owns the floating point unit,                */
 	/* save its state in the u block,                               */
 	/* and set the TS bit so that the unit will be re-initialized   */
 	/* if it used in the signal handler.                             */
 	if (fp_proc == u.u_procp) {
 		fpsave();
 		setts();
 	}
#ifdef WEITEK
 	if (u.u_procp == weitek_proc)
 		weitek_save();
#endif

	/* 
	 * Save machine context 
	 */
	prgetregs(PTOU(pp), ucp->uc_mcontext.gregs);
	prgetfpregs(pp, &ucp->uc_mcontext.fpregs);

	/* At this point, u.u_fpvalid indicates whether the process
	 * is using floating point, since we are after the call to
	 * fpsave.
	 */
	if (u.u_fpvalid)
		u.u_fpvalid = 0;
	else
		ucp->uc_flags &= ~UC_FP;

#ifdef WEITEK
 	if (u.u_procp != weitek_proc)
		ucp->uc_flags &= ~UC_WEITEK;
#endif

	/* save signal mask */
	sigktou(&mask,&ucp->uc_sigmask);

 	((flags_t *)&u.u_ar0[EFL])->fl_tf = 0;    /* disable single step */
}

struct setcontexta {
	int flag;
	caddr_t *ucp;
};

/*
 * This is the entry point for the getcontext(2) and
 * setcontext(2) system calls.
 */
/* ARGSUSED */
setcontext(uap, rvp)
	register struct setcontexta *uap;
	rval_t *rvp;
{
	ucontext_t uc;

	/*
	 * In future releases, when the ucontext structure grows,
	 * getcontext should be modified to only return the fields
	 * specified in the uc_flags.
	 * That way, the structure can grow and still be binary
	 * compatible will all .o's which will only have old fields
	 * defined in uc_flags
	 */

	switch (uap->flag) {

	default:
		return EINVAL;

	case GETCONTEXT:
		savecontext(&uc, u.u_procp->p_hold);
		if (copyout((caddr_t)&uc,(caddr_t)uap->ucp,sizeof(ucontext_t)))
			return EFAULT;
		return 0;

	case SETCONTEXT:
		if (uap->ucp == NULL)
			exit(CLD_EXITED, 0);
		if (copyin((caddr_t)uap->ucp,(caddr_t)&uc,sizeof(ucontext_t)))
			return EFAULT;
		restorecontext(&uc);
		/* 
		 * On return from system calls, eax and edx are overwritten with 
		 * r_val1 and r_val2 respectively, so set r_val1 and r_val2 to 
		 * eax and edx here.
		 */
                rvp->r_val1 = u.u_ar0[EAX];
		rvp->r_val2 = u.u_ar0[EDX];
		return 0;
	}
}

/* These structures define what is pushd on the stack */

struct argpframe {
	void		(*retadr)();
	u_int		signo;
	siginfo_t	*sip;
	ucontext_t	*ucp;
};

struct compat_frame {
	void		(*retadr)();
	u_int		signo;
	gregset_t	gregs;
	char		*fpsp;
	char		*wsp;
};

/*
 * Set up signal stack and
 * dispatch signal handler.
 */

int
sendsig(sig, sip, hdlr)
	int sig;
	k_siginfo_t *sip;
	register void (*hdlr)();
{
	ucontext_t uc;
	siginfo_t si;
	int newstack;		/* if true, switching to alternate stack */
	int minstacksz;		/* size of stack required to catch signal */
 	register uint sp, ap;
	proc_t	*p = u.u_procp;
	int	setsegregs = 0;		/* set segment register flag */
	char 	*dp;			/* pointer a stack descriptor in LDT */
	struct argpframe argpframe;
	int	old_style;

#ifdef VPIX
	flags_t	*flags = (flags_t *)&(u.u_ar0[EFL]);

	/* This routine is called only when we are going to user
	 * mode. If it is a dual mode process and we are going
	 * back to 386 user mode process normally. If we are going
	 * back to v86 mode, then force a virtual interrupt. This
	 * force a switch to 386 user mode.
	 */

	if (vpixenable && flags->fl_vm) {       /* If going to V86 mode */
 		v86sighdlint(hdlr, sig);	/* Signal handler routine */
		return 1;
	}
#endif

	old_style = sigismember(&u.u_oldsig, sig);
	if (old_style) {
		minstacksz =
			sizeof(ucontext_t) + 	/* user context structure */
			sizeof(struct compat_frame); 	/* current signal */
		sip = NULL;
	} else {
		minstacksz = 
			sizeof(ucontext_t) + 	/* user context structure */
			sizeof(struct argpframe); 	/* current signal */
		if (sip != NULL) {
			bzero((caddr_t)&si, sizeof(si));
			bcopy((caddr_t)sip, (caddr_t)&si, sizeof(k_siginfo_t));
			minstacksz += sizeof(siginfo_t);
		}
	}
 
 	newstack = (sigismember(&u.u_sigonstack, sig)
	  && !(u.u_sigaltstack.ss_flags & (SS_ONSTACK|SS_DISABLE))); 

	if (newstack != 0) {
 		if (minstacksz >= u.u_sigaltstack.ss_size) {
			return 0;
		}
 		sp = (uint)u.u_sigaltstack.ss_sp + u.u_sigaltstack.ss_size;
 	} else {
 		register uint sub;

		sub = u.u_ar0[UESP];

		/* if the stack segment selector is not the
		 * 386 user data selector, convert the SS:SP
	  	 * to the equivalent 386 virtual address;
		 * and set the flag to load SS and DS with
		 * the correct values after they have been saved.
		 */

		if (u.u_ar0[SS] != USER_DS) {
			dp = (char *)(((struct dscr *)(u.u_procp->p_ldt))
				+ seltoi(u.u_ar0[SS]));
			sub &= 0xFFFF;
			sub += (dp[7] << 24) | (*(int *)&dp[2] & 0x00FFFFFF);
			setsegregs++;
		}
 		
		sp = sub;
		sub -= minstacksz;
 		if (sub < (uint)u.u_sub && !grow((int *)sub)) {
			return 0;
		}
	}

	/*
	 * Copy signal information to stack.
 	 */
	if (sip != NULL) {
		sp -= sizeof(siginfo_t);
		if (copyout((caddr_t)&si, (caddr_t)sp, sizeof(siginfo_t)) < 0) 
			return 0;
		argpframe.sip = (siginfo_t *)sp;
	} else
		argpframe.sip = NULL;

	/*
	 * Get the current user context and copy to stack.
	 */
	savecontext(&uc, u.u_sigoldmask);

#ifdef WEITEK
	if (sig == SIGFPE)
		weitek_reset_intr();
#endif /* WEITEK */

	sp -= sizeof(ucontext_t);
	if (copyout((caddr_t)&uc, (caddr_t)sp, sizeof(ucontext_t)) < 0) 
		return 0;
	argpframe.ucp = (ucontext_t *)sp;

	/*
	 * Set up the argument frame structure and copy to stack.
	 */
	if (old_style) {
		struct compat_frame	cframe;
		int	user_signo;		/* Signal number to send to the user */


/* Enhanced Application Compatibility Support */
		/* 
		 * Convert the SVR4 Signal number to the signal number the process
		 * may be expecting.  This may different because of the ISC signal 
		 * emulation which uses old style signal handling.
		 */
		user_signo = sig;	
	        if (ISC_USES_POSIX)
	                switch (sig) {
	                case SIGCONT:
	                        user_signo = ISC_SIGCONT;
	                        break;
	                case SIGSTOP:
	                        user_signo = ISC_SIGSTOP;
	                        break;
	                case SIGTSTP:
	                        user_signo = ISC_SIGTSTP;
	                        break;
	                }
/* End Enhanced Application Compatibility Support */

		cframe.retadr = u.u_sigreturn;
		cframe.signo = user_signo;
		bcopy((caddr_t)uc.uc_mcontext.gregs, (caddr_t)cframe.gregs,
				sizeof(gregset_t));
		cframe.fpsp = (char *)&argpframe.ucp->uc_mcontext.fpregs.fp_reg_set;
		cframe.wsp = (char *)&argpframe.ucp->uc_mcontext.fpregs.f_wregs[0];

		sp -= sizeof(struct compat_frame);
		if (copyout((caddr_t)&cframe, (caddr_t)sp,
					sizeof(struct compat_frame)) < 0) 
			return 0;
	} else {
		argpframe.retadr = (void (*)())0xFFFFFFFF;
					/* Shouldn't return via this;
					   if they do, fault. */
		argpframe.signo = sig;

		sp -= sizeof(struct argpframe);
		if (copyout((caddr_t)&argpframe, (caddr_t)sp,
					sizeof(struct argpframe)) < 0) 
			return 0;
	}

	/* now that we can no longer fault, update the u-block */

	/* push context */
	u.u_oldcontext = argpframe.ucp;

	u.u_ar0[EIP] = (unsigned int) hdlr;
	u.u_ar0[UESP] = (unsigned int) sp;
	
	if (setsegregs) {
		u.u_ar0[DS] = u.u_ar0[ES] = u.u_ar0[SS] = USER_DS;
		u.u_ar0[CS] = USER_CS;
	}

	((flags_t *)&u.u_ar0[EFL])->fl_tf = 0;	/* disable single step */

	if (newstack) {
		u.u_sigaltstack.ss_flags |= SS_ONSTACK;
 		u.u_sub = (ulong)u.u_sigaltstack.ss_sp; 
		u.u_userstack = (ulong)(u.u_sub + u.u_sigaltstack.ss_size -
					sizeof(int));
		u.u_procp->p_stkbase = (caddr_t)u.u_userstack;
		u.u_procp->p_stksize = (u_int)u.u_sigaltstack.ss_size;
	}
	return 1; 
}

/*
 * Restore user's context after execution of user signal handler
 * This code restores all registers to what they were at the time
 * signal occured. So any changes made to things like flags will
 * disappear.
 *
 * The saved context is assumed to be at esp+xxx address on the user's
 * stack. If user has mucked with his stack, he will suffer.
 * Called from the sig_clean.
 *
 * On entry, assume all registers are pushed.  r0ptr points to registers
 * on stack.
 * This function returns like other system calls.
 */

sigclean(r0ptr)
register int	*r0ptr;		/* registers on stack */
{
	register struct compat_frame *cframe;
	ucontext_t	uc, *ucp;

	/* The user's stack pointer currently points into compat_frame
	 * on the user stack.  Adjust it to the base of compat_frame.
	 */
	cframe = (struct compat_frame *)(r0ptr[UESP] - 2 * sizeof(int));
	
	ucp = (ucontext_t *)(cframe + 1);
 
	if (copyin((caddr_t)ucp, (caddr_t)&uc, sizeof(ucontext_t)) == -1) {
		u.u_ar0 = r0ptr;	/* core will use these registers */
		exit( (core("core", u.u_procp, u.u_cred,
			u.u_rlimit[RLIMIT_CORE].rlim_cur, SIGSEGV) ?
				CLD_DUMPED|CLD_KILLED : CLD_KILLED), SIGSEGV);
		return;
	}
 
	/* Old stack frame has gregs in a different place.
	   Copy it into the ucontext structure. */
	if (copyin((caddr_t)&cframe->gregs, (caddr_t)&uc.uc_mcontext.gregs,
						sizeof(gregset_t)) == -1) {
		u.u_ar0 = r0ptr;	/* core will use these registers */
		exit( (core("core", u.u_procp, u.u_cred,
			u.u_rlimit[RLIMIT_CORE].rlim_cur, SIGSEGV) ?
				CLD_DUMPED|CLD_KILLED : CLD_KILLED), SIGSEGV);
		return;
	}

	restorecontext(&uc);
}
