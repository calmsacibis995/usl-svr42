/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/fork.c	1.13"
#ident	"$Header: $"

/*
 * This file contains entry points for the process creation
 * system calls fork() and vfork().
 */

#include <acc/audit/audit.h>
#include <fs/file.h>
#include <fs/procfs/procfs.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/rm.h>
#include <mem/seg_u.h>
#include <mem/vmparam.h>
#include <proc/acct.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/fp.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef WEITEK
#include <util/weitek.h>
#endif
#ifdef VPIX
#include <vpix/v86.h>
#endif

#if defined(__STDC__)
STATIC int procdup(proc_t *, proc_t *, int, int);
STATIC int setuctxt(proc_t *, user_t *, caddr_t, caddr_t, ulong, ulong, ulong);
STATIC int fork1(char *, rval_t *, int);
#else
STATIC int procdup();
STATIC int setuctxt();
STATIC int fork1();
#endif

/*
 * fork system call.
 */

fork(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	/*
	 * fork1() actually does the work.
	 * The third argument is the "isvfork" flag,
	 * set to 0 for fork.
	 */
	return fork1(uap, rvp, 0);
}

/*
 * vfork system call.
 */

vfork(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	/*
	 * fork1() actually does the work.
	 * The third argument is the "isvfork" flag,
	 * set to 1 for vfork.
	 */
	return fork1(uap, rvp, 1);
}

/*
 * fork1() is an internal routine, which performs both fork and
 * vfork operations, depending on the value of the "isvfork" argument
 *	(0 -> fork, 1 -> vfork).
 *
 * The "uap" argument is unused, since fork and vfork accept
 * no user arguments.
 */

/* ARGSUSED */
STATIC int
fork1(uap, rvp, isvfork)
	char *uap;
	rval_t *rvp;
	int isvfork;
{
	register npcond;
	pid_t newpid;
	int error = 0;

	sysinfo.sysfork++;

	/*
	 * Set flags for newproc():
	 *	NP_FAILOK - don't panic if unable to create process.
	 *	NP_VFORK  - vfork (share address space).
	 */
	npcond = NP_FAILOK | (isvfork ? NP_VFORK : 0);

	switch (newproc(npcond, &newpid, &error)) {
	case 1:	/* child -- successful newproc */
		rvp->r_val1 = u.u_procp->p_ppid;
		rvp->r_val2 = 1;	/* child */
		u.u_start = hrestime.tv_sec;
		u.u_ticks = lbolt;
		u.u_mem = rm_assize(u.u_procp->p_as);
		u.u_ior = u.u_iow = u.u_ioch = 0;
		u.u_procvirt = 0;
		u.u_uservirt = 0;
		u.u_acflag = AFORK;
		u.u_lock = 0;
		nfc_forkch(&error);	/* added for OpenNET NFA */
		break;
	case 0: /* parent */
		rvp->r_val1 = (int) newpid;
		rvp->r_val2 = 0;	/* parent */
		nfc_forkpar(&error);	/* added for OpenNET NFA */
		break;
	default:	/* couldn't fork */
		cmn_err(CE_NOTE, "fork1: newproc failed\n");
		error = EAGAIN;
		break;
	}
	return error;
}

/*
 * Create a new process -- internal version of sys fork().
 *
 * This changes the new proc structure and
 * alters only the u.u_procp of its u-area.
 *
 * It returns 1 in the new process, 0 in the old, -1 on failure.
 */

