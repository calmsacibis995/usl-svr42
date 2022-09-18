/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_vfsops.c	1.25.3.16"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * With the UFS/SFS merge, ufs_vfsops and sfs_vfsops are defined in
 * sfs_vfsops.c.  They differ only in their mount and mountroot operations.
 * All other operations in ufs_vfsops are the same as those in sfs_vfsops.
 *
 * Note that the type of VFS private data for UFS and SFS are the same.
 * For UFS, the vfs_flags is set to SFS_UFSMOUNT so that the common
 * code can distinguish between UFS and SFS at run time.  An alternative
 * would be to use ufs_vfsops to distinguish the types, but this would
 * require ufs_vfsops to be globally defined.
 */

/*
 * sfs vfs operations.
 */
STATIC int sfs_mount();
STATIC int sfs_unmount();
STATIC int sfs_root();
STATIC int sfs_statvfs();
STATIC int sfs_sync();
STATIC int sfs_vget();
STATIC int sfs_mountroot();
STATIC int sfs_setceiling();	

void sfs_sbupdate();
STATIC int sfs_unmount1();

/*
 * Common UFS/SFS functions
 */
STATIC int usfs_mountfs();
STATIC int usfs_mount();
STATIC int usfs_mountroot();

struct vfsops sfs_vfsops = {
	sfs_mount,
	sfs_unmount,
	sfs_root,
	sfs_statvfs,
	sfs_sync,
	sfs_vget,
	sfs_mountroot,
	fs_nosys,	/* not used */
	sfs_setceiling,
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*
 * ufs vfs operations.
 */
STATIC int ufs_mount();
STATIC int ufs_mountroot();

struct vfsops ufs_vfsops = {
	ufs_mount,
	sfs_unmount,
	sfs_root,
	sfs_statvfs,
	sfs_sync,
	sfs_vget,
	ufs_mountroot,
	fs_nosys,	/* not used */
	sfs_setceiling,
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*
 * sfs/ufs filesystem head for dynamic inodes
 */
extern struct fshead	sfs_fshead;


STATIC int
sfs_mount(vfsp, mvp, uap, cr)
	struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cr;
{
	/*
	 * Call the common SFS/UFS mount function
	 */
	return(usfs_mount(vfsp, mvp, uap, cr, SFS_MAGIC));
}

STATIC int
ufs_mount(vfsp, mvp, uap, cr)
	struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cr;
{
	/*
	 * Call the generic SFS/UFS mount function
	 */
	return(usfs_mount(vfsp, mvp, uap, cr, UFS_MAGIC));
}

/*
 * Common UFS/SFS mount routine.
 */
STATIC int
usfs_mount(vfsp, mvp, uap, cr, magic)
	register struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cr;
	long magic;
{
	register dev_t dev;
	struct vnode *bvp;
	struct pathname dpn;
	struct vfs *dvfsp;
	register int error;
	enum whymountroot why;

	if (pm_denied(cr, P_MOUNT)) 
		return (EPERM);

	if (mvp->v_type != VDIR)
		return (ENOTDIR);
	if ((uap->flags & MS_REMOUNT) == 0 &&
		(mvp->v_count != 1 || (mvp->v_flag & VROOT)))
			return (EBUSY);

	/*
	 * Get arguments
	 */
	if (error = pn_get(uap->dir, UIO_USERSPACE, &dpn)) { 

		return (error);
	}

	/*
	 * Resolve path name of special file being mounted.
	 */
	if (error = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &bvp)) {
		pn_free(&dpn);
		/*
		 * UFS mount command needs translation for error reporting.
		 */
		if (magic == UFS_MAGIC) {
			if (error == ENOENT)
				return (ENODEV);
		}
		return (error);
	}
	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		pn_free(&dpn);
		return (ENOTBLK);
	}
	dev = bvp->v_rdev;
	VN_RELE(bvp);
	/*
	 * Ensure that this device isn't already mounted,
	 * unless this is a REMOUNT request
	 */
	dvfsp = vfs_devsearch(dev);
	if (uap->flags & MS_REMOUNT) {
		/*
		 * Remount requires that the device already be mounted,
		 * and on the same mount point.
		 *
		 * This code really should be done at the generic
		 * level, but since other file system types
		 * peform this check, it is done here as well.
		 */
		if (dvfsp == NULL) {
			pn_free(&dpn);
			return EINVAL;
		}
		if (dvfsp != vfsp) {
			pn_free(&dpn);
			return EBUSY;
		}
		why = ROOT_REMOUNT;
	} else {
		if (dvfsp != NULL) {
			pn_free(&dpn);
			return EBUSY;
		}
		why = ROOT_INIT;
	}

