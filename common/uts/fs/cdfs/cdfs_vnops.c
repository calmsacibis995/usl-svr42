/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1991,1992  Intel Corporation
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)uts-comm:fs/cdfs/cdfs_vnops.c	1.18"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/cdrom.h>
#include <fs/cdfs/iso9660.h>
#include <fs/dirent.h>
#include <fs/fbuf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/rm.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <mem/vmmeter.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/resource.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#if ((defined CDFS_DEBUG)  && (!defined DEBUG))
#define		DEBUG	YES
#include	<util/debug.h>
#undef		DEBUG
#else
#include	<util/debug.h>
#endif
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include "cdfs.h"

/*
 * External references not defined by any kernel header file.
 */
extern	int				cleanlocks();
extern	struct seg		*segkmap;



STATIC	int cdfs_open();
STATIC	int cdfs_close();
STATIC	int cdfs_read();
STATIC	int cdfs_write();
STATIC	int cdfs_getattr();
STATIC	int cdfs_setattr();
STATIC	int cdfs_access();
STATIC	int cdfs_lookup();
STATIC	int cdfs_create();
STATIC	int cdfs_remove();
STATIC	int cdfs_link();
STATIC	int cdfs_rename();
STATIC	int cdfs_mkdir();
STATIC	int cdfs_rmdir();
STATIC	int cdfs_readdir();
STATIC	int cdfs_symlink();
STATIC	int cdfs_readlink();
STATIC	int cdfs_fsync();
STATIC	void cdfs_inactive();
STATIC	int cdfs_fid();
STATIC	void cdfs_rwlock();
STATIC	void cdfs_rwunlock();
STATIC	int cdfs_seek();
STATIC	int cdfs_frlock();
STATIC  int cdfs_space();

STATIC	int cdfs_getpage();
STATIC	int cdfs_putpage();
STATIC	int cdfs_map();
STATIC	int cdfs_addmap();
STATIC	int cdfs_delmap();
STATIC	int cdfs_allocstore();
STATIC	int cdfs_rdonly();




struct vnodeops cdfs_vnodeops = {
	cdfs_open,
	cdfs_close,
	cdfs_read,
	cdfs_rdonly,	/* vop_write	*/
	cdfs_ioctl,
	fs_setfl,
	cdfs_getattr,
	cdfs_rdonly,	/* vop_setattr	*/
	cdfs_access,
	cdfs_lookup,
	cdfs_rdonly,	/* vop_create	*/
	cdfs_rdonly,	/* vop_remove	*/
	cdfs_rdonly,	/* vop_link		*/
	cdfs_rdonly,	/* vop_rename	*/
	cdfs_rdonly,	/* vop_mkdir	*/
	cdfs_rdonly,	/* vop_rmdir	*/
	cdfs_readdir,
	cdfs_rdonly,	/* vop_symlink	*/
	cdfs_readlink,
	cdfs_fsync,
	cdfs_inactive,
	cdfs_fid,
	cdfs_rwlock,
	cdfs_rwunlock,
	cdfs_seek,
	fs_cmp,
	cdfs_frlock,
	cdfs_rdonly,	/* vop_space	*/
	fs_nosys,		/* vop_realvp	*/
	cdfs_getpage,
	cdfs_rdonly,	/* vop_putpage	*/
	cdfs_map,
	cdfs_addmap,
	cdfs_delmap,
	fs_poll,
	fs_nosys,		/* vop_dump		 */
	fs_pathconf,
	cdfs_rdonly,	/* vop_allocstore */
	fs_nosys,		/* vop_filler	 */
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
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};



/*
 * Return an error to indicate that CDFS currently
 * Read-only file systems.
 */
int
cdfs_rdonly()
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	return(EROFS);
}



/*
 * Open an ordinary file.
 *
 * Note: Device nodes are handled by the lookup() routine.
 */
/* ARGSUSED */
STATIC int
cdfs_open(vpp, flag, cr)
struct vnode	**vpp;
int				flag;
struct cred		*cr;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Nothing special needs to be done other than to verify
	 * that this is a file-type we know how to deal with.
	 */
	if (((*vpp)->v_type == VREG) ||
		((*vpp)->v_type == VDIR)) {
		return(0);
	}

	return(EACCES);
}




/*ARGSUSED*/
STATIC int
cdfs_close(vp, flag, count, offset, cr)
struct vnode	*vp;
int				flag;
int				count;
off_t			offset;
struct cred		*cr;
{
	struct cdfs_inode *ip;
	
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Close the device:
	 * - Lock the inode.
	 * - Clean up the system-level locks.
	 * - Unlock the inode.
	 */
	ip = VTOI(vp);
	cdfs_LockInode(ip);
	cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
	cdfs_UnlockInode(ip);
	return(0);
}




/*ARGSUSED*/
STATIC int
cdfs_read(vp, uiop, ioflag, cr)
struct vnode	*vp;
struct uio		*uiop;
int				ioflag;
struct cred		*cr;
{
	struct cdfs_inode	*ip;
	int					retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);
	ASSERT((ip->i_Flags & IRWLOCKED) != 0);

	retval = cdfs_readi(ip, uiop, UIO_READ, ioflag);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}
		
	return(0);
}



/*
 * Read from an Inode.
 */
