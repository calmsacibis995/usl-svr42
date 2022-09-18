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

#ident	"@(#)uts-comm:fs/cdfs/cdfs_inode.c	1.10"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/iso9660.h>
#include <fs/dnlc.h>
#include <fs/fs_subr.h>
#include <fs/mode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#if ((defined CDFS_DEBUG)  && (!defined DEBUG))
#define		DEBUG	YES
#include	<util/debug.h>
#undef		DEBUG
#else
#include	<util/debug.h>
#endif
#include <util/param.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include "cdfs.h"

extern	struct vnodeops		cdfs_vnodeops;


/*
 * Initialize an Inode structure.
 */
int
cdfs_InitInode(ip)
struct cdfs_inode	*ip;
{
	/*
	 * Set all Inode fields to zero/NULL, except
	 * for the fields that require special values.
	 */
	bzero((caddr_t)ip, sizeof(*ip)); 
	ip->i_Fid = CDFS_NULLFID;
	ip->i_ParentFid = CDFS_NULLFID;
	ip->i_DevNum = NODEV;
	ip->i_Vnode = &ip->i_VnodeStorage;
	return(RET_OK);
}




/*
 * Clean up an Inode structure for reuse.
 * Note: Removes Inode from Free list and Hash Table.
 */
void
cdfs_CleanInode(ip)
struct cdfs_inode	*ip;
{
	struct vnode		*vp;
	struct cdfs_drec	*drec;
	struct cdfs_xar		*xar;
	struct cdfs_rrip	*rrip;
	struct pathname		*pn;
	

	vp = ITOV(ip);

	ASSERT((ip->i_Flags & (ILOCKED|IRWLOCKED)) == ILOCKED);
	ASSERT(ip->i_LockOwner == curproc->p_slot);
	ASSERT(vp->v_count == 0); 

	/*
	 * Make sure the Inode is removed from the Free list and Hash table.
	 */
	if ((ip->i_FreeFwd != NULL) ||
		(ip->i_FreeBack != NULL)) {
		cdfs_IrmFree(ip);
	}

	if ((ip->i_HashFwd != NULL) ||
		(ip->i_HashBack != NULL)) {
		cdfs_IrmHash(ip);
	}

	/*
	 * Now that the Inode is not on the Free list or Hash list, we
	 * Now no other process can obtain a reference to this Inode.
	 * Also, the only other existing reference may be from
	 * cdfs_FindInode() with the process waiting for the Inode lock
	 * to be released.  However, after the lock is released,
	 * cdfs_FindInode MUST recheck the hash table.  The recheck
	 * will fail, thus giving us the only valid reference to the Inode.
	 * Therefore, following the unlock, we can do anything to the 
	 * Inode without concern for hitting a race-condition.
	 */
	cdfs_UnlockInode(ip);

	/*
	 * Release vnode's data pages.
	 */
	if (vp->v_pages != NULL) {
		pvn_vptrunc(vp, 0, 0);
		ASSERT(vp->v_pages == NULL);
	}

	/*
	 * Release Inode's Dir Rec structures.
	 * Note: The first Dir Rec is allocated as part of the
	 * Inode struct so it doesn't get freed.
	 */
	if (ip->i_DirRec != NULL) {
		drec = ip->i_DirRec->drec_PrevDR;
		while (drec != ip->i_DirRec) {
			cdfs_DrecRm(&ip->i_DirRec, drec);
			cdfs_DrecPut(&cdfs_DrecFree, drec);
			drec = ip->i_DirRec->drec_PrevDR;
		}
		ip->i_DirRec = NULL;
	}

	/*
	 * Release Inode's XAR structure and its sub-structures.
	 * Note: Since the main structure is allocated as part of
	 * the Inode structure, it gets deallocated implicitely.
	 * However, all sub-structures need to be released explicitely.
	 *
	 * Note - Any allocated sub-structures of the XAR (e.g. Escape
	 * Sequence and/or Application Use Data buffers) need to
	 * be released if they are allocated.  Currently, they are
	 * not allocated.
	 */

	/*
	 * Release Inode's RRIP structure and its sub-structures.
	 * Note: Since the main structure is allocated as part of
	 * the Inode structure, it gets deallocated implicitely.
	 * However, all sub-structures need to be released explicitely.
	 */
	rrip = ip->i_Rrip;
	if (rrip != NULL) {
		pn = &rrip->rrip_SymLink;
		if (pn->pn_buf != NULL) {
			pn_free(pn);
		}

		pn = &rrip->rrip_AltName;
		if (pn->pn_buf != NULL) {
			pn_free(pn);
		}
	}

	/*
	 * Release Inode's VNODE structure and its sub-structures.
	 * Note: Since the main structure is allocated as part of
	 * the Inode structure, it gets deallocated implicitly.
	 * However, all sub-structures need to be released explicitely.
	 */
	cdfs_InitInode(ip);

	return;
}




/*
 * Get the Inode structure identified by the File ID structure.
 * The Inode is returned after being locked by the calling process.
 *
 * Note: Each file in a CDFS file system can be uniquely identified
 * by a File ID structure.  Using the specified File ID structure,
 * this routine locates or builds an Inode describing the file or dir.
 */
int
cdfs_GetInode(vfsp, fid, drec_buf, ipp)
struct vfs			*vfsp;					/* File system's VFS structure	*/
struct cdfs_fid		*fid;					/* Unique File ID of Inode		*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec (if set) of 1st D.Rec*/
struct cdfs_inode	**ipp;					/* Ret addr of Inode pointer	*/
{
	struct cdfs_inode	*ip;				/* Inode pntr to be returned	*/
	struct cdfs_inode	*tmp_ip;			/* Temporary Inode pntr			*/
	struct cdfs_fid 	tmp_fid;			/* Temporary FID				*/
	struct cdfs_iobuf	tmp_drec_buf;		/* Temp. Dir Rec buffer struct	*/
	int 				retval;				/* Return value of called procs	*/

	/*
	 * Update system accounting statistics.
	 */
	sysinfo.iget++;