	if (getmajor(dev) >= bdevcnt) {
		pn_free(&dpn);
		return (ENXIO);
	}

	/*
	 * If the device is a tape, mount it read only
	 */
	if ((*bdevsw[getmajor(dev)].d_flag & D_TAPE) == D_TAPE)
		vfsp->vfs_flag |= VFS_RDONLY;

	if (uap->flags & MS_RDONLY)
		vfsp->vfs_flag |= VFS_RDONLY;

	/*
	 * Mount the filesystem.
	 */
	error = usfs_mountfs(vfsp, why, dev, dpn.pn_path, cr, magic);
	pn_free(&dpn);
	return (error);
}

STATIC int
sfs_mountroot(vfsp, why)
	struct vfs *vfsp;
	enum whymountroot why;
{
	/*
	 * Call the common SFS/UFS mountroot function.
	 */
	return(usfs_mountroot(vfsp, why, SFS_MAGIC));
}

STATIC int
ufs_mountroot(vfsp, why)
	struct vfs *vfsp;
	enum whymountroot why;
{
	/*
	 * Call the common SFS/UFS mountroot function.
	 */
	return(usfs_mountroot(vfsp, why, UFS_MAGIC));
}

/*
 * Mount root file system.
 * "why" is ROOT_INIT on initial call, ROOT_REMOUNT is called to
 * remount the root file system after fsck at boot, and ROOT_UNMOUNT 
 * if called to unmount the root (e.g., as part of a system shutdown).
 *
 * root is mounted read only initially, after FS check the root is remount.
 * 
 * XXX - this may be partially machine-dependent; it, along with the VFS_SWAPVP
 * operation, goes along with auto-configuration.  A mechanism should be
 * provided by which machine-INdependent code in the kernel can say "get me the
 * right root file system" and "get me the right initial swap area", and have
 * that done in what may well be a machine-dependent fashion.
 * Unfortunately, it is also file-system-type dependent (NFS gets it via
 * bootparams calls, SFS/UFS gets it from various and sundry machine-dependent
 * mechanisms, as SPECFS does for swap).
 */
STATIC int
usfs_mountroot(vfsp, why, magic)
	struct vfs *vfsp;
	enum whymountroot why;
	long magic;
{
	register struct fs *fsp;
	register int error;
	static int rootdone = 0;
	struct vnode *vp;
	struct cred *cr = u.u_cred;
	int ovflags = vfsp->vfs_flag;

	if (why == ROOT_INIT) {
		if (rootdone)
			return (EBUSY);
		if (rootdev == (dev_t)NODEV)
			return (ENODEV);
		vfsp->vfs_dev = rootdev;
		vfsp->vfs_flag |= VFS_RDONLY;
	}
	else  if (why == ROOT_REMOUNT) {
		vfsp->vfs_flag |= VFS_REMOUNT;
		vfsp->vfs_flag &= ~VFS_RDONLY;
	}		
	else if (why == ROOT_UNMOUNT) {
		sfs_update();
		fsp = getfs(vfsp);
		if (fsp->fs_state == FSACTIVE) {
			fsp->fs_time = hrestime.tv_sec;
			if (vfsp->vfs_flag & VFS_BADBLOCK)
				fsp->fs_state = FSBAD;
			else
				fsp->fs_state = FSOKAY - (long)fsp->fs_time;
			vp = ((struct sfs_vfs *)vfsp->vfs_data)->vfs_devvp;
			sfs_sbupdate(vfsp);
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1, (off_t)0, cr);
			VN_RELE(vp);
		}
		bdwait(NODEV);
		return 0;
	}		
	error = vfs_lock(vfsp);
	if (error) {
		vfsp->vfs_flag = ovflags;
		return error;
	}
	error = usfs_mountfs(vfsp, why, rootdev, "/", cr, magic);
	if (error) {
		vfs_unlock(vfsp);
		vfsp->vfs_flag = ovflags;
		return (error);
	}
	if (why == ROOT_INIT)
		vfs_add((struct vnode *)0, vfsp,
			(vfsp->vfs_flag & VFS_RDONLY) ? MS_RDONLY : 0);
	vfs_unlock(vfsp);
	fsp = getfs(vfsp);
	clkset(fsp->fs_time);
	rootdone = 1;
	return (0);
}

