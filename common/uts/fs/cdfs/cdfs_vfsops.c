/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991 ,1992  Intel Corporation	*/
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

#ident	"@(#)uts-comm:fs/cdfs/cdfs_vfsops.c	1.10"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/cdrom.h>
#include <fs/cdfs/iso9660.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/ddi.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#if ((defined CDFS_DEBUG)  && (!defined DEBUG))
#define		DEBUG	YES
#include	<util/debug.h>
#undef		DEBUG
#else
#include	<util/debug.h>
#endif
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include "cdfs.h"



/*
 * External references not defined by in any kernel header file.
 */
extern void			clkset();



/*
 * Global CDFS kernel constants, strings, and structures:
 */
const uchar_t		CDFS_ISO_STD_ID[] = ISO_STD_ID;
const uchar_t		CDFS_HS_STD_ID[] = HS_STD_ID;

const uchar_t		CDFS_DOT[] = {ISO_DOT, '\0'};
const uchar_t		CDFS_DOTDOT[] = {ISO_DOTDOT, '\0'};

const uchar_t		CDFS_POSIX_DOT[] = ".";
const uchar_t		CDFS_POSIX_DOTDOT[] = "..";

const struct cdfs_fid	CDFS_NULLFID = {0, 0};



/*
 * Global CDFS kernel variables:
 */
int					cdfs_fstype;			/* VFS ID # assigned to CDFS	*/

caddr_t				cdfs_Mem;				/* Inode pool allocated memory	*/
ulong_t				cdfs_MemSz;				/* Amount of memory allocated	*/
uint_t				cdfs_MemCnt;			/* Reference count				*/

struct cdfs_inode	*cdfs_InodeCache;		/* CDFS in-core Inode cache		*/
struct cdfs_inode	*cdfs_InodeFree;		/* CDFS Inode Free List			*/
struct cdfs_inode	**cdfs_InodeHash;		/* CDFS Inode Hash Table		*/

struct cdfs_drec	*cdfs_DrecCache;		/* Multi-extent Dir Rec cache	*/
struct cdfs_drec	*cdfs_DrecFree;			/* Dir Rec cache Free list		*/

caddr_t				cdfs_TmpBufPool;		/* Temp Buf Pool				*/

#ifdef CDFS_DEBUG
uint_t		cdfs_dbflags = DB_ENTER;		/* Flags to select desired DEBUG*/
#endif




struct vfsops cdfs_vfsops = {
	cdfs_mount,
	cdfs_unmount,
	cdfs_root,
	cdfs_statvfs,
	cdfs_sync,
	cdfs_vget,
	cdfs_mountroot,
	fs_nosys,		/* vfs_swapvp */
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
 * CDFS Entry Point Stub used for debugging.
 */
#ifdef CDFS_DEBUG
void		(*cdfs_DebugPtr)() = cdfs_entry;/* Default trap CDFS entry points*/

void
cdfs_entry()
{
	return;
}
#endif	/* CDFS_DEBUG */



/*
 * CDFS initialization routine called only ONCE at system start-up.
 * Deals only with global CDFS initialization tasks that are 
 * independant of any specific CDFS file-system or file.
 */
void
cdfsinit(vswp)
struct vfssw *vswp;
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Register the cdfs_vfsops[] table in the vfs structure.
	 *
	 * Note: Memory allocation and initialization
	 * happens on the first mount and is released
	 * following the last unmount.  This allows CDFS
	 * to consume relatively few resources when not in use.
	 */
	vswp->vsw_vfsops = &cdfs_vfsops;
	
	return;
}



/*
 * Allocate and initialize the necessary memory for CDFS.
 */
int
cdfs_AllocMem()
{
	uint_t				cache_mem;			/* Size of Inode cache memory	*/
	uint_t				hash_mem;			/* Size of Inode Hash Tbl memory*/
	uint_t				drec_mem;			/* Size of Dir Rec Cache memory	*/

	struct cdfs_inode	*ip;				/* Roving Inode pointer			*/
	struct cdfs_inode	**ipp;				/* Roving "Inode pointer" pntr	*/
	struct cdfs_drec	*drec;				/* Roving Dir Rec pointer		*/
	int					i;					/* Loop counter					*/

	/*
	 * Compute the amount of memory needed.
	 * - Inode cache.
	 * - Inode hash-table.
	 * - Multi-extent Dir Rec cache.
	 */
	cache_mem = cdfs_InodeCnt * sizeof(*cdfs_InodeCache);
	hash_mem = cdfs_IhashCnt * sizeof(*cdfs_InodeHash);
	drec_mem = cdfs_DrecCnt * sizeof(*cdfs_DrecCache);

	cdfs_MemSz = cache_mem + hash_mem + drec_mem;

	/*
	 * Allocate the required memory and assign it to the
	 * various CDFS resources.
	 */
	cdfs_Mem = kmem_zalloc(cdfs_MemSz, KM_SLEEP); 
	if (cdfs_Mem == NULL) {
		cmn_err(CE_WARN,
			"cdfs_AllocMem(): Unable to allocate 0x%x (%d) bytes of memory.\n",
			cdfs_MemSz, cdfs_MemSz);
		return(ENOMEM);
	}
	
	cdfs_InodeCache = (struct cdfs_inode *) cdfs_Mem;
	cdfs_InodeHash = (struct cdfs_inode **) &cdfs_InodeCache[cdfs_InodeCnt];
	cdfs_DrecCache = (struct cdfs_drec *) &cdfs_InodeHash[cdfs_IhashCnt]; 

	/*
	 * Initialize the Inodes in the cache
	 * and add them to the Inode free-list.
	 */
	cdfs_InodeFree = NULL;
	ip = &cdfs_InodeCache[0];
	for (i=0; i < cdfs_InodeCnt; i++, ip++) {
		cdfs_InitInode(ip);
		cdfs_IputFree(ip);
	}

	/*
	 * Initialize the Inode Hash Table.
	 */
	ipp = &cdfs_InodeHash[0];
	for (i=0; i < cdfs_IhashCnt; i++, ipp++) {
		*ipp = NULL;
	}

	/*
	 * Initialize all of the structures in the Multi-extent
	 * Dir Rec cache and add them to the free list.
	 */
	cdfs_DrecFree = NULL;
	drec = &cdfs_DrecCache[0];
	for (i=0; i < cdfs_DrecCnt; i++, drec++) {
		cdfs_DrecPut(&cdfs_DrecFree, drec);
	}

	return(RET_OK);
}




