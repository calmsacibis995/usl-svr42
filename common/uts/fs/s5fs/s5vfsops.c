/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/s5fs/s5vfsops.c	1.10.3.18"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5ino.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5param.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/uio.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>


#define SUPERBLKNO	1
#define	SBSIZE	sizeof(struct filsys)

STATIC int s5vfs_init();
extern struct fshead	s5fshead;	/* s5 dynamic inodes filesystem head */

/*
 * UNIX file system VFS operations vector.
 */
STATIC int s5mount(), s5unmount(), s5root(), s5statvfs();
STATIC int s5sync(), s5vget(), s5mountroot();

struct vfsops s5_vfsops = {
	s5mount,
	s5unmount,
	s5root,
	s5statvfs,
	s5sync,
	s5vget,
	s5mountroot,
	fs_nosys,	/* not used */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};


STATIC int
s5mount(vfsp, mvp, uap, cr)
	register struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cr;
{
	register dev_t dev;
	register struct filsys *fp;
	register struct s5vfs *s5vfsp;
	struct vfs *dvfsp;
	struct inode *rip;
	struct vnode *rvp, *bvp;
	int rdonly = (uap->flags & MS_RDONLY);
	int remount = (uap->flags & MS_REMOUNT);
	enum vtype type;
	int error;
	struct fbuf *fbp;
	struct filsys *oldfp = NULL;
	struct buf *bp;

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	if (mvp->v_type != VDIR)
		return ENOTDIR;
	if (remount == 0 && (mvp->v_count > 1 || (mvp->v_flag & VROOT)))
		return EBUSY;

	/*
	 * Resolve path name of special file being mounted.
	 */
	if (error = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &bvp))
		return error;

	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		return ENOTBLK;
	}

	dev = bvp->v_rdev;

	/*
	 * Ensure that this device isn't already mounted, unless this is
	 * a remount request.
	 */
	dvfsp = vfs_devsearch(dev);
	if (remount) {
		VN_RELE(bvp);
		/*
		 * Remount requires that the device already be mounted
		 * read-only, and on the same mount point.
		 */
		if (dvfsp == NULL)
			return EINVAL;
		if (dvfsp != vfsp)
			return EBUSY;
		fp = getfs(vfsp);
		if (!fp->s_ronly)
			return EINVAL;
		s5vfsp = S5VFS(vfsp);
		bvp = s5vfsp->vfs_devvp;
	} else {
		/*
		 * Ordinary mount.
		 */
		if (dvfsp != NULL) {
			VN_RELE(bvp);
			return EBUSY;
		}
		/*
		 * Allocate VFS private data.
		 */
		if ((s5vfsp = (struct s5vfs *)
		  kmem_alloc(sizeof(struct s5vfs), KM_SLEEP)) == NULL) {
			VN_RELE(bvp);
			return EBUSY;
		}
		vfsp->vfs_bcount = 0;
		vfsp->vfs_data = (caddr_t) s5vfsp;

		/*
		 * Open the special device.  We will keep this vnode around
		 * for as long as the filesystem is mounted.  All subsequent
		 * fs I/O is done on this vnode.
		 */
		bvp = makespecvp(dev,VBLK);

		if (error = VOP_OPEN(&bvp, rdonly ? FREAD : FREAD|FWRITE, cr))
			goto out;
		vfsp->vfs_dev = dev;
		vfsp->vfs_fsid.val[0] = dev;
		vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
		s5vfsp->vfs_bufp = geteblk();
		s5vfsp->vfs_devvp = bvp;
	}

	/*
	 * Invalidate all pages associated with this device.
	 */
	(void) VOP_PUTPAGE(bvp, 0, 0, B_INVAL, cr);
	if (remount) {
		if (error = s5iremount(vfsp))
			goto closeout;
	}
	/*
	 * Read the superblock.  We do this in the remount case as well
	 * because it might have changed (if fsck was applied, for example).
	 *
	 * In the remount case we should really loop until the ilock and
	 * flock become free.  For the moment we trust that remount will only
	 * be done on a quiescent file system.
	 */
	fp = getfs(vfsp);
	bp = bread(vfsp->vfs_dev,SUPERBLKNO,SBSIZE);
	if(bp->b_flags & B_ERROR) {
		error = geterror(bp);
		brelse(bp);
		goto closeout;
	}
	if (remount) {
		/*
		 * Save the contents of the in-core superblock so it
		 * can be restored if the mount fails.
		 */
		oldfp = (struct filsys *)kmem_alloc(SBSIZE, KM_SLEEP);
		bcopy((caddr_t)fp, (caddr_t)oldfp, SBSIZE);
	}
	bcopy(bp->b_un.b_addr,(caddr_t) fp, SBSIZE);

	if (fp->s_magic != FsMAGIC) {
		bp->b_flags |= B_STALE|B_AGE;
		brelse(bp);
		error = EINVAL;
		goto closeout;
	}

	fp->s_ilock = 0;
	fp->s_flock = 0;
	fp->s_ninode = 0;
	fp->s_inode[0] = 0;
	fp->s_fmod = 0;
	fp->s_ronly = (char)rdonly;

	if (rdonly) {
		vfsp->vfs_flag |= VFS_RDONLY;
		brelse(bp);
	} else {
		if (fp->s_state + (long)fp->s_time == FsOKAY) {
			fp->s_state = FsACTIVE;
			bcopy((caddr_t)fp, bp->b_un.b_addr, SBSIZE);
			bp->b_flags |= B_AGE|B_STALE;
			bwrite(bp);
		} else {
			bp->b_flags |= B_AGE|B_STALE;
			brelse(bp);
			error = ENOSPC;
			goto closeout;
		}
	}


	/*
	 * Determine blocksize.
	 */
	vfsp->vfs_bsize = FsBSIZE(fp->s_bshift);
	if (vfsp->vfs_bsize < FsMINBSIZE || vfsp->vfs_bsize > MAXBSIZE) {
		error = EINVAL;
		goto closeout;
	}

	s5vfs_init(s5vfsp, vfsp->vfs_bsize);

	if (remount == 0) {
		if (error = iget(vfsp, S5ROOTINO, &rip))
			goto closeout;
		rvp = ITOV(rip);
		rvp->v_flag |= VROOT;
		s5vfsp->vfs_root = rvp;
		IUNLOCK(rip);
		s5fshead.f_mount++;
	}

	return 0;

