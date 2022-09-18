/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:proc/procset.c	1.6.2.4"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <util/var.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <fs/vnode.h>
#include <svc/errno.h>
#include <svc/syscall.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <util/cmn_err.h>
#include <proc/session.h>
#include <acc/mac/mac.h>
#include <acc/mac/covert.h>
#include <acc/audit/audit.h>
#include <mem/kmem.h>

static id_t	getmyid();
static int	checkprocset();

/*
 * The dotoprocs function locates the process(es) specified
 * by the procset structure pointed to by psp.  If funcp
 * is non-NULL then it points to a function which dotoprocs
 * will call for each process in the specified set.  The
 * arguments to this function will be a pointer to the
 * current process from the set and arg.
 * If the called function returns -1, it means that processing of the
 * procset should stop and a normal (non-error) return should be made
 * to the caller of dotoprocs.
 * If the called function returns any other non-zero value the search
 * is terminated and the function's return value is returned to
 * the caller of dotoprocs.  This will normally be an error code.
 * Otherwise, dotoprocs will return zero after processing the entire
 * process set unless no processes were found in which case ESRCH will
 * be returned.
 */
int
dotoprocs(psp, funcp, arg)
	register procset_t	*psp;
	int			(*funcp)();
	char			*arg;
{
	register proc_t	*prp;	/* A process from the set */
	register int	error;
	register int	nfound;	/* Nbr of processes found.	*/
	register proc_t	*lastprp;	/* Last proc found.	*/
	register int	nsearched;	/* procs searched in loop */
	register pid_t  *pidlistp = NULL; /* list of pids for audit */
	int		psearch;	/* min procs search */
	int		count;	/* count of pids for audit */	

	/*
	 * Check that the procset_t is valid.
	 */
	error = checkprocset(psp);
	if (error) {
		return error;
	}

	/*
	 * Check for the special value P_MYID in either operand
	 * and replace it with the correct value.  We don't check
	 * for an error return from getmyid() because the idtypes
	 * have been validated by the checkprocset() call above.
	 */

	if (psp->p_lid == P_MYID) {
		psp->p_lid = getmyid(psp->p_lidtype);
	}

	if (psp->p_rid == P_MYID) {
		psp->p_rid = getmyid(psp->p_ridtype);
	}

	nfound = 0;
	error  = 0;
	nsearched = 0;
	count=0;

	if ((u.u_syscall == SYS_kill) || (u.u_syscall == SYS_sigsendsys)) { 
		if (audit_on) {
			ASSERT(u.u_procp->p_aprocp != NULL);
			if ((!(u.u_procp->p_aprocp->a_flags & AEXEMPT)) 
			       && EVENTCHK(ADT_KILL,u.u_procp->p_aprocp->a_procemask)) {
				/* 
				 * Allocate space to maintain a list of pids to
				 * which the signal will be posted. It will be
			         * used by the audit recording function.
				 */
				pidlistp=(pid_t *)kmem_alloc((v.v_proc * 
					sizeof(pid_t)), KM_SLEEP);
			}
		}
	}
			
			
	for (prp = practive; prp != NULL; prp = prp->p_next) {
		nsearched++;
		if (prp->p_stat == SIDL || prp->p_stat == SZOMB)
			continue;
		if (procinset(prp, psp)) {
			if (funcp != NULL && prp != proc_init) {
				/* Update the list of pids to be used for audit */
				if (pidlistp) {
					*(pidlistp + count) = prp->p_pid;
					count++;
				}
					
				error = (*funcp)(prp, arg);
				switch (error) {
				case -1:
					return 0;
				case 0:
					nfound++;
					lastprp = prp;
					break;
				case ESRCH:
					/*
					 * ESRCH is returned if the function
					 * doesn't want us to recognize that
					 * the process exists for covert
					 * channel reasons.  When we get
					 * ESRCH back we pretend the process
					 * is not in the process set.
					 */
					error = 0;
					break;
				default:
					if (pidlistp)
						kmem_free((caddr_t)pidlistp, v.v_proc * sizeof(pid_t));
					return error;
				}
			} else {
				nfound++;
				lastprp = prp;
			}
		}
	}

	if (audit_on)
		(void)adt_kill(((sigsend_t *)arg)->sig, pidlistp, count);
	if (pidlistp)
		kmem_free((caddr_t)pidlistp, v.v_proc * sizeof(pid_t));

	/*
	 * Search a minimum number of processes to treat
	 * a covert channel resulting from the ability
	 * to estimate the number of procs in the system by
	 * timing a kill().
	 */
	if (mac_installed && nsearched < (psearch = cc_getinfo(CC_PSEARCHMIN))) {
		nsearched = psearch - nsearched;
		for (; --nsearched >= 0; prp = prp->p_next) {
			if (prp == NULL)
				prp = practive;
			if (prp->p_stat == SIDL || prp->p_stat == SZOMB)
				continue;
			(void)procinset(prp, psp);
		}
	}

	if (nfound == 0)
		return ESRCH;

	if (nfound == 1 && lastprp == proc_init && funcp != NULL)
		error = (*funcp)(lastprp, arg);

	if (error == -1)
		error = 0;

	return error;
}

/*
 * Check if a procset_t is valid.  Return zero or an errno.
 */