	/*
	 * It may take many attempts before complete success or failure.
	 */
	for (;;) {
		/*
		 * Search the Inode cache for the desired Inode.
		 * If found, then return.
		 */
		retval = cdfs_FindInode(vfsp, fid, ipp);
		if (retval == RET_OK) {
			return(RET_OK);
		}

		/*
		 * Inode not in cache, so allocate a free one and
		 * build the one we need.
		 *
		 * WARNING: There are many scenareos that can cause a
		 * race-conditions.  In general, if the allocation
		 * operation allowed the process to be preempted, then
		 * there's a theoretical chance that the Inode were
		 * looking for was build and cached by another process.
		 * Therefore, if preemption may have occured, the returned
		 * Inode is put back on the Free-list and the cache is rechecked.
		 *
		 * Note: Return value indicates 1 of 3 conditions:
		 *	- Found an available Inode without sleeping.
		 *	- Found an available Inode but had to sleep, the one
		 *	  want may have been built while sleeping..
		 *	- Could not find an available inode.
		 *
		 * Note: Must not sleep between new Inode allocation
		 * and locking it and putting it in the Hash Table.
		 */
		retval = cdfs_AllocInode(&ip);
		if (retval == RET_OK) {
			break;
		} else if (retval == RET_SLEEP) {
			cdfs_IputFree(ip);
			cdfs_InodeFree = ip;
			continue;
		} else {
			return(retval);
		}
	}
				
	/*
	 * Register the empty Inode into cache as soon as possible
	 * especially be we're preempted.  Otherwise, there's a
	 * theoretical chance that some other process will build
	 * and cache the same Inode.
	 *
	 * - Mark and lock the Inode.
	 * - Fillin the Unique File ID structure.
	 * - Put it in the Hash Table.
	 *
	 * Note: Must not sleep between new Inode allocation
	 * and locking it/putting it in the Hash Table.
	 */
	ASSERT(ip->i_Flags == 0);
	ip->i_Flags = IREF;
	ip->i_Fid = *fid;
	cdfs_LockInode(ip);
	cdfs_IputHash(ip);

	/*
	 * If caller didn't pass in a Dir Rec buffer for the
	 * file/dir, we need to create the buffer ourselves.
	 */
	if (drec_buf == NULL) {
		/*
		 * Read the Dir Rec of the file/dir from the media.
		 *
		 * Note: Since we don't have access to the Inode/Vnode of
		 * the Parent directory, we have to do the I/O directly
		 * from the driver using the 'struct buf' (bp) interface.
		 */
		drec_buf = &tmp_drec_buf;
		CDFS_SETUP_IOBUF(drec_buf, CDFS_BUFIO);
		drec_buf->sb_dev = CDFS_DEV(vfsp);
		drec_buf->sb_sect = fid->fid_SectNum;
		drec_buf->sb_offset = fid->fid_Offset;
		retval = cdfs_ReadDrec(vfsp, drec_buf);
		if (retval != RET_OK) {
			goto Cleanup;
		}
	}
	
	/*
	 * Build the desired Inode using the allocated Inode stucture
	 * and the Dir Rec data from the media.
	 */
	retval = cdfs_BldInode(vfsp, drec_buf, ip);
	if (retval != RET_OK) {
		goto Cleanup;
	}

	/*
	 * If the Inode has been "relocated" (via a RRIP 'CL' or
	 * 'PL' SUF) then build a new Inode from the "relocated"
	 * data and fix things up with the desired FID.  Otherwise
	 * we "hold" on to the vnode we already have and return it.
	 *
	 * Note: A "relocated" inode/vnode is implicitly "locked and held"
	 * via the (recursive) call to cdfs_GetInode().
	 */
	if ((ip->i_Flags & CDFS_INODE_RRIP_REL) != 0) {

		tmp_fid.fid_SectNum = (ip->i_DirRec)->drec_ExtLoc;
		tmp_fid.fid_Offset = ISO_DOT_OFFSET;
		retval = cdfs_GetInode(vfsp, &tmp_fid, NULL, &tmp_ip);

		/*
		 * If the new Inode was successfully built, then
		 * abandon the original Inode and fixup the new Inode
		 * (including the Hash Table) with the desired FID.
		 *
		 * Note: Since we've gotten here, we are building
		 * the original Inode from scratch, which means that
		 * no one else is referencing it, except for someone
		 * sleeping on the inode lock, cdfs_LockInode(),
		 * via cdfs_FindInode().  Therefore, as long as
		 * cdfs_FindInode() verifies that the Inode's ID has
		 * not change while sleeping, then there is no
		 * problem shuffling Inode ID's and cleaning up
		 * the original Inode and shuffling
		 * around Inode ID's.
		 */
		if (retval == RET_OK) {
			cdfs_IrmHash(ip);
			cdfs_IrmHash(tmp_ip);

			tmp_ip->i_Fid = ip->i_Fid;
			tmp_ip->i_ParentFid = ip->i_ParentFid;
			cdfs_IputHash(tmp_ip);

			cdfs_CleanInode(ip);
			cdfs_IputFree(ip);
			cdfs_InodeFree = ip;
			ip = tmp_ip;
		}
	} else {
		VN_HOLD(ITOV(ip));
	}

Cleanup:
	/*
	 * If a temp IOBUF was allocated, then release it.
	 */
	if (drec_buf == &tmp_drec_buf) {
		CDFS_RELEASE_IOBUF(drec_buf);
	}

	/*
	 * The inode is not valid, so clean it up and
	 * put it at HEAD of Free list.
	 * Note: cdfs_CleanInode() unlocks the Inode and
	 * removes it from the Hash Table.
	 */
	if (retval != RET_OK) {
		cdfs_CleanInode(ip);
		cdfs_IputFree(ip);
		cdfs_InodeFree = ip;
		ip = NULL; 
	}

	*ipp = ip;
	return(retval);
}



/*
 * Search the Inode cache for a specific Inode.
 */