/*
 * Mount a CDFS file system. 
 */
STATIC int
cdfs_mount(vfsp, mvp, uap, cr)
struct vfs *vfsp;							/* VFS struct to be used		*/
struct vnode *mvp;							/* Vnode of mount-point			*/
struct mounta *uap;							/* Mount arguments from mount(2)*/
struct cred *cr;							/* User's credential struct		*/
{
	struct vnode		*devvp;				/* Vnode of the device-node		*/
	dev_t				devnum;				/* Device # (Maj/Min) of device */

	struct cdfs		 	*cdfs;				/* Private VFS data: CDFS struct*/
	struct cdfs_mntargs	mntargs;			/* Mount flags from CDFS-mount()*/	
	boolean_t 			mntargs_valid;		/* Indicates validity of mntargs*/

	int 				retval;				/* Return value of called routines*/

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Validate pre-conditions:
	 * - Mount-point is a directory.
	 * - Reference count of mount-point must be 1 (Except REMOUNT).
	 * - Mount-point is not 'root' of another file-system (Except REMOUNT).
	 */
	ASSERT(pm_denied(cr, P_FILESYS) == 0);

	if (mvp->v_type != VDIR) {
		return(ENOTDIR);
	}
	if ((uap->flags & MS_REMOUNT) == 0) {
		if ((mvp->v_count != 1) ||
			(mvp->v_flag & VROOT)) {
			return(EBUSY);
		}
	}

	/*
	 * Get the device # (major/minor) of the device being mounted.
	 * - Get and verify the device's Vnode via its pathname.
	 * - Get and verify the device number from the Vnode.
	 * - Release the Vnode.
	 */
	retval = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &devvp);
	if (retval != 0) {
		return(retval);
	}

	if (devvp->v_type != VBLK) {
		VN_RELE(devvp);
		return(ENOTBLK);
	}

	devnum = devvp->v_rdev;	
	if (getmajor(devnum) >= bdevcnt) {
		return(ENXIO);
	}
	VN_RELE(devvp);

	/*
	 * Verify that the state of the device (mounted v.s. unmounted)
	 * matches the type of mount request (REMOUNT v.s. non-REMOUNT).
	 * - Search the current list of allocated VFS structures to see
	 *   if the device is being referenced (i.e. mounted).
	 * - If its already mounted, this had better be a REMOUNT request.
	 */
	if (vfs_devsearch(devnum) == NULL) {
		if ((uap->flags & MS_REMOUNT) == 0) {
			vfsp->vfs_flag &= ~VFS_REMOUNT;
		} else {
			return(EINVAL);
		}
	} else {
		if ((uap->flags & MS_REMOUNT) != 0) {
			vfsp->vfs_flag |= VFS_REMOUNT;
		} else {
			return(EBUSY);
		}
	}

	/*
	 * Mount the file system as Read-Only if appropriate:
	 * - If specified by user.
	 * - If the device is a tape.
	 */
	if ((uap->flags & MS_RDONLY) ||
		((*bdevsw[getmajor(devnum)].d_flag & D_TAPE) == D_TAPE)) {
		vfsp->vfs_flag |= VFS_RDONLY;
	}

	/*
	 * Get the CDFS-specific mount arguments passed in from
	 * the CDFS-specific mount(1M) command.
	 */
	if (((uap->flags & MS_DATA) != 0) &&
		(uap->dataptr != NULL) &&
		(uap->datalen >= sizeof(mntargs)) &&
		(copyin(uap->dataptr, (caddr_t)&mntargs, sizeof(mntargs)) == 0)) {;
		mntargs_valid = B_TRUE;
	} else {
		mntargs_valid = B_FALSE;
	}

	/*
	 * Mount the file system.
	 */
	if (mntargs_valid == B_TRUE) {
		retval = cdfs_mountfs(vfsp, devnum, &mntargs, cr);
	} else {
		retval = cdfs_mountfs(vfsp, devnum, NULL, cr);
	}
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	/*
	 * Get and store mount-point and device node pathnames.
	 */
	cdfs = (struct cdfs *)vfsp->vfs_data;

	retval = pn_get(uap->dir, UIO_USERSPACE, &cdfs->cdfs_MntPnt);
	if (retval != 0) {
		cmn_err(CE_NOTE,
			"cdfs_mount(): Unable to obtain pathname of mount-point.\n");
		cdfs->cdfs_MntPnt.pn_buf[0] = '\0';
		cdfs->cdfs_MntPnt.pn_pathlen = 0;
	}

	retval = pn_get(uap->spec, UIO_USERSPACE, &cdfs->cdfs_DevNode);
	if (retval != 0) {
		cmn_err(CE_NOTE,
			"cdfs_mount(): Unable to obtain pathname of device-file.\n");
		cdfs->cdfs_DevNode.pn_buf[0] = '\0';
		cdfs->cdfs_DevNode.pn_pathlen = 0;
	}

	return(0);
}