STATIC int
usfs_mountfs(vfsp, why, dev, path, cr, magic)
	struct vfs *vfsp;
	enum whymountroot why;
	dev_t dev;
	char *path;
	struct cred *cr;
	long magic;
{
	struct vnode *devvp = 0;
	register struct fs *fsp;
	register struct sfs_vfs *sfs_vfsp = 0;
	register struct buf *bp = 0;
	struct buf *tp = 0;
	int error;
	int blks;
	caddr_t space = 0;
	caddr_t base = 0;
	int i;
	long size;
	size_t len;
	int needclose = 0;
	struct inode *rip;
	struct vnode *rvp;
	
	ASSERT(magic == SFS_MAGIC || magic == UFS_MAGIC);
	if (why == ROOT_INIT) {
		/*
		 * Open the device.
		 */
		devvp = makespecvp(dev, VBLK);

		/*
		 * Open block device mounted on.
		 * When bio is fixed for vnodes this can all be vnode
		 * operations.
		 */
		error = VOP_OPEN(&devvp,
		    (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, cr);
		if (error) {
			VN_RELE(devvp);
			return (error);
		}
		needclose = 1;

		/*
		 * Refuse to go any further if this
		 * device is being used for swapping.
		 */
		if (IS_SWAPVP(devvp)) {
			error = EBUSY;
			goto out;
		}
	}
	/*
	 * check for dev already mounted on
	 */

	if (vfsp->vfs_flag & VFS_REMOUNT) {
		sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
		devvp = sfs_vfsp->vfs_devvp;
	}
	ASSERT(devvp != 0);
	/* Invalidate pages for this dev */
	(void) VOP_PUTPAGE(devvp, 0, 0, B_INVAL, cr);
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		fsp = getfs(vfsp);
		if (!fsp->fs_ronly) {
			error = EINVAL;
			goto out;
		}
		if (error = sfs_iremount(vfsp))
			goto out;
	}		
	/*
	 * read in superblock
	 */
	tp = bread(dev, BBSIZE/SBSIZE, SBSIZE);
	if (tp->b_flags & B_ERROR) {
		goto out;
	}
	fsp = (struct fs *)tp->b_un.b_addr;
	if (fsp->fs_magic != magic || fsp->fs_bsize > MAXBSIZE || 
	    fsp->fs_frag > MAXFRAG || fsp->fs_bsize < sizeof (struct fs) || 
		fsp->fs_bsize < PAGESIZE/2) {
		error = EINVAL;	/* also needs translation */
		goto out;
	}
	/*
	 * Allocate VFS private data.
	 */
	if (!(vfsp->vfs_flag & VFS_REMOUNT)) {
		if ((sfs_vfsp = (struct sfs_vfs *)
				kmem_zalloc(sizeof(struct sfs_vfs), KM_SLEEP)) 
				== NULL) {
			error = ENOMEM;
			goto out;
		}
		vfsp->vfs_bcount = 0;
		vfsp->vfs_data = (caddr_t) sfs_vfsp;
		vfsp->vfs_dev = dev;
		vfsp->vfs_flag |= VFS_NOTRUNC;
		vfsp->vfs_fsid.val[0] = dev;
		sfs_vfsp->vfs_devvp = devvp;
		if (magic == SFS_MAGIC) {
			vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
		} else {
			vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
			sfs_vfsp->vfs_flags = SFS_UFSMOUNT;
		}
	}
		
	/*
	 * Copy the super block into a buffer in its native size.
	 * Use ngeteblk to allocate the buffer
	 */
	bp = ngeteblk((int)fsp->fs_bsize);
	bp->b_bcount = fsp->fs_sbsize;
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
	   (u_int)fsp->fs_sbsize);
	tp->b_flags |= B_STALE | B_AGE;
	brelse(tp);
	tp = 0;

	fsp = (struct fs *)bp->b_un.b_addr;
	/*
	 * Currently we only allow a remount to change from
	 * read-only to read-write.
	 */
	if (vfsp->vfs_flag & VFS_RDONLY) {
		ASSERT((vfsp->vfs_flag & VFS_REMOUNT) == 0);
		fsp->fs_ronly = 1;
		fsp->fs_fmod = 0;
	} else {
		if (vfsp->vfs_flag & VFS_REMOUNT) {
			if (fsp->fs_state == FSACTIVE) {
				error = EINVAL;
				goto out;
			}
		}
		if ((fsp->fs_state + (long)fsp->fs_time) == FSOKAY)
			fsp->fs_state = FSACTIVE;
		else {
			error = ENOSPC;
			goto out;
		}
		/* write out to disk synchronously */
		tp = ngeteblk((int)fsp->fs_bsize);
		tp->b_edev = dev;
		tp->b_dev = cmpdev(dev);
		tp->b_blkno = SBLOCK;
		tp->b_bcount = fsp->fs_sbsize;
		bcopy((char *)fsp, tp->b_un.b_addr, 
		(u_int)fsp->fs_sbsize);
		bwrite(tp);
		tp = 0;
		fsp->fs_fmod = 1;
		fsp->fs_ronly = 0;
	}
	vfsp->vfs_bsize = fsp->fs_bsize;

	/*
	 * Read in cyl group info
	 */
        blks = howmany(fsp->fs_cssize, fsp->fs_fsize);
	base = space = (caddr_t)kmem_alloc((u_int)fsp->fs_cssize, KM_SLEEP);
	if (space == 0) {
		error = ENOMEM;
		goto out;
	}
	for (i = 0; i < blks; i += fsp->fs_frag) {
		size = fsp->fs_bsize;
		if (i + fsp->fs_frag > blks)
			size = (blks - i) * fsp->fs_fsize;
		tp = bread(dev, (daddr_t)fragstoblks(fsp, fsp->fs_csaddr+i),
			fsp->fs_bsize);
		if (tp->b_flags & B_ERROR) {
			goto out;
		}
		bcopy((caddr_t)tp->b_un.b_addr, space, (u_int)size);
		fsp->fs_csp[fragstoblks(fsp, i)] = (struct csum *)space;
		space += size;
		tp->b_flags |= B_AGE;
		brelse(tp);
		tp = 0;
	}
	copystr(path, fsp->fs_fsmnt, sizeof (fsp->fs_fsmnt) - 1, &len);
	bzero(fsp->fs_fsmnt + len, sizeof (fsp->fs_fsmnt) - len);
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		struct fs *tfsp =
			(struct fs *)(sfs_vfsp->vfs_bufp)->b_un.b_addr;
		int s = splhi();
		kmem_free((caddr_t)tfsp->fs_csp[0], (u_int)tfsp->fs_cssize);
		bcopy((caddr_t)bp->b_un.b_addr,
			(caddr_t)tfsp,
			(u_int)fsp->fs_sbsize);
		(void)splx(s);
		bp->b_flags |= B_AGE;
		brelse(bp);
	} else {
		sfs_vfsp->vfs_bufp = bp;
		if (error = sfs_iget(vfsp, fsp, SFSROOTINO, &rip, cr))
			goto out;
		rvp = ITOV(rip);
		rvp->v_flag |= VROOT;
		sfs_vfsp->vfs_root = rvp;
		IUNLOCK(rip);
	}

	if (why == ROOT_INIT)
		sfs_fshead.f_mount++;

	return (0);