int
newproc(cond, pidp, perror)
	int cond;	/* Flags defined in proc.h "NP_xxxx" */
	pid_t *pidp;	/* Return new PID of child to parent	*/
	int *perror;	/* Return error status	*/
{
	extern void shmfork();
	extern int xsemfork();
	extern int xsdfork();

	register proc_t *pp, *cp;
	register n;
	proc_t *cpp;
	pid_t newpid;
	file_t *fp;
	int length=0;

#ifdef	VPIX
	extern	char	v86procflag;
#endif

	/*
	 * Get process ID for the new process.
	 */
	if ((newpid = pid_assign(cond, &cpp)) == -1) {
		cmn_err(CE_NOTE, "newproc: pid_assign failed\n");
		/* no proc table entry is available */
		if (cond & NP_FAILOK) {
			return -1;	/* out of memory or proc slot */
		} else {
			cmn_err(CE_PANIC, "newproc - no procs\n");
		}
	}

	/*
	 * Make proc entry for new proc.
	 */
	cp = cpp;
	pp = u.u_procp;
	cp->p_cstime = 0;
	cp->p_stime = 0;
	cp->p_cutime = 0;
	cp->p_utime = 0;
	cp->p_italarm[0] = NULL;
	cp->p_italarm[1] = NULL;
	cp->p_uid = pp->p_uid;
	cp->p_cred = pp->p_cred;
	crhold(pp->p_cred);
#ifdef VPIX
	cp->p_v86 = NULL;
#endif
#ifdef MERGE386
	cp->p_vm86p = NULL;
#endif	/* MERGE386 */
	cp->p_ignore = pp->p_ignore;
	cp->p_hold = pp->p_hold;
	cp->p_siginfo = pp->p_siginfo;
	cp->p_stat = SIDL;
	cp->p_alarmid = 0;
	cp->p_alarmtime = 0;
	cp->p_flag = SLOAD | (pp->p_flag & (SJCTL|SNOWAIT));

	/* Enforce per-user licensing */

	if (!(cp->p_user_license = (pp->p_user_license & PU_LIM_OK)) &&
	    cp->p_cred->cr_uid && cp->p_cred->cr_ruid &&
	    cttydev(pp) != NODEV &&
	    (pp->p_pgrp == 0)) {
		if (enable_user_alloc(EUA_FORK) != 0) { /* Can we register it? */
			cmn_err (CE_NOTE, 
			"Un-registered user attempted to fork, uid = %d, pid = %d",
				cp->p_cred->cr_uid, pp->p_pid);
			crfree(cp->p_cred);
			pid_exit(cp);	/* free the proc table entry */
			return -1;
		}
		cp->p_user_license |= PU_LOGIN_PROC|PU_LIM_OK;
	}

	cp->p_sessp = pp->p_sessp;
	SESS_HOLD(pp->p_sessp);

	/*
	 * Add the child process to its parent's process group.
	 */
	pgjoin(cp, pp->p_pgidp);

	if (cond & (NP_SYSPROC | NP_INIT)) {
		cp->p_exec = NULL;
		cp->p_flag |= (SSYS | SLOCK);
	} else {
		cp->p_exec = pp->p_exec;
	}

	cp->p_brkbase = pp->p_brkbase;
	cp->p_brksize = pp->p_brksize;
	cp->p_stkbase = pp->p_stkbase;
	cp->p_stksize = pp->p_stksize;
	cp->p_swlocks = 0;
	cp->p_segacct = 0;

	if (cond & NP_VFORK)	{
		cp->p_flag |= SVFORK;
	}

	cp->p_pid = newpid;

	if (newpid <= SHRT_MAX)	{
		cp->p_opid = (o_pid_t)newpid;
	} else	{
		cp->p_opid = (o_pid_t)NOPID;
	}

	cp->p_epid = newpid;
	cp->p_ppid = pp->p_pid;
	cp->p_oppid = pp->p_opid;
	cp->p_cpu = 0;
	cp->p_pri = pp->p_pri;
	cp->p_inoutage = 0;
	cp->p_slptime = 0;

	/*
	 * If inherit-on-fork, copy /proc tracing flags to child.
	 * New system processes never inherit tracing flags.
	 */
	if ((pp->p_flag & (SPROCTR|SPRFORK)) == (SPROCTR|SPRFORK) &&
	    !(cond & (NP_SYSPROC|NP_INIT))) {

		cp->p_flag |= (SPROCTR|SPRFORK);
		cp->p_sigmask = pp->p_sigmask;
		cp->p_fltmask = pp->p_fltmask;
	} else {
		sigemptyset(&cp->p_sigmask);
		premptyset(&cp->p_fltmask);
		/*
		 * Syscall tracing flags are in the u-block.
		 * They are cleared when the child begins execution, below.
		 */
	}
	cp->p_cid = pp->p_cid;
	cp->p_clfuncs = pp->p_clfuncs;
	cp->p_usize = pp->p_usize;

	/*
	 * Allocate and initialize a class specific
	 * process scheduling structure.
	 */
	if (CL_FORK(pp, pp->p_clproc, cp, &cp->p_stat, &cp->p_pri,
	  &cp->p_flag, &cp->p_cred, &cp->p_clproc)) {

		cmn_err(CE_NOTE, "newproc: Class specific fork fails\n");

		crfree(cp->p_cred);
		pid_exit(cp);	/* free the proc table entry */
		return -1;
	}

	/*
	 * Initialize child process's async request count.
	 */
	cp->p_aiocount = 0;
	cp->p_aiowcnt = 0;

#ifdef ASYNCIO
	/* 
	 * Initialize child process' raw disk async I/O count
	 */
	cp->p_raiocnt = 0;
#endif /* ASYNC IO */

	/*
	 * Link up to parent-child-sibling chain.  No need to lock
	 * in general since only a call to freeproc() (done by the
	 * same parent as newproc()) diddles with the child chain.
	 */
	cp->p_sibling = pp->p_child;
	cp->p_parent = pp;
	pp->p_child = cp;

	cp->p_nextorph = pp->p_orphan;
	cp->p_nextofkin = pp;
	pp->p_orphan = cp;

	cp->p_sysid = pp->p_sysid;	/* RFS HOOK */

	/*
	 * Make duplicate entries where needed.
	 */
	for (n = 0; n < u.u_nofiles; n++) {
		if (getf(n, &fp) == 0)	{
			fp->f_count++;
		}
	}

	VN_HOLD(u.u_cdir);
	if (u.u_rdir)	{
		VN_HOLD(u.u_rdir);
	}

        /*
         * save the floating point state for this process
	 */
	if (pp == fp_proc)
		fpsave();

#ifdef WEITEK
	if (pp == weitek_proc) {
		weitek_save();
	}
#endif
	
	/*
	 * Copy process.
	 */
	switch (procdup(cp, pp, (cond & (NP_VFORK|NP_SHARE)),
			(cond & NP_SYSPROC))) {
	case 0:
		/* Successful copy */
		break;
	case -1:
		
		cmn_err(CE_NOTE, "newproc: procdup fails \n");

		if (!(cond & NP_FAILOK))
			cmn_err(CE_PANIC, "newproc - fork failed\n");

		/* Reset all incremented counts. */

		pexit();

		CL_EXITCLASS(cp, cp->p_clproc);

                /*
                 * Clean up parent-child-sibling pointers.  No lock
                 * necessary since nobody else could be diddling with
                 * them here.
                 *
                 * The parent could have been sleeping in procdup and
                 * its children could have exit()'ed.  Exit() would
                 * then change the parent's p_orphan list and or
                 * p_child list (for the init process).  So do not
                 * assume that the new child is on the top of the
                 * list but search the list like freeproc().
                 */
		CLEAN_PCS(cp);

		crfree(cp->p_cred);
		pid_exit(cp);		/* free the proc table entry */
		if (pidp)	{
			*pidp = -1;
		}
		return -1;
	case 1:
		/* Child resumes here */
		if ((u.u_procp->p_flag & SPROCTR) == 0) {
			/*
			 * /proc tracing flags have not been
			 * inherited; clear syscall flags.
			 */
			u.u_systrap = 0;
			premptyset(&u.u_entrymask);
			premptyset(&u.u_exitmask);
		}
		if (pidp)	{
			*pidp = cp->p_ppid;
		}

		if (xsemfork())	{
			cmn_err(CE_NOTE, "newproc: xsemfork fails \n");
			return -1;
		}
		if (pp->p_sdp)	{
			*perror = xsdfork(cp, pp);
		}

		return 1;
	}


	/*
	 * Set the appropriate links for shared memory,
	 * if needed.
	 */
	if (u.u_nshmseg)	{
		shmfork(pp, cp);
	}

	cp->p_stat = SRUN;

	/*
	 * If we just created init process put it in the
	 * correct scheduling class.
	 */
	if (cond & NP_INIT) {
		if (getcid(initclass, &cp->p_cid) || cp->p_cid <= 0) {

			cmn_err(CE_PANIC,
"Illegal or unconfigured class (%s) specified for init process.\n\
Change INITCLASS configuration parameter.", initclass);
		}

		if (CL_ENTERCLASS(&class[cp->p_cid], NULL, cp, &cp->p_stat,
		  &cp->p_pri, &cp->p_flag, &cp->p_cred,
		  &cp->p_clproc, NULL, NULL)) {

			cmn_err(CE_PANIC,
"Init process cannot enter %s scheduling class.\n\
Verify that %s class is properly configured.", initclass, initclass);
		}

		cp->p_clfuncs = class[cp->p_cid].cl_funcs;

		/*
		 * We call CL_FORKRET with a NULL parent pointer
		 * in this special case because the parent may
		 * be in a different class from the function we are
		 * calling and its class specific data would be
		 * meaningless to the function.
		 */
		CL_FORKRET(cp, cp->p_clproc, NULL);
	} else {
		CL_FORKRET(cp, cp->p_clproc, pp->p_clproc);
	}

	if (pidp)	{
		*pidp = cp->p_pid;	/* parent returns pid of child */
	}

	if (cp->p_exec)	{
		VN_HOLD(cp->p_exec);
	}
	return 0;
}