int
cdfs_FindInode(vfs, fid, ipp)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_fid		*fid;					/* File ID structure			*/
struct cdfs_inode	**ipp;					/* Ret addr of Inode pointer	*/
{
	struct cdfs_inode	*ip;				/* Temporary Inode pointer		*/
	struct cdfs_inode	*hash;				/* Head of Inode's Hash-list	*/

	/*
	 * Search the Inode cache for the desired Inode.
	 * Note: If the Inode if found, but is "busy" (i.e. locked),
	 * we need to sleep for a while then do another complete scan.
	 */
	for (;;) {
		/*
		 * Locate the Hash list that should contain the Inode.
		 */
		hash = cdfs_InodeHash[CDFS_INOHASH(fid)];
		if (hash == NULL) {
			return(RET_NOT_FOUND);
		}

		/*
		 * Scan hash list to see if Inode is already in-core.
		 */
		ip = hash;
		do {
			if ((CDFS_CMPFID(&ip->i_Fid, fid) == B_TRUE) &&
				(ip->i_vfs == vfs)) {
				break;
			}
			ip = ip->i_HashFwd;
		} while (ip != hash);

		if ((CDFS_CMPFID(&ip->i_Fid, fid) != B_TRUE) ||
			(ip->i_vfs != vfs)) {
			return(RET_NOT_FOUND);
		}

		/*
		 * The desire Inode was found, but we need to verify that it's
		 * in a usable state, e.g. not locked by another process.
		 */
		if ((ip->i_Flags & (IRWLOCKED | ILOCKED)) != 0) {
			if (ip->i_LockOwner != curproc->p_slot) {
				/*
				 * The Inode we want is locked by another process.
				 * - Let it be known that we want this Inode.
				 * - Then sleep until it becomes available.
				 * Note: Since the Inode may get thrown away
				 * while we sleep, we need to rescan the hash table.
				 */
				ip->i_Flags |= IWANT;
				(void) sleep((caddr_t)ip, PINOD);
				continue;
			}
		}

		/*
		 * The desired Inode was found and is usable.
		 */
		break;
	}

	/*
	 * The desire Inode was in the Inode cache and was unlocked.
	 * However, we need to make sure that it doesn't get thrown
	 * away (e.g. reallocated) while we're using it.
	 * - Mark the Inode as being "referenced"
	 * - Remove the Inode from the Free list.
	 */
	if ((ip->i_Flags & IREF) == 0) {
		ASSERT((ip->i_Flags & (ILOCKED|IRWLOCKED)) == 0);
		ASSERT(ip->i_FreeFwd != NULL);
		ASSERT(ip->i_FreeBack != NULL);
		ASSERT(ITOV(ip)->v_count == 0);
		ip->i_Flags |= IREF;
		cdfs_IrmFree(ip); 
	}

	/* Prevent the Vnode from being released (and reallocated) and
	 * lock the Inode so it is not modified by some other process.
	 *
	 * Note: Incrementing the Vnode count (i.e. Holding onto the Vnode)
	 * must be done BEFORE attempting to lock the Inode.  The Inode may
	 * already be locked causing a sleep.  If the Vnode count is not
	 * incremented before the sleep, the Vnode may be released (and
	 * reallocated) without any indication.
	 */
	VN_HOLD(ITOV(ip));
	cdfs_LockInode(ip);

	ASSERT(CDFS_CMPFID(&ip->i_Fid, fid) == B_TRUE);
	ASSERT((ip->i_Flags & (IREF|ILOCKED)) == (IREF|ILOCKED));
	ASSERT(ip->i_LockOwner == curproc->p_slot);
	ASSERT(ip->i_Count == 1);
	ASSERT(ip->i_FreeFwd == NULL);
	ASSERT(ip->i_FreeBack == NULL);
	ASSERT(ITOV(ip)->v_count >= 1);

	*ipp = ip;
	return(RET_OK);
}



/*
 * Allocate an Inode from the Free-list.  If the Free-list
 * is empty, try to coerce some Inodes into the Free-list.
 *
 * Note: If an Inode An Inode is returned to the caller only if an Inode
 * can be allocated and sanitized (i.e. made ready for reuse)
 * without sleeping.  If the thread was (potentially) preempted,
 * then there's a chance that the caller no longer needs to
 * allocated an Inode.  For example, the Inode that the caller
 * is interested in may have been built and hashed by another
 * process while this process was sleeping.
 *
 * Therefore, if preemption may have occured, the now sanitized
 * Inode is returned to the Free-list and the caller is informed
 * of the possible preemption.  If the caller still needs to
 * allocate an Inode, the next call will find the sanitized Inode
 * on the Free-list and will return it without any preemption.
 */
int
cdfs_AllocInode(ipp)
struct cdfs_inode	**ipp;					/* Ret addr for the Inode pointer*/
{
	struct cdfs_inode	*ip;				/* Temp Inode pointer			*/
	struct vnode		*vp;
	boolean_t			sleep_flag;			/* Process may have slept		*/
	int					retval;				/* Return value of called procs	*/