STATIC int
cdfs_readi(ip, uio, rw, ioflag)
struct cdfs_inode	*ip;
struct uio			*uio;
enum uio_rw			rw;
int					ioflag;
{
	ulong_t		bytes_left;
	ulong_t		pagestart;
	ulong_t		pageoff;
	ulong_t		blkoff;
	ulong_t		count;
	uint_t		type;

	struct vfs		*vfs;
	addr_t			base;
	struct vnode	*vp;
	int				retval;
	u_int			flags;

	vp = ITOV(ip);
	type = ip->i_Mode & IFMT;
	ASSERT((type == IFREG) || (type == IFDIR) || (type == IFLNK));

	/*
	 * If record locking is manditory, verify the effected
	 * portions of the file are locked.
	 */
	if (MANDLOCK(vp, cdfs_GetPerms(vfs,ip))) {
		retval = chklock(vp, FREAD, uio->uio_offset, uio->uio_resid,
			uio->uio_fmode);
		if (retval != RET_OK) {
			return(retval);
		}
	}

	/*
	 * Validate UIO parameters.
	 */
	if ((uio->uio_offset < 0) ||
		(uio->uio_offset + uio->uio_resid < 0)) {
		return(EINVAL);
	}

	if (uio->uio_resid == 0) {
		return(0);
	}
 
	if (ioflag & IO_SYNC) {
		ip->i_Flags |= ISYNC;
	}

	vfs = ip->i_vfs;

	while (uio->uio_resid > 0) {
		/*
		 * Make sure were not beyond the end of the file.
		 */
		if (uio->uio_offset >= ip->i_Size) {
			retval = RET_OK;
			break;
		}
		bytes_left = ip->i_Size - uio->uio_offset;

		/*
		 * Compute the file offset corrsponding to the largest
		 * possible block.  Also, compute the offset relative
		 * to the start of the I/O block.
		 */
		pagestart = uio->uio_offset & MAXBMASK;
		pageoff = uio->uio_offset & MAXBOFFSET;

		/*
		 * Compute the transfer count for this iteration.
		 * - The transfer count is limited by:
		 *	 1) The # of bytes remaining in the request.
		 *	 2) The # of bytes left in the block.
		 *	 3) The # of bytes left in the file.
		 */
		blkoff = pageoff;
		count = MIN(uio->uio_resid, MAXBSIZE - blkoff);
		count = MIN(count, bytes_left);
		ASSERT(count != 0);

		/*
		 * Get a kernel mapping for file offset and move data.
		 */
		base = segmap_getmap(segkmap, vp, pagestart);
		retval = uiomove(base + pageoff, (long)count, UIO_READ, uio);

		if (retval != 0) {
			(void)segmap_release(segkmap, base, 0);
			break;
		}
			
		/*
		 * The transfer was successful, so if it ended
		 * on a block boundry or at the end of the file,
		 * we probably won't need the data again any time soon.
		 */
		flags = 0;
		if ((blkoff + count == MAXBSIZE) ||
			(uio->uio_offset >= ip->i_Size)) {
				flags |= SM_DONTNEED;
		}
		retval = segmap_release(segkmap, base, flags);
		if (retval != 0) {
			break;
		}
	}

out:
	ip->i_Flags &= ~(ISYNC | INOACC);
	return(retval);
}




/* ARGSUSED */
STATIC int
cdfs_getattr(vp, vap, flags, cr)
struct vnode	*vp;
struct vattr	*vap;
int				flags;
struct cred		*cr;
{
	struct vfs			*vfsp;
	struct cdfs_inode	*ip;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);
	vfsp = vp->v_vfsp;
	cdfs_LockInode(ip);

	/*
	 * Copy from inode table.
	 */
	vap->va_type = vp->v_type;

	vap->va_mode = cdfs_GetPerms(vfsp, ip) & MODEMASK;
	vap->va_uid = cdfs_GetUid(vfsp, ip);
	vap->va_gid = cdfs_GetGid(vfsp, ip);
		
	vap->va_fsid = CDFS_DEV(vfsp);

	vap->va_nodeid = CDFS_INUM(vfsp, ip->i_Fid.fid_SectNum,
		ip->i_Fid.fid_Offset); 

	vap->va_nlink = ip->i_LinkCnt;
	vap->va_size = ip->i_Size;

	vap->va_vcode = ip->i_VerCode;

	if ((vp->v_type == VCHR) || (vp->v_type == VBLK)) {
		vap->va_rdev = cdfs_GetDevNum(vfsp, ip);
	} else {
		vap->va_rdev = NODEV;
	}

	vap->va_atime = ip->i_AccessDate;
	vap->va_mtime = ip->i_ModDate;
	vap->va_ctime = ip->i_CreateDate;

	switch (ip->i_Mode & IFMT) {
		case IFBLK:
		case IFCHR: {
			vap->va_blksize = MAXBSIZE;
			break;
		}
		default: {
			vap->va_blksize = CDFS_BLKSZ(vfsp);
			break;
		}
	}

	vap->va_nblocks =
		((ip->i_Size + CDFS_BLKSZ(vfsp)-1) & ~(CDFS_BLKMASK(vfsp))) >>
			DEV_BSHIFT;

	cdfs_UnlockInode(ip);
	return(0);
}




/*ARGSUSED*/
STATIC int
cdfs_access(vp, mode, flags, cr)
struct vnode	*vp;
int				mode;
int				flags;
struct cred		*cr;
{
	struct cdfs_inode	*ip;
	int					retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);
	cdfs_LockInode(ip);
	retval = cdfs_iaccess(vp->v_vfsp, ip, mode, cr);
	cdfs_UnlockInode(ip);

	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	return(0);
}