/*
 * procdup() is an internal routine which duplicates the resources
 * of the parent process for the child process.
 */
STATIC int
procdup(cp, pp, isvfork, no_swap)
	proc_t	*cp;
	proc_t	*pp;
	register int	isvfork;
	int	no_swap;
{
	register user_t		*uservad;

	if (pp->p_as != NULL) {
		/*
		 * Duplicate address space of current process.  For
		 * vfork we just share the address space structure.
		 */
		if (isvfork) {
			cp->p_as = pp->p_as;
		} else {
			cp->p_as = as_dup(pp->p_as);
			if (cp->p_as == NULL) {
				cmn_err(CE_NOTE, "procdup: as_dup failed\n");
				return -1;
			}
		}
	}
	
	/* LINTED */
	if ((cp->p_segu = (struct seguser *)segu_get(cp, no_swap)) == NULL) {
		cmn_err(CE_NOTE, "procdup: segu_get failed\n");
		if (isvfork == 0 && cp->p_as != NULL)
			as_free(cp->p_as);
		cp->p_as = NULL;
		return -1;
	}
	uservad = PTOU(cp);
	cp->p_ldt = (caddr_t)uservad + (pp->p_ldt - (caddr_t)PTOU(pp));

	if (audit_on) {
		/* copy parent's process audit structure to child */
		if (adt_dupaproc(pp, cp)) {
			cmn_err(CE_NOTE, "procdup: adt_dupaproc failed\n");
			if (isvfork == 0 && cp->p_as != NULL)
				as_free(cp->p_as);
			cp->p_as = NULL;
			segu_release(cp);
                        return -1;
		}
	}

	/*
	 * Assumption that nothing past this point fails.
	 * Otherwise, segu_release() would have to handle 
	 * being called on a process that was not ONPROC.
	 */

	/*
	 * Setup child u-block.  The third argument is really just
	 *	a place-holder; see the comment on setuctxt below.
	 */
	if (setuctxt(cp, uservad, _esp(), _ebp(), _ebx(), _edi(), _esi()) == 1)
		return 1;

	/*
	 * Put the child on the run queue.
	 */
	cp->p_flag |= SULOAD;
	return 0;
}

