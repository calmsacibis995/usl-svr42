/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_vnops.c	1.38.4.21"
#ident	"$Header: $"

#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/fbuf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/pathname.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_fsdir.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/specfs/snode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/poll.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/rm.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/resource.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef QUOTA
#include <fs/sfs/sfs_quota.h>
#endif

#define ISVDEV(t)	(((t) != VREG) && (t) == VCHR || (t) == VBLK || (t) == VFIFO \
				|| (t) == VXNAM)

STATIC	int sfs_open();
STATIC	int sfs_close();
STATIC	int sfs_read();
STATIC	int sfs_write();
STATIC	int sfs_ioctl();
STATIC	int sfs_setfl();
STATIC	int sfs_getattr();
STATIC	int sfs_setattr();
STATIC	int sfs_access();
STATIC	int sfs_lookup();
STATIC	int sfs_create();
STATIC	int sfs_remove();
STATIC	int sfs_link();
STATIC	int sfs_rename();
STATIC	int sfs_mkdir();
STATIC	int sfs_rmdir();
STATIC	int sfs_readdir();
STATIC	int sfs_symlink();
STATIC	int sfs_readlink();
STATIC	int sfs_fsync();
STATIC	void sfs_inactive();
STATIC	int sfs_fid();
STATIC	void sfs_rwlock();
STATIC	void sfs_rwunlock();
STATIC	int sfs_seek();
STATIC	int sfs_frlock();
STATIC  int sfs_space();

STATIC	int sfs_getpage();
STATIC	int sfs_putpage();
STATIC	int sfs_map();
STATIC	int sfs_addmap();
STATIC	int sfs_delmap();
STATIC	int sfs_poll();
STATIC	int sfs_pathconf();
STATIC	int sfs_getacl();
STATIC	int sfs_setacl();
STATIC	int sfs_getaclcnt();
STATIC	int sfs_setlevel();
STATIC	int sfs_makemld();
STATIC	int sfs_readi();
STATIC	int sfs_writei();
STATIC	int sfs_allocstore();

struct vnodeops sfs_vnodeops = {
	sfs_open,
	sfs_close,
	sfs_read,
	sfs_write,
	sfs_ioctl,
	sfs_setfl,
	sfs_getattr,
	sfs_setattr,
	sfs_access,
	sfs_lookup,
	sfs_create,
	sfs_remove,
	sfs_link,
	sfs_rename,
	sfs_mkdir,
	sfs_rmdir,
	sfs_readdir,
	sfs_symlink,
	sfs_readlink,
	sfs_fsync,
	sfs_inactive,
	sfs_fid,
	sfs_rwlock,
	sfs_rwunlock,
	sfs_seek,
	fs_cmp,
	sfs_frlock,
	sfs_space,
	fs_nosys,	/* realvp */
	sfs_getpage,
	sfs_putpage,
	sfs_map,
	sfs_addmap,
	sfs_delmap,
	sfs_poll,
	fs_nosys,	/* not used */
	sfs_pathconf,
	sfs_allocstore,
	sfs_getacl,	
	sfs_getaclcnt,
	sfs_setacl,	
	sfs_setlevel,	
	fs_nosys, 	/* getdvstat */
	fs_nosys, 	/* setdvstat */
	sfs_makemld,
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*
 * No special action required for ordinary files.  (Devices are handled
 * through the device file system.)
 */
/* ARGSUSED */
STATIC int
sfs_open(vpp, flag, cr)
	struct vnode **vpp;
	int flag;
	struct cred *cr;
{
	struct inode *ip = VTOI(*vpp);
	struct vfs *vfsp = (*vpp)->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ip->i_opencnt++;
	return 0;
}

/*ARGSUSED*/
STATIC int
sfs_close(vp, flag, count, offset, cr)
	struct vnode *vp;
	int flag;
	int count;
	off_t offset;
	struct cred *cr;
{
	register struct inode *ip = VTOI(vp);

	ILOCK(ip);
	ASSERT(ip->i_opencnt > 0);
	if (count == 1)			/* only decrement open count on last */
		ip->i_opencnt--;	/* close for this file table entry */
	ITIMES(ip);
	cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
	IUNLOCK(ip);
	return 0;
}

/*ARGSUSED*/
STATIC int
sfs_read(vp, uiop, ioflag, fcr)
	struct vnode *vp;
	struct uio *uiop;
	int ioflag;
	struct cred *fcr;
{
	struct cred *cr = VCURRENTCRED(fcr);	/* refer to vnode.h */
	struct inode *ip = VTOI(vp);
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	int error;

	ASSERT(fcr != NULL);

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ASSERT(ip->i_flag & IRWLOCKED);

	error = sfs_readi(ip, uiop,ioflag, cr);
	ACC_TIMES(ip);
	return error;
}

/*ARGSUSED*/
STATIC int
sfs_write(vp, uiop, ioflag, fcr)
	struct vnode *vp;
	struct uio *uiop;
	int ioflag;
	struct cred *fcr;
{
	struct cred *cr = VCURRENTCRED(fcr);	/* refer to vnode.h */
	struct inode *ip;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	int error;

	ASSERT(fcr != NULL);

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	/*
	 * NOTE:  this assertion is consistent with the agreed on
	 * vnode interface provisions for preserving atomicity of
	 * reads and writes, but it necessarily implies that the
	 * sfs_ilock() call in sfs_rdwr is recursive.
	 */
	ip = VTOI(vp);
	ASSERT((ip->i_flag & IRWLOCKED)
	  || (vp->v_type != VREG && vp->v_type != VDIR));

	if (vp->v_type == VREG &&
	  (error = fs_vcode(vp, &ip->i_vcode)))
		return error;

	if ((ioflag & IO_APPEND) != 0 && (ip->i_mode & IFMT) == IFREG) {
		/*
		 * In append mode start at end of file.
		 */
		uiop->uio_offset = ip->i_size;
	}
	error = sfs_writei(ip, uiop, ioflag, cr);
	UPD_TIMES(ip);
	return (error);
}


/*
 * Don't cache write blocks to files with the sticky bit set.
 * Used to keep swap files from blowing the page cache on a server.
 */
STATIC int sfs_stickyhack = 1;

/* ARGSUSED */
STATIC int
sfs_readi(ip, uio, ioflag, cr)
	register struct inode *ip;
	register struct uio *uio;
	int ioflag;
	struct cred *cr;
{
	register addr_t base;
	register int n, on, mapon;
	register struct fs *fs;
	register off_t offset = uio->uio_offset;
	struct vnode *vp;
	int type, error = 0;
	int diff;
	u_int off;
	u_int flags;
	struct uio tmpuio, *olduio;
	struct iovec tmpiov;
	int isshortlink = 0, docopy = 1;

	type = ip->i_mode & IFMT;
	ASSERT(type == IFREG || type == IFDIR || type == IFLNK);
	vp = ITOV(ip);
	if (MANDLOCK(vp, ip->i_mode)
	  && (error = chklock(vp, FREAD, 
	    offset, uio->uio_resid, uio->uio_fmode)))
		return (error);
	if (offset< 0) 
		return (EINVAL);
	if (uio->uio_resid == 0)
		return (0);
	if (WRITEALLOWED(vp, cr))
		ip->i_flag |= IACC;

	fs = ip->i_fs;
	/*
	 * If the inode is a short symlink, and the target is already
	 * being kept in the db list, then just copy the target to the
	 * uio structure.  If the target is not in the db list, but could
	 * be, then redirect the uio structure so that the uiomove copies
	 * the data into the inode.  Once it's in the inode, it will then
	 * be copied to the original uio structure.
	 */
	if ((type == IFLNK) && (ip->i_size <= SHORTSYMLINK)) {
		if (ip->i_db[1] != 0)
			return (uiomove((char *)&ip->i_db[1], ip->i_size,
				UIO_READ, uio));
		tmpiov.iov_base = (char *)&ip->i_db[1];
		tmpiov.iov_len = ip->i_size;
		tmpuio.uio_iov = &tmpiov;
		tmpuio.uio_iovcnt = 1;
		tmpuio.uio_offset = 0;
		tmpuio.uio_segflg = UIO_SYSSPACE;
		tmpuio.uio_resid = ip->i_size;
		olduio = uio;
		uio = &tmpuio;
		++isshortlink;
	}
	do {
		diff = ip->i_size - offset;

		off = offset & MAXBMASK;
		mapon = offset & MAXBOFFSET;
		on = blkoff(fs, offset);
		n = MIN(fs->fs_bsize - on, uio->uio_resid);

		if (diff <= 0) 	/* check to make sure don't need to set error */
			break;
	
		if (diff < n)
			n = diff;

		base = segmap_getmap(segkmap, vp, off);

		if ((error = uiomove(base + mapon, (long)n, UIO_READ, uio)) == 0) {
			offset = uio->uio_offset;
			if (isshortlink)
				flags = SM_INVAL;
			else if (n + mapon == MAXBSIZE || offset == ip->i_size)
				flags = SM_DONTNEED;
			else
				flags = 0;
		  	error = segmap_release(segkmap, base, flags);
		} else
			(void)segmap_release(segkmap, base, 0);

	} while (error == 0 && uio->uio_resid > 0 && n != 0);

	if ((isshortlink) && (error == 0)) {
		error = uiomove((char *)&ip->i_db[1], ip->i_size, UIO_READ,
			olduio);
	}
	return (error);
}

STATIC int
sfs_writei(ip, uio, ioflag, cr)
	register struct inode *ip;
	register struct uio *uio;
	int ioflag;
	struct cred *cr;
{
	register u_int off;
	register addr_t base;
	register int n, on, mapon;
	register struct fs *fs;
	struct vnode *vp;
	int type, error, pagecreate;
	int alloc_only;
	rlim_t limit = uio->uio_limit;
	u_int flags;
	int iupdat_flag;
	long old_blocks;
	long oresid = uio->uio_resid;
	page_t *iolpl[MAXBSIZE/PAGESIZE + 2];
	page_t **ppp;
	int isshortlink = 0;
	struct uio tmpuio;
	struct iovec tmpiov;


	/*
	 * ip->i_size is incremented before the uiomove
	 * is done on a write.  If the move fails (bad user
	 * address) reset ip->i_size.
	 * The better way would be to increment ip->i_size
	 * only if the uiomove succeeds.
	 */
	int i_size_changed = 0;
	int old_i_size;

	type = ip->i_mode & IFMT;
	ASSERT(type == IFREG || type == IFDIR || type == IFLNK);
	vp = ITOV(ip);
	if (MANDLOCK(vp, ip->i_mode)
	  && (error = chklock(vp, FWRITE,
	    uio->uio_offset, uio->uio_resid, uio->uio_fmode)))
		return (error);
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0)
		return (EINVAL);
	if (uio->uio_resid == 0)
		return (0);

	ip->i_flag |= INOACC;	/* don't update ref time in getpage */