	/*
	 * Finding an Inode that can be reused may take several attempts.
	 */
	sleep_flag = B_FALSE; 
	for (;;) {
		/*
		 * If the Free list is empty, try to coerce some
		 * Inodes into the Free-list.
		 */
		if (cdfs_InodeFree == NULL) {
			/*
			 * As long as the Free list is empty, keep purging entries
			 * out of the DNLC Name Cache.  If the Name Cache becomes
			 * empty, then there's nothing left to do but display a
			 * message and return and error.
			 *
			 * XXX - dnlc_purge1() may return 0 if the cache is being
			 * modified, even though the cache is not empty.  Perhaps
			 * we should try more than once before giving up.
			 */
			while (cdfs_InodeFree == NULL) {
				retval = dnlc_purge1();
				if (retval == 0) {
					cmn_err(CE_WARN,
						"cdfs_AllocInode(): Out of CDFS Inodes:");
					cmn_err(CE_CONT,
						"CDFS Inode allocation request denied\n\n");
					syserr.inodeovf++;
					return(ENFILE);
				}
			}
			/*
			 * dnlc_purge1() may have slepted.
			 */
			sleep_flag = B_TRUE;
		}

		/*
		 * XXX - The remaining portion of the for-loop should
		 * really have the following algorithm structure:
		 * - Scan free-list for a Inode that is truely re-allocatable,
		 *		i.e. Not Wanted and, if writable, v_count == 0.
		 *		Each non-reallocatable Inode goes to tail of free-list.
		 * - If no reallocatable Inodes are found, then preempt and continue.
		 * - If the reallocatable Inode contains valid data, then clean it up.
		 * - Break out of for-loop.
		 * 
		 * Unfortunately, its too late to make these kind of changes,
		 * so we'll have to do it some other time.
		 */ 

		/*
		 * Allocate an Inode from the Free-list and update
		 * the system's accounting statistics.
		 */
		ip = cdfs_InodeFree;
		vp = ITOV(ip);
		cdfs_IrmFree(ip); 

		/*
		 * XXX - Need to find a place to accumulate System Info data
		 * for CDFS.  The UFS version is as follows:
		 *
		 * if ((ip->i_Mode != 0) && (ip->i_Vnode->v_pages != NULL)) {
		 * 	sysinfo.ufsipage++;
		 * } else {
		 * 	sysinfo.ufsinopage++;
		 * }
		 */

		/*
		 * If Inode is vacant (i.e. Inode data is not valid),
		 * then we can just use it.
		 */
		if ((ip->i_HashFwd == NULL) &&
			(ip->i_HashBack == NULL)) {
			ASSERT((ip->i_Flags & (IREF|ILOCKED|IRWLOCKED|IWANT)) == 0);
			ASSERT(vp->v_pages == NULL);
			ASSERT(vp->v_count == 0);
			break;
		}

		/*
		 * If this Inode is wanted by someone else, then
		 * get another one and let them have this one.
		 *
		 * Note: We can't just get the next Free Inode,
		 * because for all we know, every Inode on the
		 * Free List is in this unusable state.
		 */
		if ((ip->i_Flags & IWANT) != 0) {
			cmn_err(CE_NOTE,
				"cdfs_AllocInode(): Wanted Inode found on Free-list.");
			cmn_err(CE_CONT,
				"Additional CDFS Inodes may be needed.\n\n");
			cdfs_IputFree(ip);
			preempt();
			sleep_flag = B_TRUE;
			continue;
		}

		/*
		 * Verify that the Vnode is not being referenced.
		 * 
		 * Note:
		 * The following code checks to be sure that putpages
		 * from the page layer have not activated the vnode
		 * while the inode is on the free list.  If we hit this
		 * case we put the inode back on the tail of the free
		 * list and try to find another Inode.  If there is not
		 * another inode on the free list then we put this inode
		 * back and call preempt so that some other process can
		 * do work to free an inode.
		 *
		 * Note: We can't just get the next Free Inode,
		 * because for all we know, every Inode on the
		 * Free List is in this unusable state.
		 *
		 * XXX - This should never happen as long as CDFS remains Read-Only.
		 */
		if (vp->v_count > 0) {
			/*
			 * The Vnode still being referenced, so put it on the
			 * tail of the Free-list.
			 */
			cmn_err(CE_NOTE,
				"cdfs_AllocInode(): Referenced Vnode found on Free-list.\n");

			cdfs_IputFree(ip);
			preempt();
			sleep_flag = B_TRUE;
			continue;
		}

		/*
		 * XXX - CDFS Write Support:
		 * Need to sync media with in-core inode struct and data pages.
		 * Watch out for sleeps.
		 */

		/*
		 * Prepare the Inode struct for reuse.
		 * Note: We need to insure that locking the Inode does not
		 * sleep.  Otherwise, some other process may get a reference
		 * to the Inode (via cdfs_FindInode), while we're sleeping.
		 * If this happens, we would clean the Inode (and reassign it)
		 * while the other process is in the middle of using it.
		 *
		 * Note: The reference flag and lock are cleared by cdfs_CleanInode().
		 */
		ASSERT((ip->i_Flags & (IREF|ILOCKED|IRWLOCKED|IWANT)) == 0);
		ASSERT(vp->v_count == 0);
		ip->i_Flags |= IREF;
		cdfs_LockInode(ip);
		cdfs_CleanInode(ip);
		break;
	}

	*ipp = ip;
	if (sleep_flag == B_TRUE) {
		return(RET_SLEEP);
	}

	return(RET_OK);
}



/*
 * Build an Inode from the Dir Rec data on the media.
 */
int	
cdfs_BldInode(vfs, drec_buf, ip)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
struct cdfs_inode	*ip;					/* Inode struct (already hashed)*/
{
	struct cdfs_drec	*drec;				/* Dir Rec template				*/
	struct cdfs_xar		*xar;				/* XAR template					*/
	struct cdfs_rrip	*rrip;				/* RRIP template				*/
	struct vnode		*vp;				/* Vnode associated with Inode	*/
	int					RetVal;				/* Return value of called procs */

	ip->i_vfs = vfs;

	/*
	 * Copy the relevent Dir Rec data to the Inode.
	 */
	drec = &ip->i_DirRecStorage;

	drec->drec_Loc = drec_buf->sb_sect;
	drec->drec_Offset = drec_buf->sb_offset - drec_buf->sb_sectoff;

	RetVal = cdfs_ConvertDrec(drec,
		(union media_drec *)drec_buf->sb_ptr, CDFS_TYPE(vfs));
	if (RetVal != RET_OK) {
		return(RetVal);
	}
		
	RetVal = cdfs_MergeDrec(ip, drec);
	if (RetVal != RET_OK) {
		return(RetVal); 
	}