/* ARGSUSED */
STATIC int
cdfs_readlink(vp, uiop, cr)
struct vnode	*vp;
struct uio		*uiop;
struct cred		*cr;
{
	struct cdfs_inode	*ip;
	struct pathname		*symlink;
	int					retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_type != VLNK) {
		return EINVAL;
	}

	ip = VTOI(vp);
	symlink = &(ip->i_Rrip->rrip_SymLink);

	retval = uiomove(symlink->pn_buf, (long)symlink->pn_pathlen,
		UIO_READ, uiop);
	if (retval != 0) {
		if (retval < 0) {
			retval = EINVAL;
		}
		return(retval);
	}
	return(0);
}




/* ARGSUSED */
STATIC int
cdfs_fsync(vp, cr)
struct vnode	*vp;
struct cred		*cr;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * XXX - CDFS WRITE SUPPORT:
	 * Need to synch media with in-core data.
	 * See ufs_syncip() for details.
	 */
	return(0);
}





/*ARGSUSED*/
STATIC void
cdfs_inactive(vp, cr)
struct vnode	*vp;
struct cred		*cr;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	cdfs_iinactive(VTOI(vp));
	return;
}




/*
 * Don't cache write blocks to files with the sticky bit set.
 * Used to keep swap files from blowing the page cache on a server.
 */
int stickyhack = 1;

/*
 * Unix file system operations having to do with directory manipulation.
 */
/* ARGSUSED */
STATIC int
cdfs_lookup(dvp, nm, vpp, pnp, flags, rdir, cr)
struct vnode	*dvp;
char			*nm;
struct vnode	**vpp;
struct pathname	*pnp;
int				flags;
struct vnode	*rdir;
struct cred		*cr;
{
	struct vfs			*vfs;
	struct cdfs_inode	*ip;
	struct vnode		*vp;
	struct vnode		*newvp;
	mode_t				mode;
	int					retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	vfs = dvp->v_vfsp; 

	/*
	 * Check for special case pathname components:
	 * - NULL is a synonym for the current directory.
	 * - CDFS_DOT yields the current directory.
	 * - CDFS_DOTDOT yields the parent directory.
	 */
	if ((nm[0] == '\0') ||
		(strcmp((caddr_t)nm, (caddr_t)CDFS_DOT) == 0)) {
		VN_HOLD(dvp);
		*vpp = dvp;
		return(0);
	}

	if (strcmp((caddr_t)nm, (caddr_t)CDFS_DOTDOT) == 0) {
		retval = cdfs_DirLookup(vfs, VTOI(dvp), (uchar_t *)CDFS_POSIX_DOTDOT,
			&ip, cr);
	} else {
		retval = cdfs_DirLookup(vfs, VTOI(dvp), (uchar_t *)nm, &ip, cr);
	}

	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	vp = ITOV(ip);
	mode = cdfs_GetPerms(vfs,ip);
	if (((mode & ISVTX) != 0) &&
		((mode & (IEXEC | IFDIR)) == 0) &&
		(stickyhack != 0)) {
		vp->v_flag |= VISSWAP;
	}

	/*
	 * If vnode is a device-type vnode, then return the SPECFS
	 * vnode instead of the CDFS vnode.  Otherwise, unlock
	 * the CDFS vnode and return it.
	 * Note: We need to recompute the device # to allow for
	 * XCDR device mapping.
	 */
	if ((vp->v_type == VCHR) || (vp->v_type == VBLK) ||
		(vp->v_type == VFIFO) || (vp->v_type == VXNAM)) {
		vp->v_rdev = cdfs_GetDevNum(vfs, ip);
		cdfs_UnlockInode(ip);
		newvp = specvp(vp, vp->v_rdev, vp->v_type, cr);
		VN_RELE(vp);
		if (newvp == NULL) {
			*vpp = NULL;
			return(ENOSYS);
		}
		vp = newvp;
	} else {
		cdfs_UnlockInode(ip);
	}

	*vpp = vp;
	return(0);
}