	if (ioflag & IO_SYNC) {
		ip->i_flag |= ISYNC;
		old_blocks = ip->i_blocks;
		iupdat_flag = 0;
	}
	fs = ip->i_fs;
	/*
	 * If the inode being written is a short symlink, then create
	 * a second uio structure which shows where the target is being
	 * copied from.  The target will be copied twice, once using the
	 * original uio structure and once using this new one.
	 */
	if ((type == IFLNK) && (uio->uio_resid <= SHORTSYMLINK) &&
			(uio->uio_iovcnt == 1)) {
		++isshortlink;
		tmpuio = *uio;
		tmpiov = *tmpuio.uio_iov;
		tmpuio.uio_iov = &tmpiov;
	}
	do {
		off = uio->uio_offset & MAXBMASK;
		mapon = uio->uio_offset & MAXBOFFSET;
		on = blkoff(fs, uio->uio_offset);
		n = MIN(fs->fs_bsize - on, uio->uio_resid);

                old_i_size = ip->i_size;

		if (type == IFREG && uio->uio_offset + n >= limit) {
			if (uio->uio_offset >= limit) {
				error = EFBIG;
				goto out;
			}
			n = limit - uio->uio_offset;
		}

		/*
		 * as_iolock will determine if we are properly
		 * page-aligned to do the pagecreate case, and if so,
		 * will hold the "from" pages until after the uiomove
		 * to avoid deadlocking and to catch the case of
		 * writing a file to itself.
		 */
		n = as_iolock(uio, iolpl, n, vp, old_i_size,
				&pagecreate);
		if (n == 0) {
			error = EFAULT;
			break;
		}

		if (uio->uio_offset + n > old_i_size) {
			i_size_changed = 1;
			iupdat_flag = 1;
		}
	
		/*
		 * On pagecreate, bmap() can do "allocation only"
		 * if bsize <= PAGESIZE or a full block is being written.
		 */
		alloc_only = pagecreate &&
			(fs->fs_bsize <= PAGESIZE ||
			 (on == 0 && roundup(n, PAGESIZE) >= fs->fs_bsize));

		/*
		 * sfs_bmap is used so that we are sure that
		 * if we need to allocate new blocks, that it
		 * is done here before we up the file size.
		 */
		ILOCK(ip);
		error = sfs_bmap(ip,
		    (daddr_t)lblkno(fs, uio->uio_offset),
		    (daddr_t *)NULL, (daddr_t *)NULL,
		    (int)(on + n), S_WRITE, alloc_only,cr);
		IUNLOCK(ip);
		if (error) {
			for (ppp = iolpl; *ppp; ppp++)
				PAGE_RELE(*ppp);
			break;
		}

		if (i_size_changed)
			ip->i_size = uio->uio_offset + n;

		base = segmap_getmap(segkmap, vp, off);

		if (pagecreate)
			segmap_pagecreate(segkmap, base + mapon, (u_int)n, 0);

		error = uiomove(base + mapon, (long)n, UIO_WRITE, uio);

		/* Now release any pages held by as_iolock */
		for (ppp = iolpl; *ppp; ppp++)
			PAGE_RELE(*ppp);

		if (pagecreate && uio->uio_offset <
		    roundup(off + mapon + n, PAGESIZE)) {
			/*
			 * We created pages w/o initializing them completely,
			 * thus we need to zero the part that wasn't set up.
			 * This happens on most EOF write cases and if
			 * we had some sort of error during the uiomove.
			 */
			int nzero, nmoved;

			nmoved = uio->uio_offset - (off + mapon);
			ASSERT(nmoved >= 0 && nmoved <= n);
			nzero = roundup(n, PAGESIZE) - nmoved;
			ASSERT(nzero > 0 && mapon + nmoved + nzero <= MAXBSIZE);
			(void) kzero(base + mapon + nmoved, (u_int)nzero);
		}

		if (error) {
			/*
			 * Invalidate any pages that have been
			 * allocated.
			 */
			(void) segmap_release(segkmap, base, SM_INVAL); 
			/*
			 * Truncate file to last successful write
			 * if file was being extended.  This
			 * takes care of block leakage, but more
			 * importantly prevents the misuse of an
			 * allocated fragment as a full logical
			 * block.
			 */
			if (i_size_changed) {
				ILOCK(ip);
				(void) sfs_itrunc(ip, old_i_size, cr);
				IUNLOCK(ip);
			}
		} else {
			flags = 0;
			/*
			 * Force write back for synchronous write cases.
			 */
			if ((ioflag & IO_SYNC) || type == IFDIR) {
			/*
			 * If the sticky bit is set but the
			 * execute bit is not set, we do a
			 * synchronous write back and free
			 * the page when done.  We set up swap
			 * files to be handled this way to
			 * prevent servers from keeping around
			 * the client's swap pages too long.
			 * XXX - there ought to be a better way.
			 */
				if (IS_SWAPVP(vp)) {
					flags = SM_WRITE | SM_FREE | SM_DONTNEED;
				} else {
					iupdat_flag = 1;
					flags = SM_WRITE;
				}
			} else if (n + mapon == MAXBSIZE || IS_SWAPVP(vp)) {
				/*
				 * Have written a whole block.
				 * Start an asynchronous write and
				 * mark the buffer to indicate that
				 * it won't be needed again soon.
				 */
				flags = SM_WRITE | SM_ASYNC | SM_DONTNEED;
			} else if (isshortlink) {
				/*
				 * if it's a short symlink, then copy the
				 * the target into the disk block list, and
				 * release the page.
				 */
				if (uiomove((char *)&ip->i_db[1],
						tmpuio.uio_resid, UIO_WRITE,
						&tmpuio) == 0)
					flags = SM_WRITE | SM_ASYNC | SM_INVAL;
				else
					bzero((char *)&ip->i_db[1],
						SHORTSYMLINK);
			}
			ip->i_flag |= IUPD | ICHG;

			/*
			 * Determine if either the setuid-on-exec or setgid-
			 * on-exec bits are set in the file permission bits.
			 */
			if (((ip->i_mode & (VSGID|(VEXEC>>3))) ==
			     (VSGID|(VEXEC>>3))) || (ip->i_mode & ISUID)) {
				/*
				 * If the set[ug]id bits are set, determine if
				 * the process is privileged.  If not, clear
				 * both bits unconditionally.
				 */
				if (pm_denied(cr, P_OWNER)) {
					ip->i_mode &= ~(ISGID);
					ip->i_mode &= ~(ISUID);
				}
			}
			error = segmap_release(segkmap, base, flags);
		}

	} while (error == 0 && uio->uio_resid > 0 && n != 0);

	/*
	 * If we are doing synchronous write the only time we should
	 * not be sync'ing the ip here is if we have the sfs_stickyhack
	 * activated, the file is marked with the sticky bit and
	 * no exec bit, the file length has not been changed and
	 * no new blocks have been allocated during this write.
	 */
	if ((ioflag & IO_SYNC) != 0 &&
	    (iupdat_flag != 0 || old_blocks != ip->i_blocks)) {
		sfs_iupdat(ip, IUP_SYNC);
	}

out:
	/*
	 * If we've already done a partial-write, terminate
	 * the write but return no error.
	 */
	if (oresid != uio->uio_resid)
		error = 0;

	ip->i_flag &= ~(ISYNC | INOACC);
	return (error);
}



/* ARGSUSED */
STATIC int
sfs_ioctl(vp, cmd, arg, flag, cr, rvalp)
	struct vnode *vp;
	int cmd;
	int arg;
	int flag;
	struct cred *cr;
	int *rvalp;
{
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;
#ifdef QUOTA
	if (UFSIP(VTOI(vp))) {
		if (cmd == Q_QUOTACTL)
			return (sfs_quotactl(vp, arg, cr));
		else
			return ENOTTY;
	} else
#endif QUOTA
	return ENOTTY;
}

/* ARGSUSED */
STATIC int
sfs_setfl(vp, oflags, nflags, cr)
	struct vnode *vp;
	int oflags;
	int nflags;
	struct cred *cr;
{
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;
	return (fs_setfl(vp, oflags, nflags, cr));
}

/* ARGSUSED */
STATIC int
sfs_getattr(vp, vap, flags, cr)
	struct vnode *vp;
	register struct vattr *vap;
	int flags;
	struct cred *cr;
{
	register struct inode *ip = VTOI(vp);
	struct fs *fsp = getfs(vp->v_vfsp);
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	/*
	 * Return all the attributes.  This should be refined so
	 * that it only returns what's asked for.
	 */

	/*
	 * Copy from inode table.
	 */
	vap->va_type = vp->v_type;
	vap->va_mode = ip->i_mode & MODEMASK;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_fsid = ip->i_dev;
	vap->va_nodeid = ip->i_number;
	vap->va_nlink = ip->i_nlink;
	vap->va_size = ip->i_size;
	vap->va_vcode = ip->i_vcode;
	if (vp->v_type == VCHR || vp->v_type == VBLK || vp->v_type == VXNAM)
		vap->va_rdev = ip->i_rdev;
	else
		vap->va_rdev = 0;	/* not a b/c spec. */
	vap->va_atime.tv_sec = ip->i_atime.tv_sec;
	vap->va_atime.tv_nsec = ip->i_atime.tv_usec*1000;
	vap->va_mtime.tv_sec = ip->i_mtime.tv_sec;
	vap->va_mtime.tv_nsec = ip->i_mtime.tv_usec*1000;
	vap->va_ctime.tv_sec = ip->i_ctime.tv_sec;
	vap->va_ctime.tv_nsec = ip->i_ctime.tv_usec*1000;

	switch (ip->i_mode & IFMT) {

	case IFBLK:
		vap->va_blksize = MAXBSIZE;		/* was BLKDEV_IOSIZE */
		break;

	case IFCHR:
		vap->va_blksize = MAXBSIZE;
		break;

	default:
		vap->va_blksize = VBSIZE(vp);
		break;
	}
	vap->va_nblocks = ip->i_blocks;
	return (0);
}

STATIC int
sfs_setattr(vp, vap, flags, cr)
	register struct vnode *vp;
	register struct vattr *vap;
	int flags;
	struct cred *cr;
{
	int error = 0;
	register long int mask = vap->va_mask;
	register struct inode *ip;
	int issync = 0;
#ifdef QUOTA
	register long change;
#endif
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;
	
	/*
	 * Cannot set these attributes.
	 */
	if (mask & AT_NOSET)
		return EINVAL;

	ip = VTOI(vp);

	if (ip->i_fs->fs_ronly)
		return EROFS;

	IRWLOCK(ip);
	ILOCK(ip);