out:
	if (error == 0)
		error = EIO;

	if (base)
		kmem_free((void *)base, (u_int)fsp->fs_cssize);
		
	if (bp) {
		bp->b_flags |= B_AGE;
		brelse(bp);
	}
	if (tp) {
		tp->b_flags |= B_AGE;
		brelse(tp);
	}
	if (!(vfsp->vfs_flag & VFS_REMOUNT)) {
		if (sfs_vfsp) 
			kmem_free((caddr_t)sfs_vfsp, sizeof(struct sfs_vfs));
	}
	if (needclose) {
		(void) VOP_CLOSE(devvp, (vfsp->vfs_flag & VFS_RDONLY) ?
		      FREAD : FREAD|FWRITE, 1, 0, cr);
		binval(dev);
		VN_RELE(devvp);
	}
	return (error);
}

/*
 * vfs operations
 */
STATIC int
sfs_unmount(vfsp, cr)
	struct vfs *vfsp;
	struct cred *cr;
{

	return (sfs_unmount1(vfsp, 0, cr));
}

STATIC int
sfs_unmount1(vfsp, forcibly, cr)
	register struct vfs *vfsp;
	int forcibly;
	struct cred *cr;
{
	dev_t dev = vfsp->vfs_dev;
	register struct fs *fs;
	register int stillopen;
	register struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	int flag;
	struct vnode *bvp, *rvp;
	struct inode *rip;
	struct inode *qip;
	struct buf *bp;
	extern int sfs_updlock;