/*
 * Mount a CDFS file system as the system's root file system.
 *
 * The "why" argument specified the type of operation to be
 * performed on the root file system:
 * - ROOT_INIT: Initial mount request.
 * - ROOT_REMOUNT: Remount request following the initial root.
 * - ROOT_UNMOUNT: Unmount the file system (e.g. system shutdown).
 *
 * XXX - this may be partially machine-dependent; it, along with the VFS_SWAPVP
 * operation, goes along with auto-configuration.  A mechanism should be
 * provided by which machine-INdependent code in the kernel can say "get me the
 * right root file system" and "get me the right initial swap area", and have
 * that done in what may well be a machine-dependent fashion.
 * Unfortunately, it is also file-system-type dependent (NFS gets it via
 * bootparams calls, CDFS gets it from various and sundry machine-dependent
 * mechanisms, as SPECFS does for swap).
 */
STATIC int
cdfs_mountroot(vfsp, why)
struct vfs *vfsp;							/* VFS structure				*/
enum whymountroot why;						/* Mount operation requested	*/
{
	/*
	 * WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING
	 *
	 * This routine has NOT BEEN TESTED!!  Due to testing limitations,
	 * this code is a BEST GUESS as to what needs to be done.  The code
	 * has NEVER BEEN EXECUTED.  For this feature to be operational,
	 * some amount of debugging and fixing will be necessary.
	 */ 

	static int cdfsrootdone = 0;

	struct cdfs 	*cdfs;				/* VFS private data: CDFS struct	*/
	int				retval;				/* Return value of various calls	*/	

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	switch (why) {
		case ROOT_INIT: {
			/*
			 * Initial mount of the system's root file system.
			 * - Make sure this is the 1st call.
			 * - Verify and store the device number of the device
			 *   being mounted.
			if (rootdone != 0) {
				return (EBUSY);
			}
			 */
			if (rootdev == (dev_t)NODEV) {
				return (ENODEV);
			}
			CDFS_DEV(vfsp) = rootdev;
			break;
		}
		case ROOT_REMOUNT: {
			/*
			 * Remount of the system's root file system.
			 * - Verify that the CDFS is not active.
			 * - Set the REMOUNT flag.
			fsp = getfs(vfsp);
			if (fsp->fs_state == FSACTIVE)
				return(EINVAL);
			 */
			vfsp->vfs_flag |= VFS_REMOUNT;
			break;
		}
		case ROOT_UNMOUNT: {
			/*
			 * Unmount the root file system.
			 */

			/*
			 * XXX - CDFS WRITE SUPPORT:
			 * - Update media with in-core data structures.
			 *
			 * Note: This 'rootvp' is a system global variable that
			 * stores the address of the Vnode for the device that
			 * holds the system's root directory.  This is not the
			 * Vnode for the root of the CDFS file system.
			 *
			 * Original code:
			vp = ((struct cdfs *)vfsp->vfs_data)->vfs_devvp;
			(void) VOP_CLOSE(vp, FREAD, 1, (off_t)0, u.u_cred);
			VN_RELE(vp);
			*/
			cdfs_unmountfs(vfsp, u.u_cred); 
			return(0);
		}
		default: {
			return(EINVAL);
		}
	}

	/*
	 * Lock VFS while mounting the file system.
	 */
	retval = vfs_lock(vfsp);
	if (retval != 0) {
		return(retval);
	}

	/*
	 * Mount the file system stored on the specified device.
	 */
	retval = cdfs_mountfs(vfsp, rootdev, NULL, u.u_cred);
	if (retval != RET_OK) {
		vfs_unlock(vfsp);
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}
	cdfs = (struct cdfs *)vfsp->vfs_data;

	/*
	 * XXX - Was in orginal code, which had 'rootvp' set in mountfs().
	 * Don't think this should be here, since the release would have
	 * happened during the cleanup of a detected error.
	 *
	 * Note: This 'rootvp' is a system global variable that stores the
	 * address of the Vnode for the device that holds the system's root
	 * directory.  This is not the Vnode for the root of the CDFS file system.
	 * 
	if (rootvp != NULL) {
		VN_RELE(rootvp);
		rootvp = (struct vnode *) NULL;
	}
	 */

	/*
	 * Set the pathname of the mount-point.
	 * The pathname for the device node is unknown.
	 */
	retval = pn_set(&cdfs->cdfs_MntPnt, "/");
	if (retval != 0) {
		cmn_err(CE_WARN,
			"cdfs_mountroot(): Unable to set pathname of mount-point.\n");
		cdfs->cdfs_MntPnt.pn_buf[0] = '\0';
		cdfs->cdfs_MntPnt.pn_pathlen = 0;
	}

