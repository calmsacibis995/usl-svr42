/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:proc/pid.c	1.8.4.3"
#ident	"$Header: $"

#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/types.h>
#include <util/var.h>

/* directory entries for /proc */
union procent {
	proc_t *pe_proc;
	union procent *pe_next;
};

/* active process chain */
proc_t *practive;

/*
 * pid structure for process 0.
 */
struct pid pid0 = {
	0,		/* pid_prinactive */
	1,		/* pid_pgorphaned */
	3,		/* pid_ref	*/
	0,		/* pid_prslot	*/
	0,		/* pid_id	*/
	NULL,		/* pid_pglink	*/
	NULL		/* pid_link	*/
};

#define HASHSZ		64	/* size of pid hash table */

/*
 * This macro returns the hash table entry for pid.
 */
#define HASHPID(pid)	(pidhash[((pid)&(HASHSZ-1))])

STATIC u_int nproc;
STATIC struct pid **pidhash;
STATIC pid_t minpid;	
STATIC pid_t mpid;
STATIC union procent *procdir;
STATIC union procent *procentfree;

/* control structure for covert channel limiter */
STATIC ccevent_t cc_re_proc = { CC_RE_PROC, CCBITS_RE_PROC };

/*
 * Find the pid structure associated with the process ID pid.
 */
STATIC struct pid *
pid_lookup(pid)
	register pid_t pid;
{
	register struct pid *pidp;

	/*
	 * Search the appropriate hash chain for the pid and
	 * return a pointer to its pid structure, if successful.
	 */
	for (pidp = HASHPID(pid); pidp; pidp = pidp->pid_link) {
		if (pidp->pid_id == pid) {
			ASSERT(pidp->pid_ref > 0);
			break;
		}
	}
	return pidp;
}

/*
 * Set the minimum value for pids. This value prevents the
 * system from handing out pids associated with the initial
 * system processes.
 */
void
pid_setmin()
{
	minpid = mpid + 1;
}

/*
 * This function assigns a pid for use in a fork request.  It checks
 * to see that there is an empty slot in the proc table, that the
 * requesting user does not have too many processes already active,
 * and that the last slot in the proc table is not being allocated to
 * anyone who should not use it.
 *
 * After a proc slot is allocated, it will try to allocate a proc
 * structure for the new process. 
 *
 * If all goes well, pid_assign() will return a new pid and set up the
 * proc structure pointer for the child process.  Otherwise it will
 * return -1.
 */

pid_t
pid_assign(cond, pp)
	int	cond;	/* allow assignment of last slot? */
	proc_t	**pp;	/* child process proc structure pointer */
{
	register struct pid *pidp;
	register proc_t *prp;
	register int	maxup = 0;
	register int	uid_procs = 0;
	register uid_t	ruid = u.u_cred->cr_ruid;
	register u_long	rand;
	union procent	*pep;

	/*
	 * If NPROC processes already running,
	 * or NPROC-1 processes running and not privileged,
	 * fail immediately
	 */

	if (nproc >= v.v_proc - 1) {
		if (nproc == v.v_proc) {
			syserr.procovf++;
			return -1;
		}
		if (pm_denied(u.u_cred, P_SYSOPS))
			return -1;
	}

	/*
	 * If not privileged then make certain that the maximum
	 * number of children don't already exist.
	 *
	 * The user ID for the scheduler is also used for
	 * all daemon processes run by the system.  Since
	 * the root user ID is no longer guaranteed to be
	 * privileged, the number of children assigned to
	 * that user ID is doubled so that a process running
	 * as that user ID is allowed more children than other
	 * user IDs.  This is not a privilege check, but rather
	 * a check imposed due to system limitations.
	*/
	if (proc_sched->p_uid == ruid
			|| proc_sched->p_uid == u.u_cred->cr_uid) {
		maxup = (2 * v.v_maxup);
	}
	else {
		maxup = v.v_maxup;
	}

	/*
	 * Determine the number of processes the user has active.
	 */
	for (prp = practive; prp != NULL; prp = prp->p_next) {
		if (prp->p_uid == ruid)
			uid_procs++;
	}
	/*
	 * If the per-user process limit has been exceeded, and the
	 * process is non-privileged, return failure.
	 */
	if (uid_procs >= maxup && pm_denied(u.u_cred, P_SYSOPS)) {
		cc_limiter(&cc_re_proc, u.u_cred);
		return -1;
	}

	prp = (proc_t *)kmem_zalloc(sizeof(*prp), KM_NOSLEEP);
	pidp = (struct pid *)kmem_zalloc(sizeof(*pidp), KM_NOSLEEP);

	if (prp == NULL || pidp == NULL) {
		if ((cond & NP_FAILOK) == 0)
			cmn_err(CE_PANIC, "newproc - fork failed\n");
		if (prp != NULL)
			kmem_free(prp, sizeof(*prp));
		if (pidp != NULL)
			kmem_free(pidp, sizeof(*pidp));
		return -1;
	}

	/*
	 * Allocate a pid.
	 * If we're concerned about covert channels, start the pid search
	 * at a random place rather than where the previous search stopped.
	 * (There's a chance that we'll generate a random number < minpid,
	 * but the consequences are small, so we don't bother checking.)
	 * If the random() routine is stubbed out, it will return 0,
	 * in which case we want to revert to the sequential method.
	 * We also use the sequential method for the first few processes,
	 * i.e., those created in main() prior to the assignment of minpid.
	 */

	if (mac_installed && minpid != 0 &&
		(rand = random((u_long)MAXPID-2)) != 0)
		mpid = (pid_t)rand;

	do  {
		if (++mpid >= MAXPID)
			mpid = minpid;
	} while (pid_lookup(mpid) != NULL);

	/* 
	 * Allocate a /proc directory entry
	 */

	ASSERT(procentfree != NULL);

	pep = procentfree;
	procentfree = procentfree->pe_next;
	pep->pe_proc = prp;

	PID_HOLD(pidp);
	pidp->pid_id = mpid;
	pidp->pid_prslot = pep - procdir;
	pidp->pid_link = HASHPID(mpid);
	HASHPID(mpid) = pidp;

	prp->p_stat = SIDL;
	prp->p_opid = mpid;
	prp->p_pidp = pidp;
	prp->p_next = practive;
	practive = prp;

	*pp = prp;
	nproc++;
	return mpid;
}