	/*
	 * If there this is a multi-extent file, then get the additional
	 * Dir Recs from the media and merge them into the Inode structure.
	 */
	while ((drec->drec_Flags & ISO_DREC_MULTI) != 0) {
		drec_buf->sb_ptr += drec_buf->sb_reclen;
		drec_buf->sb_offset +=  drec_buf->sb_reclen;
		RetVal = cdfs_ReadDrec(vfs, drec_buf);
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		RetVal = cdfs_AllocDrec(&drec);
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		drec->drec_Loc = drec_buf->sb_sect;
		drec->drec_Offset = drec_buf->sb_offset - drec_buf->sb_sectoff;

		RetVal = cdfs_ConvertDrec(drec,
			(union media_drec *)drec_buf->sb_ptr, CDFS_TYPE(vfs));
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		RetVal = cdfs_MergeDrec(ip, drec);
		if (RetVal != RET_OK) {
			break; 
		}
	}

	/*
	 * If the last Dir Rec of the file has an XAR, then get it
	 * and merge the data into the Inode.
	 *
	 * Note: Storage for the XAR is allocated within the Inode
	 * itself but this need not be the case.
	 */
	if (drec->drec_XarLen > 0) {
		xar = &ip->i_XarStorage;

		RetVal = cdfs_GetXar(vfs, drec, xar);
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		RetVal = cdfs_MergeXar(ip, xar); 
		if (RetVal != RET_OK) {
			return(RetVal);
		}
	}
	
	/*
	 * If RRIP is active, get the RRIP data and merge it with
	 * the Inode.
	 * Note: Storage for the RRIP data is allocated with the Inode
	 * structure, but this need not be the case.
	 * Note: It's not an error if we don't find any RRIP data.
	 */
	if ((CDFS_FLAGS(vfs) & CDFS_RRIP_ACTIVE) != 0) {
		rrip = &ip->i_RripStorage;

		RetVal = cdfs_GetRrip(vfs, drec_buf, rrip);
		switch (RetVal) {
			case RET_OK: {
				RetVal = cdfs_MergeRrip(vfs, ip, rrip); 
				if (RetVal != RET_OK) {
					return(RetVal);
				}
				break;
			}
			case RET_EOF:
			case RET_NOT_FOUND: {
				RetVal = RET_OK;
				break;
			}
			default: {
				return(RetVal); 
			}
		}
	}
	
	/*
	 * Setup the Vnode portion of the Inode.
	 */
	vp = &ip->i_VnodeStorage;
	ip->i_Vnode = vp;

	vp->v_op = &cdfs_vnodeops;
	vp->v_vfsp = vfs;
	vp->v_flag = 0;
	vp->v_count = 0;
	vp->v_stream = NULL;
	vp->v_pages = NULL;
	vp->v_filocks = NULL;

	vp->v_type = IFTOVT(ip->i_Mode);
	vp->v_rdev = cdfs_GetDevNum(vfs,ip);
	vp->v_data = (caddr_t)ip;
	if (CDFS_CMPFID(&ip->i_Fid, &CDFS_ROOTFID(vfs)) == B_TRUE) {
		vp->v_flag |= VROOT;
	}
	return(RET_OK);
}



/*
 * Allocate an empty  Dir Rec structure from the free list.
 */
int
cdfs_AllocDrec(drec)
struct cdfs_drec	**drec;		
{
	if (cdfs_DrecFree == NULL) {
		cmn_err(CE_WARN,
			"cdfs_AllocDrec(): No more multi-extent Directory Records.");
		cmn_err(CE_CONT,
			"The CDFS Directory Record Cache may be too small\n\n");
		return(ENOMEM);
	}

	*drec = cdfs_DrecFree; 
	cdfs_DrecRm(&cdfs_DrecFree, *drec);
	return(RET_OK);
}



/*
 * Merge the Dir Rec data into an Inode structure.
 */
int
cdfs_MergeDrec(ip, drec)
struct cdfs_inode	*ip;
struct cdfs_drec	*drec;
{
	/*
	 * Add the Dir Rec to tail of the Inode's Dir Rec list.
	 */
	cdfs_DrecPut(&ip->i_DirRec, drec);
	ip->i_DRcount++;

	if (ip->i_DRcount == 1) {
		/*
		 * For the first Dir Rec, use the Dir Rec data to
		 * set the appropriate Inode fields.
		 * - Set the appropriate Inode flags.
		 * - Set the file type.
		 * - Set # of bytes in file.
		 * - Set Access/Modify/Create date and time.
		 */
		ip->i_Flags |= (
			(((drec->drec_Flags & CDFS_DREC_EXIST) == 0) ?
				CDFS_INODE_HIDDEN : 0) ||
			(((drec->drec_Flags & CDFS_DREC_ASSOC) == 0) ?
				0 : CDFS_INODE_ASSOC)
		);

		if ((drec->drec_Flags & CDFS_DREC_DIR) == 0) {
			ip->i_Mode = IFREG;
			ip->i_LinkCnt = 1;
		} else {
			ip->i_Mode = IFDIR;
			ip->i_LinkCnt = 2;
		}

		ip->i_Size = drec->drec_DataLen;

		ip->i_AccessDate = drec->drec_Date;
		ip->i_ModDate = drec->drec_Date;
		ip->i_CreateDate = drec->drec_Date;

	} else {
		/*
		 * Check for consistency between Dir Recs of a multi-extent file.
		 * - Verify the flags fields are identical.
		 * - Update the size of the file.
		 * - Update the time-stamps to the most recent dates. 
		 *
		 * XXX - Additional checks should be added to ensure
		 * consistency between multi-extent files.
		 */
		if ((drec->drec_Flags & ~CDFS_DREC_MULTI) != 
			((drec->drec_PrevDR)->drec_Flags & ~CDFS_DREC_MULTI)) {
			cmn_err(CE_NOTE,
				"cdfs_MergeDrec(): Inconsistent Directory Record type:");
			cmn_err(CE_CONT,"Device= 0x%x     Sector=%d     Offset=%d\n\n",
				CDFS_DEV(ip->i_vfs), ip->i_Fid.fid_SectNum,
				ip->i_Fid.fid_Offset);
		}

		ip->i_Size += drec->drec_DataLen;

		/*
		 * Use the latest date of all Dir Recs in this file.
		 */
		if ((drec->drec_Date.tv_sec > ip->i_AccessDate.tv_sec) ||
			(drec->drec_Date.tv_nsec > ip->i_AccessDate.tv_nsec)) {
			ip->i_AccessDate = drec->drec_Date;
			ip->i_ModDate = drec->drec_Date;
			ip->i_CreateDate = drec->drec_Date;
		}
	}