	cdfs->cdfs_DevNode.pn_buf = NULL;
	cdfs->cdfs_DevNode.pn_path = NULL;
	cdfs->cdfs_DevNode.pn_pathlen = 0;

	/*
	 * Add the VFS to the active list.
	 */
	vfs_add((struct vnode *)NULL, vfsp, MS_RDONLY);
	vfs_unlock(vfsp);

	/*
	 * Set the system's time to the date of the CD-ROM.
	 * Hopefully, some other sub-system will set the system time
	 * to something more reasonable.
	 *
	 * XXX - Since CDFS times are static, it may not be valid to
	 * make this call.
	 * XXX - 'clkset' should be run at SPL5 per 'clkset' comment.
	 */
	clkset(cdfs->cdfs_EffectDate);
	/* rootdone = 1; */
	return(0);
}



/*
 * Mount the file system stored on the device.
 * - Open the device.
 * - Get and process the super-block.
 * - Initialize the VFS and CDFS structures
 * - Get the Vnode for the file system root directory. 
 */
STATIC int
cdfs_mountfs(vfsp, devnum, mntargs, cr)
struct vfs			*vfsp;					/* VFS struct of file system	*/
dev_t				devnum;					/* Device # (Maj/Min) of device */
struct cdfs_mntargs	*mntargs;				/* CDFS-specific mount arguments*/
struct cred			*cr;					/* User's credential structure	*/
{
	struct vnode		*devvp;				/* Vnode of device to be mounted*/

	struct cdfs			*cdfs;				/* CDFS structure				*/
	struct cdfs_iobuf	pvd_buf;			/* PVD I/O buffer structure		*/
	struct cdfs_iobuf	drec_buf;			/* Dir Rec I/O buffer structure	*/
	struct cdfs_fid		pvd_fid;			/* PVD Root-Dir-Rec FID			*/
	struct cdfs_inode	*pvd_ip;			/* PVD Root-Dir-Rec Inode		*/
	struct cdfs_inode	*root_ip;	 		/* Root Inode					*/

	u_int				secsize;			/* Logical sector size			*/
	enum cdfs_type		fstype;				/* CDFS type: ISO_9660/HiSierra	*/
	int					cleanup_cnt;		/* # of items to be cleaned up 	*/
	u_int				count;				/* Miscellanious counter.		*/
	int					retval;				/* Return value for various calls*/
	
	/*
	 * Initialize allocated resources in order to aid
	 * in the cleanup task following an error.
	 */
	cleanup_cnt = 0;

	/*
	 * Get and verify the Vnode for the device being mounted.
	 * If this is a REMOUNT, the Vnode is already known.
	 */
	if ((vfsp->vfs_flag & VFS_REMOUNT) == 0) {
		/*
		 * This is an initial mount request (not a REMOUNT) so the
		 * device must be opened and verified:
		 * - Build a Vnode for the device using the device number. 
		 * - Open the device.
		 * - Verify that the device is not being used for SWAP space.
		 * - Invalidate all pages associated with the device.
		 *
		 * Note: Even though the Vnode for the device was obtained in
		 * cdfs_mount(), this routine must be able to build a Vnode
		 * using only the device number.  The cdfs_mountroot() routine,
		 * which also calles this routine, only has the device number of
		 * the root device, there is no pathname to pass to 'lookupname()'.
		 *
		 * XXX - When bio is fixed for vnodes, this can all be vnode operations.
		 */
		devvp = makespecvp(devnum, VBLK);
		cleanup_cnt++;

		retval = VOP_OPEN(&devvp, FREAD, cr);
		if (retval != 0) {
			cmn_err(CE_WARN,
				"cdfs_mountfs(): Unable to open device (maj=%d, min=%d)\n",
				getmajor(devnum), getminor(devnum));
			goto Cleanup;
		}
		cleanup_cnt++;

		/*
		 * If the device is being used for swap space,
		 * something is really screwed-up.
		 */
		if (IS_SWAPVP(devvp)) {
			cmn_err(CE_WARN,
				"cdfs_mountfs(): Device currently used as a swap device.\n");
			retval = EBUSY;
			goto Cleanup;
		}

		binval(devnum);
	} else {
		/*
		 * REMOUNT NOT SUPPORTED:
		 * - Get the device's Vnode from the VFS structure.
		 * - Flush current data structures to the device.
		 * - Release previously allocated storage.
		 * - Incr cleanup_cnt so that its value is the same regardless
		 *	 of which branch of the if-then-else was taken.
		 *	
		 */
		devvp = ((struct cdfs *)vfsp->vfs_data)->cdfs_DevVnode;
		cleanup_cnt += 2;
	}

	/*
	 * Verify that the necessary memory for the CDFS module has
	 * already been allocated.
	 */
	if (cdfs_MemCnt == 0) {
		retval = cdfs_AllocMem();
		if (retval != RET_OK) {
			cmn_err(CE_WARN,
				"cdfs_mountfs(): CDFS initialization failed.\n");
			if (retval < RET_OK) {
				retval = ENOMEM;
			}
			goto Cleanup;
		}
	}
	cdfs_MemCnt++;
	cleanup_cnt++;

