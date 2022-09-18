/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_qcalls.c	1.11.2.7"
#ident	"$Header: $"

/*
 * Quota system calls.
 */
#ifdef QUOTA
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

STATIC int	sfs_opendq();
STATIC int	sfs_setquota();
STATIC int	sfs_getquota();
STATIC int	sfs_qsync();

extern struct fshead	sfs_fshead;	/* sfs dynamic inodes filesystem head */

/*
 * Sys call to allow users to find out
 * their current position wrt quota's
 * and to allow privileged users to alter it.
 */


/*ARGSUSED*/
int
sfs_quotactl(vp, arg, cr)
	struct vnode *vp;
	int arg;
	struct cred *cr;
{
	struct quotctl quot;
	struct sfs_vfs *sfs_vfsp;
	register int error = 0;

	if (copyin((caddr_t)arg, (caddr_t)&quot, sizeof(struct quotctl)))
		return EFAULT;
	if (quot.uid < 0)
		quot.uid = cr->cr_ruid;
	if (quot.op == Q_SYNC && vp == NULL) {
		sfs_vfsp = NULL;
	} else if (quot.op != Q_ALLSYNC) {
		sfs_vfsp = (struct sfs_vfs *)(vp->v_vfsp->vfs_data);
	}
	switch (quot.op) {

	case Q_QUOTAON:
		error = sfs_opendq(sfs_vfsp, quot.addr, cr);
		break;

	case Q_QUOTAOFF:
		error = sfs_closedq(sfs_vfsp, cr);
		break;

	case Q_SETQUOTA:
	case Q_SETQLIM:
		error = sfs_setquota(quot.op, (uid_t)quot.uid, sfs_vfsp, quot.addr, cr);
		break;

	case Q_GETQUOTA:
		error = sfs_getquota((uid_t)quot.uid, sfs_vfsp, quot.addr, cr);
		break;

	case Q_SYNC:
		error = sfs_qsync(sfs_vfsp, cr);
		break;

	case Q_ALLSYNC:
		(void)sfs_qsync(NULL, cr);
		break;

	default:
		error = EINVAL;
		break;
	}
	return error;
}

/*
 * Set the quota file up for a particular file system.
 * Called as the result of a sfs_setquota system call.
 */
STATIC int
sfs_opendq(sfs_vfsp, addr, cr)
	register struct sfs_vfs *sfs_vfsp;
	caddr_t addr;			/* quota file */
	struct cred *cr;
{
	struct vnode *vp;
	struct dquot *dqp;
	int error;

	if (pm_denied(cr, P_SYSOPS))
		return (EPERM);

	if (((struct fs *)sfs_vfsp->vfs_bufp->b_un.b_addr)->fs_ronly)
		return (EROFS);
	error =
	    lookupname(addr, UIO_USERSPACE, FOLLOW, (struct vnode **)NULL,
	    &vp);
	if (error)
		return (error);
	if ((struct sfs_vfs *)(vp->v_vfsp->vfs_data) != sfs_vfsp || vp->v_type != VREG) {
		VN_RELE(vp);
		return (EACCES);
	}
	if (sfs_vfsp->vfs_qflags & MQ_ENABLED)
		(void) sfs_closedq(sfs_vfsp, cr);
	if (sfs_vfsp->vfs_qinod != NULL) {	/* open/close in progress */
		VN_RELE(vp);
		return (EBUSY);
	}
	sfs_vfsp->vfs_qinod = VTOI(vp);
	/*
	 * The file system time limits are in the uid 0 user dquot.
	 * The time limits set the relative time the other users
	 * can be over quota for this file system.
	 * If it is zero a default is used (see quota.h).
	 */
	error = sfs_getdiskquota((uid_t)0, sfs_vfsp, 1, &dqp, cr);
	if (error == 0) {
		sfs_vfsp->vfs_btimelimit =
		    (dqp->dq_btimelimit? dqp->dq_btimelimit: DQ_BTIMELIMIT);
		sfs_vfsp->vfs_ftimelimit =
		    (dqp->dq_ftimelimit? dqp->dq_ftimelimit: DQ_FTIMELIMIT);
		sfs_dqput(dqp);
		sfs_vfsp->vfs_qflags = MQ_ENABLED;	/* enable quotas */
	} else {
		/*
		 * Some sort of I/O error on the quota file.
		 */
		sfs_irele(sfs_vfsp->vfs_qinod);
		sfs_vfsp->vfs_qinod = NULL;
	}
	return (error);
}

