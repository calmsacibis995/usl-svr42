/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_qlims.c	1.5.2.2"
#ident	"$Header: $"

/*
 * Routines used in checking limits on file system usage.
 */

#include <acc/priv/privilege.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/vfs.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Common code macro for quota file and block allocation.
 *
 * Disallow allocation if it would bring the current usage over
 * the hard limit or if the user is over his soft limit and his time
 * has run out.
 */
#define	SFS_QALLOC(hard, soft, cur, time, ftime, cflags, flag, chg, id, cr, err) { \
	u_long curtmp;							\
	curtmp = (cur) + (chg);						\
	if (curtmp >= (hard) && (hard) && !force) {			\
		if (((cflags) & (flag)) == 0 && (id) == (cr)->cr_ruid)	\
			(cflags) |= (flag);				\
		(err) = ENOSPC;						\
	} else if (curtmp >= (soft) && (soft)) {			\
		if ((cur) < (soft) || (time) == 0)			\
			(time) = hrestime.tv_sec + (ftime);		\
		else if (hrestime.tv_sec > (time) && !force) {		\
			if (((cflags)&(flag)) == 0 && (id) == (cr)->cr_ruid) \
				(cflags) |= (flag);			\
			(err) = ENOSPC;					\
		}							\
	}								\
	if ((err) && !pm_denied(cr, P_FILESYS))				\
		(err) = 0;						\
	if ((err) == 0)							\
		(cur) = curtmp;						\
}


/*
 * Find the dquot structure that should
 * be used in checking i/o on inode ip.
 */
struct dquot *
sfs_getinoquota(ip, cr)
	register struct inode *ip;
	register struct cred *cr;
{
	register struct dquot *dqp;
	register struct sfs_vfs *sfs_vfsp;
	struct dquot *xdqp;

	if (!UFSIP(ip))
		return (NULL);
	sfs_vfsp = (struct sfs_vfs *)((ITOV(ip))->v_vfsp->vfs_data);
	/*
	 * Check for quotas enabled.
	 */
	if ((sfs_vfsp->vfs_qflags & MQ_ENABLED) == 0)
		return (NULL);
	/*
	 * Check for someone doing I/O to quota file.
	 */
	if (ip == sfs_vfsp->vfs_qinod)
		return (NULL);
	if (sfs_getdiskquota((uid_t)ip->i_uid, sfs_vfsp, 0, &xdqp, cr))
		return (NULL);
	dqp = xdqp;
	if (dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) {
		sfs_dqput(dqp);
		dqp = NULL;
	} else {
		DQUNLOCK(dqp);
	}
	return (dqp);
}

/*
 * Update disk usage, and take corrective action.
 */
int
sfs_chkdq(ip, change, force, cr)
	struct inode *ip;
	long change;
	int force;
	struct cred *cr;
{
	register struct dquot *dqp;
	register u_long ncurblocks;
	int error = 0;

	if (!UFSIP(ip))
		return (0);
	if (change == 0)
		return (0);
	dqp = ip->i_dquot;
	if (dqp == NULL)
		return (0);
	dqp->dq_flags |= DQ_MOD;
	if (change < 0) {
		if ((int)dqp->dq_curblocks + change >= 0)
			dqp->dq_curblocks += change;
		else
			dqp->dq_curblocks = 0;
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit)
			dqp->dq_btimelimit = 0;
		dqp->dq_flags &= ~DQ_BLKS;
		return (0);
	}

	SFS_QALLOC(dqp->dq_bhardlimit, dqp->dq_bsoftlimit,
		dqp->dq_curblocks, dqp->dq_btimelimit,
		((struct sfs_vfs *)(ITOV(ip))->v_vfsp->vfs_data)->vfs_btimelimit,
		dqp->dq_flags, DQ_BLKS, change, ip->i_uid, cr, error);

	return error;
}

/*
 * Check the inode limit, applying corrective action.
 */
int
sfs_chkiq(sfs_vfsp, ip, uid, force, cr)
	struct sfs_vfs *sfs_vfsp;
	struct inode *ip;
	uid_t uid;
	int force;
	struct cred *cr;
{
	register struct dquot *dqp;
	register u_long ncurfiles;
	struct dquot *xdqp;
	int error = 0;

	/*
	 * Free.
	 */
	if (ip != NULL) {
		dqp = ip->i_dquot;
		if (dqp == NULL)
			return (0);
		dqp->dq_flags |= DQ_MOD;
		if (dqp->dq_curfiles)
			dqp->dq_curfiles--;
		if (dqp->dq_curfiles < dqp->dq_fsoftlimit)
			dqp->dq_ftimelimit = 0;
		dqp->dq_flags &= ~DQ_FILES;
		return (0);
	}

	/*
	 * Allocation. Get dquot for uid.
	 * Check for quotas enabled.
	 */
	if ((sfs_vfsp->vfs_qflags & MQ_ENABLED) == 0)
		return (0);
	if (sfs_getdiskquota(uid, sfs_vfsp, 0, &xdqp, cr))
		return (0);
	dqp = xdqp;
	if (dqp->dq_fsoftlimit == 0 && dqp->dq_fhardlimit == 0) {
		sfs_dqput(dqp);
		return (0);
	}
	dqp->dq_flags |= DQ_MOD;

	SFS_QALLOC(dqp->dq_fhardlimit, dqp->dq_fsoftlimit,
		dqp->dq_curfiles, dqp->dq_ftimelimit,
		sfs_vfsp->vfs_ftimelimit,
		dqp->dq_flags, DQ_FILES, 1, uid, cr, error);

	sfs_dqput(dqp);
	return error;
}

/*
 * Release a dquot.
 */
void
sfs_dqrele(dqp, cr)
	register struct dquot *dqp;
	register struct cred *cr;
{

	if (dqp != NULL) {
		DQLOCK(dqp);
		if (dqp->dq_cnt == 1 && dqp->dq_flags & DQ_MOD)
			sfs_dqupdate(dqp, cr);
		sfs_dqput(dqp);
	}
}