	/*
	 * Initialize the VFS and CDFS structures with the currently
	 * known data.
	 * - Allocate memory for the CDFS structure.
	 * - Init the known VFS fields.
	 * - Init the know CDFS fields.
	 * 
	 * NOTE: This is done early, so that some of the lower-layer,
	 * which depend on certain VFS and CDFS values, can be used
	 * at this point in time.
	 */
	cdfs = (struct cdfs *)kmem_zalloc(sizeof(struct cdfs), KM_SLEEP);
	if (cdfs == NULL) {
			cmn_err(CE_WARN,
				"cdfs_mountfs(): Unable to allocate CDFS structure memory.\n");
		retval = ENOMEM;
		goto Cleanup;
	}
	cleanup_cnt++;

	vfsp->vfs_dev = devnum;
	vfsp->vfs_fsid.val[0] = devnum;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_flag |= VFS_NOTRUNC;
	vfsp->vfs_data = (caddr_t) cdfs;
	vfsp->vfs_bcount = 0;
	
	if (mntargs != NULL) {
		cdfs->cdfs_Flags = mntargs->mnt_Flags & CDFS_ALL_FLAGS;
	} else {
		cdfs->cdfs_Flags = CDFS_SUSP | CDFS_RRIP;
	}
	cdfs->cdfs_DevVnode = devvp;

	/*
	 * Initialize XCDR and RRIP default and mapping structures.
	 */
	cdfs->cdfs_Dflts.def_uid = cdfs_InitialUID;
	cdfs->cdfs_Dflts.def_gid = cdfs_InitialGID;
	cdfs->cdfs_Dflts.def_fperm = cdfs_InitialFPerm;
	cdfs->cdfs_Dflts.def_dperm = cdfs_InitialDPerm;
	cdfs->cdfs_Dflts.dirsperm = cdfs_InitialDirSearch;
	cdfs->cdfs_NameConv = cdfs_InitialNmConv;
	for (count = 0; count < CD_MAXUMAP; count ++) {
		cdfs->cdfs_UidMap[count].from_uid = CDFS_UNUSED_MAP_ENTRY;
		cdfs->cdfs_UidMap[count].to_uid = CDFS_UNUSED_MAP_ENTRY;
	}
	cdfs->cdfs_UidMapCnt = 0;

	for (count = 0; count < CD_MAXGMAP; count ++) {
		cdfs->cdfs_GidMap[count].from_gid = CDFS_UNUSED_MAP_ENTRY;
		cdfs->cdfs_GidMap[count].to_gid = CDFS_UNUSED_MAP_ENTRY;
	}
	cdfs->cdfs_GidMapCnt = 0;

	for (count = 0; count < CD_MAXDMAP; count ++) {
		cdfs->cdfs_DevMap[count].fileid = CDFS_NULLFID;
		cdfs->cdfs_DevMap[count].to_num = NODEV;
	}
	cdfs->cdfs_DevMapCnt = 0;

	/*
	 * Get the Logical Sector size for this CDFS.
	 * - If the user specified it on the command line, then use that value.
	 * - Otherwise, it has to be computed.
	 * - Validate the Logical Sector size.
	 */
	if ((mntargs != NULL) &&
		((mntargs->mnt_Flags & CDFS_USER_BLKSZ) != 0)) {
		secsize = mntargs->mnt_LogSecSz;
	} else {
		retval = cdfs_GetSectSize(devnum, &secsize);
		if (retval != RET_OK) {
			cmn_err(CE_WARN,
				"cdfs_mountfs(): Unable to determine logical sector size of media.\n");
			if (retval < RET_OK) {
				retval = EINVAL;
			}
			goto Cleanup;
		}
	}

	if (secsize > MAXBSIZE) {
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Invalid Logical Sector: 0x%x", secsize);
		cmn_err(CE_CONT,
			"Maximum logical sector size is 0x%x\n\n", MAXBSIZE);
		retval = EINVAL;
		goto Cleanup;
	}

	cdfs->cdfs_LogSecSz = secsize;
	cdfs->cdfs_LogSecMask = secsize-1;
	for (count=0; (ulong_t)(1 << count) <= (ulong_t)MAXBSIZE; count++) {
		if ((1 << count) == secsize) {
			break;
		}; 
	}

	if ((1 << count) != secsize) {
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Invalid Logical Sector size: 0x%x", secsize);
		cmn_err(CE_CONT,
			"Not an integral power-of-two multiple\n\n");
		retval = EINVAL;
		goto Cleanup;
	}
	cdfs->cdfs_LogSecShift = count;
		
	/*
	 * Read the Primary Volume Descriptor (Super-Block) from the device.
	 * - Setup the PVD I/O Buffer.
	 */
	CDFS_SETUP_IOBUF(&pvd_buf, CDFS_BUFIO);
	pvd_buf.sb_dev = CDFS_DEV(vfsp);
	pvd_buf.sb_sect = ISO_VD_LOC;
	cleanup_cnt++;

	retval = cdfs_ReadPvd(vfsp, &pvd_buf, &fstype);
	if (retval != RET_OK) {
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Unable to locate Primary Volume Descriptor.\n");
		if (retval < RET_OK) {
			retval = EIO;
		}
		goto Cleanup;
	}