closeout:
	/* 
	 * Clean up on error.
	 */
	if (oldfp) {
		bcopy((caddr_t)oldfp, (caddr_t)fp, SBSIZE);
		kmem_free((caddr_t)oldfp, SBSIZE);
	}
	if (remount)
		return error;
	(void) VOP_CLOSE(bvp, rdonly ? FREAD : FREAD|FWRITE, 1, (off_t) 0, cr);
	brelse(s5vfsp->vfs_bufp);

out:
	ASSERT(error);
	kmem_free((caddr_t) s5vfsp, sizeof(struct s5vfs));
	VN_RELE(bvp);
	return error;
}

STATIC int
s5unmount(vfsp, cr)
	struct vfs *vfsp;
	struct cred *cr;
{
	dev_t dev;
	register struct vnode *bvp, *rvp;
	register struct filsys *fp;
	register struct inode *rip;
	register struct s5vfs *s5vfsp = S5VFS(vfsp);
	struct fbuf *fbp;
	int error;
	int flag;
	struct buf *bp;

	ASSERT(vfsp->vfs_op == &s5_vfsops);

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	dev = vfsp->vfs_dev;

	if (iflush(vfsp, 0) < 0)
		return EBUSY;

	/*
	 * Mark inode as stale.
	 */
	inull(vfsp);
	/*
	 * Flush root inode to disk.
	 */
	rvp = s5vfsp->vfs_root;
	ASSERT(rvp != NULL);
	rip = VTOI(rvp);
	ILOCK(rip);
	/*
	 * At this point there should be no active files on the
	 * file system, and the super block should not be locked.
	 * Break the connections.
	 */
	bvp = s5vfsp->vfs_devvp;
	fp = getfs(vfsp);
	if (!fp->s_ronly) {
		bflush(dev);
		fp->s_time = hrestime.tv_sec;
		if (vfsp->vfs_flag & VFS_BADBLOCK)
			fp->s_state = FsBAD;
		else
			fp->s_state = FsOKAY - (long)fp->s_time;
		bp = ngeteblk(SBSIZE);
		bp->b_edev = vfsp->vfs_dev;
		bp->b_dev = (o_dev_t)cmpdev(bp->b_edev);
		bp->b_blkno = SUPERBLKNO;
		bcopy((caddr_t)fp, bp->b_un.b_addr, SBSIZE);
		bp->b_flags |= B_AGE|B_STALE;
		bwrite(bp);
	}
	flag = !fp->s_ronly;
	(void) VOP_PUTPAGE(bvp, 0, 0, B_INVAL, cr);
	if (error = VOP_CLOSE(bvp,
	  (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
	  1, (off_t) 0, cr)) {
		IUNLOCK(rip);
		return error;
	}
	VN_RELE(bvp);
	binval(dev);
	brelse(s5vfsp->vfs_bufp);
	iput(rip);
	iunhash(rip);
	kmem_free((caddr_t)s5vfsp, sizeof(struct s5vfs));
	rvp->v_vfsp = 0;

	s5fshead.f_mount--;
	if (s5fshead.f_mount == 0) {
		if (fs_cleanall(&s5fshead))
			cmn_err(CE_NOTE, "s5unmount: Could not release s5fshead resources \n");
	}

	/*
	 * If not mounted read only then call bdwait()
	 * to wait for async i/o to complete.
	 */
	if (flag)
		bdwait(dev);

	return 0;
}

STATIC int
s5root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	struct s5vfs *s5vfsp = S5VFS(vfsp);
	struct vnode *vp = s5vfsp->vfs_root;

