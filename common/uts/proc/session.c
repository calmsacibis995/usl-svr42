/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:proc/session.c	1.6.3.2"
#ident	"$Header: $"

/*
 * Session routines.
 */

#include <acc/priv/privilege.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Initial session assigned to process 0.
 */
sess_t session0 = {
	1,	/* s_ref   */
	0555,	/* s_mode  */
	0,	/* s_uid   */
	0,	/* s_gid   */
	0,	/* s_ctime */
	NODEV,	/* s_dev   */
	NULL,	/* s_vp    */
	&pid0,	/* s_sidp  */
	NULL	/* s_cred  */
};

/*
 * Release session.
 */
int
sess_rele(sp)
	register sess_t *sp;
{
	ASSERT(sp != &session0);
	/*
	 * Release the pid structure for the session.
	 */
	PID_RELE(sp->s_sidp);
	/*
	 * Deallocate the memory for the sess_t structure.
	 */
	kmem_free(sp,sizeof(sess_t));
	return 1;
}

/*
 * Create a new session for the current process.
 */
void
sess_create()
{
	register proc_t *pp;
	register proc_t *cp;
	register sess_t *sp;
	register struct pid *pgp, *mypgp;


	pp = u.u_procp;

	/*
	 * Check to see if the process groups of any of pp's
	 * children will be orphaned by the removal of pp
	 * from its current session.
	 */
	mypgp = pp->p_pgidp;
	for (cp = pp->p_child; cp; cp = cp->p_sibling) {
		/* pgp = mypgp will be orphaned in pgexit() */
		if ((pgp = cp->p_pgidp)->pid_pgorphaned || (pgp == mypgp))
			continue;
		pgorphan(pgp, pp, 1);
	}

	/*
	 * Delete the current process from its process group.
	 */
	pgexit(pp);

	/*
	 * Release old session, if reference count drops to zero.
	 */
	SESS_RELE(pp->p_sessp);

	/*
	 * Allocate structure for new session.
	 */
	sp = (sess_t *)kmem_zalloc(sizeof (sess_t), KM_SLEEP);

	sp->s_sidp = pp->p_pidp;	/* session id is pid of session leader */
	sp->s_ref = 1;
	sp->s_dev = NODEV;
	pp->p_sessp = sp;		/* process points to new session */
        u.u_ttyp = NULL;		/* compatibility */

	/*
	 * Make pp the process group leader of a new process group
	 * (pgid == pid).
	 */
	pgjoin(pp, pp->p_pidp);

	/*
	 * Increment the reference count of the process group.
	 */
	PID_HOLD(sp->s_sidp);
}

/*
 * Free the controlling terminal for the session "sp".
 */
void
freectty(sp)
	register sess_t *sp;
{
	extern void tp_disconnect();
	register vnode_t *vp;
	int i;

	vp = sp->s_vp;		/* pointer to tty's vnode */
	ASSERT(vp != NULL);

	if (vp->v_stream != NULL) {
		/*
		 * If this Stream is a Trusted Path channel disconnect it from
		 * its associated TP device.
		 * NOTE for the MP Release:  This function call may want to
		 * migrate down to strfreectty() once this becomes the only
		 * place where strfreectty() gets called.
		 */
		tp_disconnect(sp->s_dev);
		strfreectty(vp->v_stream);
	} else { /* may be clist driver */
		ASSERT(u.u_ttyp != NULL);
		signal((pid_t)(*u.u_ttyp), SIGHUP);
		*u.u_ttyp = 0;
	}

	/*
	 * Close all references to the terminal.
	 */
	for (i = vp->v_count; i; i--) {
		VOP_CLOSE(vp, 0, 1, (off_t)0, sp->s_cred);
		VN_RELE(vp);
	}

	crfree(sp->s_cred);
	sp->s_vp = NULL;

}

/*
 * Return the controlling terminal for the process "pp".
 */
dev_t
cttydev(pp)
proc_t *pp;
{
	register sess_t *sp = pp->p_sessp;
	if (sp->s_vp == NULL)
		return NODEV;
	return sp->s_dev;
}