/*
 * Close off disk quotas for a file system.
 */
int
sfs_closedq(sfs_vfsp, cr)
	register struct sfs_vfs *sfs_vfsp;
	register struct cred *cr;
{
	register struct dquot *dqp;
	register struct inode *qip;
	register struct inode *ip;
	register union ihead *hip;

	if (pm_denied(cr, P_SYSOPS))
		return (EPERM);
	if ((sfs_vfsp->vfs_qflags & MQ_ENABLED) == 0)
		return (0);
	qip = sfs_vfsp->vfs_qinod;
	ASSERT(qip != NULL);
	sfs_vfsp->vfs_qflags = 0;	/* disable quotas */
loop:
	/*
	 * Run down the inode table and release all dquots assciated with
	 * inodes on this filesystem.
	 */
	/*
	 * This search runs through the hash chains (rather
	 * than the entire inode table) so that we examine
	 * inodes that we know are currently valid.
	 */
        for (hip=sfs_ihead; hip < &sfs_ihead[INOHSZ]; hip++) {
                for (ip = hip->ih_chain[0]; ip != (struct inode *)hip; ip = ip->i_forw) {

			ASSERT(ITOV(ip) == &(ip->i_vnode));
			ASSERT(ITOV(ip)->v_op == &sfs_vnodeops);

			dqp = ip->i_dquot;
			if (dqp != NULL && dqp->dq_sfs_vfsp == sfs_vfsp) {
				if (dqp->dq_flags & DQ_LOCKED) {
					dqp->dq_flags |= DQ_WANT;
					(void) sleep((caddr_t)dqp, PINOD+2);
					goto loop;
				}
				dqp->dq_flags |= DQ_LOCKED;
				sfs_dqput(dqp);
				ip->i_dquot = NULL;
			}
		}
	}
	/*
	 * Run down the dquot table and clean and invalidate the
	 * dquots for this file system.
	 */
	for (dqp = sfs_dquot; dqp < sfs_dquotNDQUOT; dqp++) {
		if (dqp->dq_sfs_vfsp == sfs_vfsp) {
			if (dqp->dq_flags & DQ_LOCKED) {
				dqp->dq_flags |= DQ_WANT;
				(void) sleep((caddr_t)dqp, PINOD+2);
				goto loop;
			}
			dqp->dq_flags |= DQ_LOCKED;
			if (dqp->dq_flags & DQ_MOD)
				sfs_dqupdate(dqp, cr);
			sfs_dqinval(dqp);
		}
	}
	/*
	 * Sync and release the quota file inode.
	 */
	sfs_ilock(qip);
	(void) sfs_syncip(qip, 0, IUP_SYNC);
	sfs_iput(qip);
	sfs_vfsp->vfs_qinod = NULL;
	return (0);
}

/*
 * Set various fields of the dqblk according to the command.
 * Q_SETQUOTA - assign an entire dqblk structure.
 * Q_SETQLIM - assign a dqblk structure except for the usage.
 */
STATIC int
sfs_setquota(cmd, uid, sfs_vfsp, addr, cr)
	int cmd;
	uid_t uid;
	struct sfs_vfs *sfs_vfsp;
	caddr_t addr;
	struct cred *cr;
{
	register struct dquot *dqp;
	struct dquot *xdqp;
	struct dqblk newlim;
	int error;