/* ARGSUSED */
STATIC int
cdfs_readdir(vp, uiop, cr, eofp)
struct vnode	*vp;						/* Directory's Vnode structure	*/
struct uio		*uiop;						/* Caller's buffer to put data	*/
struct cred		*cr;						/* Caller's credential structure*/
int				*eofp;						/* Ret Addr for EOF flag		*/
{
	struct vfs			*vfs;				/* Addr of VFS structure		*/
	struct cdfs_inode	*ip;				/* Directory's Inode structure	*/
	struct cdfs_iobuf	drec_buf;			/* I/O buffer to scan directory */
	struct pathname		name;				/* Name of current Dir Rec		*/
	struct pathname		last;				/* Name of Dir Rec last copied 	*/
	ino_t				inum;				/* Inode # of current Dir Rec	*/

	struct iovec		*iovp;				/* I/O Vector of output buffer	*/ 
	uint_t				bytes_wanted;		/* Total # of bytes wanted		*/

	caddr_t				tmpbuf;				/* Tmp buf to build output data	*/
	struct dirent		*dp;				/* Pntr to cur. output struct 	*/
	uint_t				reclen;				/* Reclen of cur. output struct	*/

	int					i;					/* Misc counter					*/
	int					retval;				/* Ret value of called routines	*/

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);
	ASSERT((ip->i_Flags & IRWLOCKED) != 0);
	vfs = ip->i_vfs;

	/*
	 * Compute and validate the starting Vnode offset and the
	 * # of bytes to be transfered.
	 * - Validate the Vnode offset.
	 * - Compute the transfer count.
	 * - Validate the transfer count.
	 */
	if (uiop->uio_offset >= ip->i_Size) {
		if (eofp != NULL) {
			*eofp = 1;
		}
		return(0);
	}

	bytes_wanted = 0;
	iovp = uiop->uio_iov;
	for (i=0; i < uiop->uio_iovcnt; i++) {
		bytes_wanted += iovp->iov_len;
		iovp++;
	}

	if (bytes_wanted == 0) {
		return(0);
	}

	if (bytes_wanted < sizeof(struct dirent)) {
		return(EINVAL);
	}

	/*
	 * Allocate a temporary buffer to buffer-up several
	 * dirent structures.  It is hoped that coping each 
	 * dirent structure to a temp buffer and doing a
	 * single 'uiomove()' is faster than doing two
	 * 'uiomove()'s (1 for the name string and 1 for the
	 * other dirent data) for each dirent structure.
	 */
	tmpbuf = (caddr_t)kmem_fast_alloc(&cdfs_TmpBufPool,
		cdfs_TmpBufSz, 1, KM_SLEEP);
	dp = (struct dirent *)tmpbuf;

	cdfs_LockInode(ip);

	CDFS_SETUP_IOBUF(&drec_buf, CDFS_FBUFIO);
	drec_buf.sb_vp = vp;
		
	if (uiop->uio_offset >= ip->i_DirOffset) {
		drec_buf.sb_offset = ip->i_DirOffset;
	} else {
		drec_buf.sb_offset = 0;
	}

	*eofp = 0;
	pn_alloc(&name);
	pn_alloc(&last);
	while (bytes_wanted != 0) {
		retval = cdfs_ReadDrec(vfs, &drec_buf);
		if (retval != RET_OK) {
			/*
			 * EOF condition signifies when to stop searching
			 * and is expected at some point.  Therefore, set 
			 * the caller's EOF flag and reset the error condition.
			 */
			if (retval == RET_EOF) {
				*eofp = 1;
				retval = RET_OK;
			}
			break;
		}

		/*
		 * If the caller wants this Dir Rec, then add it
		 * to the output buffer.
		 */
		if (drec_buf.sb_offset >= uiop->uio_offset) {
			/*
			 * Get the name of the Dir Rec entry.
			 * If RRIP is active, then try to obtain the RRIP name.
			 * If RIP is not active or we couldn't get the RRIP name
			 * then we get the ISO name and apply the XCDR Name conversion.
			 *
			 * Note: This algorithm should complement the lookup
			 * algorithm used in cdfs_CmpDrecName().
			 */

			if (((CDFS_FLAGS(vfs) & CDFS_RRIP_ACTIVE) == 0) ||
				(cdfs_GetRripName(vfs, &drec_buf, &name) != RET_OK) ||
				(name.pn_pathlen == 0)) {

				if (cdfs_GetIsoName(vfs, &drec_buf, &name) ||
					(cdfs_XcdrName(vfs, (uchar_t *)name.pn_buf,
							name.pn_pathlen, &name) != RET_OK) ||
					(name.pn_pathlen == 0)) {

					/*
					 * Didn't find a name - All we can do is quit.
					 */
					retval = RET_ERR; 
					break;
				}
			}

			/*
			 * We add the name of this Dir Rec to the buffer only if:
			 * - The Dir Rec is not to be "hidden" from the user, AND
			 * - The Dir Rec does not immediately follow a Dir Rec
			 *	 having the same name (including any XCDR conversion).
			 *	 This prevents multiple listings of the same filename
			 *	 for multi-extent and/or multi-version files.
			 */
			if ((cdfs_HiddenDrec(vfs, &drec_buf) == RET_FALSE) &&
				((name.pn_pathlen != last.pn_pathlen) ||
				(strncmp(&name.pn_buf[0], &last.pn_buf[0],
					last.pn_pathlen) != 0))) {

				/*
				 * Check for special case directory entries 
				 * (DOT and DOTDOT).  If found, then the name
				 * must be changed to their POSIX counter-part
				 * ('.' and '..') and the TRUE Inode Number must
				 * obtained in order to maintain consistency.
				 */
				if ((strncmp(name.pn_buf, (caddr_t)CDFS_DOT,
						name.pn_pathlen) == 0) &&
					(CDFS_DOT[name.pn_pathlen] == '\0')) {
					/*
					 * Note: Since this is the current directory, we
					 * can just use it's FID to compute the Inode Num.
					 */
					pn_set(&name, (caddr_t)CDFS_POSIX_DOT);
					inum = CDFS_INUM(vfs, ip->i_Fid.fid_SectNum,
						ip->i_Fid.fid_Offset); 

				} else
				if ((strncmp(name.pn_buf, (caddr_t)CDFS_DOTDOT,
						name.pn_pathlen) == 0) &&
					(CDFS_DOTDOT[name.pn_pathlen] == '\0')) {
					/*
					 * Note: If the Parent FID is valid then we can
					 * just use it.  Otherwise, there is no choice
					 * but to get the Parent Inode and use its FID.
					 */
					pn_set(&name, (caddr_t)CDFS_POSIX_DOTDOT);
					if (CDFS_CMPFID(&ip->i_ParentFid, &CDFS_NULLFID) ==
							B_FALSE) {
						inum = CDFS_INUM(vfs, ip->i_ParentFid.fid_SectNum,
							ip->i_ParentFid.fid_Offset); 
					} else {
						struct cdfs_inode	*pip;
						retval = cdfs_GetParent(vfs, ip, &pip, cr);
						if (retval != RET_OK) {
							break;
						}
						inum = CDFS_INUM(vfs, pip->i_Fid.fid_SectNum,
							pip->i_Fid.fid_Offset); 
						cdfs_UnlockInode(pip);
						VN_RELE(ITOV(pip));
					}

				} else {
					/*
					 * No special case (DOT and/or DOTDOT) so just
					 * compute the Inode # based on the location
					 * of this Dir Rec entry.
					 */
					inum = CDFS_INUM(vfs, drec_buf.sb_sect,
						(drec_buf.sb_ptr - drec_buf.sb_start));
				}

				/*
				 * If there is not enough room in the output
				 * buffer, then flush it to the caller's space.
				 *
				 * Note: Since 'bytes_wanted' > 0, the entire
				 * contents of the buffer should be transfered.
				 */
				reclen = CDFS_STRUCTOFF(dirent, d_name) + name.pn_pathlen + 1;
				reclen = roundup(reclen, sizeof(int));

				if (reclen > bytes_wanted) {
					break;
				}

				if (reclen > (PAGESIZE - ((caddr_t)dp - tmpbuf))) {
					uiomove(tmpbuf, ((caddr_t)dp - tmpbuf), UIO_READ, uiop);
					dp = (struct dirent *)tmpbuf;
				}

				/*
				 * Copy the dirent data to the output buffer.
				 *
				 * Note: Save the name of the Dir Rec so that we can
				 * avoid duplicate entries as coded above.
				 *
				 * XXX - According to the dirent(4) (Admin Ref)
				 * man-page, 'd_off' contains the offset (within
				 * the directory data) of the CURRENT directory
				 * entry.  However, the implementations of readir(3C)
				 * and telldir(3C) (directory(3C) man-page) library
				 * routines require that 'd_off' be set to the offset
				 * of the NEXT directory entry.  If 'd_off' is set per
				 * the dirent(4) man-page, 'du', as an example, will
				 * enter an infinate-loop reprocessing the first
				 * 'leaf' directory of the sub-tree.
				 */
				dp->d_ino = inum;
				dp->d_reclen = reclen;
				dp->d_off = drec_buf.sb_offset + drec_buf.sb_reclen;

				strncpy(&dp->d_name[0], &name.pn_buf[0], name.pn_pathlen);
				dp->d_name[name.pn_pathlen] = '\0';

				cdfs_pn_set(&last, (uchar_t *)&name.pn_buf[0],
					name.pn_pathlen);

				dp = (struct dirent *)((caddr_t)(dp) + reclen); 
				bytes_wanted -= reclen;
			}
		}
		
		/*
		 * Increment next Dir Rec.
		 */
		drec_buf.sb_ptr += drec_buf.sb_reclen;
		drec_buf.sb_offset += drec_buf.sb_reclen;
		
		if (drec_buf.sb_offset >= ip->i_Size) {
			*eofp = 1;
			break;
		}
	}
	pn_free(&name);
	pn_free(&last);

	/*
	 * Send any residual buffer data to caller.
	 */
	if ((caddr_t)dp != tmpbuf) {
		uiomove(tmpbuf, ((caddr_t)dp - tmpbuf), UIO_READ, uiop);
	}
	uiop->uio_offset = drec_buf.sb_offset;

	/*
	 * Release the Dir Rec buffer, if still allocated.
	 */
	CDFS_RELEASE_IOBUF(&drec_buf);

	ip->i_DirOffset = drec_buf.sb_offset;
	cdfs_UnlockInode(ip);

	/*
	 * Release the temporary buffer.
	 */
	kmem_fast_free(&cdfs_TmpBufPool, tmpbuf);

	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	return(0);
}