static int
checkprocset(psp)
	register procset_t	*psp;
{
	switch (psp->p_lidtype) {
	case P_PID:
	case P_PPID:
	case P_PGID:
	case P_SID:
	case P_CID:
	case P_UID:
	case P_GID:
	case P_ALL:
		break;
	default:
		return EINVAL;
	}

	switch (psp->p_ridtype) {
	case P_PID:
	case P_PPID:
	case P_PGID:
	case P_SID:
	case P_CID:
	case P_UID:
	case P_GID:
	case P_ALL:
		break;
	default:
		return EINVAL;
	}

	switch (psp->p_op) {
	case POP_DIFF:
	case POP_AND:
	case POP_OR:
	case POP_XOR:
		break;
	default:
		return EINVAL;
	}

	return 0;
}

/*
 * procinset returns 1 if the process pointed to
 * by pp is in the process set specified by psp and is not in
 * the sys scheduling class - otherwise 0 is returned.
 *
 * This function expects to be called with a valid procset_t.
 * The set should be checked using checkprocset() before calling
 * this function.
 */
int
procinset(pp, psp)
	register proc_t		*pp;
	register procset_t	*psp;
{
	register int	loperand = 0;
	register int	roperand = 0;

	/*
	 * If process is in the sys class return 0.
	 */
	if (pp->p_cid == 0) {
		return 0;
	}

	switch (psp->p_lidtype) {

	case P_PID:
		if (pp->p_pid == psp->p_lid)
			loperand++;
		break;

	case P_PPID:
		if (pp->p_ppid == psp->p_lid)
			loperand++;
		break;

	case P_PGID:
		if (pp->p_pgrp == psp->p_lid)
			loperand++;
		break;

	case P_SID:
		if (pp->p_sessp->s_sid == psp->p_lid)
			loperand++;
		break;
 
	case P_CID:
		if (pp->p_cid == psp->p_lid)
			loperand++;
		break;

	case P_UID:
		if (pp->p_cred->cr_uid == psp->p_lid)
			loperand++;
		break;

	case P_GID:
		if (pp->p_cred->cr_gid == psp->p_lid)
			loperand++;
		break;

	case P_ALL:
		loperand++;
		break;

	default:
#ifdef DEBUG
		cmn_err(CE_WARN, "procinset called with bad set");
		return 0;
#else
		return 0;
#endif
	}

	switch (psp->p_ridtype) {

	case P_PID:
		if (pp->p_pid == psp->p_rid)
			roperand++;
		break;

	case P_PPID:
		if (pp->p_ppid == psp->p_rid)
			roperand++;
		break;

	case P_PGID:
		if (pp->p_pgrp == psp->p_rid)
			roperand++;
		break;

	case P_SID:
		if (pp->p_sessp->s_sid == psp->p_rid)
			roperand++;
		break;
 
	case P_CID:
		if (pp->p_cid == psp->p_rid)
			roperand++;
		break;

	case P_UID:
		if (pp->p_cred->cr_uid == psp->p_rid)
			roperand++;
		break;

	case P_GID:
		if (pp->p_cred->cr_gid == psp->p_rid)
			roperand++;
		break;

	case P_ALL:
		roperand++;
		break;

	default:
#ifdef DEBUG
		cmn_err(CE_WARN, "procinset called with bad set");
		return 0;
#else
		return 0;
#endif
	}

	switch (psp->p_op) {
	
	case POP_DIFF:
		if (loperand && !roperand)
			return 1;
		else
			return 0;
		break;

	case POP_AND:
		if (loperand && roperand)
			return 1;
		else
			return 0;
		break;

	case POP_OR:
		if (loperand || roperand)
			return 1;
		else
			return 0;
		break;

	case POP_XOR:
		if ((loperand || roperand) && !(loperand && roperand))
			return 1;
		else
			return 0;
		break;

	default:
#ifdef DEBUG
		cmn_err(CE_WARN, "procinset called with bad set");
		return 0;
#else
		return 0;
#endif
	}
}

/*
 * Check for common cases of procsets which specify only the
 * current process.  cur_inset_only() returns B_TRUE when
 * the current process is the only one in the set.  B_FALSE
 * is returned to indicate that this may not be the case.
 */
boolean_t
cur_inset_only(psp)
	register procset_t	*psp;
{
	if (psp->p_lidtype == P_PID &&
	    (psp->p_lid == P_MYID || psp->p_lid == u.u_procp->p_pid) &&
	    psp->p_op == POP_AND && psp->p_ridtype == P_ALL)
		return B_TRUE;

	if (psp->p_ridtype == P_PID &&
	    (psp->p_rid == P_MYID || psp->p_rid == u.u_procp->p_pid) &&
	    psp->p_op == POP_AND && psp->p_lidtype == P_ALL)
		return B_TRUE;

	return B_FALSE;
}

static id_t
getmyid(idtype)
	idtype_t	idtype;
{
	register proc_t	*pp;

	pp = u.u_procp;

	switch (idtype) {
	case P_PID:
		return pp->p_pid;
	
	case P_PPID:
		return pp->p_ppid;

	case P_PGID:
		return pp->p_pgrp;

	case P_SID:
		return pp->p_sessp->s_sid;

	case P_CID:
		return pp->p_cid;

	case P_UID:
		return pp->p_cred->cr_uid;

	case P_GID:
		return pp->p_cred->cr_gid;

	case P_ALL:
		/*
		 * The value doesn't matter for P_ALL.
		 */
		return 0;

	default:
		return -1;
	}
}