	if (pm_denied(cr, P_FILESYS))
		return (EPERM);
	if ((sfs_vfsp->vfs_qflags & MQ_ENABLED) == 0)
		return (ESRCH);
	error = copyin(addr, (caddr_t)&newlim, sizeof (struct dqblk));
	if (error)
		return (error);
	error = sfs_getdiskquota(uid, sfs_vfsp, 0, &xdqp, cr);
	if (error)
		return (error);
	dqp = xdqp;
	/*
	 * Don't change disk usage on Q_SETQLIM
	 */
	if (cmd == Q_SETQLIM) {
		newlim.dqb_curblocks = dqp->dq_curblocks;
		newlim.dqb_curfiles = dqp->dq_curfiles;
	}
	dqp->dq_dqb = newlim;
	/*
	 * The following check is NOT a privilege check but rather a
	 * conditional check to determine if the uid value passed as
	 * an argument to this routine can perform this function.
	 * It should NEVER be changed to a pm_denied() call!!
	 */
	if (uid == 0) {
		/*
		 * Timelimits for the value uid == 0 set the relative time
		 * the other users can be over quota for this file system.
		 * If it is zero a default is used (see quota.h).
		 */
		sfs_vfsp->vfs_btimelimit =
		    newlim.dqb_btimelimit? newlim.dqb_btimelimit: DQ_BTIMELIMIT;
		sfs_vfsp->vfs_ftimelimit =
		    newlim.dqb_ftimelimit? newlim.dqb_ftimelimit: DQ_FTIMELIMIT;
	} else {
		/*
		 * If the user is now over quota, start the timelimit.
		 * The user will not be warned.
		 */
		if (dqp->dq_curblocks >= dqp->dq_bsoftlimit &&
		    dqp->dq_bsoftlimit && dqp->dq_btimelimit == 0)
			dqp->dq_btimelimit = hrestime.tv_sec + sfs_vfsp->vfs_btimelimit;
		else
			dqp->dq_btimelimit = 0;
		if (dqp->dq_curfiles >= dqp->dq_fsoftlimit &&
		    dqp->dq_fsoftlimit && dqp->dq_ftimelimit == 0)
			dqp->dq_ftimelimit = hrestime.tv_sec + sfs_vfsp->vfs_ftimelimit;
		else
			dqp->dq_ftimelimit = 0;
		dqp->dq_flags &= ~(DQ_BLKS|DQ_FILES);
	}
	dqp->dq_flags |= DQ_MOD;
	sfs_dqupdate(dqp, cr);
	sfs_dqput(dqp);
	return (0);
}

/*
 * Q_GETQUOTA - return current values in a dqblk structure.
 */
STATIC int
sfs_getquota(uid, sfs_vfsp, addr, cr)
	uid_t uid;
	struct sfs_vfs *sfs_vfsp;
	caddr_t addr;
	struct cred *cr;
{
	register struct dquot *dqp;
	struct dquot *xdqp;
	int error;

	if (uid != cr->cr_ruid && pm_denied(cr, P_OWNER))
		return (EPERM);
	if ((sfs_vfsp->vfs_qflags & MQ_ENABLED) == 0)
		return (ESRCH);
	error = sfs_getdiskquota(uid, sfs_vfsp, 0, &xdqp, cr);
	if (error)
		return (error);
	dqp = xdqp;
	if (dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) {
		error = ESRCH;
	} else {
		error =
		    copyout((caddr_t)&dqp->dq_dqb, addr, sizeof (struct dqblk));
	}
	sfs_dqput(dqp);
	return (error);
}

/*
 * Q_SYNC - sync quota files to disk.
 */
STATIC int
sfs_qsync(sfs_vfsp, cr)
	register struct sfs_vfs *sfs_vfsp;
	register struct cred *cr;
{
	register struct dquot *dqp;

	if (sfs_vfsp != NULL && (sfs_vfsp->vfs_qflags & MQ_ENABLED) == 0)
		return (ESRCH);
	for (dqp = sfs_dquot; dqp < sfs_dquotNDQUOT; dqp++) {
		if ((dqp->dq_flags & DQ_MOD) &&
		    (sfs_vfsp == NULL || dqp->dq_sfs_vfsp == sfs_vfsp) &&
		    (dqp->dq_sfs_vfsp->vfs_qflags & MQ_ENABLED) &&
		    (dqp->dq_flags & DQ_LOCKED) == 0) {
			dqp->dq_flags |= DQ_LOCKED;
			sfs_dqupdate(dqp, cr);
			DQUNLOCK(dqp);
		}
	}
	return (0);
}

#endif QUOTA