	VN_HOLD(vp);
	*vpp = vp;
	return 0;
}

STATIC int
s5statvfs(vfsp, sp)
	struct vfs *vfsp;
	register struct statvfs *sp;
{
	register struct filsys *fp;
	register struct s5vfs *s5vfsp = S5VFS(vfsp);
	register i;
	char *cp;

	if ((fp = getfs(vfsp))->s_magic != FsMAGIC)
		return EINVAL;

	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize = sp->f_frsize = vfsp->vfs_bsize;
	sp->f_blocks = fp->s_fsize;
	sp->f_bfree = sp->f_bavail = fp->s_tfree;
	sp->f_files = (fp->s_isize - 2) * s5vfsp->vfs_inopb;
	sp->f_ffree = sp->f_favail = fp->s_tinode;
	sp->f_fsid = vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = DIRSIZ;
	cp = &sp->f_fstr[0];
	for (i=0; i < sizeof(fp->s_fname) && fp->s_fname[i] != '\0'; i++,cp++)
		*cp = fp->s_fname[i];
	*cp++ = '\0';
	for (i=0; i < sizeof(fp->s_fpack) && fp->s_fpack[i] != '\0'; i++,cp++)
		*cp = fp->s_fpack[i];
	*cp = '\0';

	return 0;
}

STATIC void s5update(), s5flushsb();
STATIC int s5updlock, s5updwant;

/* ARGSUSED */
STATIC int
s5sync(vfsp, flag, cr)
	struct vfs *vfsp;
	short flag;
	struct cred *cr;
{
	while (s5updlock) {
		s5updwant = 1;
		sleep((caddr_t)&s5updlock, PINOD);
	}
	s5updlock = 1;
	if (flag & SYNC_ATTR)
		s5flushi(SYNC_ATTR);
	else
		s5update();
	s5updlock = 0;
	if (s5updwant) {
		s5updwant = 0;
		wakeprocs((caddr_t)&s5updlock, PRMPT);
	}
	return 0;
}

/*
 * s5update() goes through the disk queues to initiate sandbagged I/O,
 * through the inodes to write modified nodes, and through the VFS list
 * table to initiate modified superblocks.
 */
STATIC void
s5update()
{
	register struct vfs *vfsp;
	extern struct vfsops s5_vfsops;

	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
		if (vfsp->vfs_op == &s5_vfsops)
			s5flushsb(vfsp);
	s5flushi(0);
	bflush(NODEV);	/* XXX */
}

int
s5flushi(flag)
	short flag;
{
	register struct vnode *vp;
	register struct inode *ip;
	register struct inode *nip;
	register struct hinode *hip;
	register int cheap = flag & SYNC_ATTR;


	/*
	 * See if we can shrink the inode pool
	 */
	 if (s5fshead.f_flag & DEALLOC)
		fs_cleanone(&s5fshead, 0, 0);

	/*
	 * Search through the hash chains (rather
	 * than the entire inode table) so that we examine
	 * inodes that we know are currently valid.
	 */
	for (hip=hinode; hip < &hinode[NHINO]; hip++) {
		for (ip = hip->i_forw; ip != (struct inode *) hip; ip = nip) {

			ASSERT(ITOV(ip) != NULL);
			ASSERT(ITOV(ip)->v_op == &s5vnodeops);

			/* The next ip should be checked each time it sleeps.
			 * If it has changed it should be set to the begining of
			 * the hash list.
			 * The inode pool is implicitely locked down because
			 * the inode is locked.  This prevents the inode pool
			 * from being released while it is sleeping.
			 * Please Note that PREEMPT() will be called after each
			 * hash pool instead of after each inode.
			 */
			nip = ip->i_forw; 

			vp = ITOV(ip);
			if ((ip->i_flag & (ILOCKED|IRWLOCKED)) == 0
			  && vp->v_count != 0
			  && vp->v_vfsp != 0
			  && (vp->v_pages != NULL
			    || ip->i_flag & (IACC|IUPD|ICHG|IMOD))) {
				ILOCK(ip);
				VN_HOLD(vp);
				/*
				 * If this is an inode sync for file system hardening
				 * or this is a full sync but file is a swap file,
				 * don't sync pages but make sure the inode is up
				 * to date. In other cases, push everything out.
				 */

				if (cheap || IS_SWAPVP(vp)) {
					if (ip->i_flag & (IACC|IUPD|ICHG|IMOD))
						iupdat(ip);
				} else
					(void) syncip(ip, B_ASYNC,1);
				if(ip->i_forw != nip)
					nip = hip->i_forw;
				iput(ip);
			}
		}
		PREEMPT();
	}

	return 0;
}