	/*
	 * Change file access modes.  Must be owner or privileged.
	 */
	if (mask & AT_MODE) {
		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}
		ip->i_mode &= IFMT;
		ip->i_mode |= vap->va_mode & ~IFMT;
		/*
		 * A non-privileged user can set the sticky bit
		 * on a directory.
		 */
		if (vp->v_type != VDIR)
			if ((ip->i_mode & ISVTX) && pm_denied(cr, P_OWNER))
				ip->i_mode &= ~ISVTX;
		if (!groupmember((uid_t)ip->i_gid, cr) && pm_denied(cr, P_OWNER))
			ip->i_mode &= ~ISGID;
		ip->i_flag |= ICHG;
	}
	if (mask & (AT_UID|AT_GID)) {
		int checksu = 0;

		/*
		 * To change file ownership, a process not running with
		 * privilege must be running as the owner of the file.
		 */
		if (cr->cr_uid != ip->i_uid)
			checksu = 1;
		else {
			if (rstchown) {
				/*
				 * "chown" is restricted.  A process not
				 * running with privilege cannot change the
				 * owner, and can only change the group to a
				 * group of which it's currently a member.
				 */
				if (((mask & AT_UID) && vap->va_uid != ip->i_uid)
				    || ((mask & AT_GID) && !groupmember(vap->va_gid, cr)))
					checksu = 1;
			}
		}

		if (checksu && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}

		if (pm_denied(cr, P_OWNER)) {
			if ((ip->i_mode & (VSGID|(VEXEC>>3))) ==
		     	    (VSGID|(VEXEC>>3)))
				ip->i_mode &= ~(ISGID);
			ip->i_mode &= ~(ISUID);
		}
		if (mask & AT_UID) {
#ifdef QUOTA
			/*
			 * Remove the blocks, and the file, from the old user's
			 * quota.
			 * Checking whether the UID is really changing or not
			 * just speeds things a little.
			 */
			if (ip->i_uid == vap->va_uid)
				change = 0;
			else
				change = ip->i_blocks;
			if (UFSIP(ip)) {
				(void) sfs_chkdq(ip, -change, 1, cr);
				(void) sfs_chkiq((struct sfs_vfs *)
			    		((ITOV(ip))->v_vfsp->vfs_data), ip,
			    		(uid_t)ip->i_uid, 1, cr);
				sfs_dqrele(ip->i_dquot, cr);
			}
#endif
			ip->i_uid = vap->va_uid;
		}
		if (mask & AT_GID)
			ip->i_gid = vap->va_gid;
		ip->i_flag |= ICHG;
#ifdef QUOTA
		if (mask & AT_UID) {
			/*
			 * Add the blocks, and the file, to the old user's
			 * quota.
			 * XXX - could this be done before setting ICHG?  It
			 * wasn't in the old code; was this necessary or was it
			 * just an accident?
			 */
			if (UFSIP(ip)) {
				ip->i_dquot = sfs_getinoquota(ip, cr);
				(void) sfs_chkdq(ip, change, 1, cr);
				(void) sfs_chkiq((struct sfs_vfs *)
					((ITOV(ip))->v_vfsp->vfs_data),
			    		(struct inode *)NULL, (uid_t)ip->i_uid,
					 1, cr);
			} else
				ip->i_dquot = NULL;
		}
#endif
	}
	/*
	 * Truncate file.  Must have write permission and not be a directory.
	 */
	if (mask & AT_SIZE) {
		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}
		if (error = sfs_iaccess(ip, IWRITE, cr))
			goto out;
		if (vp->v_type == VREG &&
		  (error = fs_vcode(vp, &ip->i_vcode)))
			goto out;
		if (error = sfs_itrunc(ip, vap->va_size, cr))
			goto out;
		issync++;
	}
	/*
	 * Change file access or modified times.
	 */
	if (mask & (AT_ATIME|AT_MTIME)) {
		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			if (flags & ATTR_UTIME)
				error = EPERM;
			else
				error = sfs_iaccess(ip, IWRITE, cr);
			if (error)
				goto out;
		}
		if (mask & AT_ATIME) {
			ip->i_atime.tv_sec = vap->va_atime.tv_sec;
			ip->i_atime.tv_usec = vap->va_atime.tv_nsec/1000;
			ip->i_flag &= ~IACC;
		}
		if (mask & AT_MTIME) {
			ip->i_mtime.tv_sec = vap->va_mtime.tv_sec;
			ip->i_mtime.tv_usec = vap->va_mtime.tv_nsec/1000;
			ip->i_ctime.tv_sec = hrestime.tv_sec;
			ip->i_ctime.tv_usec = hrestime.tv_nsec/1000;
			ip->i_flag &= ~(IUPD|ICHG);
			ip->i_flag |= IMODTIME;
		}
		ip->i_flag |= IMOD;
	}
out:
	if (!(flags & ATTR_EXEC))
		sfs_iupdat(ip, issync ? IUP_SYNC : IUP_LAZY);

	IUNLOCK(ip);
	IRWUNLOCK(ip);
	return (error);
}

/*ARGSUSED*/
STATIC int
sfs_access(vp, mode, flags, cr)
	struct vnode *vp;
	int mode;
	int flags;
	struct cred *cr;
{
	register struct inode *ip = VTOI(vp);
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	int error;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ILOCK(ip);
	error = sfs_iaccess(ip, mode, cr);
	sfs_iunlock(ip);
	return (error);
}

/* ARGSUSED */
STATIC int
sfs_readlink(vp, uiop, cr)
	struct vnode *vp;
	struct uio *uiop;
	struct cred *cr;
{
	register struct inode *ip;
	register int error;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	if (vp->v_type != VLNK)
		return EINVAL;
	ip = VTOI(vp);
	error = sfs_readi(ip, uiop, 0, cr);
	ACC_TIMES(ip);
	return (error);
}

/* ARGSUSED */
STATIC int
sfs_fsync(vp, cr)
	struct vnode *vp;
	struct cred *cr;
{
	register struct inode *ip = VTOI(vp);
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	int error;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ILOCK(ip);
	error = sfs_syncip(ip, 0, IUP_SYNC);	/* Do synchronous writes */
	ITIMES(ip);			/* XXX: is this necessary ??? */
	sfs_iunlock(ip);
	return (error);
}

/*ARGSUSED*/
STATIC void
sfs_inactive(vp, cr)
	struct vnode *vp;
	struct cred *cr;
{

	sfs_iinactive(VTOI(vp), cr);
}

/*
 * Unix file system operations having to do with directory manipulation.
 */
/* ARGSUSED */
STATIC int
sfs_lookup(dvp, nm, vpp, pnp, flags, rdir, cr)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct pathname *pnp;
	int flags;
	struct vnode *rdir;
	struct cred *cr;
{
	register struct inode *ip;
	struct inode *xip;
	register int error;
	struct vfs *vfsp = dvp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	/*
	 * Null component name is a synonym for directory being searched.
	 */
	if (*nm == '\0') {
		VN_HOLD(dvp);
		*vpp = dvp;
		return 0;
	}

	ip = VTOI(dvp);
	error = sfs_dirlook(ip, nm, &xip, cr);
	ITIMES(ip);
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		if ((ip->i_mode & ISVTX) && !(ip->i_mode & (IEXEC | IFDIR)) &&
		    sfs_stickyhack) {
			(*vpp)->v_flag |= VISSWAP;
		}
		ITIMES(ip);
		sfs_iunlock(ip);
		/*
		 * If vnode is a device return special vnode instead.
		 */
		if (ISVDEV((*vpp)->v_type)) {
			struct vnode *newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type,
			    cr);
			VN_RELE(*vpp);
			if (newvp == NULL)
				error = ENOSYS;
			else
				*vpp = newvp;
		}
	}
	return error;
}

STATIC int
sfs_create(dvp, name, vap, excl, mode, vpp, cr)
	struct vnode *dvp;
	char *name;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct vnode **vpp;
	struct cred *cr;
{
	register int error;
	register struct inode *ip = VTOI(dvp);
	struct inode *xip;
	struct vfs *vfsp = dvp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	/* must be privileged to set sticky bit */
	if ((vap->va_mode & VSVTX) && pm_denied(cr, P_OWNER))
		vap->va_mode &= ~VSVTX;	

	if (*name == '\0') {
		/*
		 * Null component name refers to the directory itself.
		 */
		VN_HOLD(dvp);
		ILOCK(ip);
		ITIMES(ip);
		error = EEXIST;
	} else {
		if (ISVDEV(vap->va_type)) {
			/*
			 * Try to catch any specfs problems before writing
			 * directory.
			 */
			if (error = specpreval(vap->va_type, vap->va_rdev, cr))
				return error;
		}
		xip = NULL;
		error = sfs_direnter(ip, name, DE_CREATE, (struct inode *) 0,
		  (struct inode *) 0, vap, &xip, cr);
		ITIMES(ip);
		ip = xip;
	}

	/*
	 * If the file already exists and this is a non-exclusive create,
	 * check permissions and allow access for non-directories.
	 * Read-only create of an existing directory is also allowed.
	 * We fail an exclusive create of anything which already exists.
	 */
	if (error == EEXIST) {
		struct vnode *vp = ITOV(ip);
		/*
		 * MAC write checks are necessary in sfs dependent
		 * code because the time between lookup and VOP_CREATE
		 * at independent level is rather long.  This
		 * particular check is here because the "mode"
		 * argument is not passed along to sfs_direnter().
		 * Other MAC checks are performed in sfs_direnter().
		 */
		if (vp->v_type != VCHR
		&&  vp->v_type != VBLK
		&&  MAC_VACCESS(vp, mode, cr))
			error = EACCES;
		else if (excl == NONEXCL) {
			if (((ip->i_mode & IFMT) == IFDIR) && (mode & IWRITE))
				error = EISDIR;
			else if (mode)
				error = sfs_iaccess(ip, mode, cr);
			else
				error = 0;
		}
		if (!error && (ip->i_mode & IFMT) == IFREG
		    && (vap->va_mask & AT_SIZE) && vap->va_size == 0) {
			/*
			 * Truncate regular files, if requested by caller.
			 *
			 * Need to acquire IRWLOCK before truncation
			 * to avoid race; IRWLOCK must be acquired
			 * before locking the inode to avoid deadlock.
			 */
			IUNLOCK(ip);
			IRWLOCK(ip);
			ILOCK(ip);

			error = sfs_itrunc(ip, (u_long)0, cr);

			IRWUNLOCK(ip);
		}
		if (error)
			sfs_iput(ip);
	}
	if (error)
		return error;
	*vpp = ITOV(ip);
	ITIMES(ip);
	if (((ip->i_mode & IFMT) == IFREG) && (vap->va_mask & AT_SIZE))
		error = fs_vcode(ITOV(ip), &ip->i_vcode);
	sfs_iunlock(ip);
	/*
	 * If vnode is a device return special vnode instead.
	 */
	if (!error && ISVDEV((*vpp)->v_type)) {
		struct vnode *newvp;

		newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cr);
		VN_RELE(*vpp);
		if (newvp == NULL) {
			/*
			 * Note in this case, directory entry isn't cleaned up.
			 * That's why we try to catch specfs problems earlier
			 * with call to specpreval().
			 */
			error = ENOSYS;
		}
		*vpp = newvp;
	}

	return (error);
}