STATIC int
cdfs_fid(vp, fidpp)
struct vnode	*vp;
struct fid		**fidpp;
{
	struct fid		*fid;
	struct cdfs_fid	*cdfs_fid;
	uint_t			size;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Allocate memory for a generic 'fid' structure.
	 */
	size = CDFS_STRUCTOFF(fid, fid_data[0]) + sizeof(*cdfs_fid);
	fid = (struct fid *)kmem_zalloc(size, KM_SLEEP);

	/*
	 * Fill in the 'fid' structure with the length info
	 * and the CDFS FID info.
	 */
	fid->fid_len = sizeof(*cdfs_fid);

	cdfs_fid = (struct cdfs_fid *) &(fid->fid_data[0]);
	*cdfs_fid = VTOI(vp)->i_Fid;

	*fidpp = fid;
	return(0);
}






STATIC void
cdfs_rwlock(vp)
struct vnode	*vp;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	cdfs_irwlock(VTOI(vp));
	return;
}





STATIC void
cdfs_rwunlock(vp)
struct vnode	*vp;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	cdfs_irwunlock(VTOI(vp));
	return;
}
			




/* ARGSUSED */
STATIC int
cdfs_seek(vp, ooff, noffp)
struct vnode	*vp;
off_t			ooff;
off_t			*noffp;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (*noffp < 0) {
		return(EINVAL);
	}
	
	return(0);
}




/* ARGSUSED */
STATIC int
cdfs_frlock(vp, cmd, bfp, flag, offset, cr)
struct vnode	 *vp;
int				 cmd;
struct flock	 *bfp;
int				 flag;
off_t			 offset;
cred_t			 *cr;
{
	struct vfs			*vfs;
	struct cdfs_inode	*ip;
	int					retval; 

	ip = VTOI(vp);
	vfs = ip->i_vfs;
 
	/*
	 * If file is being mapped, disallow frlock.
	 */
	if ((ip->i_mapcnt > 0) && (MANDLOCK(vp, cdfs_GetPerms(vfs,ip)))) {
		return(EAGAIN);
	}
 
	retval = fs_frlock(vp, cmd, bfp, flag, offset, cr);
	return(retval);
}