	return(RET_OK);
}



/*
 * Get the XAR associated with a Dir Rec (File Section) and
 * merge its data into the Inode structure.
 */
int
cdfs_GetXar(vfs, drec, xar)
struct vfs			*vfs;
struct cdfs_drec	*drec;
struct cdfs_xar		*xar;
{
	union media_xar		*xar_m;
	struct buf			*bp;

	/*
	 * Read in XAR sector.
	 *
	 * XXX - Currently we only care about the "fixed" portion
	 * of the media-resident XAR.  Hence, we only read in the
	 * one block's worth of data from the media.  If we ever
	 * need to worry about the "variable-length" portion of the
	 * XAR (e.g. Escape Sequence/Application Use Data), then
	 * additional buffer space needs to be allocated (and release
	 * via cdfs_CleanInode()) and additional block of data need
	 * to be read.
	 */
	bp = bread(CDFS_DEV(vfs), drec->drec_ExtLoc, CDFS_BLKSZ(vfs));
	if ((bp->b_error & B_ERROR) != 0) {
		brelse(bp);
		return(EIO);
	}
	
	xar_m = (union media_xar *)bp->b_un.b_addr;
	cdfs_ConvertXar(xar, xar_m, CDFS_TYPE(vfs));
	
	brelse(bp);
	return(RET_OK);
}




/*
 * Merge the XAR information into the Inode structure.
 */
int
cdfs_MergeXar(ip, xar)
struct cdfs_inode	*ip;
struct cdfs_xar		*xar;
{
	ip->i_Xar = xar;

	/*
	 * Merge the XAR's UID, GID, and Permission bits into the inode.
	 *
	 * Note: Per ISO-9660 Section 9.1.6, the XAR's UID, GID, and PERMS
	 * are only valid if the PROTECTION bit is set in the Dir Rec
	 * associated with this XAR (assumed to be the last Dir Rec processed).
	 */
	if (((ip->i_DirRec->drec_PrevDR)->drec_Flags & CDFS_DREC_PROTECT) != 0) {
		/*
		 * Merge in the XAR's UID and GID.
		 * Note: Per ISO-9660 specification Section 9.5.1 and 9.5.2,
		 * a UID/GID of 0 means the UID/GID is undefined.  Additionally,
		 * if the GID is undefined, then the UID is also not undefined.
		 */
		if (xar->xar_GroupID != 0) {
			if (xar->xar_UserID != 0) {
				ip->i_Flags |= CDFS_INODE_UID_OK;
				ip->i_UserID = xar->xar_UserID;
			}
			ip->i_Flags |= CDFS_INODE_GID_OK;
			ip->i_GroupID = xar->xar_GroupID;
		}

		/*
		 * Merge in the XAR permissions.
		 */
		ip->i_Flags |= CDFS_INODE_PERM_OK;
		ip->i_Mode &= ~(
			IREAD_USER | IWRITE_USER | IEXEC_USER |
			IREAD_GROUP | IWRITE_GROUP | IEXEC_GROUP |
			IREAD_OTHER | IWRITE_OTHER | IEXEC_OTHER
		);
		ip->i_Mode |= (
			(((xar->xar_Perms & CDFS_XAR_OWNREAD) == 0) ? 0 : IREAD_USER) |
			(((xar->xar_Perms & CDFS_XAR_OWNEXEC) == 0) ? 0 : IEXEC_USER) |
			(((xar->xar_Perms & CDFS_XAR_GROUPREAD) == 0) ? 0 : IREAD_GROUP) |
			(((xar->xar_Perms & CDFS_XAR_GROUPEXEC) == 0) ? 0 : IEXEC_GROUP) |
			(((xar->xar_Perms & CDFS_XAR_OTHERREAD) == 0) ? 0 : IREAD_OTHER) |
			(((xar->xar_Perms & CDFS_XAR_OTHEREXEC) == 0) ? 0 : IEXEC_OTHER)
		);
	}

	/*
	 * Merge in the XAR's time-stamp values.
	 * Note: Per ISO-9660 section 8.4.26.1, a value of zero means
	 * the time-stamp is not specified.
	 */
	if ((xar->xar_CreateDate.tv_sec != 0) ||
		(xar->xar_CreateDate.tv_nsec != 0)) {
		ip->i_CreateDate = xar->xar_CreateDate;
	}

	if ((xar->xar_ModDate.tv_sec != 0) ||
		(xar->xar_ModDate.tv_nsec != 0)) {
		ip->i_ModDate = xar->xar_ModDate;
	}

	if ((xar->xar_EffectDate.tv_sec != 0) ||
		(xar->xar_EffectDate.tv_nsec != 0)) {
		ip->i_EffectDate = xar->xar_EffectDate;
	}

	if ((xar->xar_CreateDate.tv_sec != 0) ||
		(xar->xar_CreateDate.tv_nsec != 0)) {
		ip->i_ExpireDate = xar->xar_ExpireDate;
	}

	return(RET_OK);
}



/*
 * Vnode is no longer referenced, write the inode out
 * and if necessary, truncate and deallocate the file.
 */
void
cdfs_iinactive(ip)
struct cdfs_inode *ip;
{
	ASSERT((ip->i_Flags & (IREF | ILOCKED | IRWLOCKED)) == IREF);
	ASSERT(ip->i_FreeFwd == NULL);
	ASSERT(ip->i_FreeBack == NULL);

	cdfs_LockInode(ip);

	/*
	 * XXX - CDFS_WRITE_SUPPORT:
	 * Mark iinactive in progress.	This allow VOP_PUTPAGE to abort
	 * a concurrent attempt to flush a page due to pageout/fsflush.
	 */