/*ARGSUSED*/
STATIC int
sfs_remove(vp, nm, cr)
	struct vnode *vp;
	char *nm;
	struct cred *cr;
{
	register struct inode *ip = VTOI(vp);
	register int error;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	error = sfs_dirremove(ip, nm, (struct inode *)0, (struct vnode *)0,
	    DR_REMOVE, cr);
	ITIMES(ip);
	return error;
}

/*
 * Link a file.  Links to diretories are no longer allowed.
 */
STATIC int
sfs_link(tdvp, svp, tnm, cr)
	register struct vnode *tdvp;
	struct vnode *svp;
	char *tnm;
	struct cred *cr;
{
	register struct inode *sip;
	register struct inode *tdp;
	register int error;
	struct vnode *realvp;
	struct vfs *vfsp;
	struct sfs_vfs *sfs_vfsp;

        MAC_GIVESVAL(svp->v_op->vop_link, sfs_link);

	vfsp = svp->v_vfsp;
	sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	if (VOP_REALVP(svp, &realvp) == 0)
		svp = realvp;
	if (svp->v_type == VDIR)
		return EPERM;
	sip = VTOI(svp);
	tdp = VTOI(tdvp);
	error = sfs_direnter(tdp, tnm, DE_LINK, (struct inode *) 0,
	    sip, (struct vattr *)0, (struct inode **)0, cr);
	ITIMES(sip);
	ITIMES(tdp);
	return error;
}

/*
 * Rename a file or directory.
 * We are given the vnode and entry string of the source and the
 * vnode and entry string of the place we want to move the source
 * to (the target). The essential operation is:
 *	unlink(target);
 *	link(source, target);
 *	unlink(source);
 * but "atomically".  Can't do full commit without saving state in
 * the inode on disk, which isn't feasible at this time.  Best we
 * can do is always guarantee that the TARGET exists.
 */
/*ARGSUSED*/
STATIC int
sfs_rename(sdvp, snm, tdvp, tnm, cr)
	struct vnode *sdvp;		/* old (source) parent vnode */
	char *snm;			/* old (source) entry name */
	struct vnode *tdvp;		/* new (target) parent vnode */
	char *tnm;			/* new (target) entry name */
	struct cred *cr;
{
	struct inode *sip;		/* source inode */
	register struct inode *sdp;	/* old (source) parent inode */
	register struct inode *tdp;	/* new (target) parent inode */
	register int error;
	struct vfs *vfsp;
	struct sfs_vfs *sfs_vfsp;

        MAC_GIVESVAL(tdvp->v_op->vop_rename, sfs_rename);

	vfsp = sdvp->v_vfsp;
	sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);
	/*
	 * Look up inode of file we're supposed to rename.
	 */
	if (error = sfs_dirlook(sdp, snm, &sip, cr))
		return error;
	sfs_iunlock(sip);	/* unlock inode (it's held) */
	/*
	 * Make sure we can delete the source entry.  This requires
	 * write permission on the containing directory.  If that
	 * directory is "sticky" it further requires (except for a
	 * privileged user) that the user own the directory or the source 
	 * entry, or else have permission to write the source entry.
	 */
	if ((error = sfs_iaccess(sdp, IWRITE, cr)) != 0
	    || ((sdp->i_mode & ISVTX) && cr->cr_uid != sdp->i_uid
	    && cr->cr_uid != sip->i_uid && pm_denied(cr, P_OWNER)
	    && (error = sfs_iaccess(sip, IWRITE, cr)) != 0)) {
		sfs_irele(sip);
		return error;
	}

	/*
	 * Check for renaming '.' or '..' or alias of '.'
	 */
	if (strcmp(snm, ".") == 0 || strcmp(snm, "..") == 0 || sdp == sip) {
		error = EINVAL;
		goto out;
	}

	/*
	 * Link source to the target.
	 */
	if (error = sfs_direnter(tdp, tnm, DE_RENAME, sdp, sip,
	    (struct vattr *)0, (struct inode **)0, cr)) {
		/*
		 * ESAME isn't really an error; it indicates that the
		 * operation should not be done because the source and target
		 * are the same file, but that no error should be reported.
		 */
		if (error == ESAME)
			error = 0;
		goto out;
	}

	/*
	 * Unlink the source.
	 * Remove the source entry.  sfs_dirremove() checks that the entry
	 * still reflects sip, and returns an error if it doesn't.
	 * If the entry has changed just forget about it.  Release
	 * the source inode.
	 */
	if ((error = sfs_dirremove(sdp, snm, sip, (struct vnode *)0,
	    DR_RENAME, cr)) == ENOENT)
		error = 0;

out:
	ITIMES(sdp);
	ITIMES(tdp);
	sfs_irele(sip);
	return (error);
}

/*ARGSUSED*/
STATIC int
sfs_mkdir(dvp, dirname, vap, vpp, cr)
	struct vnode *dvp;
	char *dirname;
	register struct vattr *vap;
	struct vnode **vpp;
	struct cred *cr;
{
	register struct inode *ip;
	struct inode *xip;
	register int error;
	struct vfs *vfsp = dvp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	ip = VTOI(dvp);
	error = sfs_direnter(ip, dirname, DE_MKDIR, (struct inode *) 0,
	    (struct inode *) 0, vap, &xip, cr);
	ITIMES(ip);
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		ITIMES(ip);
		sfs_iunlock(ip);
	} else if (error == EEXIST)
		sfs_iput(xip);
	return (error);
}

/*ARGSUSED*/
STATIC int
sfs_rmdir(vp, nm, cdir, cr)
	struct vnode *vp;
	char *nm;
	struct vnode *cdir;
	struct cred *cr;
{
	register struct inode *ip = VTOI(vp);
	register int error;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	error = sfs_dirremove(ip, nm, (struct inode *)0, cdir, DR_RMDIR, cr);
	ITIMES(ip);
	return error;
}

/* ARGSUSED */
STATIC int
sfs_readdir(vp, uiop, fcr, eofp)
	struct vnode *vp;
	struct uio *uiop;
	struct cred *fcr;
	int *eofp;
{
	struct cred *cr = VCURRENTCRED(fcr);	/* refer to vnode.h */
	register struct iovec *iovp;
	register struct inode *ip;
	register struct direct *idp;
	register struct dirent *odp;
	register u_int offset;
	register int incount = 0;
	register int outcount = 0;
	register u_int bytes_wanted, total_bytes_wanted;
	caddr_t outbuf;
	size_t bufsize;
	int error = 0;
	struct fbuf *fbp;
	int fastalloc;
	static caddr_t dirbufp;
	int direntsz;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ip = VTOI(vp);
	ASSERT((ip)->i_flag & IRWLOCKED);

	iovp = uiop->uio_iov;

	/*
	* Make sure that total_bytes_wanted is at least one
	* dirent struct in size. If it is less than this then
	* the system would hang as ufs_readdir() would get
	* into a loop of fbread() and fbrelse() calls.
	*/
	if ((total_bytes_wanted = iovp->iov_len) < sizeof(struct dirent))
		return(EINVAL);

	/* Force offset to be valid (to guard against bogus lseek() values) */
	offset = uiop->uio_offset & ~(DIRBLKSIZ - 1);

	/* Quit if at end of file */
	if (offset >= ip->i_size) {
		if (eofp)
			*eofp = 1;
		return (0);
	}

	/*
	 * Get space to change directory entries into fs independent format.
	 * Do fast alloc for the most commonly used-request size (filesystem
	 * block size).
	 */
	fastalloc = (total_bytes_wanted == MAXBSIZE);
	bufsize = total_bytes_wanted + sizeof (struct dirent) + SFS_MAXNAMLEN;
	if (fastalloc)
		outbuf = (caddr_t) kmem_fast_alloc(&dirbufp, bufsize, 1 ,
			KM_SLEEP);
	else
		outbuf = (caddr_t) kmem_alloc(bufsize, KM_SLEEP);
	odp = (struct dirent *)outbuf;
	direntsz = (char *) odp->d_name - (char *) odp;

        ILOCK(ip);
nextblk:
	bytes_wanted = total_bytes_wanted;

	/* Truncate request to file size */
	if (offset + bytes_wanted > ip->i_size)
		bytes_wanted = ip->i_size - offset;

	/* Comply with MAXBSIZE boundary restrictions of fbread() */
	if ((offset & MAXBOFFSET) + bytes_wanted > MAXBSIZE)
		bytes_wanted = MAXBSIZE - (offset & MAXBOFFSET);

	/* Read in the next chunk */
	if (error = fbread(vp, (long)offset, bytes_wanted, S_OTHER, &fbp))
		goto out;

	if (WRITEALLOWED(vp, cr))
		ip->i_flag |= IACC;

	incount = 0;
	idp = (struct direct *)fbp->fb_addr;
	if (idp->d_ino == 0 && idp->d_reclen == 0 &&
		idp->d_namlen == 0) {
		cmn_err(CE_WARN, "sfs_readir: bad dir, inumber = %d\n", ip->i_number);
		fbrelse(fbp, S_OTHER);
		error = ENXIO;
		goto out;
	}	
	/* Transform to file-system independent format */
	while (incount < bytes_wanted) {
		/* Skip to requested offset and skip empty entries */
		if (idp->d_ino != 0 && offset >= uiop->uio_offset) {
			odp->d_ino = idp->d_ino;
			odp->d_reclen = (direntsz + idp->d_namlen + 1 + (NBPW-1)) & ~(NBPW-1);
			odp->d_off = offset + idp->d_reclen;
			strcpy(odp->d_name, idp->d_name);
			outcount += odp->d_reclen;
			/* Got as many bytes as requested, quit */
			if (outcount > total_bytes_wanted) {
				outcount -= odp->d_reclen;
				/*
				 * If you call getdents() with a small number
				 * for the nbyte argument so that there are
				 * directory entries larger then nbyte bytes
				 * then sfs_readdir() ends in an infinite
				 * loop and the process hangs.
				 */
				if(outcount == 0){
					fbrelse(fbp,S_OTHER);
					error = EINVAL;
					goto out;
				}
				break;
			}
			odp = (struct dirent *)((int)odp + odp->d_reclen);
		}
		incount += idp->d_reclen;
		offset += idp->d_reclen;
		idp = (struct direct *)((int)idp + idp->d_reclen);
	}
	/* Release the chunk */
	fbrelse(fbp, S_OTHER);

	/* Read whole block, but got no entries, read another if not eof */
	if (offset < ip->i_size && !outcount)
		goto nextblk;

	/* Copy out the entry data */
	if (error = uiomove(outbuf, (long)outcount, UIO_READ, uiop))
		goto out;

	uiop->uio_offset = offset;
out:
	ITIMES(ip);
	IUNLOCK(ip);
	if (fastalloc)
		kmem_fast_free(&dirbufp, outbuf);
	else
		kmem_free((void *)outbuf, bufsize);
	if (eofp && error == 0)
		*eofp = (uiop->uio_offset >= ip->i_size);
	return (error);
}

