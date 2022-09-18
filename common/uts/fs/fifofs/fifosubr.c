/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/fifofs/fifosubr.c	1.21.3.7"
#ident	"$Header: $"

/*
 * The routines defined in this file are supporting routines for FIFOFS 
 * file sytem type.
 */

#include <acc/mac/cca.h>
#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <fs/fifofs/fifonode.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * The next set of lines define the bit map used to assign
 * unique pipe inode numbers.  The value chosen for FIFOMAXID
 * is arbitrary, but must fit in a short.
 *
 * fifomap   --> bitmap with one bit per pipe ino
 * testid(x) --> is ino x already in use?
 * setid(x)  --> mark ino x as used
 * clearid(x)--> mark ino x as unused
 */

#define FIFOMAXID	SHRT_MAX
#define testid(i)	((fifomap[(i)/NBBY] & (1 << ((i)%NBBY))))
#define setid(i)	((fifomap[(i)/NBBY] |= (1 << ((i)%NBBY))), (fifoids++))
#define clearid(i)	((fifomap[(i)/NBBY] &= ~(1 << ((i)%NBBY))), (fifoids--))

STATIC char fifomap[FIFOMAXID/NBBY + 1];
STATIC ushort fifoids;

/* control structure for covert channel limiter */
STATIC ccevent_t cc_re_pipe = { CC_RE_PIPE, CCBITS_RE_PIPE };

/*
 * Define routines/data structures within this file.
 */
STATIC struct fifonode	*fifoalloc;
STATIC struct vfs	*fifovfsp;
dev_t			fifodev;

static void		fifoinsert();
static struct fifonode	*fifofind();
STATIC struct vnode	*makepipe();
STATIC ushort		fifogetid();

/*
 * Declare external routines/variables.
 */
extern struct vnodeops	fifo_vnodeops;

void			fifoclearid();
extern void		fifo_rwlock(),	fifo_rwunlock(),	freemsg();
extern int		stropen(),	fifo_close(),		closef();
extern int		strclose();

STATIC struct vfsops fifovfsops = {
	fs_nosys,	/* mount */
	fs_nosys,	/* umount */
	fs_nosys,	/* root */
	fs_nosys,	/* statvfs */
	fs_sync,
	fs_nosys,	/* vget */
	fs_nosys,	/* mountroot */
	fs_nosys,	/* not used */
	fs_nosys,	/* setceiling */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*
 * Save file system type/index, initialize vfs operations vector, get
 * unique device number for FIFOFS and initialize the FIFOFS hash.
 * Create and initialize a "generic" vfs pointer that will be placed
 * in the v_vfsp field of each pipes vnode.
 */
int
fifoinit(vswp, fstype)
	register struct vfssw *vswp;
	int fstype;
{
	register int dev;

	vswp->vsw_vfsops = &fifovfsops;
	if ((dev = getudev()) == -1) {
		cmn_err(CE_WARN, "fifoinit: can't get unique device number");
		dev = 0;
	}
	fifodev = makedevice(dev, 0);
	fifoalloc = NULL;
	fifovfsp = (struct vfs *)kmem_zalloc(sizeof(struct vfs), KM_NOSLEEP);
	fifovfsp->vfs_next = NULL;
	fifovfsp->vfs_op = &fifovfsops;
	fifovfsp->vfs_vnodecovered = NULL;
	fifovfsp->vfs_flag = 0;
	fifovfsp->vfs_bsize = FIFOBSIZE;
	fifovfsp->vfs_fstype = fstype;
	fifovfsp->vfs_fsid.val[0] = fifodev;
	fifovfsp->vfs_fsid.val[1] = fstype;
	fifovfsp->vfs_data = NULL;
	fifovfsp->vfs_dev = fifodev;
	fifovfsp->vfs_bcount = 0;
	return 0;
}

/*
 * Provide a shadow for a vnode. If vp already has a shadow in the hash list,
 * return its shadow. Otherwise, create a vnode to shadow vp, hash the 
 * new vnode and return its pointer to the caller.
 */
struct vnode *
fifovp(vp, crp)
	struct vnode *vp;
	struct cred *crp;
{
	register struct fifonode *fnp;
	register struct vnode *newvp;
	struct vattr va;

	if ((fnp = fifofind(vp)) == NULL) {
		fnp = (struct fifonode *)kmem_zalloc(sizeof(struct fifonode),
			KM_SLEEP);
		       
		/*
		 * initialize the times from vp.
		 */
		va.va_mask = AT_TIMES;
		if (vp && VOP_GETATTR(vp, &va, 0, crp) == 0) {
			fnp->fn_atime = va.va_atime.tv_sec;
			fnp->fn_mtime = va.va_mtime.tv_sec;
			fnp->fn_ctime = va.va_ctime.tv_sec;
		}

                /*
                 * Strictly speaking, this MAC_ASSUME is incorrect.
                 * No MAC check has been done on this vnode.  However,
                 * this ensures that the CCA tool understands that
                 * *vp and *sp are at the same level, and the later
                 * MAC_UNKLEV(fnp) ensures that the level relationship
                 * to *sp is correct.
                 */
                MAC_ASSUME(vp, MAC_SAME);

		fnp->fn_realvp = vp;
		newvp = FTOV(fnp);
		newvp->v_op = &fifo_vnodeops;
		newvp->v_count = 1;
		newvp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);
		/*
		 * Clear the fifonode's level.  Let the fs independent
		 * code take care of setting the vnode's level.
		 */
		if (vp->v_macflag & VMAC_SUPPORT)
			newvp->v_lid = vp->v_lid;
		else
			newvp->v_lid = vp->v_vfsp->vfs_macfloor;
		newvp->v_data = (caddr_t)fnp;
		if (vp != NULL) {        /* hold as long as shadow is active */
			VN_HOLD(vp);
			newvp->v_type = VFIFO;
			newvp->v_vfsp = vp->v_vfsp;
			newvp->v_rdev = vp->v_rdev;
		} 
		fifoinsert(fnp);

                MAC_UNKLEV (fnp);
	}
	return FTOV(fnp);
}