	if (pm_denied(cr, P_MOUNT))
		return (EPERM);

#ifdef QUOTA
	qip = sfs_vfsp->vfs_qinod;
#else
	qip = (struct inode *)NULL;
#endif
	if ((stillopen = sfs_iflush(vfsp, qip, cr)) < 0 && !forcibly)
		return (EBUSY);
	if (stillopen < 0)
		return (EBUSY);		/* XXX */
#ifdef QUOTA
	if (UFSVFSP(vfsp)) {
		(void) sfs_closedq(sfs_vfsp, cr);
		/*
	 	 * Here we have to sfs_iflush again to get rid of the quota
		 *  inode.  A drag, but it would be ugly to cheat, & this 
		 * doesn't happen often
	 	*/
		(void) sfs_iflush(vfsp, (struct inode *)NULL, cr);
	}
#endif
	/* Flush root inode to disk */
	rvp = sfs_vfsp->vfs_root;
	ASSERT(rvp != NULL);
	rip = VTOI(rvp);
	ILOCK(rip);
	sfs_iupdat(rip, IUP_SYNC);

	fs = getfs(vfsp);
	bp = sfs_vfsp->vfs_bufp;
	bvp = sfs_vfsp->vfs_devvp;
	kmem_free((caddr_t)fs->fs_csp[0], (u_int)fs->fs_cssize);
	flag = !fs->fs_ronly;
	while(sfs_updlock)
		(void)sleep((caddr_t)&sfs_updlock, PINOD);
	sfs_updlock++;
	if (!fs->fs_ronly && ((sfs_vfsp->vfs_flags & SFS_FSINVALID) == 0)) {
		bflush(dev);
		fs->fs_time = hrestime.tv_sec;
		if (vfsp->vfs_flag & VFS_BADBLOCK)
			fs->fs_state = FSBAD;
		else
			fs->fs_state = FSOKAY - (long)fs->fs_time;
                bcopy((char *)fs, bp->b_un.b_addr, (long)fs->fs_sbsize);
                bp->b_edev = dev;
                bp->b_dev = cmpdev(dev);
                bp->b_bcount = fs->fs_sbsize;
                bp->b_blkno = SBLOCK;
                bwrite(bp);
	}
	else {
		bp->b_flags |= B_AGE;
		brelse(bp);
	}
	sfs_updlock = 0;
	wakeprocs((caddr_t)&sfs_updlock, PRMPT);
	(void) VOP_PUTPAGE(bvp, 0, 0, B_INVAL, cr);
	(void) VOP_CLOSE(bvp, flag, 1, 0, cr);
	VN_RELE(bvp);
	binval(dev);
	sfs_iput(rip);
	remque(rip);
	kmem_free((caddr_t)sfs_vfsp, sizeof(struct sfs_vfs));

	sfs_fshead.f_mount--;
	if (sfs_fshead.f_mount == 0) {
		if (fs_cleanall(&sfs_fshead))
			printf("sfs_unmount1: Could not release sfs_fshead resources\n");
	}
	/*
	 * If not mounted read only then call bdwait()
	 * to  wait for async i/o to complete.
	 */
	if (flag)
		bdwait(dev);

	return (0);
}

STATIC int
sfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	struct vnode *vp = sfs_vfsp->vfs_root;

	VN_HOLD(vp);
	*vpp = vp;
	return(0);
}

/*
 * Get file system statistics.
 */
STATIC int
sfs_statvfs(vfsp, sp)
	register struct vfs *vfsp;
	struct statvfs *sp;
{
	register struct fs *fsp;
	int blk, i;
	long bavail;