/*ARGSUSED*/
STATIC int
sfs_symlink(dvp, linkname, vap, target, cr)
	register struct vnode *dvp;	/* ptr to parent dir vnode */
	char *linkname;			/* name of symbolic link */
	struct vattr *vap;		/* attributes */
	char *target;			/* target path */
	struct cred *cr;		/* user credentials */
{
	struct inode *ip, *dip = VTOI(dvp);
	register int error;
	struct vfs *vfsp = dvp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ip = (struct inode *)0;
	vap->va_type = VLNK;
	vap->va_rdev = 0;
	if ((error = sfs_direnter(dip, linkname, DE_CREATE,
	    (struct inode *)0, (struct inode *)0, vap, &ip, cr)) == 0) {
		error = sfs_rdwri(UIO_WRITE, ip, target, (int)strlen(target),
		    (off_t)0, UIO_SYSSPACE, (int *)0, cr);
		sfs_iput(ip);
	} else if (error == EEXIST)
		sfs_iput(ip);
	ITIMES(VTOI(dvp));
	return (error);
}

/*
 * SFS specific routine used to do sfs io.
 */
int
sfs_rdwri(rw, ip, base, len, offset, seg, aresid, cr)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	off_t offset;
	enum uio_seg seg;
	int *aresid;
	struct cred *cr;
{
	struct uio auio;
	struct iovec aiov;
	register int error;

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = (short)seg;
	auio.uio_resid = len;
	if (rw == UIO_WRITE) {
		auio.uio_fmode = FWRITE;
		error = sfs_writei(ip, &auio, 0, cr);
	} else {
		auio.uio_fmode = FREAD;
		error = sfs_readi(ip, &auio, 0, cr);
	}

	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
		error = EIO;
	}
	return (error);
}


STATIC int
sfs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct ufid *ufid;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ufid = (struct ufid *)kmem_zalloc(sizeof (struct ufid), KM_SLEEP);
	ufid->ufid_len = sizeof (struct ufid) - sizeof (ushort);
	ufid->ufid_ino = VTOI(vp)->i_number;
	ufid->ufid_gen = VTOI(vp)->i_gen;
	*fidpp = (struct fid *)ufid;
	return 0;
}

STATIC void
sfs_rwlock(vp)
	struct vnode *vp;
{
	IRWLOCK(VTOI(vp));
}

STATIC void
sfs_rwunlock(vp)
	struct vnode *vp;
{
	IRWUNLOCK(VTOI(vp));
}
			
/* ARGSUSED */
STATIC int
sfs_seek(vp, ooff, noffp)
	struct vnode *vp;
	off_t ooff;
	off_t *noffp;
{
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;
	return *noffp < 0 ? EINVAL : 0;
}
			
/* ARGSUSED */
STATIC int
sfs_frlock(vp, cmd, bfp, flag, offset, cr)
	register struct vnode *vp;
	int cmd;
	struct flock *bfp;
	int flag;
	off_t offset;
	cred_t *cr;
{
	register struct inode *ip = VTOI(vp);
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;
 
	/*
	 * If file is being mapped, disallow frlock.
	 */
	if (ip->i_mapcnt > 0 && MANDLOCK(vp, ip->i_mode))
		return EAGAIN;
 
	return fs_frlock(vp, cmd, bfp, flag, offset, cr);
}

/*
 * Free storage space associated with the specified inode.  The portion
 * to be freed is specified by lp->l_start and lp->l_len (already
 * normalized to a "whence" of 0).
 *
 * This is an experimental facility whose continued existence is not
 * guaranteed.  Currently, we only support the special case
 * of l_len == 0, meaning free to end of file.
 *
 * Blocks are freed in reverse order.  This FILO algorithm will tend to
 * maintain a contiguous free list much longer than FIFO.
 * See also sfs_itrunc() in sfs_inode.c.
 */
STATIC int
sfs_freesp(vp, lp, flag, cr)
	register struct vnode *vp;
	register struct flock *lp;
	int flag;
	struct cred *cr;
{
	register int i;
	register struct inode *ip = VTOI(vp);
	int error;
	
	ASSERT(vp->v_type == VREG);
	ASSERT(lp->l_start >= 0);	/* checked by convoff */

	if (lp->l_len != 0)
		return EINVAL;
	if (ip->i_size == lp->l_start)
		return 0;

	/*
	 * Check if there is any active mandatory lock on the
	 * range that will be truncated/expanded.
	 */

	if (MANDLOCK(vp, ip->i_mode)) {
		int save_start;

		save_start = lp->l_start;

		if (ip->i_size < lp->l_start) {
			/*
			 * "Truncate up" case: need to make sure there
			 * is no lock beyond current end-of-file. To
			 * do so, we need to set l_start to the size
			 * of the file temporarily.
			 */
			lp->l_start = ip->i_size;
		}
		lp->l_type = F_WRLCK;
		lp->l_sysid = u.u_procp->p_sysid;
		lp->l_pid = u.u_procp->p_epid;
		i = (flag & (FNDELAY|FNONBLOCK)) ? 0 : SLPFLCK;
		if ((i = reclock(vp, lp, i, 0, lp->l_start)) != 0
		  || lp->l_type != F_UNLCK)
			return i ? i : EAGAIN;

		lp->l_start = save_start;
	}

	IRWLOCK(ip);
	ILOCK(ip);

 	if (vp->v_type == VREG && (error = fs_vcode(vp, &ip->i_vcode))) {
 		IUNLOCK(ip);
		IRWUNLOCK(ip);
 		return error;
 	}

	error = sfs_itrunc(ip, lp->l_start, cr);
	IUNLOCK(ip);
	IRWUNLOCK(ip);
	return error;
}

/* ARGSUSED */
STATIC int
sfs_space(vp, cmd, bfp, flag, offset, cr)
	struct vnode *vp;
	int cmd;
	struct flock *bfp;
	int flag;
	off_t offset;
	struct cred *cr;
{
	int error;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	if (cmd != F_FREESP)
		return EINVAL;
	if ((error = convoff(vp, bfp, 0, offset)) == 0)
		error = sfs_freesp(vp, bfp, flag, cr);
	return error;
}

STATIC int sfs_ra = 1;
STATIC int sfs_lostpage;	/* number of times we lost original page */

/*
 * Called from pvn_getpages or sfs_getpage to get a particular page.
 * When we are called the inode is already locked.
 *
 *	If rw == S_WRITE and block is not allocated, need to alloc block.
 *	If ppp == NULL, async I/O is requested.
 *
 * bsize must be at least PAGESIZE/2.  With bsize == PAGESIZE/2,
 * two reads are done to get the data in a page, and read ahead
 * is disgarded.
 */