/*
 * Return all the pages from [off..off+len] in given file.
 */
STATIC int
cdfs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cr)
struct vnode	*vp;
u_int			off;
u_int			len;
u_int			*protp;
struct page		*pl[];
u_int			plsz;
struct seg		*seg;
addr_t			addr;
enum seg_rw		rw;
struct cred		*cr;
{
	struct cdfs_inode	 *ip;
	int					 err;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if ((vp->v_flag & VNOMAP) != 0) {
		return (ENOSYS);
	}

	ip = VTOI(vp);
	cdfs_LockInode(ip);

	/*
	 * This check for beyond EOF allows the request to extend up to
	 * the page boundary following the EOF.	 Strictly speaking,
	 * it should be (off + len > (ip->i_Size + PAGEOFFSET) % PAGESIZE),
	 * but in practice, this is equivalent and faster.
	 *
	 * Also, since we may be called as a side effect of a bmap or
	 * dirsearch() using fbread() when the blocks might be being
	 * allocated and the size has not yet been up'ed.  In this case
	 * we disable the check and always allow the getpage to go through
	 * if the segment is seg_map, since we want to be able to return
	 * zeroed pages if bmap indicates a hole in the non-write case.
	 * For cdfs, we also might have to read some frags from the disk
	 * into a page if we are extending the number of frags for a given
	 * lbn in cdfs_bmap().
	 */

	if ((off + len > ip->i_Size + PAGEOFFSET) &&
		((seg != segkmap) || (rw != S_OTHER))) {
		cdfs_UnlockInode(ip);
		return (EFAULT);	/* beyond EOF */
	}

	if (protp != NULL) {
		*protp = PROT_ALL;
	}

	if (len <= PAGESIZE) {
		err = cdfs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cr);
	} else {
		err = pvn_getpages(cdfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cr);
	}

	/*
	 * If the inode is not already marked for IACC (in cdfs_readi() for read)
	 * and the inode is not marked for no access time update (in cdfs_readi()
	 * for write) then update the inode access time and mod time now.
	if ((ip->i_Flags & (IACC | INOACC)) == 0) {
		if (rw != S_OTHER)
			ip->i_Flags |= IACC;
		if (rw == S_WRITE)
			ip->i_Flags |= IUPD;
	}
	 */

	cdfs_UnlockInode(ip);

	return(err);
}




/* ARGSUSED */
STATIC int
cdfs_map(vp, off, as, addrp, len, prot, maxprot, flags, cr)
struct vnode	*vp;
uint			off;
struct as		*as;
addr_t			*addrp;
uint_t			len;
uchar_t			prot;
uchar_t			maxprot;
uint_t			flags;
struct cred		*cr;
{
	struct cdfs_inode	*ip;
	struct segvn_crargs	vn_a;
	int					retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_flag & VNOMAP) {
		return (ENOSYS);
	}

	if (((int)off < 0) || ((int)(off + len) < 0)) {
		return(EINVAL);
	}

	if (vp->v_type != VREG) {
		return(ENODEV);
	}

	ip = VTOI(vp);

	/*
	 * If file is being locked, disallow mapping.
	 */
	if ((vp->v_filocks != NULL) &&
		(MANDLOCK(vp, cdfs_GetPerms(ip->i_vfs, ip)) != 0)) {
		return(EAGAIN);
	}

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 1);
		if (*addrp == NULL) {
			return(ENOMEM);
		}
	} else {
		/*
		 * User specified address - blow away any previous mappings
		 */
		(void) as_unmap(as, *addrp, len);
	}

	cdfs_LockInode(ip);

	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = prot;
	vn_a.maxprot = maxprot;
	vn_a.cred = cr;
	vn_a.amp = NULL;

	retval = as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a);
	cdfs_UnlockInode(ip);
	return(retval);	
}




/* ARGSUSED */
STATIC int
cdfs_addmap(vp, off, as, addr, len, prot, maxprot, flags, cr)
struct vnode	*vp;
uint_t			off;
struct as		*as;
addr_t			addr;
uint_t			len;
uchar_t			prot;
uchar_t			maxprot;
uint_t			flags;
struct cred		*cr;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	VTOI(vp)->i_mapcnt += btopr(len);
	return(0);
}




/*ARGSUSED*/
STATIC int
cdfs_delmap(vp, off, as, addr, len, prot, maxprot, flags, cr)
struct vnode	*vp;
uint_t			off;
struct as		*as;
addr_t			addr;
uint_t			len;
uchar_t			prot;
uchar_t			maxprot;
uint_t			flags;
struct cred		*cr;
{
	struct cdfs_inode *ip;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_flag & VNOMAP) {
		return(ENOSYS);
	}

	ip = VTOI(vp);
	ip->i_mapcnt -= btopr(len); 	/* Count released mappings */
	ASSERT(ip->i_mapcnt >= 0);
	return(0);
}


int cdfs_ra = 1;
int cdfs_lostpage;	/* number of times we lost original page */

#define CDFS_MAXIO	16
#define CDFS_HOLE	0

/*
 * Called from pvn_getpages() or cdfs_getpage() to get a particular page.
 * When we are called the inode is already locked.
 *
 * If rw == S_WRITE and block is not allocated, need to alloc block.
 * If ppp == NULL, async I/O is requested.
 */