/*
 * Pre-validate attributes that will be passed to a subsequent fifovp() call.
 */
/*ARGSUSED*/
int
fifopreval(type, dev, cr)
	vtype_t type;
	dev_t dev;
	struct cred *cr;
{
	/*
	 * fifovp() always succeeds, so return success here.
	 */
	return 0;
}

/*
 * Create a pipe end by...
 * allocating a vnode-fifonode pair, intializing the fifonode,
 * setting the ISPIPE flag in the fifonode and assigning a unique
 * ino to the fifonode.
 *
 * This used to be an external interface.
 * Post-SVR4ES, it has been replaced by fifo_mkpipe().
 * This interface is now used within fifofs only.
 * As such, it is defined static to this file, and the interface
 * has changed to accept, as an argument, a credentials pointer.
 */
STATIC struct vnode *
makepipe(crp)
	struct cred *crp;
{
	register struct fifonode *fnp;
	register struct vnode *newvp;

	fnp = (struct fifonode *)kmem_zalloc(sizeof(struct fifonode), KM_SLEEP);

	fnp->fn_rcnt = fnp->fn_wcnt = 1;
	fnp->fn_atime = fnp->fn_mtime = fnp->fn_ctime = hrestime.tv_sec;
	fnp->fn_flag |= ISPIPE;

	newvp = FTOV(fnp);
	newvp->v_count = 1;
	newvp->v_lid = crp->cr_lid;
	newvp->v_op = &fifo_vnodeops;
	newvp->v_vfsp = fifovfsp;
	newvp->v_type = VFIFO;
	newvp->v_rdev = fifodev;
	newvp->v_data = (caddr_t) fnp;
	newvp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);
	return newvp;
}

/*
 * External interface to create the two pipe ends,
 * and do proper initialization.  It is the responsibility of
 * the caller to associate file pointers with the returned
 * vnodes.
 */
int
fifo_mkpipe(vpp1, vpp2, crp)
	struct vnode **vpp1, **vpp2;
	struct cred *crp;
{
	struct vnode *vp1, *vp2;
	register int error = 0;

	/*
	 * Allocate and initialize two vnodes.
	 */
	vp1 = makepipe(crp);
	vp2 = makepipe(crp);


	/*
	 * Create two stream heads and attach to each vnode.
	 */
	if (error = fifo_stropen(&vp1, FREAD|FWRITE, crp)) {
		VN_RELE(vp1);
		VN_RELE(vp2);
		return error;
	}

	if (error = fifo_stropen(&vp2, FREAD|FWRITE, crp)) {
		(void) strclose(vp1, 0, crp);
		VN_RELE(vp1);
		VN_RELE(vp2);
		return error;
	}

	/*
	 * Twist the stream head queues so that the write queue
	 * points to the other stream's read queue.
	 */
	vp1->v_stream->sd_wrq->q_next = RD(vp2->v_stream->sd_wrq);
	vp2->v_stream->sd_wrq->q_next = RD(vp1->v_stream->sd_wrq);

	/*
	 * Tell each pipe about its other half.
	 */
	VTOF(vp1)->fn_mate = vp2;
	VTOF(vp2)->fn_mate = vp1;
	VTOF(vp1)->fn_ino = VTOF(vp2)->fn_ino = fifogetid();