/* ARGSUSED */
STATIC int
sfs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cr)
	struct vnode *vp;
	register u_int off;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cr;
{
	register struct inode *ip;
	register struct fs *fs;
	register int bsize;
	u_int xlen;
	struct buf *bp, *bp2;
	struct vnode *devvp;
	struct page *pp, *pp2, **ppp, *pagefound;
	daddr_t lbn, bn, bn2;
	u_int io_off, io_len;
	u_int lbnoff, blksz;
	int err, nio, do2ndread, pgoff;
	dev_t dev;
	
	ip = VTOI(vp);
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	devvp = ip->i_devvp;
	dev = devvp->v_rdev;
        ASSERT((ip->i_flag & ILOCKED) && (ip->i_owner == curproc->p_slot));

reread:
	err = 0;
	bp = NULL;
	bp2 = NULL;
	pagefound = NULL;
	pgoff = 0;
	lbn = lblkno(fs, off);
	lbnoff = off & fs->fs_bmask;
	if (pl != NULL)
		pl[0] = NULL;

	err = sfs_bmap(ip, lbn, &bn, &bn2, (int)blksize(fs, ip, lbn), rw, 0, cr);
	if (err)
		goto out;

	if (bn == SFS_HOLE && protp != NULL)
		*protp &= ~PROT_WRITE;

	/*
	 * Determine if 2nd read is to be done.
	 * Do 2nd read on read-ahead or when PAGESIZE > bsize.
	 * Of course, this only makes sense if there is file data
	 * to read.
	 */
	if (lbnoff + fs->fs_bsize < ip->i_size) {
		if (PAGESIZE > bsize) {
			if (bsize == PAGESIZE / 2) {
				nio = 2;
				do2ndread = 1;
			} else
				cmn_err(CE_PANIC, "sfs_getapage bad bsize");
		} else {
			nio = 1;
			if (sfs_ra && ip->i_nextr == off && bn2 != SFS_HOLE)
				do2ndread = 1;
			else
				do2ndread = 0;
		}
	} else {
		nio = 1;
		do2ndread = 0;
	}

again:
	if ((pagefound = page_find(vp, off)) == NULL) {
		/*
		 * Page doesn't exist, need to create it.
		 * First compute size we really want to get.
		 */
		if (lbn < NDADDR) {
			/*
			 * Direct block, use standard blksize macro.
			 */
			blksz = blksize(fs, ip, lbn);
		} else {
			/*
			 * Indirect block, round up to smaller of
			 * page boundary or file system block size.
			 */
			blksz = MIN(roundup(ip->i_size, PAGESIZE) - lbnoff,
			    bsize);
		}
		if (bn == SFS_HOLE || off >= lbnoff + blksz) {
			/*
			 * Block for this page is not allocated or the offset
			 * is beyond the current allocation size (from
			 * sfs_bmap) and the page was not found.  If we need
			 * a page, allocate and return a zero page.
			 */
			if (pl != NULL) {
				pp = rm_allocpage(seg, addr, PAGESIZE, P_CANWAIT);
				if (page_enter(pp, vp, off)) {
					PAGE_RELE(pp);
					goto again;
				}
				pagezero(pp, 0, PAGESIZE);
				page_unlock(pp);
				pp->p_nio = nio;
				if (nio > 1) {
					pp->p_intrans = pp->p_pagein= 1;
					PAGE_HOLD(pp);
				}
				pl[0] = pp;
				pl[1] = NULL;
			}
			if (protp != NULL)
				*protp &= ~PROT_WRITE;
		} else {
			/*
			 * Need to really do disk I/O to get the page(s).
			 */
			pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
			    lbnoff, blksz, 0);
			/*
			 * If someone else got there first, try again.
			 */
			if (pp == NULL)
				goto again;

			if (pl != NULL) {
				register int sz;

				if (plsz >= io_len) {
					/*
					 * Everything fits, set up to load
					 * up and hold all the pages.
					 */
					pp2 = pp;
					sz = io_len;
				} else {
					/*
					 * Set up to load plsz worth
					 * starting at the needed page.
					 */
					for (pp2 = pp; pp2->p_offset != off;
					    pp2 = pp2->p_next) {
						ASSERT(pp2->p_next->p_offset !=
						    pp->p_offset);
					}
					sz = plsz;
				}

				ppp = pl;
				do {
					PAGE_HOLD(pp2);
					*ppp++ = pp2;
					pp2 = pp2->p_next;
					sz -= PAGESIZE;
				} while (sz > 0);
				*ppp = NULL;		/* terminate list */
			}

			if (nio == 2)
				pp->p_nio = 2;

			bp = pageio_setup(pp, io_len, devvp, pl == NULL ?
			    (B_ASYNC | B_READ) : B_READ);

			bp->b_edev = dev;
			bp->b_dev = cmpdev(dev);
			bp->b_blkno = fsbtodb(fs, bn) +
			    btodb(blkoff(fs, io_off));

			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now.
			 */
			xlen = io_len & PAGEOFFSET;
			if (xlen != 0)
				pagezero(pp->p_prev, xlen, PAGESIZE - xlen);

			(*bdevsw[getmajor(dev)].d_strategy)(bp);

			ip->i_nextr = io_off + io_len;
#ifdef notneeded
			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;	/* count as `read' */
#endif
			vminfo.v_pgin++;
			vminfo.v_pgpgin += btopr(io_len);
			/*
			 * Mark the level of the process actually
			 * faulting in the page.  Anonymous pages
			 * are skipped.
			 *
			 * Note that the level of the process is
			 * retrieved from the u's cred since VM
			 * does not pass in the process cred when
			 * calling VOP_GETPAGE().  This should be
			 * changed accordingly when VM provides
			 * this capability.
			 */
			if (mac_installed && !IS_SWAPVP(vp))
				pp->p_lid = u.u_cred->cr_lid;
		}
	}

	lbn++;
	lbnoff += fs->fs_bsize;


	/*
	 * Cannot do the second read when the PAGE is found and  PAGESIZE > bsize
	 * The page pointer array (pp) is setup only when the page is not found.
	 * This pointer is referenced in the do2ndread block.
	 */
	if (do2ndread && !((pagefound != NULL) && (PAGESIZE > bsize))) {

		addr_t addr2;

		addr2 = addr + (lbnoff - off);

		if (lbn < NDADDR) {
			/*
			 * Direct block, use standard blksize macro.
			 */
			blksz = blksize(fs, ip, lbn);
		} else {
			/*
			 * Indirect block, round up to smaller of
			 * page boundary or file system block size.
			 */
			blksz = MIN(roundup(ip->i_size, PAGESIZE) - lbnoff,
			    bsize);
		}

		/*
		 * Either doing read-ahead (nio == 1)
		 * or second read for page (nio == 2).
		 */
		if (nio == 1) {
			/*
			 * If addr is now in a different seg,
			 * don't bother with read-ahead.
			 */
			if (addr2 >= seg->s_base + seg->s_size)
				pp2 = NULL;
			else {
				pp2 = pvn_kluster(vp, lbnoff, seg, addr2,
				    &io_off, &io_len, lbnoff, blksz, 1);
			}
			pgoff = 0;
		} else {
			if (bn2 == SFS_HOLE || rw == S_WRITE) {
				/*
				 * Read-ahead number didn't suffice,
				 * retry as primary block now.  Note
				 * that if we are doing allocation
				 * with nio == 2, we will force both
				 * logical blocks to be allocated.
				 */
				err = sfs_bmap(ip, lbn, &bn2, (daddr_t *)0,
				    (int)blksize(fs, ip, lbn), rw, 0, cr);
				if (err)
					goto out;
				if (bn2 == SFS_HOLE) {
					if (protp != NULL)
						*protp &= ~PROT_WRITE;
					goto out;
				}
			}
			pp2 = pp;
			io_len = blksz;
			io_off = off;
			pgoff = bsize;
		}

		if (pp2 != NULL) {
			bp2 = pageio_setup(pp2, io_len, devvp,
			    (pl == NULL || nio == 1) ?
			    (B_ASYNC | B_READ) : B_READ);

			bp2->b_edev = dev;
			bp2->b_dev = cmpdev(dev);
			bp2->b_blkno = fsbtodb(fs, bn2) +
			    btodb(blkoff(fs, io_off));
			bp2->b_un.b_addr = (caddr_t)pgoff;
			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now.
			 */
			xlen = (io_len + pgoff) & PAGEOFFSET;
			if (xlen != 0)
				pagezero(pp2->p_prev, xlen, PAGESIZE - xlen);

			(*bdevsw[getmajor(dev)].d_strategy)(bp2);
			vminfo.v_pgin++;
			vminfo.v_pgpgin += btopr(io_len);
			/*
			 * Mark the level of the process actually
			 * faulting in the page.  Anonymous pages
			 * are skipped.
			 *
			 * Note that the level of the process is
			 * retrieved from the u's cred since VM
			 * does not pass in the process cred when
			 * calling VOP_GETPAGE().  This should be
			 * changed accordingly when VM provides
			 * this capability.
			 */
			if (mac_installed && !IS_SWAPVP(vp))
				pp2->p_lid = u.u_cred->cr_lid;
		}
	}

out:
	if (bp != NULL && pl != NULL) {
		if (err == 0)
			err = biowait(bp);
		else
			(void) biowait(bp);
		pageio_done(bp);
		if (nio == 2 && bp2 != NULL) {
			if (err == 0)
				err = biowait(bp2);
			else
				(void) biowait(bp2);
			pageio_done(bp2);
		}
	} else if (pagefound != NULL) {
		int s;

		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if (pagefound->p_vnode == vp && pagefound->p_offset == off) {
			/*
			 * If the page is still intransit or if
			 * it is on the free list call page_lookup
			 * to try and wait for / reclaim the page.
			 */
			if (pagefound->p_intrans || pagefound->p_free)
				pagefound = page_lookup(vp, off);
		}
		(void) splx(s);
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			sfs_lostpage++;
			goto reread;
		}
		if (pl != NULL) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			ip->i_nextr = off + PAGESIZE;
		}
	}

	if (err && pl != NULL) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

	return (err);
}

/*
 * Return all the pages from [off..off+len) in given file
 */
STATIC int
sfs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cr)
	struct vnode *vp;
	u_int off;
	u_int len;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cr;
{
	struct inode *ip = VTOI(vp);
	int err;

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	ILOCK(ip);

	/*
	 * This check for beyond EOF allows the request to extend up to
	 * the page boundary following the EOF.	 Strictly speaking,
	 * it should be (off + len > (ip->i_size + PAGEOFFSET) % PAGESIZE),
	 * but in practice, this is equivalent and faster.
	 *
	 * Also, since we may be called as a side effect of a bmap or
	 * dirsearch() using fbread() when the blocks might be being
	 * allocated and the size has not yet been up'ed.  In this case
	 * we disable the check and always allow the getpage to go through
	 * if the segment is seg_map, since we want to be able to return
	 * zeroed pages if bmap indicates a hole in the non-write case.
	 * For ufs, we also might have to read some frags from the disk
	 * into a page if we are extending the number of frags for a given
	 * lbn in sfs_bmap().
	 */
	if (off + len > ip->i_size + PAGEOFFSET &&
                        !(seg == segkmap && rw == S_OTHER)) {
		IUNLOCK(ip);
		return (EFAULT);	/* beyond EOF */
	}
	if (protp != NULL)
		*protp = PROT_ALL;

	if (len <= PAGESIZE)
		err = sfs_getapage(vp, off, protp, pl, plsz, seg, addr,
		    rw, cr);
	else
		err = pvn_getpages(sfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cr);

	/*
	 * If the inode is not already marked for IACC (in sfs_readi() for read)
	 * and the inode is not marked for no access time update (in sfs_writei()
	 * for write) then update the inode access time and mod time now.
	 */
	if ((ip->i_flag & (IACC | INOACC)) == 0
	&&  WRITEALLOWED(vp, cr)) {
		if (rw != S_OTHER)
			ip->i_flag |= IACC;
		if (rw == S_WRITE)
			ip->i_flag |= IUPD;
		ITIMES(ip);
	}

	IUNLOCK(ip);

	return (err);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}
 */