	/*
	 * XXX - CDFS_WRITE_SUPPORT:
	 * - If this is not "READ-ONLY":
	 *	 - Set the IINACTIVE flag to prevent conflich with VOP_PUTPAGE().
	 *	 - Lock the Inode.
	 *	 - If Inode is no longer referenced (i.e. been removed):
	 *		- Truncate the file to 0 bytes.
	 *		- Clear out the in-core Inode structure.
	 *		- Freeup the disk resident Inode.
	 *	 else
	 *		- Sync the in-core contents with the disk. 
	 *	 - Unlock the Inode.
	 */

	/*
	 * Release the Inode:
	 * - Mark the Inode as not being referenced.
	 * - Put the Inode on the Free-list.
	 *
	 * XXX - If the Inode is wanted by another process (IWANT),
	 * it may be better to not put the on the Free list at all.
	 * The only down side is that, the waiting process had better
	 * respond to the wakeup (cdfs_UnlockInode) and eventually put
	 * the Inode back on the Free List.  Otherwise, the Inode
	 * will get lost.
	 */
	ip->i_Flags &= ~IREF;
	cdfs_IputFree(ip);

	/*
	 * If the inode requires a small amount of cleanup (it has
	 * no data pages, or it is not a complete/valid inode), then
	 * put it at the head of the freelist.
	 */
	if ((ITOV(ip)->v_pages == NULL) ||
		(ip->i_HashFwd == NULL)) {
		cdfs_InodeFree = ip; 
	}

	cdfs_UnlockInode(ip);

	return;
}



/*
 * Remove any inodes in the inode cache belonging to dev
 *
 * There should not be any active ones, return error if any are found but
 * still invalidate others (N.B.: this is a user error, not a system error).
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the inode table here anyway, we might as well get the
 * extra benefit.
 *
 * This is called from umount1()/cdfs_vfsops.c when dev is being unmounted.
 */
int
cdfs_FlushInodes(vfsp)
struct vfs *vfsp;
{
	struct cdfs_inode	*ip;
	struct vnode		*vp;
	struct vnode		*rvp;
	dev_t				dev;
	uint_t				i;
	int					RetVal;

	dev = CDFS_DEV(vfsp);
	rvp = CDFS_ROOT(vfsp);

	RetVal = RET_OK;
	ip = &cdfs_InodeCache[0];
	for (i=0; i < cdfs_InodeCnt; i++, ip++) {
		/*
		 * Examine all Inode belonging to this CDFS instance.
		 * All Inode of this CDFS should not be referenced.
		 */
		if (ip->i_vfs == vfsp) {
			/*
			 * If the Inode is not referenced, then clean it up.
			 * Note: The ROOT Inode will still be referenced but
			 * should have a reference count of exactly 1.
			 */
			vp = ITOV(ip);
			if ((ip->i_Flags & IREF) == 0) {
				/*
				 * Since IREF is not set, the Inode should
				 * already be on the free list.
				 */
				ASSERT(vp != rvp);
				ASSERT(vp->v_count == 0);
				ASSERT(ip->i_FreeFwd != NULL);
				ASSERT(ip->i_FreeBack != NULL);

				/*
				 * If the Inode data is not valid, then
				 * it has already been "flushed".
				 */
				if ((ip->i_HashFwd == NULL) ||
					(ip->i_HashBack == NULL)) {
					continue;
				}

				/*
				 * XXX - CDFS Write Support:
				 * Sync media with in-core Inode data.
				 */

				/*
				 * Cleanup the Inode and put it at the head of the
				 * freelist.
				 * Note: Lock is cleared by cdfs_CleanInode().
				 */
				cdfs_LockInode(ip);
				cdfs_CleanInode(ip);
				cdfs_IputFree(ip);
				cdfs_InodeFree = ip;

			} else {
				/*
				 * The Root Inode will still have a reference count
				 * of 1.  It will be released at unmount time.
				 */ 
				if ((vp == rvp) && (vp->v_count == 1)) {
					continue;
				}

				/*
				 * Set error indicator for return value,
				 * but continue invalidating other inodes.
				 */
				RetVal = EBUSY;
			}
		}
	}
	return(RetVal);
}



/*
 * Check mode permission on inode.  Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the read-only status of the file system
 * is checked.  The mode is shifted to select the owner/group/other
 * fields.  The super user is granted all permissions except
 * writing to read-only file systems.
 */
int
cdfs_iaccess(vfs, ip, mode, cr)
struct vfs	 		*vfs;
struct cdfs_inode	*ip;
mode_t				mode;
struct cred			*cr;
{
	mode_t		imode;

	/*
	 * Disallow write attempts on read-only
	 * file systems, unless the file is a block
	 * or character device or a FIFO.
	 * Note: Currently, CDFS is ALWAYS Read-Only.
	 */
	if ((mode & IWRITE) != 0) {
		if ((vfs->vfs_flag & VFS_RDONLY) != 0) { 
			if ((ip->i_Mode & IFMT) != IFCHR &&
			    (ip->i_Mode & IFMT) != IFBLK &&
			    (ip->i_Mode & IFMT) != IFIFO) {
				return(EROFS);
			}
		}
	}

	/*
	 * Priveldged users always have access regardless of the perms.
	 */
	if (pm_denied(cr, P_FILESYS) == 0) {
		return(RET_OK);
	}

	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group perms.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (cr->cr_uid == cdfs_GetUid(vfs, ip)) {
		mode >>= IUSER_SHIFT;
	} else if (groupmember(cdfs_GetGid(vfs,ip), cr) != 0) {
		mode >>= IGROUP_SHIFT;
	} else {
		mode >>= IOTHER_SHIFT;
	}

	imode = cdfs_GetPerms(vfs, ip);
	if ((imode & mode) == mode) {
		return(RET_OK);
	}

	if ((is286EMUL != 0) && ((imode & IEXEC) == IEXEC)) {
		return(RET_OK);
	}

	return(EACCES);
}



/*
 * Lock an inode.
 */