	*vpp1 = vp1;
	*vpp2 = vp2;
	return 0;
}

/*
 * External interface to dismantle the pipe ends.
 */
void
fifo_rmpipe(vp1, vp2, crp)
	struct vnode *vp1, *vp2;
	struct cred *crp;
{
	ushort ino;

	ASSERT(VTOF(vp1)->fn_ino == VTOF(vp2)->fn_ino);

	ino = VTOF(vp1)->fn_ino;
	(void) strclose(vp1, 0, crp);
	(void) strclose(vp2, 0, crp);
	VN_RELE(vp1);
	VN_RELE(vp2);
	fifoclearid(ino);
}

/*
 * Release a pipe-ino.
 */
void
fifoclearid(ino)
	ushort ino;
{
	clearid(ino);
}

/*
 * Attempt to establish a unique pipe id. Start searching the bit map where 
 * the previous search stopped. If a free bit is located, set the bit and 
 * return the new position in the bit map.
 */
STATIC ushort
fifogetid()
{
	register ushort i;
	register ushort j;
	static ushort prev = 0;

	/*
	 * If we're concerned about covert channels, start the id search
	 * at a random place rather than where the previous search stopped.
	 * If the random() routine is stubbed out, it will return 0,
	 * in which case we want to revert to the sequential method.
	 */
	if (!mac_installed || (i = random((ulong)FIFOMAXID)) == 0)
		i = prev;

	for (j = FIFOMAXID; j--; ) {
		if (i++ >= (ushort)FIFOMAXID)
			i = 1;

		if (!testid(i)) {
			setid(i);
			prev = i;
			if (fifoids > (ushort)(FIFOMAXID - RANDMINFREE))
				cc_limiter(&cc_re_pipe, u.u_cred);
			return i;
		}
	}

	cc_limiter(&cc_re_pipe, u.u_cred);
	cmn_err(CE_WARN, "fifogetid: could not establish a unique node id\n");
	return 0;
}

/*
 * Stream a pipe/FIFO.
 * The FIFOPASS flag is used when CONNLD is pushed on the stream.
 * If the flag is set, a new vnode is being passed to the upper
 * layer file system as the vnode representing an open request.
 * In that case, this process will sleep until the FIFOPASS flag
 * has been turned off.
 *
 * After returning from stropen, if the FIFOPASS flag has been set,
 * CONNLD is on the pipe and has placed a new vnode in the
 * fn_unique field of the fifonode. In that case, return the new
 * vnode to the upper layer and release the current vnode.
 */
int
fifo_stropen(vpp, flag, crp)
	struct vnode **vpp;
	int flag;
	struct cred *crp;
{
	register error = 0;
	register struct vnode *oldvp = *vpp;
	struct fifonode *fnp = VTOF(*vpp);
	struct stdata *stp;
	struct queue *wqp;
	dev_t pdev = 0;


	if (fnp->fn_flag & FIFOPASS)
		fifo_rwlock(FTOV(fnp));
	
	if ((error = stropen(oldvp, &pdev, flag, crp)) != 0) {
		if (fnp->fn_flag & FIFOPASS)
			fifo_rwunlock(FTOV(fnp));
		return error;
	}
	
	fnp->fn_open++;

	/*
	 * If the vnode was switched (connld on the pipe), return the
	 * new vnode (in fn_unique field) to the upper layer and 
	 * release the old/original one.
	 */
	if (fnp->fn_flag & FIFOPASS) {
		*vpp = fnp->fn_unique;
		fnp->fn_unique->v_flag |= VNOMAP;
		fnp->fn_flag &= ~FIFOPASS;
		fifo_rwunlock(FTOV(fnp));
		(void) fifo_close(oldvp, 0, 0, 0, crp);
		VN_RELE(oldvp);
	}

	/*
	 * Set up the stream head in order to maintain compatibility.
	 * Check the hi-water, low-water and packet sizes to ensure 
	 * the user can at least write PIPE_BUF bytes to the stream 
	 * head and that a message at least PIPE_BUF bytes can be 
	 * packaged and placed on the stream head's read queue
	 * (atomic writes).
	 */
	stp = (*vpp)->v_stream;
	stp->sd_flag |= OLDNDELAY;
	wqp = stp->sd_wrq;
	if (wqp->q_hiwat < PIPE_BUF) {
		wqp->q_hiwat = PIPE_BUF;
		RD(wqp)->q_hiwat = PIPE_BUF;
	}
	wqp->q_lowat = PIPE_BUF -1;
	RD(wqp)->q_lowat = PIPE_BUF -1;