STATIC int
sfs_writelbn(ip, bn, pp, len, pgoff, flags)
	register struct inode *ip;
	daddr_t bn;
	struct page *pp;
	u_int len;
	u_int pgoff;
	int flags;
{
	struct buf *bp;
	int err;

	if ((bp = pageio_setup(pp, len, ip->i_devvp, B_WRITE | flags))
	    == NULL) {
		pvn_fail(pp, B_WRITE | flags);
		return (ENOMEM);
	}

	bp->b_edev = ip->i_dev;
	bp->b_dev = cmpdev(ip->i_dev);
	bp->b_blkno = bn;
	bp->b_un.b_addr = (caddr_t)pgoff;
	(*bdevsw[getmajor(ip->i_dev)].d_strategy)(bp);

	/*
	 * If async, assume that pvn_done will handle the pages
	 * when I/O is done.
	 */
	if (flags & B_ASYNC)
		return (0);

	err = biowait(bp);
	pageio_done(bp);

	return (err);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_FORCE}
 * If len == 0, do from off to EOF.
 *
 * The normal cases should be len == 0 & off == 0 (entire vp list),
 * len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 * (from pageout).
 *
 * Note that for sfs it is possible to have dirty pages beyond
 * roundup(ip->i_size, PAGESIZE).  This can happen the file
 * length is long enough to involve indirect blocks (which are
 * always fs->fs_bsize'd) and PAGESIZE < bsize while the length
 * is such that roundup(blkoff(fs, ip->i_size), PAGESIZE) < bsize.
 *
 * bsize must be at least PAGESIZE/2, as there is no provision
 * by this routine to write more than two blocks in a page.
 */
/* ARGSUSED */
STATIC int
sfs_putpage(vp, off, len, flags, cr)
	register struct vnode *vp;
	u_int off, len;
	int flags;
	struct cred *cr;
{
	register struct inode *ip;
	register struct page *pp;
	register struct fs *fs;
	struct page *dirty, *io_list;
	register u_int io_off, io_len;
	daddr_t lbn, bn;
	u_int lbn_off;
	int bsize;
	int vpcount;
	int err = 0;
	int do2ndwrite;

	ip = VTOI(vp);

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	/*
	 * The following check is just for performance
	 * and therefore doesn't need to be foolproof.
	 * The subsequent code will gracefully do nothing
	 * in any case.
	 */
	if (vp->v_pages == NULL || off >= ip->i_size)
		return (0);

	/*
	 * Return if an iinactive is already in progress on this
	 * inode since the page will be written out by that process.
	 */
	if ((ip->i_flag & ILOCKED) && (ip->i_owner != curproc->p_slot) &&
            (ip->i_flag & IINACTIVE))
		return (EAGAIN);

	vpcount = vp->v_count;
	VN_HOLD(vp);
	fs = ip->i_fs;

again:
	if (len == 0) {

		/*
		 * Search the entire vp list for pages >= off
		 */
		pvn_vplist_dirty(vp, off, flags);
		/*
		 * We have just sync'ed back all the pages
		 * on the inode; turn off the IMODTIME flag.
		 */
		if (off == 0)
			ip->i_flag &= ~IMODTIME;

		goto vcount_chk;
	} else {
		/*
		 * Do a range from [off...off + len) via page_find.
		 * We set limits so that we kluster to bsize boundaries.
		 */
		if (off >= ip->i_size)
			dirty = NULL;
		else {
			u_int fsize, eoff;

			/*
			 * Use MAXBSIZE rounding to get indirect block pages
			 * which might beyond roundup(ip->i_size, PAGESIZE);
			 */
			fsize = (ip->i_size + MAXBOFFSET) & MAXBMASK;
			eoff = MIN(off + len, fsize);
			dirty = pvn_range_dirty(vp, off, eoff,
			    (u_int)(off & fs->fs_bmask),
			    (u_int)((eoff + fs->fs_bsize - 1) & fs->fs_bmask),
			    flags);
		}
	}

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  All the pages on the pp list need to still
	 * be dealt with here.  Verify that we can really can do the
	 * write back to the filesystem and if not and we have some
	 * dirty pages, return an error condition.
	 */
	if (fs->fs_ronly && dirty != NULL)
		err = EROFS;
	else
		err = 0;

	if (dirty != NULL) {
		/*
		 * Destroy the read ahead value now since we are
		 * really going to write.
		 */
		ip->i_nextr = 0;

		/*
		 * This is an attempt to clean up loose ends left by
		 * applications that store into mapped files.  It's
		 * insufficient, strictly speaking, for ill-behaved
		 * applications, but about the best we can do.
		 */
		if ((ip->i_flag & IMODTIME) == 0 || (flags & B_FORCE)) {
			ip->i_flag |= IUPD;
			ITIMES(ip);
			if (vp->v_type == VREG) {
				err = fs_vcode(vp, &ip->i_vcode);
			}
		}
	}

	/*
	 * Handle all the dirty pages not yet dealt with.
	 */
	while (err == 0 && (pp = dirty) != NULL) {
		/*
		 * Pull off a contiguous chunk that fixes in one lbn
		 */
		io_off = pp->p_offset;
		lbn = lblkno(fs, io_off);
		bsize = blksize(fs, ip, lbn);

		/*
		 * Since the blocks should already be allocated for
		 * any dirty pages, we only need to use S_OTHER
		 * here and we should not get back a bn == SFS_HOLE.
		 */
		err = sfs_bmap(ip, lbn, &bn, (daddr_t *)0, bsize, S_OTHER,
		    0, cr);
		if (err)
			break;
		ASSERT(bn != SFS_HOLE);

		page_sub(&dirty, pp);
		io_list = pp;
		io_len = PAGESIZE;
		lbn_off = lbn << fs->fs_bshift;

		while (dirty != NULL && dirty->p_offset < lbn_off + bsize &&
		    dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}

		/*
		 * Check for page length rounding problems.
		 */
		if (io_off + io_len > lbn_off + bsize) {
			ASSERT((io_off+io_len) - (lbn_off+bsize) < PAGESIZE);
			io_len = lbn_off + bsize - io_off;
		}

		/*
		 * I/O may be asynch, so need to set nio first.
		 * nio depends on whether a 2nd write will be
		 * done on the same page.  This is needed if
		 * the file system block size is less than
		 * the pagesize and we are not at EOF yet.
		 */
		if (fs->fs_bsize < PAGESIZE && ip->i_size > lbn_off + bsize) {
			pp->p_nio = 2;
			do2ndwrite = 1;
		} else
			do2ndwrite = 0;

		/*
		 * XXX - should zero any bytes beyond EOF.
		 */

		bn = fsbtodb(fs, bn) + btodb(io_off - lbn_off);
		err = sfs_writelbn(ip, bn, io_list, io_len, 0, flags);

		/*
		 * Do 2nd write of the same page.
		 */
		if (do2ndwrite) {
			if (err) {
				pvn_fail(pp, B_WRITE | flags);
			} else {
				lbn++;
				bsize = blksize(fs, ip, lbn);
				err = sfs_bmap(ip, lbn, &bn, (daddr_t *)0,
				    bsize, S_OTHER, 0, cr);
				if (err) {
					pvn_fail(pp, B_WRITE | flags);
					break;
				}
				ASSERT(bn != SFS_HOLE);
				err = sfs_writelbn(ip, fsbtodb(fs, bn), pp,
				    (u_int)bsize, (u_int)fs->fs_bsize, flags);
			}
		}
	}

	if (err) {
		if (dirty != NULL)
		pvn_fail(dirty, B_WRITE | flags);
	} else if (off == 0 && len >= ip->i_size) {
		/*
		 * If doing "synchronouse invalidation" make
		 * sure that all the pages are actually gone.
		 */
		if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL 
                   && !pvn_vpempty(vp))
			goto again;

	/*
	 * If we have just sync'ed back all the pages on
	 * the inode, we can turn off the IMODTIME flag.
	 */
		ip->i_flag &= ~IMODTIME;

	}
	/*
	 * Instead of using VN_RELE here we are careful to
	 * call the inactive routine only if the vnode
	 * reference count is now zero but wasn't zero
	 * coming into putpage.  This is to prevent calling
	 * the inactive routine on a vnode that is already
	 * considered to be in the "inactive" state.
	 * Inactive is a relative term here.
	 */
vcount_chk:
	if (--vp->v_count == 0 && vpcount > 0)
		sfs_iinactive(ip, cr);
	return (err);
}

/* ARGSUSED */
STATIC int
sfs_map(vp, off, as, addrp, len, prot, maxprot, flags, fcr)
	struct vnode *vp;
	uint off;
	struct as *as;
	addr_t *addrp;
	uint len;
	u_char prot, maxprot;
	uint flags;
	struct cred *fcr;
{
	struct cred *cr = VCURRENTCRED(fcr);	/* refer to vnode.h */
	struct segvn_crargs vn_a;
	int error;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

	if (vp->v_type != VREG)
		return (ENODEV);

 
	/*
	 * If file is being locked, disallow mapping.
	 */
	if (vp->v_filocks != NULL && MANDLOCK(vp, VTOI(vp)->i_mode))
		return EAGAIN;

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 1);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address - blow away any previous mappings
		 */
		(void) as_unmap(as, *addrp, len);
	}       

	ILOCK(VTOI(vp));
	(void) sfs_allocmap(VTOI(vp));

	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = prot;
	vn_a.maxprot = maxprot;
	vn_a.cred = cr;
	vn_a.amp = NULL;

	error = as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a);
	IUNLOCK(VTOI(vp));
	return error;
}

/* ARGSUSED */
STATIC int
sfs_addmap(vp, off, as, addr, len, prot, maxprot, flags, cr)
	struct vnode *vp;
	uint off;
	struct as *as;
	addr_t addr;
	uint len;
	u_char prot, maxprot;
	uint flags;
	struct cred *cr;
{
	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	VTOI(vp)->i_mapcnt += btopr(len);
	return 0;
}

/*ARGSUSED*/
STATIC int
sfs_delmap(vp, off, as, addr, len, prot, maxprot, flags, cr)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cr;
{
	struct inode *ip;

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	ip = VTOI(vp);
	ip->i_mapcnt -= btopr(len); 	/* Count released mappings */
	ASSERT(ip->i_mapcnt >= 0);
	return (0);
}

/* ARGSUSED */
STATIC int
sfs_poll(vp, events, anyyet, reventsp, phpp)
	struct vnode *vp;
	register short events;
	int anyyet;
	register short *reventsp;
	struct pollhead **phpp;
{
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;
	return (fs_poll(vp, events, anyyet, reventsp, phpp));
}

/* ARGSUSED */
STATIC int
sfs_pathconf(vp, cmd, valp, cr)
	struct vnode *vp;
	int cmd;
	u_long *valp;
	struct cred *cr;
{
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;
	return (fs_pathconf(vp, cmd, valp, cr));
}

/*ARGSUSED*/
STATIC int
sfs_getacl(vp, nentries, dentriesp, aclbufp, cr, rvalp)
register struct vnode	*vp;
register long  		nentries;
register long		*dentriesp;
register struct acl 	*aclbufp;
struct	cred		*cr;
int			*rvalp;
{
	register struct inode	*ip = VTOI(vp);
	register int 		error;
	struct vfs		*vfsp = vp->v_vfsp;
	struct sfs_vfs		*sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	struct acl 		base_user = {USER_OBJ, (uid_t) 0, (ushort) 0};
	struct acl 		base_group = {GROUP_OBJ, (uid_t) 0, (ushort) 0};
	struct acl 		base_class = {CLASS_OBJ, (uid_t) 0, (ushort) 0};
	struct acl 		base_other = {OTHER_OBJ, (uid_t) 0, (ushort) 0};
	struct acl		*tgt_aclp;	/* ptr to target ACL */
	int			rval;

	if (UFSIP(ip))
		return ENOSYS;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ASSERT(ip->i_aclcnt >= ip->i_daclcnt);

	rval = ip->i_aclcnt > ip->i_daclcnt
		? ip->i_aclcnt + NACLBASE - 1
		: ip->i_aclcnt + NACLBASE;
	if (rval > nentries)
		return ENOSPC;

	ILOCK(ip);

	/* 
	 * get USER_OBJ, CLASS_OBJ, & OTHER_OBJ entry permissions from file 
	 * owner class, file group class, and file other permission bits 
	 */
	base_user.a_perm = (ip->i_mode >> 6) & 07;
	base_class.a_perm = (ip->i_mode >> 3) & 07;
	base_other.a_perm = ip->i_mode & 07;

	tgt_aclp = aclbufp;

	/* copy USER_OBJ into caller's buffer */
	bcopy((caddr_t)&base_user, (caddr_t)tgt_aclp, sizeof(struct acl));
	tgt_aclp++;

	if (ip->i_aclcnt == ip->i_daclcnt) {
		/* 
		 * No Actual non-default ACL entries stored.
		 * Set GROUP_OBJ entry permissions same as CLASS_OBJ,
		 * and copy GROUP_OBJ, CLASS_OBJ, & OTHER_OBJ into
		 * caller's buffer.
		 */
		base_group.a_perm = base_class.a_perm;
		bcopy((caddr_t)&base_group, (caddr_t)tgt_aclp, sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp, sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp, sizeof(struct acl));
		tgt_aclp++;
		
		/* copy default entries if any */
		if (ip->i_daclcnt
		&&  (error = sfs_aclget(ip, tgt_aclp, 0)))
			goto out;
		
		*dentriesp = ip->i_daclcnt;
		*rvalp = rval;
		IUNLOCK(ip);
		return 0;
	}

	/*
	 * There are non-default entries.
	 */
	if (ip->i_daclcnt) {
		char *tmpbuf;		/* tmp buffer to store ACL */
		struct acl *tmpaclp;	/* ptr to current ACL entry */
		int tmpsize;		/* size of allocated tmp buffer */
		long aentries;		/* non-default entries */

		/*
		 * There are default entries.
		 * Allocate temporary buffer to contain stored
		 * ACL entries.  Copy the non-default entries first,
		 * followed by the CLASS_OBJ and OTHER_OBJ entries,
		 * followed by the default entries.
		 * Free the temporary buffer.
		 */
		tmpsize = ip->i_aclcnt * sizeof(struct acl);
		tmpbuf = kmem_alloc(tmpsize, KM_SLEEP);
		tmpaclp = (struct acl *)tmpbuf;

		if (error = sfs_aclget(ip, tmpaclp, 0)) {
			kmem_free(tmpbuf, tmpsize);
			goto out;
		}
		
		aentries = ip->i_aclcnt - ip->i_daclcnt;
		bcopy((caddr_t)tmpaclp, (caddr_t)tgt_aclp,
			aentries * sizeof(struct acl));
		tmpaclp += aentries;
		tgt_aclp += aentries;
		bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp, sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp, sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)tmpaclp, (caddr_t)tgt_aclp,
			ip->i_daclcnt * sizeof(struct acl));

		kmem_free(tmpbuf, tmpsize);

		*dentriesp = ip->i_daclcnt;
		*rvalp = rval;
		IUNLOCK(ip);
		return 0;
	}

	/*
	 * There are no default entries.
	 * Copy stored ACL entries directly, followed by
	 * the CLASS_OBJ and OTHER_OBJ entries.
	 */
	if (error = sfs_aclget(ip, tgt_aclp, 0))
		goto out;
	tgt_aclp += ip->i_aclcnt;
	bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp, sizeof(struct acl));
	tgt_aclp++;
	bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp, sizeof(struct acl));
	
	*dentriesp = 0;
	*rvalp = rval;