/*
 * Allocate a controlling terminal for process "pp".
 */
void
alloctty(pp, vp)
register proc_t *pp;
vnode_t *vp;	/* vnode of new terminal */
{
	register sess_t *sp = pp->p_sessp;	/* pointer to processes' session */
	register cred_t *crp = pp->p_cred;	/* pointer to processes' cred */

	/*
	 * Assign values for new terminal to session.
	 */
	sp->s_vp = vp;
	sp->s_dev = vp->v_rdev;

	crhold(crp);
	sp->s_cred = crp;
	sp->s_uid = crp->cr_uid;
	sp->s_ctime = hrestime.tv_sec;

	if (session0.s_mode & VSGID)
		sp->s_gid = session0.s_gid;
	else
		sp->s_gid = crp->cr_gid;

	/*
	 * Apply umask to session's modes.
	 */
        CATCH_FAULTS(CATCH_SEGU_FAULT)
		sp->s_mode = (0666 & ~(PTOU(pp)->u_cmask));
	END_CATCH();
}

/*
 * Check access rights to the controlling terminal.
 */
static int
hascttyperm(sp, cr, mode)
	register sess_t *sp;	/* session whose terminal we're checking */
	register cred_t *cr;	/* credentials of who we're checking for */
	register mode_t mode;	/* type of access */
{
	int	sav_mode;

	sav_mode = mode;	/* used later (if necessary) in privilege check */

	if (cr->cr_uid != sp->s_uid) {
		mode >>= 3;	/* not user, we need group or other */
		if (!groupmember(sp->s_gid, cr))
			mode >>= 3;	/* not user or group, we need other */
	}

	if ((sp->s_mode & mode) == mode)
		return 1;	/* we have access permission */
	else {
		/*
		 * We don't have permission, see if we have
		 * privilege to override.
		 */
		if ((sav_mode & (VREAD | VEXEC)) && pm_denied(cr, P_DACREAD))
			return (0);
		if ((sav_mode & VWRITE) && pm_denied(cr, P_DACWRITE))
			return (0);
		return (1);
	}
}

/*
 * Reallocate the controlling terminal from the process "frompp"
 * to the process whose session is specified by "sid".
 */
int
realloctty(frompp, sid)
	proc_t *frompp;
	pid_t sid;
{
	proc_t *topp;
	register sess_t *fromsp;
	register sess_t *tosp;
	cred_t *fromcr;
	vnode_t *fromvp;

	fromsp = frompp->p_sessp;	/* session of "from" process */
	fromvp = fromsp->s_vp;		/* vnode of terminal to be reallocated */
	fromcr = frompp->p_cred;	/* credentials of "from" process */
	
	if (!hascttyperm(&session0, fromcr, VEXEC|VWRITE))
		return EACCES;

	if ((session0.s_mode & VSVTX) 
	  && fromcr->cr_uid != session0.s_uid
	  && (!hascttyperm(fromsp, fromcr, VWRITE)))
		return EACCES;

	/*
	 * If no session was specified, just free the terminal.
	 */
	if (sid == 0) {
		freectty(fromsp);
		return 0;
	}

	if (fromvp->v_stream == NULL)
		return ENOSYS;	/* must be a stream */

	/*
	 * Find the process whose pid is the same as sid
	 * (the session leader).
	 */
	if ((topp = prfind(sid)) == NULL)
		return ESRCH;

	tosp = topp->p_sessp;	/* session pointer */

	if (tosp->s_sidp != topp->p_pidp
	  || tosp->s_vp != NULL
	  || !hasprocperm(topp->p_cred, frompp->p_cred))
		return EPERM;

	strfreectty(fromvp->v_stream);	/* free the terminal from process "fromp" */
	crfree(fromsp->s_cred);

	/*
	 * Allocate the terminal to the process "topp".
	 */
	alloctty(topp, fromvp);
	stralloctty(tosp, fromvp->v_stream);

	/*
	 * Null out the terminal vnode pointer in the original session.
	 */
	fromsp->s_vp = NULL;

	return 0;
}