	if (wqp->q_minpsz > 0) {
		wqp->q_minpsz = 0;
		RD(wqp)->q_minpsz = 0;
	}
	if (wqp->q_maxpsz < PIPE_BUF) {
		wqp->q_maxpsz = PIPE_BUF;
		RD(wqp)->q_maxpsz = PIPE_BUF;
	}


	return 0;
}

/*
 * Clean up the state of a FIFO and/or mounted pipe in the
 * event that a fifo_open() was interrupted while the 
 * process was sleeping.
 */
void
fifo_setjmp(vp, flag)
	struct vnode *vp;
	int flag;
{
	register struct fifonode *fnp = VTOF(vp);

	cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);

	if (flag & FREAD) {
		fnp->fn_rcnt--;
		wakeprocs((caddr_t) &fnp->fn_wcnt, PRMPT);
	}
	if (flag & FWRITE) {
		fnp->fn_wcnt--;
		wakeprocs((caddr_t) &fnp->fn_rcnt, PRMPT);
	}
}

/*
 * Insert a fifonode-vnode pair onto the fifoalloc hash list.
 */
static void
fifoinsert(fnp)
	struct fifonode *fnp;
{
	fnp->fn_backp = NULL;
	fnp->fn_nextp = fifoalloc;
	fifoalloc = fnp;
	if (fnp->fn_nextp)
		fnp->fn_nextp->fn_backp = fnp;
}

/*
 * Find a fifonode-vnode pair on the fifoalloc hash list. 
 * vp is a vnode to be shadowed. If it's on the hash list,
 * it already has a shadow, therefore return its corresponding 
 * fifonode.
 * Since this routine is used for FIFOs, a reference needs to be created
 * on the FIFOs vnode.
 */
static struct fifonode *
fifofind(vp)
	struct vnode *vp;
{
	register struct fifonode *fnode;

	for (fnode = fifoalloc;  fnode;  fnode = fnode->fn_nextp)
		if (fnode->fn_realvp == vp) {
			VN_HOLD(FTOV(fnode));
			return fnode;
		}
	return NULL;
}

/*
 * Remove a fifonode-vnode pair from the fifoalloc hash list.
 * This routine is called from the fifo_inactive() routine when a
 * FIFO is being released.
 * If the link to be removed is the only link, set fifoalloc to NULL.
 */
void
fiforemove(fnp)
	struct fifonode *fnp;
{
	register struct fifonode *fnode;

	for (fnode = fifoalloc;  fnode;  fnode = fnode->fn_nextp)
		if (fnode == fnp) {
			if (fnode == fifoalloc)
				fifoalloc = fnode->fn_nextp;
			if (fnode->fn_nextp)
				fnode->fn_nextp->fn_backp = fnode->fn_backp;
			if (fnode->fn_backp)
				fnode->fn_backp->fn_nextp = fnode->fn_nextp;
			break;
		}
				
}

/*
 * Flush "all" messages on qp. 
 * If pending PASSFD messages on the queue, close the file.
 * If flow control has been lifted, enable the queues.
 */
void
fifo_flush(qp)
register queue_t *qp;
{
	mblk_t *mp, *tmp;
	int wantw;
	queue_t *nq;
	register int s;

	s = splstr();
	wantw = qp->q_flag & QWANTW;
	mp = qp->q_first;
	qp->q_first = qp->q_last = NULL;
	qp->q_count = 0;
	qp->q_flag &= ~(QFULL | QWANTW);
	splx(s);
	while (mp) {
		tmp = mp->b_next;
		if (mp->b_datap->db_type == M_PASSFP) 
			closef(((struct strrecvfd *)mp->b_rptr)->f.fp);
		freemsg(mp);
		mp = tmp;
	}

	/*
	 * Only data messages can be queued on the
	 * stream head read queue.  We just flushed
	 * the queue, so there is no need to check
	 * if q_count < q_lowat.
	 */
	if (wantw) {
		/* find nearest back queue with service proc */
		for (nq = backq(qp); nq && !nq->q_qinfo->qi_srvp; nq = backq(nq))
			;
		if (nq)
			qenable(nq);
	}
}

/* XENIX Support */
/*
 * XENIX rdchk support.
 */
int
fifo_rdchk(vp)
struct vnode *vp;
{
	struct fifonode *fnp = VTOF(vp);

	if (vp->v_type != VFIFO || vp->v_op != &fifo_vnodeops)
		return 0;

	if (fnp->fn_flag & ISPIPE)
		/*
		 * If it's a pipe and the other end is still open,
		 * return 1. Otherwise, return 0.
		 */
		if (fnp->fn_mate)
			return 1;
		else
			return 0;
	else
		/*
		 * For non-pipe FIFO, return number of writers.
		 */
		return fnp->fn_wcnt;
}
/* End XENIX Support */