out:
	IUNLOCK(ip);
	return error;
}

/*ARGSUSED*/
STATIC int
sfs_getaclcnt(vp, cr, rvalp)
register struct vnode	*vp;
struct	cred		*cr;
int			*rvalp;
{
	register struct inode	*ip = VTOI(vp);
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (UFSIP(ip))
		return ENOSYS;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	if (ip->i_aclcnt > ip->i_daclcnt)
		/* 
		 * if we've got additional users and/or additional groups,
		 * add the count for USER_OBJ, CLASS_OBJ, and OTHER_OBJ.
		 * If we've only got defaults, add the count for
		 * USER_OBJ, GROUP_OBJ, CLASS_OBJ, and OTHER_OBJ.
		 */
		*rvalp = ip->i_aclcnt + NACLBASE - 1; 
	else
		*rvalp = ip->i_aclcnt + NACLBASE;
	return 0;
}



/*
 *	sfs_setacl - Set a File's ACL              
 *						  
 *	Input:   Pointer to the file's inode  	  
 *		 Pointer to user's ACL entries  
 *		 Number of ACL entries to save
 *		 Pointer to number of default ACL entries
 *						  
 */

STATIC int
sfs_setacl(vp, nentries, dentries, aclbufp, cr)
register struct vnode	*vp;
register long  		nentries;
register long		dentries;
register struct acl 	*aclbufp;
struct	cred		*cr;
{
	register struct inode	*ip = VTOI(vp);
	register struct acl	*src_aclp;
	register uint 		bsize;
	register int 		i;
	struct vfs		*vfsp = vp->v_vfsp;
	struct sfs_vfs		*sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	struct buf 		*bp;
	struct fs		*fsp = ip->i_fs;
	struct aclhdr 		*ahdrp;
	struct acl 		*tmpaclp = NULL;
	daddr_t 		laclblk;
	daddr_t 		oldaclblk;
	long			aentries;	/* non-default ACL entries */
	long 			entries;	/* entries to store */
	uint 			err = 0;
	int 			oldaclcnt;
	int 			olddefcnt;
	dev_t 			dev = ip->i_dev;
	mode_t 			mode;
	struct acl		oldacls[NACLI];

	if (UFSIP(ip))
		return ENOSYS;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ILOCK(ip);
	if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
		IUNLOCK(ip);
		return EPERM;
	}

	oldaclblk = ip->i_aclblk;
	oldaclcnt = ip->i_aclcnt;
	olddefcnt = ip->i_daclcnt;
	for (i = 0; i < (oldaclcnt > NACLI ? NACLI : oldaclcnt); i++)
		oldacls[i] = ip->i_acl[i];
	aentries = nentries - dentries;		/* compute non-default count */
	mode = (aclbufp->a_perm & 07) << 6;	/* save owner perms */
	src_aclp = aclbufp + aentries - 1;	/* point at OTHER_OBJ */
	mode |= src_aclp->a_perm & 07;		/* save other perms */
	src_aclp--;				/* point at CLASS_OBJ */
	mode |= (src_aclp->a_perm & 07) << 3;	/* save file group class perms */

	if (aentries == NACLBASE) {
		/* No additional USER or GROUP entries */

		if (dentries == 0) {
			/* if no DEFAULT entries, go update the inode */
			ip->i_aclcnt = 0;
			ip->i_aclblk = (daddr_t)0;
			ip->i_daclcnt = 0;
			goto upd_inode;
		} else {
			/* else only defaults will be stored */
			entries = dentries;	/* store default entries */
			src_aclp += 2;      	/* point past CLASS_OBJ & */
						/* OTHER_OBJ at default entries */
		}
	} else {
		/*	additional USER or GROUP entries	*/
		entries = nentries - NACLBASE + 1; /* omit owner, class, & other */
		aentries -= (NACLBASE - 1);
		src_aclp = aclbufp + 1;		/* point past USER_OBJ */

		/*	
		 *	additional USER or GROUP entries,and default 
		 *	entries.  Allocate a kernel buffer so we can copy
		 *	all USER, GROUP_OBJ, GROUP, & DEFAULT entries 
		 *	in for storage in the file system in a reasonable 
		 *	fashion.
		 */
		if (dentries > 0) {
			tmpaclp = (struct acl *)kmem_alloc(entries * 
						sizeof(struct acl), KM_SLEEP);
			/* copy USER, GROUP_OBJ, & GROUP entries */
			bcopy((caddr_t)src_aclp, (caddr_t)tmpaclp, 
				aentries * sizeof(struct acl));
			/* skip past above entries, CLASS_OBJ, & OTHER_OBJ */
			src_aclp += aentries + 2;
			/* copy default entries */
			bcopy((caddr_t)src_aclp, (caddr_t)(tmpaclp + aentries), 
				dentries * sizeof(struct acl));
			src_aclp = tmpaclp;
		}	/* end else */
	}	/* end else */

	/* go store the entries on the file */
	if ((err = sfs_aclstore(ip, src_aclp, entries, dentries, cr)) != 0) {
		ip->i_aclblk = oldaclblk;
		ip->i_aclcnt = oldaclcnt;
		ip->i_daclcnt = olddefcnt;
		for (i = 0; i < (oldaclcnt > NACLI ? NACLI : oldaclcnt); i++)
			ip->i_acl[i] = oldacls[i];
		if (tmpaclp)
			kmem_free(tmpaclp, entries * sizeof (struct acl));
		IUNLOCK(ip);
		return(err);
	}

upd_inode:
	ip->i_mode &= ~(ushort)PERMMASK;
	ip->i_mode |= mode; 
	ip->i_flag |= (ICHG | ISYNC);
	sfs_iupdat(ip, IUP_SYNC);
	/* Release all old ACL blocks now */
	entries = (oldaclcnt > NACLI) ? (oldaclcnt - NACLI) : 0;
	if (tmpaclp)
		kmem_free(tmpaclp, entries * sizeof (struct acl));

	while (oldaclblk) {
		ASSERT(entries > 0);
		bsize = fragroundup(fsp, (entries * sizeof(struct acl)) +
					sizeof(struct aclhdr));
		if (bsize > fsp->fs_bsize) 
			bsize = fsp->fs_bsize;
		bp = pbread(dev, NSPF(fsp) * oldaclblk, bsize);
		if (bp->b_flags & B_ERROR) {
			sfs_free(ip, oldaclblk, bsize);
			brelse(bp);
			IUNLOCK(ip);
			return (err);
		}
		ahdrp = (struct aclhdr *)(bp->b_un.b_addr);
		laclblk = ahdrp->a_nxtblk;
		entries -= ahdrp->a_size;
		sfs_free(ip, oldaclblk, bsize);
		brelse(bp);
		if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
			return (err);
		oldaclblk = laclblk;
	}
	IUNLOCK(ip);
	return (err);
}

/*
 * Set mandatory access control level on file.
 */
STATIC int
sfs_setlevel(vp, level, credp)
	struct vnode *vp;
	lid_t level;
	struct cred *credp;
{
	struct inode *ip = VTOI(vp);
	int error=0;
	struct vfs *vfsp = vp->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;

	if (UFSIP(ip))
		return ENOSYS;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	ILOCK(ip);

	/*
	 * Make sure calling process is owner or has override privilege.
	 */
	
	if (ip->i_uid != credp->cr_uid && pm_denied(credp,P_OWNER)) {
		error = EPERM;
		goto out;
	}

	/*
	 * The root vnode of a file system is considered "non-tranquil".
	 */

	if (vp->v_flag & VROOT) {
		error = EBUSY;
		goto out;
	}

	/*
	 * Make sure that the inode is "tranquil". That is, make sure it
	 * is neither opened nor mapped.
	 * Don't bother checking tranquility if MAC is not installed.
	 */

	if ((ip->i_opencnt || ip->i_mapcnt)
	&&  mac_installed)
		error = EBUSY;
	else {
		vp->v_lid = ip->i_lid = level;
		ip->i_flag |= ICHG;
		sfs_iupdat(ip, IUP_LAZY);
	}

out:
	IUNLOCK(ip);
	return error;
}

/*
 * Create a multi-level directory.
 */
STATIC int
sfs_makemld(dvp, dirname, vap, vpp, credp)
	struct vnode *dvp;
	char *dirname;
	register struct vattr *vap;
	struct vnode **vpp;
	struct cred *credp;
{
	register struct inode *ip;
	struct inode *xip;
	int error;

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	ip = VTOI(dvp);
	if (UFSIP(ip))
		return ENOSYS;
	error = sfs_direnter(ip, dirname, DE_MKMLD, (struct inode *) 0,
	    (struct inode *) 0, vap, &xip, credp);
	ITIMES(ip);
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		ITIMES(ip);
		sfs_iunlock(ip);
	} else if (error == EEXIST)
		sfs_iput(xip);
	return (error);
}

STATIC int
sfs_allocstore(vp, off, len, cred)
	struct vnode	*vp;
	u_int		off;
	u_int		len;
	struct cred	*cred;
{
	struct inode	*ip = VTOI(vp);
	struct fs	*fs = ip->i_fs;
	daddr_t lbn, llbn;
	u_int	bsize;
	int	err = 0;

	ASSERT(off + len <= ip->i_size);

	ILOCK(ip);

	lbn = lblkno(fs, off);
	llbn = lblkno(fs, off + len - 1);

	while (lbn <= llbn && err == 0) {
		bsize = blksize(fs, ip, lbn);
		err = sfs_bmap(ip, lbn++, NULL, NULL, bsize, S_WRITE, 1, cred);
	}

	IUNLOCK(ip);

	return err;
}