	cdfs->cdfs_PvdLoc = pvd_buf.sb_sect;
	cdfs->cdfs_Type = fstype;

	retval = cdfs_ConvertPvd(cdfs, (union media_pvd *)pvd_buf.sb_ptr, fstype);
	if (retval != RET_OK) {
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Invalid Primary Volume Descriptor.\n");
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		goto Cleanup;
	}

	vfsp->vfs_bsize = cdfs->cdfs_LogBlkSz;

	cdfs->cdfs_LogBlkMask = cdfs->cdfs_LogBlkSz - 1;
	for (count=0; (ulong_t)(1 << count) <= (ulong_t)MAXBSIZE; count++) {
		if ((1 << count) == cdfs->cdfs_LogBlkSz) {
			break;
		}; 
	}

	if ((1 << count) != cdfs->cdfs_LogBlkSz) {
		cmn_err(CE_WARN,
			"cdfs_mount(): Invalid Logical Sector size: 0x%x", secsize);
		cmn_err(CE_CONT,
			"Not an integral power-of-two multiple\n\n");
		retval = EINVAL;
		goto Cleanup;
	}
	cdfs->cdfs_LogBlkShift = count;
		
	/*
	 * Setup the FID for the Root Inode.
	 */
	pvd_fid.fid_SectNum = pvd_buf.sb_sect;
	pvd_fid.fid_Offset = pvd_buf.sb_offset + cdfs->cdfs_RootDirOff;

	CDFS_SETUP_IOBUF(&drec_buf, CDFS_BUFIO);
	drec_buf.sb_ptr = pvd_buf.sb_ptr + cdfs->cdfs_RootDirOff;
	drec_buf.sb_offset = pvd_buf.sb_offset + cdfs->cdfs_RootDirOff;
	drec_buf.sb_sect = pvd_buf.sb_sect;
	drec_buf.sb_sectoff = pvd_buf.sb_sectoff;
	drec_buf.sb_start = drec_buf.sb_ptr;
	drec_buf.sb_end = drec_buf.sb_ptr + cdfs->cdfs_RootDirSz;
	cleanup_cnt++;

	CDFS_FLAGS(vfsp) &= ~(CDFS_SUSP_PRESENT | CDFS_RRIP_ACTIVE);
	CDFS_FLAGS(vfsp) |= CDFS_BUILDING_ROOT;

	retval = cdfs_GetInode(vfsp, &pvd_fid, &drec_buf, &pvd_ip);
	if (retval != RET_OK) {
		cmn_err(CE_WARN,
			"cdfs_mount(): Unable to build inode from PVD Dir Rec.\n");
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		goto Cleanup;
	}
	cleanup_cnt++;

	/*
	 * If requested by the user, try to locate the SUSP and/or RRIP
	 * extensions within the Root Dir Rec.
	 */
	CDFS_FLAGS(vfsp) &= ~CDFS_SUSP_PRESENT;
	CDFS_SUSPOFF(vfsp) = 0;

	if (((CDFS_FLAGS(vfsp) & CDFS_SUSP) != 0) &&
		((CDFS_FLAGS(vfsp) & CDFS_RRIP) != 0)) {
		CDFS_FLAGS(vfsp) |= CDFS_RRIP_ACTIVE;
	}
		
	retval = cdfs_DirLookup(vfsp, pvd_ip, (uchar_t *)CDFS_DOT, &root_ip, cr);
	if (retval != RET_OK) {
		cmn_err(CE_WARN, "cdfs_mount(): Unable to build Root Inode.\n");
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		goto Cleanup;
	}
	cleanup_cnt++;
	
	root_ip->i_ParentFid = root_ip->i_Fid;
	ITOV(root_ip)->v_flag |= VROOT;

	cdfs->cdfs_RootInode = root_ip;
	cdfs->cdfs_RootFid = root_ip->i_Fid; 

	CDFS_FLAGS(vfsp) &= ~(CDFS_BUILDING_ROOT);

	/*
	 * If the RRIP extensions do not exist for the Root Dir Rec,
	 * then don't bother looking for them in other Dir Recs.
	 */
	if ((CDFS_FLAGS(vfsp) & CDFS_RRIP_PRESENT) == 0) {
		CDFS_FLAGS(vfsp) &= ~(CDFS_RRIP_ACTIVE);
	}

	/*
	 * Since we're done, we can release SOME of the allocated resources.
	 * - Unlock and release the Inode built from the PVD.
	 * - Unlock the Root Inode.  The Root Inode is not released
	 *	 so that cdfs_root() can just use it without having to
	 *	 look it up and/or build it on every call.  The Root Inode
	 *	 is released by cdfs_unmountfs().
	 * - Release the Dir Rec and PVD I/O buffers.
	 */
	cdfs_UnlockInode(pvd_ip);
	VN_RELE(ITOV(pvd_ip));

	cdfs_UnlockInode(root_ip);

	CDFS_RELEASE_IOBUF(&drec_buf);
	CDFS_RELEASE_IOBUF(&pvd_buf);