/*
 * setuctxt initializes the u-block of the child.  It returns a value
 * of 0 to the parent.  The child is initialized so that, when it
 * resumes, it appears to return from setuctxt with a return value of 1.  
 *
 * Although machine dependent, setuctxt can be implemented almost
 * entirely in C.  One bit of hackery is necessary, so setuctxt
 * actually consists of one assembler instruction, and then falls
 * through to the C code, which is implemented in the procedure
 * _setuctxt.
 *
 * The purpose of the hackery is to get the stack pointer initialized
 * correctly in the child.  The child's sp should point to the stack
 * location which holds the return address from setuctxt.  The best
 * time to get this sp value is on entry to setuctxt.  Here's how
 * it works:
 *
 *	(1) procdup calls setuctxt with a place-holder value for the
 *		third argument.
 *
 *	(2) On entry to setuctxt, the third argument is overwritten
 *		with the sp value which is needed in the child.
 *
 *	(3) setuctxt falls through to _setuctxt, which does the
 *		real work.
 */
#ifdef	DEBUG
asm(".globl setuctxt");		/* STATIC int setuctxt(....) */
#endif
asm("setuctxt:");
asm("	movl %esp, 12(%esp)");	/* put in proper value for esp argument */

/* fall through */

STATIC int
_setuctxt(p, up, esp, ebp, ebx, edi, esi)
	proc_t *p;		/* child proc pointer */
	register user_t *up;	/* child u-block pointer */
	caddr_t	esp, ebp;	/* child register values */
	unsigned long ebx, edi, esi;
{
	struct ufchunk *pufp, *cufp, *tufp;
        caddr_t bottom;
        unsigned long offset;
	struct tss386 *tp;
	extern			resume();

	ASSERT((MINUSIZE <= p->p_usize) && (p->p_usize <= MAXUSIZE));

	/* Copy u-block.  XXX - The amount to copy is machine dependent. */

        bottom = (caddr_t)roundup(((unsigned long)u.u_tss + u.u_sztss),
                sizeof(struct dscr));
        bottom += u.u_ldtlimit * sizeof(struct dscr);
        offset = esp - (caddr_t)&u;
        bcopy(esp, (caddr_t)up + offset, bottom - esp);
	up->u_procp = p;
	pufp = u.u_flist.uf_next;
	cufp = &up->u_flist;
	cufp->uf_next = NULL;
	up->u_nofiles = 0;	/* temporally set to 0, avoid inconsistency */

	while (pufp) {
		tufp = (struct ufchunk *)kmem_alloc(sizeof(struct ufchunk), KM_SLEEP);
		*tufp = *pufp;
		tufp->uf_next = NULL;
		cufp->uf_next = tufp;
		cufp = tufp;
		pufp = pufp->uf_next;
	}
	up->u_nofiles = u.u_nofiles;	/* restore NFPCHUNK value from parent */
#ifdef ASYNCIO
	/*
	 * Children don't inherit RAIO locked memory
	 */
	up->u_raioaddr = 0;
	up->u_raiosize = 0;
#endif /* ASYNC IO */

	/*
	 * Set up descriptor's for the child process.
	 */
 	tp = (struct tss386 *)((char *)up + (uint)up->u_tss - UVUBLK);
 	setdscrbase(&up->u_tss_desc, (uint)tp);
	setdscrbase(&up->u_ldt_desc, p->p_ldt);
	tp->t_link = 0;
	tp->t_eip = (unsigned long)resume;
	tp->t_eflags = 2;
	tp->t_ebp = (unsigned long)ebp;
	tp->t_esp = (unsigned long)esp;
	tp->t_eax = 1;
	tp->t_ebx = ebx;
	tp->t_edx = UTSSSEL;
	tp->t_edi = edi;
	tp->t_esi = esi;
	tp->t_cs = KCSSEL;
	tp->t_ds = tp->t_es = tp->t_ss = KDSSEL;
	tp->t_fs = tp->t_gs = 0;
	return (0);
}
