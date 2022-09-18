/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_quota.c	1.5.2.2"
#ident	"$Header: $"

/*
 * Code pertaining to management of the in-core data structures.
 */
#ifdef QUOTA
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Dquot cache - hash chain headers.
 */
#define	NDQHASH		64		/* smallish power of two */
#define	DQHASH(uid, mp) \
	(((unsigned)(mp) + (unsigned)(uid)) & (NDQHASH-1))

struct	dqhead	{
	struct	dquot	*dqh_forw;	/* MUST be first */
	struct	dquot	*dqh_back;	/* MUST be second */
};

/*
 * Dquot in core hash chain headers.
 */
STATIC struct dqhead sfs_dqhead[NDQHASH];

/*
 * Dquot free list.
 */
STATIC struct dquot sfs_dqfreelist;

#define dqinsheadfree(DQP) {				\
	(DQP)->dq_freef = sfs_dqfreelist.dq_freef;	\
	(DQP)->dq_freeb = &sfs_dqfreelist;		\
	sfs_dqfreelist.dq_freef->dq_freeb = (DQP);	\
	sfs_dqfreelist.dq_freef = (DQP);		\
}

#define dqinstailfree(DQP) {				\
	(DQP)->dq_freeb = sfs_dqfreelist.dq_freeb;	\
	(DQP)->dq_freef = &sfs_dqfreelist;		\
	sfs_dqfreelist.dq_freeb->dq_freef = (DQP);	\
	sfs_dqfreelist.dq_freeb = (DQP);		\
}

#define dqremfree(DQP) {				\
	(DQP)->dq_freeb->dq_freef = (DQP)->dq_freef;	\
	(DQP)->dq_freef->dq_freeb = (DQP)->dq_freeb;	\
}

typedef	struct dquot *sfs_DQptr;
struct dquot *sfs_dquot, *sfs_dquotNDQUOT;

/*
 * Initialize quota caches.
 */
void
sfs_qtinit()
{
	register struct dqhead *dhp;
	register struct dquot *dqp;

	sfs_dquot = (struct dquot *)kmem_zalloc(sfs_ndquot*sizeof(struct dquot), KM_SLEEP);
	sfs_dquotNDQUOT = sfs_dquot + sfs_ndquot;
	/*
	 * Initialize the cache between the in-core structures
	 * and the per-file system quota files on disk.
	 */
	for (dhp = &sfs_dqhead[0]; dhp < &sfs_dqhead[NDQHASH]; dhp++)
		dhp->dqh_forw = dhp->dqh_back = (sfs_DQptr)dhp;
	sfs_dqfreelist.dq_freef = sfs_dqfreelist.dq_freeb = (sfs_DQptr)&sfs_dqfreelist;
	for (dqp = sfs_dquot; dqp < sfs_dquotNDQUOT; dqp++) {
		dqp->dq_forw = dqp->dq_back = dqp;
		dqinsheadfree(dqp);
	}
}

/*
 * Obtain the user's on-disk quota limit for file system specified.
 */
int
sfs_getdiskquota(uid, sfs_vfsp, force, dqpp, cr)
	register uid_t uid;
	register struct sfs_vfs *sfs_vfsp;
	int force;			/* don't do enable checks */
	struct dquot **dqpp;		/* resulting dquot ptr */
	struct cred *cr;		/* user credentials */
{
	register struct dquot *dqp;
	register struct dqhead *dhp;
	register struct inode *qip;
	int error;