	return(RET_OK);

Cleanup:
	switch (cleanup_cnt) {
		case 8: {
			/*
			 * Unlock and release the Root Inode.
			 */
			cdfs_UnlockInode(root_ip);
			VN_RELE(ITOV(root_ip));
		} /* FALLTHRU */

		case 7: {
			/*
			 * Unlock and release the PVD Inode.
			 */
			cdfs_UnlockInode(pvd_ip);
			VN_RELE(ITOV(pvd_ip));
		} /* FALLTHRU */

		case 6: {
			/*
			 * Release the Dir Rec I/O buffer structure.
			 */
			CDFS_RELEASE_IOBUF(&drec_buf);
		} /* FALLTHRU */

		case 5: {
			/*
			 * Release the PVD I/O buffer structure.
			 */
			CDFS_RELEASE_IOBUF(&pvd_buf);
		} /* FALLTHRU */

		case 4: {
			/*
			 * Release the CDFS data structure memory.
			 */
			kmem_free((caddr_t)cdfs, sizeof(struct cdfs));
		} /* FALLTHRU */

		case 3: {
			/*
			 * Decrement the reference count for the CDFS memory,
			 * and release it if this are no other active CDFS mounts.
			 */ 
			cdfs_MemCnt--;
			if (cdfs_MemCnt == 0) {
				kmem_free((caddr_t)cdfs_Mem, cdfs_MemSz);
				cdfs_MemSz = 0;
				cdfs_Mem = NULL;
				cdfs_InodeCache = NULL;
				cdfs_InodeHash = NULL;
				cdfs_DrecCache = NULL;
				cdfs_InodeFree = NULL;
			}
		} /* FALLTHRU */

		case 2: {
			/*
			 * Close the device and invalidate its buf headers.
			 * Note: Don't do this on a REMOUNT.
			 */
			if ((vfsp->vfs_flag & VFS_REMOUNT) == 0) {
				(void) VOP_CLOSE(devvp, FREAD, 1, 0, cr);
				binval(devnum);
			}
		} /* FALLTHRU */

		case 1: {
			/*
			 * Release the Vnode of the device.
			 * Note: Don't do this on a REMOUNT.
			 */
			if ((vfsp->vfs_flag & VFS_REMOUNT) == 0) {
				VN_RELE(devvp);
			}
		} /* FALLTHRU */

		case 0: {
			break;
		}

		default: {
			/*
			 * Unknown cleanup level so just leave things alone.
			 */
			cmn_err(CE_WARN,
				"cdfs_mountfs(): Unknown cleanup level (%d)", cleanup_cnt);
			cmn_err(CE_NOTE,
				"No cleanup action taken.\n\n");
		} /* FALLTHRU */
	}

	return(retval);
}




int
cdfs_unmount(vfsp, cr)
struct vfs	*vfsp;							/* VFS struct of file system	*/
struct cred	*cr;							/* crential structure			*/
{
	int		retval;							/* Return value of various calles*/

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	retval = cdfs_unmountfs(vfsp, cr);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	return(0);
}





/*
 * Unmount a CDFS file system.
 */
STATIC int
cdfs_unmountfs(vfsp, cr)
struct vfs	*vfsp;							/* VFS struct of file system	*/
struct cred	*cr;							/* crential structure			*/
{
	struct cdfs			*cdfs;				/* CDFS struct of file system	*/
	dev_t				devnum;				/* Device # (major/minor)		*/
	struct vnode		*devvp;				/* 'specfs' Vnode of device		*/
	struct vnode		*rootvp;			/* Vnode of Root directory		*/
	struct cdfs_inode	*rootip;			/* Inode of Root directory		*/
	int					retval;				/* Return value of various calles*/

	ASSERT(pm_denied(cr, P_FILESYS) == 0);

	/*
	 * Free all inactive Inodes/Vnodes associated with the device.
	 * If an Inode is still active, the device can not be unmounted.
	 */
	retval = cdfs_FlushInodes(vfsp);
	if (retval != RET_OK) {
		return(EBUSY);
	}

	rootvp = CDFS_ROOT(vfsp);
	cdfs = (struct cdfs *)vfsp->vfs_data;
	devvp = cdfs->cdfs_DevVnode;

	/*
	 * Release and cleanup the Root Inode and put it
	 * at the head of the Free list.
	 * Note: Lock is cleared by cdfs_CleanInode().
	 */
	ASSERT(rootvp->v_count == 1);
	VN_RELE(rootvp);
	
	rootip = VTOI(rootvp);
	ASSERT((rootip->i_Flags & (IREF|ILOCKED|IRWLOCKED|IWANT)) == 0);
	ASSERT(rootvp->v_count == 0);

	cdfs_LockInode(rootip);
	cdfs_CleanInode(rootip);
	cdfs_IputFree(rootip);
	cdfs_InodeFree = rootip;

	/*
	 * Release the memory allocated for the CDFS structure.
	 */
	if (cdfs->cdfs_MntPnt.pn_buf != NULL) {
		pn_free(&cdfs->cdfs_MntPnt);
	}

	if (cdfs->cdfs_DevNode.pn_buf != NULL) {
		pn_free(&cdfs->cdfs_DevNode);
	}

	kmem_free((caddr_t)cdfs, sizeof(struct cdfs));
	vfsp->vfs_data = NULL;

	/*
	 * Close the device, invalidate the pages associated with the
	 * device and, release the Vnode for the device.
	 */
	devnum = CDFS_DEV(vfsp);
	(void) VOP_CLOSE(devvp, FREAD, 1, 0, cr);
	binval(devnum);
	VN_RELE(devvp);