/*
 * Unlink the given pid structure from the appropriate
 * hash chain, and free it.
 */

int
pid_rele(pidp)
	register struct pid *pidp;
{
	register struct pid **pidpp;
	register int s;

	ASSERT(pidp != &pid0);

	for (pidpp = &HASHPID(pidp->pid_id); ; pidpp = &(*pidpp)->pid_link) {
		ASSERT(*pidpp != NULL);
		if (*pidpp == pidp)
			break;
	}

	s = splhi();
	*pidpp = pidp->pid_link;
	kmem_free(pidp, sizeof(*pidp));
	splx(s);

	return 0;
}

/*
 * Free the proc structure, prp.
 */
void
pid_exit(prp)
	register proc_t *prp;
{
	register proc_t **prpp;
	register struct pid *pidp;
	register int s;

	pgexit(prp);	/* delete the process from its process group */

	/*
	 * Decrement the reference count of the process' session.
	 */
	SESS_RELE(prp->p_sessp);

	/*
	 * Free the /proc entry.
	 */
	pidp = prp->p_pidp;
	pidp->pid_prinactive = 1;
	procdir[pidp->pid_prslot].pe_next = procentfree;
	procentfree = &procdir[pidp->pid_prslot];

	PID_RELE(pidp);

	/*
	 * Unlink the proc structure from the active list and free it.
	 */
	for (prpp = &practive; ; prpp = &(*prpp)->p_next) {
		ASSERT(*prpp != NULL);
		if (*prpp == prp)
			break;
	}

	s = splhi();
	*prpp = prp->p_next;	
	kmem_free(prp, sizeof(*prp));
	splx(s);

	nproc--;
}

/*
 * Find a process given its process ID.
 */

proc_t *
prfind(pid)
	register pid_t pid;
{
	struct pid *pidp;

	pidp = pid_lookup(pid);
	if (pidp != NULL && pidp->pid_prinactive == 0)
		return procdir[pidp->pid_prslot].pe_proc;
	return NULL;
}

/*
 * Return the list of processes in whose process group ID is 'pgid',
 * or NULL, if no such process group.
 */

proc_t *
pgfind(pgid)
	register pid_t pgid;
{
	register proc_t *prp;

	struct pid *pidp;

	pidp = pid_lookup(pgid);
	if (pidp != NULL)
		return pidp->pid_pglink;
			
	return NULL;
}

/*
 * Allocate and initialize the pid hash table and the /proc directory.
 */
void
pid_init()
{
	register i;

	pidhash = (struct pid **)
	  kmem_zalloc(sizeof(struct pid *)*HASHSZ, KM_NOSLEEP);

	procdir = (union procent *)
	  kmem_zalloc(sizeof(union procent)*v.v_proc, KM_NOSLEEP);

	if (pidhash == NULL || procdir == NULL)
		cmn_err(CE_PANIC, "Could not allocate space for pid tables\n");

	nproc = 1;	/* process 0 is the only process at this time */
	practive = proc_sched;	/* put process 0 on active list */
	proc_sched->p_next = NULL;
	/* process 0 is first entry in /proc */
	procdir[0].pe_proc = proc_sched;

	/*
	 * Link remaining /proc entries into free list.
	 */
	procentfree = &procdir[1];
	for (i = 1; i < v.v_proc - 1; i++)
		procdir[i].pe_next = &procdir[i+1];
	procdir[i].pe_next = NULL;

	HASHPID(0) = &pid0;	/* hash pid for process 0 */
}

/*
 * Given the /proc slot number, return a pointer to the
 * proc_t structure associated with it.
 */
proc_t *
pid_entry(slot)
	int slot;
{
	register union procent *pep;
	register proc_t *prp;

	ASSERT(slot >= 0 && slot < v.v_proc);

	pep = procdir[slot].pe_next;
	/*
	 * The entries pe_next and pe_proc occupy the same memory
	 * in the procent union. If pep points somewhere in the procdir
	 * table, the slot in question is free, since an assignment to
	 * pe_proc would overwrite the pe_next value.
	 */
	if ((pep >= procdir && pep < &procdir[v.v_proc]) || pep == NULL)
		return NULL;
	prp = procdir[slot].pe_proc;
	if (prp->p_stat == SIDL)
		return NULL;	/* process still being created */
	return prp;
}

/*
 * Version of pid_entry() which works before procdir is allocated.
 * Used only by debuggers, not mainline code.
 */
proc_t *
dbg_pid_entry(slot)
	int slot;
{
	return (procdir == NULL) ? NULL : pid_entry(slot);
}

/*
 * Send the specified signal to all processes whose process group ID is
 * equal to 'pgid'
 */

void
signal(pgid, sig)
	pid_t pgid;
	int sig;
{
	register struct pid *pidp;

	if (pgid == 0 || (pidp = pid_lookup(pgid)) == NULL)
		return;

	pgsignal(pidp, sig);
}

/*
 * Send the specified signal to the specified process
 */

void
prsignal(pidp, sig)
	register struct pid *pidp;
	int sig;
{
	if (!(pidp->pid_prinactive))
		psignal(procdir[pidp->pid_prslot].pe_proc, sig);
}