STATIC void
s5flushsb(vfsp)
	register struct vfs *vfsp;
{
	register struct filsys *fp;
	struct buf *bp;

	fp = getfs(vfsp);
	if (fp->s_fmod == 0 || fp->s_ilock != 0 || fp->s_flock != 0
	  || fp->s_ronly != 0)
		return;
	fp->s_fmod = 0;
	fp->s_time = hrestime.tv_sec;
	bp = ngeteblk(SBSIZE);
	bp->b_edev = vfsp->vfs_dev;
	bp->b_dev = (o_dev_t)cmpdev(bp->b_edev);
	bp->b_blkno = SUPERBLKNO;
	bcopy((caddr_t)fp, bp->b_un.b_addr, SBSIZE);
	bp->b_flags |= B_AGE|B_STALE;
	bwrite(bp);
}

STATIC int
s5vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	register struct ufid *ufid;
	struct inode *ip;

	ufid = (struct ufid *) fidp;
	if (iget(vfsp, ufid->ufid_ino, &ip)) {
		*vpp = NULL;
		return 0;
	}
	if (ip->i_gen != ufid->ufid_gen) {
		iput(ip);
		*vpp = NULL;
		return 0;
	}
	IUNLOCK(ip);
	*vpp = ITOV(ip);
	return 0;
}

/*
 * Mount root file system.
 * "why" is ROOT_INIT on initial call, ROOT_REMOUNT if called to
 * remount the root file system, and ROOT_UNMOUNT if called to
 * unmount the root (e.g., as part of a system shutdown).
 */
/* ARGSUSED */
STATIC int
s5mountroot(vfsp, why)
	struct vfs *vfsp;
	enum whymountroot why;
{
	register struct filsys *fp;
	struct s5vfs *s5vfsp;
	struct inode *mip;
	struct vnode *vp;
	struct fbuf *fbp;
	struct buf *bp;
	int error = 0;

	switch (why) {
	case ROOT_INIT:
		/*
		 * Initialize the root device.
		 */
		vp = makespecvp(rootdev, VBLK);
		if (error = VOP_OPEN(&vp, FREAD|FWRITE, u.u_cred)) {
			VN_RELE(vp);
			return error;
		}
		s5vfsp =
		  (struct s5vfs *) kmem_alloc(sizeof(struct s5vfs), KM_SLEEP);
		if (s5vfsp == NULL)
			cmn_err(CE_PANIC,
			  "s5mountroot: no memory for VFS private data");
		vfsp->vfs_flag |= VFS_RDONLY;
		s5vfsp->vfs_devvp = vp;
		s5vfsp->vfs_bufp = geteblk();
		vfsp->vfs_data = (caddr_t) s5vfsp;
		fp = getfs(vfsp);
		break;

	case ROOT_REMOUNT:
		fp = getfs(vfsp);
		if (!fp->s_ronly)
			return EINVAL;
		s5vfsp = S5VFS(vfsp);
		(void) VOP_PUTPAGE(s5vfsp->vfs_devvp,
		  0, 0, B_INVAL, (struct cred *) NULL);

		if (error = s5iremount(vfsp))
			return EINVAL;
		
		break;

	case ROOT_UNMOUNT:
		s5update();
		fp = getfs(vfsp);
		if (fp->s_state == FsACTIVE) {
			fp->s_time = hrestime.tv_sec;
			if (vfsp->vfs_flag & VFS_BADBLOCK)
				fp->s_state = FsBAD;
			else
				fp->s_state = FsOKAY - (long)fp->s_time;
			bp = ngeteblk(SBSIZE);
			bp->b_edev = vfsp->vfs_dev;
			bp->b_dev = (o_dev_t)cmpdev(bp->b_edev);
			bp->b_blkno = SUPERBLKNO;
			bcopy((caddr_t)fp, bp->b_un.b_addr, SBSIZE);
			bp->b_flags |= B_AGE|B_STALE;
			bwrite(bp);
			vp = S5VFS(vfsp)->vfs_devvp;
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1,
			  (off_t)0, u.u_cred);
			VN_RELE(vp);
		}
		bdwait(NODEV);
		return 0;