	/*
	 * Decrement Inode pool reference count and
	 * release the memory if no longer needed.
	 */
	cdfs_MemCnt--;
	if (cdfs_MemCnt == 0) {
		kmem_free((caddr_t)cdfs_Mem, cdfs_MemSz);
		cdfs_MemSz = 0;
		cdfs_Mem = NULL;
		cdfs_InodeCache = NULL;
		cdfs_InodeHash = NULL;
		cdfs_DrecCache = NULL;
		cdfs_InodeFree = NULL;
	}

	return(RET_OK);
}




/*
 * Get the Vnode of the file system's root directory.
 */
STATIC int
cdfs_root(vfsp, vpp)
struct vfs		*vfsp;						/* VFS strucut of file system	*/
struct vnode	**vpp;						/* Addr of Vnode Pntr to be set */
{
	struct cdfs_inode	*rootip; 

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Get the root Vnode from the CDFS structure.
	 * Also, make sure the Vnode is not released.
	 */
	rootip = CDFS_ROOT_INODE(vfsp);
	*vpp = ITOV(rootip);
	VN_HOLD(*vpp);

	return(0);
}




/*
 * Get various file system statistics.
 */
STATIC int
cdfs_statvfs(vfsp, sp)
struct vfs		*vfsp;						/* File system's VFS struct		*/
struct statvfs	*sp;						/* Generic FS statistics struct */
{
	struct cdfs *cdfs;						/* File system's CDFS struct	*/
	int			cnt;						/* Byte count for string copy	*/
	
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Clear all fields in the generic file-system statistics structure.
	 */
	(void)bzero((caddr_t)sp, (int)sizeof(*sp));

	/*
	 * Locate and validate the CDFS structure.
	 */
	cdfs = (struct cdfs *) vfsp->vfs_data;
	if ((cdfs->cdfs_Type != CDFS_ISO_9660) &&
		(cdfs->cdfs_Type != CDFS_HIGH_SIERRA)) {
		return (EINVAL);
	}

	/*
	 * Specify various file system  block counts:
	 * - Block size in bytes.
	 * - Fragment size.
	 * - Total # of blocks.
	 * - Total # of free blocks.
	 * - Total # of free block for non-privaledged users. 
	 */
	sp->f_bsize = CDFS_SECTSZ(vfsp);
	sp->f_frsize = CDFS_BLKSZ(vfsp);
	sp->f_blocks = cdfs->cdfs_VolSpaceSz;
	sp->f_bfree = 0; 
	sp->f_bavail = 0;

	/*
	 * Specify various Inode data:
	 * - Total # of inodes.
	 * - Total # of free Inode.
	 * - Total # of free Inodes available for non-privaliged users.
	 */
	sp->f_files = -1;
	sp->f_ffree = -1;
	sp->f_favail = -1;

	/*
	 * Other miscellaneous data:
	 * - File system ID.
	 * - Flags.
	 * - File system name.
	 * - Maximum length of a file name.
	 * - Volume (file sysetm) ID string.
	 */
	sp->f_fsid = CDFS_DEV(vfsp);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);

	cnt = sizeof(sp->f_basetype) - 1;
	(void) strncpy(&sp->f_basetype[0], vfssw[vfsp->vfs_fstype].vsw_name, cnt);
	sp->f_basetype[cnt] = '\0';

	if ((CDFS_FLAGS(vfsp) & CDFS_RRIP_ACTIVE) == 0) {
		sp->f_namemax = CDFS_MAX_NAME_LEN;
	} else {
		sp->f_namemax = MAXNAMELEN - 1;
	}

	cnt = MIN(sizeof(sp->f_fstr), sizeof(cdfs->cdfs_VolID)) - 1;
	(void) strncpy(&sp->f_fstr[0], (caddr_t)&cdfs->cdfs_VolID[0], cnt);
	sp->f_fstr[cnt] = '\0';

	return(0);
}




/*
 * Synchronize media with in-core data structures.
 * Note: CDFS is currently Read-Only, so this is successful NO-OP.
 */
/* ARGSUSED */
STATIC int
cdfs_sync(vfsp, flag, cr)
struct vfs	*vfsp;							/* VFS struct of the file system*/
int			flag;							/* Sync flags					*/
struct cred	*cr;							/* crential structure			*/
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	return(0);								/* Always Read-Only: Nothing to do*/
}



/*
 * Find the Vnode associated with the unique file ID structure.
 */
STATIC int
cdfs_vget(vfsp, vpp, fidp)
struct vfs *vfsp;							/* VFS struct of file system	*/
struct vnode *(*vpp);						/* Return addr for Vnode pointer*/
struct fid *fidp;							/* A unique file ID structure	*/
{
	struct cdfs_fid		*cdfs_fid;			/* Unique CDFS file ID structure*/
	struct cdfs_inode	*ip;
	int					retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Get the CDFS FID from the generic FID structure.
	 */
	cdfs_fid = (struct cdfs_fid *) &(fidp->fid_data[0]);

	/*
	 * Locate the Inode that corresponds to the unique file ID.
	 */
	retval = cdfs_GetInode(vfsp, cdfs_fid, NULL, &ip);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		*vpp = NULL;
		return(retval);
	}

	cdfs_UnlockInode(ip);
	*vpp = ITOV(ip);
	return(0);
}