/* ARGSUSED */
STATIC int
cdfs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cr)
struct vnode	*vp;
uint_t			off;
uint_t			*protp;
struct page		*pl[];
uint_t			plsz;
struct seg		*seg;
addr_t			addr;
enum seg_rw		rw;
struct cred		*cr;
{
	struct cdfs_inode	*ip;
	struct vfs			*vfs;
	uint_t				bsize;
	uint_t				xlen;

	daddr_t				bn[CDFS_MAXIO];
	daddr_t				*bnp;
	daddr_t				*bnp2;

	uint_t				boff[CDFS_MAXIO];
	uint_t				*offp;

	struct buf			*bp[CDFS_MAXIO];
	struct buf			**bufp;

	struct vnode		*devvp;

	struct page			*pp;
	struct page			*pp2;
	struct page			**ppp;
	struct page			*pagefound;

	daddr_t				lbn;
	uint_t				io_off;
	uint_t				io_len;
	uint_t				lbnoff;
	uint_t				curoff;

	int					retval;
	uint_t				nio;
	uint_t				pgoff;
	dev_t				dev;
	uint_t				pgaddr;
	uint_t				multi_io;
	uint_t				i;
	uint_t				do_ra;
	uint_t				blksz;

	ip = VTOI(vp);
	vfs = ip->i_vfs;
	bsize = CDFS_BLKSZ(vfs);
	dev = CDFS_DEV(vfs);

	retval = 0;

reread:
	do_ra = 0;
	pagefound = NULL;
	pgoff = 0;

	if (pl != NULL) {
		pl[0] = NULL;
	}

	bnp = &bn[0];
	offp = &boff[0];
	bufp = &bp[0];
	for (i=0; i < CDFS_MAXIO; i++) {
		*bnp = CDFS_HOLE;
		*offp = 0;
		*bufp = NULL;
		bnp++;
		bufp++;
		offp++;
	}

	/*
	 * Build block list for this page.
	 */
	lbn = off >> CDFS_BLKSHFT(vfs);
	lbnoff = off & ~CDFS_BLKMASK(vfs);
	curoff = lbnoff;

	bnp = &bn[0];
	offp = &boff[0];
	nio = 0; 

	if (bsize < PAGESIZE) {
		/*
		 * Multiple blocks per page.
		 */
		multi_io = PAGESIZE / bsize; 
		for (i=0; i < multi_io; i++) {
			if (curoff >= ip->i_Size) {
				break;
			}

			retval = cdfs_bmap(vfs, ip, curoff, bnp, offp, NULL);
			if (retval != RET_OK) {
				goto out;
			}

			/*
			 * HINT - CDFS Write Support:
			 * If a "hole" in the allocated file space exists,
			 * then disallow writes and don't increment the I/O count. 
			 */

			nio++;
			bnp++;
			curoff = curoff + bsize;
		}
	} else {
		/*
		 * Multiple pages per block.
		 */
		multi_io = 1;
		nio = 1;

		retval = cdfs_bmap(vfs, ip, curoff, bnp, offp, NULL);
		if (retval != RET_OK) {
			goto out;
		}

		if ((cdfs_ra != 0) && (ip->i_ReadAhead == off)) {
			retval = cdfs_bmap(vfs, ip, curoff+bsize, (bnp+1),
				(offp+1), NULL);
			/*
			 * HINT - CDFS Write Support: Also check for hole.
			 */
			if (retval == RET_OK) {
				do_ra = 1;
			} else {
				do_ra = 0;
			}
		} else {
			do_ra = 0;
		}
	}

	bnp = &bn[0];
	bufp = &bp[0];
	offp = &boff[0];

	ASSERT(nio > 0);
 
	devvp = CDFS_FS(vfs)->cdfs_DevVnode;
	blksz = CDFS_BLKSZ(vfs);

	/*
	 * Although the previous page_find() failed to find the page,
	 * we have to check again since it may have entered the cache
	 * during or after the calls to bmap().
	 */
again:
	pagefound = page_find(vp, off);
	if (pagefound != NULL) {
		if (do_ra == 0) {
			goto pagefound_out;
		} else {
			bnp = bn + 1;
			bufp = bp + 1;
			goto do_ra_out;
		}
	}