	default:
		return EINVAL;
	}
	
	vfsp->vfs_dev = rootdev;
	vfsp->vfs_fsid.val[0] = rootdev;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;

	vp = s5vfsp->vfs_devvp;

	bp = bread(vfsp->vfs_dev,SUPERBLKNO,SBSIZE);
	if(bp->b_flags & B_ERROR) {
		brelse(bp);
		VN_RELE(vp);
		return(EINVAL);
	}
	bcopy(bp->b_un.b_addr, (caddr_t)fp, SBSIZE);

	fp->s_fmod = 0;
	fp->s_ilock = 0;
	fp->s_flock = 0;
	fp->s_ninode = 0;
	fp->s_inode[0] = 0;

	if (fp->s_magic != FsMAGIC) {
		bp->b_flags |= B_AGE|B_STALE;
		brelse(bp);
		VN_RELE(vp);
		return EINVAL;
	}

	/*
	 * Determine blocksize.
	 */
	vfsp->vfs_bsize = FsBSIZE(fp->s_bshift);
	if (vfsp->vfs_bsize < FsMINBSIZE || vfsp->vfs_bsize > MAXBSIZE) {
		bp->b_flags |= B_AGE|B_STALE;
		brelse(bp);
		VN_RELE(vp);
		return EINVAL;
	}

	if (why == ROOT_REMOUNT) {
		vfsp->vfs_flag &= ~VFS_RDONLY;
		fp->s_ronly = 0;
		if ((fp->s_state + (long)fp->s_time) == FsOKAY) {
			fp->s_state = FsACTIVE;
			bcopy((caddr_t)fp, bp->b_un.b_addr, SBSIZE);
			bp->b_flags |= B_AGE|B_STALE;
			bwrite(bp);
		} else {
			bp->b_flags |= B_AGE|B_STALE;
			brelse(bp);
			return ENOSPC;
		}
	} else {	/* ROOT_INIT case */
		fp->s_ronly = 1;
		bp->b_flags |= B_AGE|B_STALE;
		brelse(bp);
	}

	s5vfs_init(s5vfsp, vfsp->vfs_bsize);

	if (why == ROOT_INIT) {
		clkset(fp->s_time);
		/* 
		 * Initialize root inode.
		 */
		if ((error = iget(vfsp, S5ROOTINO, &mip)) == 0) {

			ITOV(mip)->v_flag |= VROOT;
			IUNLOCK(mip);
			s5vfsp->vfs_root = ITOV(mip);
			if ((error = vfs_lock(vfsp)) == 0) {
				vfs_add(NULLVP, vfsp, 0);
				vfs_unlock(vfsp);
			}
		}
		if (error) {
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1,
			  (off_t)0, u.u_cred);
			VN_RELE(vp);
		} else
			s5fshead.f_mount++;
	}
	return error;
}

STATIC int
s5vfs_init(s5vfsp, bsize)
	struct s5vfs *s5vfsp;
	int bsize;
{
	int i;

	for (i = bsize, s5vfsp->vfs_bshift = 0; i > 1; i >>= 1)
		s5vfsp->vfs_bshift++;
	s5vfsp->vfs_nindir = bsize / sizeof(daddr_t);
	s5vfsp->vfs_inopb = bsize / sizeof(struct dinode);
	s5vfsp->vfs_bmask = bsize - 1;
	s5vfsp->vfs_nmask = s5vfsp->vfs_nindir - 1;
	for (i = bsize/512, s5vfsp->vfs_ltop = 0; i > 1; i >>= 1)
		s5vfsp->vfs_ltop++;
	for (i = s5vfsp->vfs_nindir, s5vfsp->vfs_nshift = 0; i > 1; i >>= 1)
		s5vfsp->vfs_nshift++;
	for (i = s5vfsp->vfs_inopb, s5vfsp->vfs_inoshift = 0; i > 1; i >>= 1)
		s5vfsp->vfs_inoshift++;
	return 0;
}