	dhp = &sfs_dqhead[DQHASH(uid, sfs_vfsp)];
loop:
	/*
	 * Check for quotas enabled.
	 */
	if ((sfs_vfsp->vfs_qflags & MQ_ENABLED) == 0 && !force)
		return (ESRCH);
	qip = sfs_vfsp->vfs_qinod;
	ASSERT(qip != NULL);
	/*
	 * Check the cache first.
	 */
	for (dqp = dhp->dqh_forw; dqp != (sfs_DQptr)dhp; dqp = dqp->dq_forw) {
		if (dqp->dq_uid != uid || dqp->dq_sfs_vfsp != sfs_vfsp)
			continue;
		if (dqp->dq_flags & DQ_LOCKED) {
			dqp->dq_flags |= DQ_WANT;
			(void) sleep((caddr_t)dqp, PINOD+1);
			goto loop;
		}
		dqp->dq_flags |= DQ_LOCKED;
		/*
		 * Cache hit with no references.
		 * Take the structure off the free list.
		 */
		if (dqp->dq_cnt == 0)
			dqremfree(dqp);
		dqp->dq_cnt++;
		*dqpp = dqp;
		return (0);
	}
	/*
	 * Not in cache.
	 * Get dquot at head of free list.
	 */
	if ((dqp = sfs_dqfreelist.dq_freef) == &sfs_dqfreelist) {
		cmn_err(CE_WARN, "dquot table full");
		return (EUSERS);
	}
	ASSERT(dqp->dq_cnt == 0 && dqp->dq_flags == 0);
	/*
	 * Take it off the free list, and off the hash chain it was on.
	 * Then put it on the new hash chain.
	 */
	dqremfree(dqp);
	remque(dqp);
	dqp->dq_flags = DQ_LOCKED;
	dqp->dq_cnt = 1;
	dqp->dq_uid = uid;
	dqp->dq_sfs_vfsp = sfs_vfsp;
	insque(dqp, dhp);
	if (dqoff(uid) < qip->i_size) {
		/*
		 * Read quota info off disk.
		 */
		error = sfs_rdwri(UIO_READ, qip, (caddr_t)&dqp->dq_dqb,
		    (int)sizeof (struct dqblk), dqoff(uid), UIO_SYSSPACE,
		    (int *)NULL, cr);
		if (error) {
			/*
			 * I/O error in reading quota file.
			 * Put dquot on a private, unfindable hash list,
			 * put dquot at the head of the free list and
			 * reflect the problem to caller.
			 */
			remque(dqp);
			dqp->dq_cnt = 0;
			dqp->dq_sfs_vfsp = NULL;
			dqp->dq_forw = dqp;
			dqp->dq_back = dqp;
			dqinsheadfree(dqp);
			DQUNLOCK(dqp);
			return (EIO);
		}
	} else
		bzero((caddr_t)&dqp->dq_dqb, sizeof (struct dqblk));
	*dqpp = dqp;
	return (0);
}

/*
 * Release dquot.
 */
void
sfs_dqput(dqp)
	register struct dquot *dqp;
{

	ASSERT (dqp->dq_cnt != 0);  
	ASSERT ((dqp->dq_flags & DQ_LOCKED) != 0);
	if (--dqp->dq_cnt == 0) {
		dqp->dq_flags = 0;
		dqinstailfree(dqp);
	}
	DQUNLOCK(dqp);
}

/*
 * Update on disk quota info.
 */
void
sfs_dqupdate(dqp, cr)
	register struct dquot *dqp;
	register struct cred *cr;
{
	register struct inode *qip;

	qip = dqp->dq_sfs_vfsp->vfs_qinod;
	ASSERT (qip != NULL);
	ASSERT ((dqp->dq_flags & (DQ_LOCKED|DQ_MOD)) == (DQ_LOCKED|DQ_MOD));
	dqp->dq_flags &= ~DQ_MOD;
	(void) sfs_rdwri(UIO_WRITE, qip, (caddr_t)&dqp->dq_dqb,
	    (int)sizeof (struct dqblk), dqoff(dqp->dq_uid), UIO_SYSSPACE,
	    (int *)NULL, cr);
}

/*
 * Invalidate a dquot.
 * Take the dquot off its hash list and put it on a private,
 * unfindable hash list. Also, put it at the head of the free list.
 */
int
sfs_dqinval(dqp)
	register struct dquot *dqp;
{

	ASSERT (dqp->dq_cnt == 0);  
	ASSERT ((dqp->dq_flags & (DQ_MOD|DQ_WANT)) == 0);
	dqp->dq_flags = 0;
	remque(dqp);
	dqremfree(dqp);
	dqp->dq_sfs_vfsp = NULL;
	dqp->dq_forw = dqp;
	dqp->dq_back = dqp;
	dqinsheadfree(dqp);
}

#endif QUOTA