	/*
	 * Need to really do disk I/O to get the page(s).
	 * If someone else got there first, try again.
	 */
	pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len, lbnoff, blksz, 0);
	if (pp == NULL) {
		goto again;
	}

	/*
	 * Setup the block list for each page to be read.
	 */
	pp2 = pp;
	if (bsize < PAGESIZE) {
		for (i=0; i < nio; i++) {
			pp2->p_dblist[i] = bn[i];
		}
	} else {
		do {
			pp2->p_dblist[0] = bn[0];
			pp2 = pp2->p_next;
		} while (pp2 != pp);
	}

	/*
	 * Fill in the Page List array with the pages to be read.
	 */
	if (pl != NULL) {
		register int sz;

		if (plsz >= io_len) {
			/*
			 * All of the pages will fit in the page list array.
			 * So, all pages get recorded and held.
			 */
			pp2 = pp;
			sz = io_len;
		} else {
			/*
			 * The page list array is too small to store all of the
			 * pages so just record 'plsz' pages starting with the
			 * page that is really needed.
			 */
			for (pp2=pp; pp2->p_offset != off; pp2 = pp2->p_next) {
				ASSERT(pp2->p_next->p_offset != pp->p_offset);
			}
			sz = plsz;
		}

		/*
		 * Fill up the page list array and hold the pages in memory.
		 */
		ppp = pl;
		while (sz > 0) {
			*ppp = pp2;
			PAGE_HOLD(pp2);
			ppp++;
			pp2 = pp2->p_next;
			sz -= PAGESIZE;
		}
		*ppp = NULL;
	}

	if (nio > 1) {
		pp->p_nio = nio;
	}

	/*
	 * Read in the first physical block of data.
	 * - Setup the buffer header
	 * - Zero out the portion of the page not being read to.
	 * - Call the device driver.
	 */
	bp[0] = pageio_setup(pp, io_len, devvp,
	  pl == NULL ? (B_ASYNC | B_READ) : B_READ);

	bp[0]->b_edev = dev;
	bp[0]->b_dev = cmpdev(dev);
	bp[0]->b_blkno = (bn[0] << (CDFS_BLKSHFT(vfs) - DEV_BSHIFT)) +
		((io_off - lbnoff) >> DEV_BSHIFT);

	xlen = io_len & PAGEOFFSET;
	if (xlen != 0) {
		pagezero(pp->p_prev, xlen, PAGESIZE - xlen);
	}

	(*bdevsw[getmajor(dev)].d_strategy)(bp[0]);
	ip->i_ReadAhead = io_off + io_len;
	vminfo.v_pgin++;
	vminfo.v_pgpgin += btopr(io_len);

	/*
	 * Read in the remaining blocks for the page.
	 */
	bnp++;
	bufp++;
	/* for (i = 1; i < nio; i++, bnp++, bufp++) */
	for (i = 1; i < nio; i++, bnp++, bufp++) {
		addr_t addr2;

		lbnoff += bsize;
		addr2 = addr + (lbnoff - off);

		/*
		 * We only need to increment keepcnt once,
		 * because only when p_nio is less than 1
		 * will PAGE_RELE() be called.
		 */
		pp2 = pp;
		if (nio < 2) {
			PAGE_HOLD(pp2);
			pp2->p_intrans = 1;
			pp2->p_pagein = 1;
		}
		io_len = bsize;
		pgoff = i * bsize;

		if (pp2 != NULL) {
			*bufp = pageio_setup(pp2, io_len, devvp,
				  pl == NULL ?
				(B_ASYNC | B_READ) : B_READ);

			(*bufp)->b_edev = dev;
			(*bufp)->b_dev = cmpdev(dev);
			(*bufp)->b_blkno = *bnp << (CDFS_BLKSHFT(vfs) - DEV_BSHIFT);
			(*bufp)->b_un.b_addr = (caddr_t)pgoff;

			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now.
			 */
			xlen = (io_len + pgoff) & PAGEOFFSET;
			if (xlen != 0)
				pagezero(pp2->p_prev, xlen,
				  PAGESIZE - xlen);
			(*bdevsw[getmajor(dev)].d_strategy)(*bufp);
			vminfo.v_pgin++;
			vminfo.v_pgpgin += btopr(io_len);
		}
	}

do_ra_out:
	if (do_ra != 0) {
		addr_t addr2;

		lbnoff += bsize;
		addr2 = addr + (lbnoff - off);
		if (addr2 >= seg->s_base + seg->s_size)
			pp2 = NULL;
		else
			pp2 = pvn_kluster(vp, lbnoff, seg, addr2, &io_off,
			  &io_len, lbnoff, blksz, 1);
		pgoff = 0;

		if (pp2 != NULL) {
			pp = pp2;
			do {
				pp->p_dblist[0] = bn[1];
				pp = pp->p_next;
			} while (pp != pp2);

			*bufp =
			  pageio_setup(pp2, io_len, devvp, (B_ASYNC | B_READ));
			(*bufp)->b_edev = dev;
			(*bufp)->b_dev = cmpdev(dev);
			(*bufp)->b_blkno = (*bnp << (CDFS_BLKSHFT(vfs) - DEV_BSHIFT)) +
				((io_off - lbnoff) >> DEV_BSHIFT);

			xlen = (io_len + pgoff) & PAGEOFFSET;
			if (xlen != 0)
				pagezero(pp2->p_prev, xlen, PAGESIZE - xlen);
			(*bdevsw[getmajor(dev)].d_strategy)(*bufp);
			vminfo.v_pgin++;
			vminfo.v_pgpgin += btopr(io_len);
		}
	}

out:
	if (pl == NULL) {
		return(retval);
	}

	bufp = bp;
	for (i=0; i < nio; i++) {
		if (*bufp != NULL) {
			if (retval == RET_OK) {
				retval = biowait(*bufp);
			} else {
				(void)biowait(*bufp);
			}
			pageio_done(*bufp);
		}
		bufp++;
	}

pagefound_out:
	if (pagefound != NULL) {
		int s;

		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if ((pagefound->p_vnode == vp) &&
			(pagefound->p_offset == off)) {
			/*
			 * If the page is still intransit or if
			 * it is on the free list call page_lookup
			 * to try and wait for/reclaim the page.
			 */
			if ((pagefound->p_intrans) ||
				(pagefound->p_free)) {
				pagefound = page_lookup(vp, off);
			}
		}
		(void) splx(s);

		if ((pagefound == NULL) ||
			(pagefound->p_offset != off) ||
		    (pagefound->p_vnode != vp) ||
			(pagefound->p_gone)) {

			cdfs_lostpage++;
			goto reread;
		}

		if (pl != NULL) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			ip->i_ReadAhead = off + PAGESIZE;
		}
	}

	if ((retval != RET_OK) && (pl != NULL)) {
		for (ppp=pl; *ppp != NULL; ppp++) {
			PAGE_RELE(*ppp);
			*ppp = NULL;
		}
	}

	return(retval);
}