	fsp = getfs(vfsp);
	if (UFSVFSP(vfsp)) {
		if (fsp->fs_magic != UFS_MAGIC) 
			return (EINVAL);
	} else if (fsp->fs_magic != SFS_MAGIC)
			return (EINVAL);
	(void)bzero((caddr_t)sp, (int)sizeof(*sp));
	sp->f_bsize = fsp->fs_bsize;
	sp->f_frsize = fsp->fs_fsize;
	sp->f_blocks = fsp->fs_dsize;
	sp->f_bfree = fsp->fs_cstotal.cs_nbfree * fsp->fs_frag +
	    fsp->fs_cstotal.cs_nffree;
	/*
	 * avail = MAX(max_avail - used, 0)
	 */
	bavail = (fsp->fs_dsize * (100 - fsp->fs_minfree) / 100) -
	    (fsp->fs_dsize - sp->f_bfree);
	sp->f_bavail = bavail < 0 ? 0 : bavail;
	if (UFSVFSP(vfsp))
		sp->f_files =  fsp->fs_ncg * fsp->fs_ipg;
	else {
		/*
	 	 * inodes - divide for alternate inodes
	 	 */
		sp->f_files =  (fsp->fs_ncg * fsp->fs_ipg) / NIPFILE;
	}
	sp->f_ffree = sp->f_favail = fsp->fs_cstotal.cs_nifree;
	sp->f_fsid = vfsp->vfs_dev;
	(char *) strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = (MAXNAMELEN-1);
	blk = fsp->fs_spc * fsp->fs_cpc / NSPF(fsp);
	for (i = 0; i < blk; i += fsp->fs_frag)
		/* void */;
	i -= fsp->fs_frag;
	blk = i / fsp->fs_frag;
	bcopy((char *)&(fsp->fs_rotbl[blk]), sp->f_fstr, 14);
	return (0);
}

/*
 * Flush any pending I/O to file system vfsp.
 * The sfs_update() routine will only flush *all* sfs files.
 */
/*ARGSUSED*/
STATIC int
sfs_sync(vfsp, flag, cr)
	struct vfs *vfsp;
	short flag;
	struct cred *cr;
{

	if (flag & SYNC_ATTR)
		sfs_flushi(SYNC_ATTR);
	else
		sfs_update();
	return 0;
}

/*
 * Update superblock of file system vfsp.
 */
void
sfs_sbupdate(vfsp)
	struct vfs *vfsp;
{
	register struct fs 	*fs = getfs(vfsp);
	struct buf		*bp;
	int blks;
	caddr_t space;
	long size;
	int 			i;

        bp = ngeteblk(fs->fs_bsize);
        bp->b_edev = vfsp->vfs_dev;
        bp->b_dev = cmpdev(vfsp->vfs_dev);
        bp->b_blkno = SBLOCK;
        bp->b_bcount = fs->fs_sbsize;
        bcopy((caddr_t)fs, bp->b_un.b_addr, (uint)fs->fs_sbsize);
        bwrite(bp);
        blks = howmany(fs->fs_cssize, fs->fs_fsize);
        space = (caddr_t)fs->fs_csp[0];
        for (i = 0; i < blks; i += fs->fs_frag) {
                size = fs->fs_bsize;
                if (i + fs->fs_frag > blks)
                        size = (blks -i) * fs->fs_fsize;
                bp = getblk(vfsp->vfs_dev,
                        (daddr_t)(fragstoblks(fs, fs->fs_csaddr+i)),
                        fs->fs_bsize);
                bcopy(space, bp->b_un.b_addr, (u_int)size);
                space += size;
                bp->b_bcount = size;
                bwrite(bp);

        }
}

STATIC int
sfs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	register struct ufid *ufid;
	register struct fs *fs = getfs(vfsp);
	struct inode *ip;

	ufid = (struct ufid *)fidp;
	if (sfs_iget(vfsp, fs, ufid->ufid_ino, &ip, u.u_cred)) {
		*vpp = NULL;
		return (0);
	}
	if (ip->i_gen != ufid->ufid_gen) {
		sfs_idrop(ip);
		*vpp = NULL;
		return (0);
	}
	IUNLOCK(ip);
	*vpp = ITOV(ip);
	if ((ip->i_mode & ISVTX) && !(ip->i_mode & (IEXEC | IFDIR))) {
		(*vpp)->v_flag |= VISSWAP;
	}
	return (0);
}

STATIC int
sfs_setceiling(vfsp, level)
	struct vfs *vfsp;
	lid_t level;

{
	/* this routine sets the ceiling of the mounted file system 
	 * checks are done at fs independet level to make sure 
	 * that that new level dominates the floor of the mounted
	 * file system.  Note that the level of the root vnode is the
	 * same as the level of floor ceiling of the file system 
         * the level of the root vnode cannot change while it is mounted
         */

	vfsp->vfs_macceiling = level;

	return 0;
}