void
cdfs_LockInode(ip)
struct cdfs_inode *ip;
{
	/*
	 * Make sure the Inode is currently not locked by another process.
	 * Its OK to "add" a lock if the Inode is already locked by the
	 * same process.
	 */
	while (((ip->i_Flags & ILOCKED) != 0) &&
	    (ip->i_LockOwner != curproc->p_slot)) {
		ip->i_Flags |= IWANT;
		(void) sleep((caddr_t)ip, PINOD);
	}

	/*
	 * Set the lock:
	 * - Store the owner of the lock.
	 * - Increment the lock count.
	 * - Set the lock flag.
	 */
	ip->i_LockOwner = curproc->p_slot;
	ip->i_Count++;
	ip->i_Flags |= ILOCKED;

	/*
	 * If this is a swap device, increment certain
	 * swap related counters and flags.
	 */
	if ((ITOV(ip)->v_flag & VISSWAP) != 0) {
		curproc->p_swlocks++;
		curproc->p_flag |= SSWLOCKS;
	}
	return;
}




void
cdfs_UnlockInode(ip)
struct cdfs_inode *ip;
{
	/*
	 * Verfiy the lock is set and that it is owned by the
	 * current process.
	 */
	ASSERT(ip->i_Count > 0);
	ASSERT(ip->i_LockOwner == curproc->p_slot);

	ip->i_Count--;

	/*
	 * If this is a swap device, update the swap related counters.
	 */
	if ((ITOV(ip)->v_flag & VISSWAP) != 0) {
		curproc->p_swlocks--;
		if (curproc->p_swlocks == 0) {
			curproc->p_flag &= ~SSWLOCKS;
		}
	}

	/*
	 * If the lock is no longer needed by this procees, then free it
	 * and wakeup the process waiting for the lock to be freed.
	 */
	if (ip->i_Count == 0) {
		ip->i_LockOwner = 0;
		ip->i_Flags &= ~ILOCKED;

		if ((ip->i_Flags & IWANT) != 0) {
			ip->i_Flags &= ~IWANT;
			wakeprocs((caddr_t)(ip), PRMPT);
		}
	}
	return;
}




/*
 * Lock an Inode's data contents from modification.
 */
void
cdfs_irwlock(ip) 
struct cdfs_inode	*ip;
{
	/*
	 * Make sure the RW-LOCK is not set.
	 */
	while ((ip->i_Flags & IRWLOCKED) != 0) {
		ip->i_Flags |= IWANT;
		(void) sleep((caddr_t)ip, PINOD);
	}

	/*
	 * Set the RW-LOCK.
	 */
	ip->i_Flags |= IRWLOCKED;

	/*
	 * If this is the SWAP device, proceed accordingly.
	 */ 
	if ((ITOV(ip)->v_flag & VISSWAP) != 0) {
		curproc->p_swlocks++;
		curproc->p_flag |= SSWLOCKS;
	}

	return;
}



/*
 * Clear an Inode's RW-LOCK.
 */
void
cdfs_irwunlock(ip)
struct cdfs_inode	*ip;
{
	ASSERT((ip->i_Flags & IRWLOCKED) != 0);

	/*
	 * If this is a SWAP device, prceed accordingly.
	 */
	if ((ITOV(ip)->v_flag & VISSWAP) != 0) {
		curproc->p_swlocks--;
		if (curproc->p_swlocks == 0) {
			curproc->p_flag &= ~SSWLOCKS;
		}
	}

	/*
	 * Clear the RW-LOCK flag.
	 */
	ip->i_Flags &= ~IRWLOCKED;

	/*
	 * Wake-up any process waiting for the lock.
	 */
	if ((ip->i_Flags & IWANT) != 0) {
		ip->i_Flags &= ~IWANT;
		wakeprocs((caddr_t)ip, PRMPT);
	}
	return;
}





/*
 * Put an Inode on the Free-list.
 */
void
cdfs_IputFree(ip)
struct cdfs_inode	*ip;					/* Inode to be added			*/
{
	CDFS_LIST_PUT(&cdfs_InodeFree, ip, i_FreeFwd, i_FreeBack);
	return;
}




/*
 * Remove an Inode from the Free-list.
 */
void
cdfs_IrmFree(ip)
struct cdfs_inode	*ip;					/* Inode to be removed			*/
{
	CDFS_LIST_RM(&cdfs_InodeFree, ip, i_FreeFwd, i_FreeBack);
	return;
}



/*
 * Put an Inode on the specified Hash-list.
 */
void
cdfs_IputHash(ip)
struct cdfs_inode	*ip;					/* Inode to be added			*/
{
	struct cdfs_inode	**hash;				/* Hash-list Head				*/
	
	hash = &cdfs_InodeHash[CDFS_INOHASH(&ip->i_Fid)];

	CDFS_LIST_PUT(hash, ip, i_HashFwd, i_HashBack);
	return;
}




/*
 * Remove an Inode from the specified Hash-list.
 */
void
cdfs_IrmHash(ip)
struct cdfs_inode	*ip;					/* Inode to be removed			*/
{
	struct cdfs_inode	**hash;				/* Hash-list Head				*/

	hash = &cdfs_InodeHash[CDFS_INOHASH(&ip->i_Fid)];
	CDFS_LIST_RM(hash, ip, i_HashFwd, i_HashBack);
	return;
}



/*
 * Put a Dir Rec on the Free-list.
 */
void
cdfs_DrecPut(head, drec)
struct cdfs_drec	**head;					/* Head of Dir Rec list			*/
struct cdfs_drec	*drec;					/* Dir Rec to be freed			*/
{
	CDFS_LIST_PUT(head, drec, drec_NextDR, drec_PrevDR);
	return;
}



/*
 * Remove a Dir Rec from the Free-list.
 */
void
cdfs_DrecRm(head, drec)
struct cdfs_drec	**head;					/* Head of Dir Rec list			*/
struct cdfs_drec	*drec;					/* Dir Rec to be removed		*/
{
	CDFS_LIST_RM(head, drec, drec_NextDR, drec_PrevDR);
	return;
}
