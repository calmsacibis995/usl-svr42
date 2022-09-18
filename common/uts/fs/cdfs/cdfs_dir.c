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

#ident	"@(#)uts-comm:fs/cdfs/cdfs_dir.c	1.9"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/cdrom.h>
#include <fs/cdfs/iso9660.h>
#include <fs/dnlc.h>
#include <fs/fbuf.h>
#include <fs/mode.h>
#include <fs/mount.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/seg.h>
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
 * Lookup a name in the specified directory.
 * If successful,  return a pointer to a LOCKED Inode for the
 * file/dir that was found.
 */
int
cdfs_DirLookup(vfs, dp, namep, ipp, cr)
struct vfs			 *vfs;					/* File system's VFS structure	*/
struct cdfs_inode	*dp;					/* Inode of directory to search	*/
uchar_t				*namep;					/* Pathname component to lookup	*/
struct cdfs_inode	**ipp;					/* Addr to return Inode pointer	*/
struct cred			*cr;					/* User's credential structure	*/
{
	struct cdfs_iobuf	drec_buf;			/* Dir Rec search buffer		*/
	struct vnode		*vp;				/* Vnode corresponding to Inode	*/
	struct pathname		pname;				/* Name of matched Dir Rec		*/
	struct cdfs_fid		fid;				/* FID structure of new Inode	*/
	boolean_t			Found;				/* B_TRUE = Dir Rec was found	*/
	boolean_t			WrapAround;			/* B_TRUE = EOF was reached		*/
	int					retval;				/* Return Value of various calls*/

	/*
	 * Verify that the inode is a directory and
	 * that the user the proper access rights.
	 */
	if ((dp->i_Mode & IFMT) != IFDIR) {
		return(ENOTDIR);
	}

	retval = cdfs_iaccess(vfs, dp, IEXEC, cr);
	if (retval != RET_OK) {
		return(retval);
	}

	/*
	 * Check the DNLC (Directory Name Lookup Cache) for the specified name.
	 */
	vp = dnlc_lookup(ITOV(dp), (char *)namep, NOCRED);
	if (vp != NULL) {
		*ipp = VTOI(vp);
		VN_HOLD(vp);
		cdfs_LockInode(*ipp);
		return(RET_OK);
	}

	/*
	 * Check for special case pathname components: '.' and '..'.
	 *
	 * Note: These are NOT the same as 'ISO_DOT' and ISO_DOTDOT'.
	 * '.' and '..' yeilds an Inode with the "true" FID.
	 * 'ISO_DOT' and 'ISO_DOTDOT' return an Inode with the FID
	 * corresponding to the 1st/2nd Dir Rec of the directory.
	 *
	 * Note: These special case pathnames need to entered into 
	 * the DNLC cache so that the Vnode is temporarily held by
	 * someone other than CDFS so that another process has a
	 * resonable chance for a CDFS Inode Cache hit.  Otherwise,
	 * when released by CDFS, the Inode would immediately be placed
	 * on the freelist and thus reallocated too quickly for a
	 * cache hit to occur.
	 */
	if (strcmp((caddr_t)namep, (caddr_t)CDFS_POSIX_DOT) == 0) {
		*ipp = dp;
		VN_HOLD(ITOV(dp));
		cdfs_LockInode(dp);
		dnlc_enter(ITOV(dp), (char *)namep, ITOV(*ipp), NOCRED);
		return(RET_OK);
	}

	if (strcmp((caddr_t)namep, (caddr_t)CDFS_POSIX_DOTDOT) == 0) {
		retval = cdfs_GetParent(vfs, dp, ipp, cr);
		if (retval != RET_OK) {
			return(retval);
		}
		dnlc_enter(ITOV(dp), (char *)namep, ITOV(*ipp), NOCRED);
		return(RET_OK);
	}

	/*
	 * Before the search can start, the directory must be locked
	 * so that its contents don't change while searching.
	 */
	cdfs_LockInode(dp);

	/*
	 * Initialize the Dir Rec buffer descriptor.
	 */
	CDFS_SETUP_IOBUF(&drec_buf, CDFS_FBUFIO);
	drec_buf.sb_vp = ITOV(dp); 
	drec_buf.sb_dev = CDFS_DEV(vfs);

	/*
	 * Begin the directory search with the Dir Rec that we were at when
	 * we stopped the previous search of this directory.  The search needs
	 * to begin at the beginning of the directory, if:
	 * 1) The current search location is beyond the end of the directory,
	 * 2) We're looking for Dir Rec for the "Current Directory (DOT)" or
	 * 	  the "Parent Directory (DOTDOT)"
	 *
	 * NOTE: It's crutial that we begin the search with first Dir Rec of
	 * a multi-extend file and with the first Dir Rec of a multi-version
	 * file, especially when XCDR NO_VERSION is active.
	 */
	if ((dp->i_DirOffset >= dp->i_Size) ||
		(strcmp((caddr_t)namep, (caddr_t)CDFS_DOT) == 0) ||
		(strcmp((caddr_t)namep, (caddr_t)CDFS_DOTDOT) == 0)) {

		dp->i_DirOffset = 0;
	}

	drec_buf.sb_offset = dp->i_DirOffset;

	/*
	 * Examine each Dir Rec contained by the directory.
	 * Stop the search when we find first Dir Rec that matches
	 * the desired name or when we've wrapped-around to the
	 * Dir Rec that we started the scan with.
	 */
	pn_alloc(&pname);
	WrapAround = B_FALSE;
	for (;;) {
		/*
		 * Locate and read the next Dir Rec into the buffer,
		 */
		retval = cdfs_ReadDrec(vfs, &drec_buf);
		if (retval != RET_OK) {
			/*
			 * If this is an EOF, we must loop back to the beginning
			 * of the directory and continue the search until we
			 * get back to where we started (e.g. i_DirOffset).
			 * If we started the search at the beginning of the
			 * directory (i_DirOffset = 0) then we don't want to continue.
			 */
			if (retval == RET_EOF) {
				drec_buf.sb_offset = 0;
				drec_buf.sb_ptr = NULL;
				WrapAround = B_TRUE;
				continue;
			}
			Found = B_FALSE;
			break;
		}

		/*
		 * If we've wrapped-around back to the Dir Rec we started
		 * with, then the desire Dir Rec is not in this directory.
		 */
	 	if ((drec_buf.sb_offset == dp->i_DirOffset) &&
			(WrapAround == B_TRUE)) {
			Found = B_FALSE;
			break;
		}
			
		/*
		 * Compare the name of the Dir Rec with the desired name.
		 * If they match (and the Dir Rec should not be hidden),
		 * then we've found the desired Dir Rec.
		 */
		retval = cdfs_CmpDrecName(vfs, &drec_buf, namep, &pname);
		if (retval == RET_TRUE) {
			if ((CDFS_FLAGS(vfs) & CDFS_RRIP_ACTIVE) == 0) {
				Found = B_TRUE;
			} else {
				if (cdfs_HiddenRrip(vfs, &drec_buf) == RET_FALSE) {
					Found = B_TRUE;
				} else {
					Found = B_FALSE;
				}
			}
			break;
		}

		/*
		 * If an error has occured, in cdfs_CmpDrecName(), then bail-out.
		 */
		if (retval != RET_FALSE) {
			Found = B_FALSE;
			break;
		}
			
		/*
		 * XXX - If we're only searching for ISO names (e.g.
		 * RRIP is not active), then the search can stop
		 * if we're beyond the desired name.  Dir Recs are sorted
		 * by the ISO name.  Don't forget about XCDR name conversions
		 */

		/*
		 * Compute the location (i.e. Vnode offset) of the next Dir Rec.
		 * - If we've gone past the end of the directory, we must
		 *	 "circle back" to the beginning of the directory.
		 * - If we've gotten back to where we started the search,
		 *	 the entry was no where to be found.
		 */
		drec_buf.sb_ptr += drec_buf.sb_reclen;
		drec_buf.sb_offset += drec_buf.sb_reclen;

		/*
		 * Check for EOF condition and loop back to the
		 * beginning of the directory to continue the search.
		 */
		if (drec_buf.sb_offset >= dp->i_Size) {
			drec_buf.sb_offset = 0;
			drec_buf.sb_ptr = NULL;
			WrapAround = B_TRUE;
		}
	}
	pn_free(&pname);

	/*
	 * If we didn't find a matching Dir Rec, return an error.
	 */
	if (Found != B_TRUE) {
		CDFS_RELEASE_IOBUF(&drec_buf);
		cdfs_UnlockInode(dp);
		return(ENOENT);
	}

	/*
	 * The desired Dir Rec was found.  This will be the first Dir Rec
	 * of a multi-extent files or the first Dir Rec of multi-version
	 * file if version numbers are not considered.
	 * - Initialize the FID structure to identify the file/dir.
	 * - Build an Inode structure for the file/dir.
	 *
	 * XXX - It would be better (especially near the end of a sector)
	 * to 'i_DirOffset' to the first Dir Rec of the NEXT file/dir.
	 * Then, the current, and possibly the following Dir Recs would
	 * not be rescanned on the next lookup.  Perhaps, the setting of
	 * 'i_DirOffset' could be set based on 'drec_buf' AFTER the call
	 * to cdfs_GetInode().
	 */ 
	dp->i_DirOffset = drec_buf.sb_offset;

	/*
	 * Unlocking the Inode is done here, before cdfs_BldInode(),
	 * in order to prevent a dead-lock with another process
	 * who is trying to locate this inode via cdfs_GetParent().
	 */
	cdfs_UnlockInode(dp);

	fid.fid_SectNum = drec_buf.sb_sect;
	fid.fid_Offset = drec_buf.sb_offset - drec_buf.sb_sectoff;

	retval = cdfs_GetInode(vfs, &fid, &drec_buf, ipp);
	CDFS_RELEASE_IOBUF(&drec_buf);

	if (retval == RET_OK) {
		/*
		 * Successfully built the new Inode.
		 * - Save its parent's FID (Used in cdfs_GetParent()).
		 * - Enter the new Inode into the DNLC cache.
		 * Note: We don't want CDFS_DOT and CDFS_DOTDOT entered
		 * into the DNLC cache.  These are transitory Inodes that
		 * are not the "TRUE" Inodes for the directory data.
		 */
		(*ipp)->i_ParentFid = dp->i_Fid;
		if ((strcmp((caddr_t)namep, (caddr_t)CDFS_DOT) != 0) &&
			(strcmp((caddr_t)namep, (caddr_t)CDFS_DOTDOT) != 0)) {
			dnlc_enter(ITOV(dp), (char *)namep, ITOV(*ipp), NOCRED);
		}
	}
	return(retval);
}



/*
 * Get the parent Inode of the Dir Recof Dir Rec of the 
 */
int
cdfs_GetParent(vfs, dp, ipp, cr)
struct vfs			 *vfs;					/* File system's VFS structure	*/
struct cdfs_inode	*dp;					/* Inode of directory to search	*/
struct cdfs_inode	**ipp;					/* Addr to return Inode pointer	*/
struct cred			*cr;					/* User's credential structure	*/
{
	struct cdfs_inode	*ip1;				/* Inode of Parent (FID=..)		*/
	struct cdfs_inode	*ip2;				/* Inode of Grandparent (FID=..)*/
	struct cdfs_inode	*ip3;				/* Inode of Parent (Real FID)	*/

	struct cdfs_iobuf	drec_buf;			/* Dir Rec search buffer		*/
	struct cdfs_fid		fid;				/* FID structure of new Inode	*/
	boolean_t			Found;				/* B_TRUE = Dir Rec was found	*/
	int					retval;				/* Return Value of various calls*/

	/*
	 * These following two trivial cases should handle most calles.
	 *
	 * Note: The VFS upper-layers should prevent the 2nd case from
	 * happening.  However, handle it anyway, just to be safe.
	 *
	 * Note: The local 'fid' variable should not be removed and
	 * replaced by 'cdfs-GetInode(...&dp->i_ParentFid...)'.  If
	 * write support is ever added, this can cause a race-condition
	 * when this process is prempted.
	 */
	if (CDFS_CMPFID(&dp->i_ParentFid, &CDFS_NULLFID) == B_FALSE) {
		fid = dp->i_ParentFid;
		retval = cdfs_GetInode(vfs, &fid, NULL, ipp);
		return(retval);
	}

	if (CDFS_CMPFID(&dp->i_Fid, &CDFS_ROOTFID(vfs)) == B_TRUE) {
		*ipp = CDFS_ROOT_INODE(vfs);
		VN_HOLD(ITOV(*ipp));
		cdfs_LockInode(*ipp);
		return(RET_OK);
	}

	/*
	 * Since we've gotten here, the Inode's parent FID is not valid.
	 * This can happen when the Parent Inode is flushed from the
	 * Inode cache and '../..' is requested.  The Parent Inode
	 * is found from the Child Inode via the case above.  However,
	 * it's Parent FID is not known when the Parent Inode is rebuild.
	 * Therefore, when the Grand Parent is requested, the Parent's
	 * Parent FID is unknown and we fall-thru to this code.
	 *
	 * Since the parent's true FID is based on the Dir Rec located
	 * in the Grandparent's directory, we can not simply build an
	 * Inode based on the "Parent Dir" Dir Rec found in this
	 * (the child) directory.  The required steps are as follows:
	 * - Build Parent Inode from "Parent Dir" Dir Rec of Child.
	 * - Build Grandparent Inode from "Parent Dir" Dir Rec of Parent.
	 * - Scan each Dir Rec in Grandparent until the Parent is found.
	 * - Build the "true" Parent Inode based on the located Dir Rec.
	 *
	 * Assumption: The location of each directory (e.g. its extent)
	 * is unique.  This is not true for files, but is true for
	 * directories.
	 */
	retval = cdfs_DirLookup(vfs, dp, (uchar_t *)CDFS_DOTDOT, &ip1, cr);
	if (retval != RET_OK) {
		return(retval);
	}

	/*
	 * If the tmp Parent Inode is the Root Directory, then we just
	 * return the Root Inode since we already have the "True" Inode.
	 */
	if ((ip1->i_DirRec)->drec_ExtLoc ==
			(CDFS_ROOT_INODE(vfs)->i_DirRec)->drec_ExtLoc) {
		cdfs_UnlockInode(ip1);
		VN_RELE(ITOV(ip1));
		*ipp = CDFS_ROOT_INODE(vfs);
		VN_HOLD(ITOV(*ipp));
		cdfs_LockInode(*ipp);
		return(RET_OK);
	}

	/*
	 * Get the tmp GrandParent Inode.
	 */
	retval = cdfs_DirLookup(vfs, ip1, (uchar_t *)CDFS_DOTDOT, &ip2, cr);
	if (retval != RET_OK) {
		cdfs_UnlockInode(ip1);
		VN_RELE(ITOV(ip1));
		return(retval);
	}

	/*
	 * Examine each Dir Rec in the Grandparent to try
	 * to find the Dir Rec of the Parent.
	 * Note: We don't need to validate the access rights on the
	 * GrandParent since the user is not accessing it directly.
	 * The access rights for the Parent Inode were validated
	 * via the previous call to cdfs-DirLookup().
	 */
	CDFS_SETUP_IOBUF(&drec_buf, CDFS_FBUFIO);
	drec_buf.sb_vp = ITOV(ip2);
	drec_buf.sb_offset = 0;

	for (;;) {
		/*
		 * Locate the next Dir Rec within the Dir Rec buffer,
		 * create it's FID and get/build an inode for it.
		 */
		retval = cdfs_ReadDrec(vfs, &drec_buf);
		if (retval != RET_OK) {
			if (retval == RET_EOF) {
				retval = ENOENT;
			}
			Found = B_FALSE;
			break;
		}

		fid.fid_SectNum = drec_buf.sb_sect;
		fid.fid_Offset = drec_buf.sb_offset - drec_buf.sb_sectoff;

		retval = cdfs_GetInode(vfs, &fid, &drec_buf, &ip3);
		if (retval != RET_OK) {
			Found = B_FALSE;
			break;
		}

		/*
		 * If the current Inode is a directory and it has same
		 * the same extent location as the temp Parent Inode,
		 * then we've found the "true" Dir Rec (and "true" Inode)
		 * of the Parent directory.
		 *
		 * XXX - Additional checks should be added to ensure that
		 * this is indeed the inode we want.
		 */
		if (((ip3->i_Mode & IFMT) == IFDIR) &&
			((ip3->i_DirRec)->drec_ExtLoc == (ip1->i_DirRec)->drec_ExtLoc)) {
			Found = B_TRUE;
			break;
		}

		/*
		 * This is not the Parent Dir Rec so release the Inode
		 * and compute the location of the next Dir Rec.
		 * If we reach an EOF condition, then we've failed.
		 */
		cdfs_UnlockInode(ip3);
		VN_RELE(ITOV(ip3));

		drec_buf.sb_ptr += drec_buf.sb_reclen;
		drec_buf.sb_offset += drec_buf.sb_reclen;

		if (drec_buf.sb_offset >= ip2->i_Size) {
			retval = ENOENT;
			Found = B_FALSE;
			break;
		}
	}

	/*
	 * Release locked/allocated resources.
	 */
	CDFS_RELEASE_IOBUF(&drec_buf);

	cdfs_UnlockInode(ip2);
	VN_RELE(ITOV(ip2));

	cdfs_UnlockInode(ip1);
	VN_RELE(ITOV(ip1));

	/*
	 * If found, return the Parent Inode to the caller.
	 * Also, if the Child Inode is not locked (or is
	 * locked by this process), then update its Parent
	 * FID so we don't have to go through all this work again.
	 *
	 * Note: If the Child Inode is locked, we can't wait for
	 * the lock to clear, otherwise a deadlock might occur
	 * while another process that is waiting for the Parent Inode.
	 */
	if (Found == B_TRUE) {
		if (((dp->i_Flags & ILOCKED) == 0) ||
			(dp->i_LockOwner == curproc->p_slot)) {
			dp->i_ParentFid = ip3->i_Fid;
		}
		*ipp = ip3;
		retval = RET_OK;
	}

	return(retval);
}





/*
 * Get the next Dir Rec stored by this directory Vnode.
 * This routine is "reenterent" in that it uses and updates
 * the significant information already computed/allocated
 * by the calling routine.
 */
int
cdfs_ReadDrec (vfs, drec_buf)
struct vfs			*vfs;					/* File system's VFS stucture	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
{
	union media_drec	*drec;				/* Dir Rec template				*/
	int					retval;				/* Return value of called procs	*/

	for (;;) {
		/*
		 * Verify the current buffer contains the desired location
		 * and that enough data is left for a minimal-size record.
		 *
		 * Note: In order to handle the case where sb_ptr and sb_end
		 * both equal NULL, the 2nd conditional should not be coded
		 * as "sb_ptr > sb_end - LEN.  This may cause an underflow,
		 * and hence incorrect behavior.
		 */
		if ((drec_buf->sb_start == NULL) ||
			(drec_buf->sb_ptr < drec_buf->sb_start) ||
			(drec_buf->sb_ptr + ISO_DREC_FIXEDLEN > drec_buf->sb_end)) {
			/*
			 * Fix up the IOBUF to read in the desired data.
			 * If the desired location (sb_ptr) is outside the
			 * bounds of the current buffer, then we just read
			 * the desired location as specified by the caller
			 * (See cdfs_ReadSect() for details).  Otherwise,
			 * the desired location begins within the current
			 * buffer, however, there's not enough data remaining
			 * in the current buffer for an entire record.
			 * Therefore, the next Dir Rec MUST begin in the
			 * next sector (ISO-9660 Sec 6.8.1.1/ HS Sec 8.3.1).
			 */
			if ((drec_buf->sb_start != NULL) &&
				(drec_buf->sb_ptr >= drec_buf->sb_start) &&
				(drec_buf->sb_ptr < drec_buf->sb_end)) {
				/*
				 * Not enough data left in the current sector
				 * for a complete record, so just skip ahead
				 * to the next sector.
				 * Note: Since this condition should not happen at
				 * the beginning of the sector, we'll ensure it doesn't.
				 */
				if (drec_buf->sb_ptr == drec_buf->sb_start) {
					cmn_err(CE_NOTE,
						"cdfs_ReadDrec(): Invalid Dir Rec sector:");
					cmn_err(CE_CONT,
						"Sector %d    Length = %d bytes\n\n",
						drec_buf->sb_sect,
						drec_buf->sb_end - drec_buf->sb_start);
					return(RET_ERR);
				}

				drec_buf->sb_offset += (drec_buf->sb_end - drec_buf->sb_ptr);
				drec_buf->sb_ptr = drec_buf->sb_end;
			}

			/*
			 * Read in the sector containing the desired location.
			 */
			retval = cdfs_ReadSect(vfs, drec_buf);
			if (retval != RET_OK) {
				return(retval);
			}

			/*
			 * We need to verify that the new buffer contains
			 * enough data for at least a minimal-size record.
			 *
			 * Note - Infinate loops are prevented by an EOF
			 * status from cdfs_ReadSect() or the
			 * "Beginning of the Sector" test within the
			 * "Not enough data in current buffer" condition
			 * listed above.
			 */
			continue;
		}

		/*
		 * Validate the current Dir Rec.
		 *
		 * Since Dir Recs do not span Logical Sectors, the last
		 * few bytes of each logical sector are likely to be
		 * unused and will be set to zero (ISO-9660 Section 6.8.1.1).
		 *
		 * We verify that we're not pointing at this unused area
		 * by making sure that the length of the current Dir Rec
		 * is at least as big as the smallest possible Dir Rec.
		 * 
		 * If we are pointing at the unused area of the sector,
		 * we set the pointer to the end of the buffer and retest
		 * for the end of buffer condition causing the next
		 * sector to be read in.
		 *
		 * XXX - More checks could be added to ensure consistency.
		 */
		drec = (union media_drec *)drec_buf->sb_ptr;
		switch (CDFS_TYPE(vfs)) {
			case CDFS_ISO_9660: {
				drec_buf->sb_reclen = drec->Iso.drec_Size;
				break;
			}
			case CDFS_HIGH_SIERRA: {
				drec_buf->sb_reclen = drec->Hs.drec_Size;
				break;
			}
			default: {
				cmn_err(CE_WARN,
					"cdfs_ReadDrec(): Invalid CDFS type: 0x%x\n",
					CDFS_TYPE(vfs));
				return(RET_ERR);
			}
		}

		/*
		 * If the Dir Rec length is zero, we're pointing
		 * to the unused portion of the sector.  Advance
		 * the pointers to the end of the buffer which will
		 * cause the next sector to be read.
		 * Note: This should not occur at the beginning of the sector.
		 */
		if ((drec_buf->sb_reclen == 0) &&
			(drec_buf->sb_ptr != drec_buf->sb_start)) {
			drec_buf->sb_offset += (drec_buf->sb_end - drec_buf->sb_ptr);
			drec_buf->sb_ptr = drec_buf->sb_end;
			continue;
		}

		/*
		 * A non-zero length Dir Rec was found, so do some
		 * sanity checks to make sure its a valid Dir Rec:
		 * - Must be larger than a minimal-sized Dir Rec.
		 * - Must not exceed beyond the end-of-buffer.
		 *
		 * Note: Dir Rec do not span sector boundries per
		 * ISO-9660 Section 6.8.1.1 and High-Sierra Section 8.3.1.
		 */
		if ((drec_buf->sb_reclen > ISO_DREC_FIXEDLEN) &&
			(drec_buf->sb_ptr + drec_buf->sb_reclen <= drec_buf->sb_end)) {
			break;
		}

		/*
		 * An invalid Dir Rec was found.  After the error message
		 * is displayed, there's nothing much to do except return
		 * an error.  We could just increment the * pointer/offset
		 * and try the next entry, but this assumes the current
		 * entry contains a valid length field.
		 */
		cmn_err(CE_NOTE,
			"cdfs_ReadDrec(): Invalid Dir Record entry found:");
		cmn_err(CE_CONT,
			"Sect %d,  Sect offset %d,  Dir Rec len %d\n",
			drec_buf->sb_sect,
			(drec_buf->sb_offset - drec_buf->sb_sectoff),
			drec_buf->sb_reclen);

		if (drec_buf->sb_reclen <= ISO_DREC_FIXEDLEN) {
			cmn_err(CE_CONT,
				"Invalid Dir Rec length - Min length is %d\n\n",
				ISO_DREC_FIXEDLEN + 1);
		} else {
			cmn_err(CE_CONT,
				"Dir Rec exceeds sector boundry - Sec size = %d bytes.\n\n",
				CDFS_SECTSZ(vfs));
		}

		/*
		 * We could try to recover by just skipping to the next
		 * sector.  However, this may cause more confusion, since
		 * we have no way of knowing where the caller has taken us.
		 */
		return(RET_ERR);
	}

	return(RET_OK);
}



/*
 * Compute the Logical Sector # and Sector offset of the
 * given file/directory offset.
 */
int
cdfs_SectNum(vfs, ip, offset, sect, sectoff, lastcont)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_inode	*ip;					/* Inode of file/dir			*/
ulong_t				offset;					/* Offset into data area		*/
daddr_t				*sect;					/* Ret addr for the sector #	*/
uint_t				*sectoff;				/* Ret addr for the sect offset	*/ 
uint_t				*lastcont;				/* Ret addr for last cont. byte	*/
{
	daddr_t		blk;						/* Log Block # of offset		*/
	uint_t		blkoff;						/* Offset within the Log Block	*/
	uint_t 		blk2sect;					/* Blk <-> Sect	shift count		*/
	int			retval;						/* Return value of called procs	*/

	/*
	 * Compute the Logical Block # and Block offset of the
	 * desired offset within the file/dir data.
	 * Note: The last continuous data byte is computed and
	 * returned by cdfs_bmap().
	 */
	retval = cdfs_bmap(vfs, ip, offset, &blk, &blkoff, lastcont);
	if (retval != RET_OK) {
		return(retval);
	}

	/*
	 * Compute the shift-count to convert a Logical
	 * Sector # to/from a Logical Block #.
	 */
	blk2sect = CDFS_SECTSHFT(vfs) - CDFS_BLKSHFT(vfs);  

	/*
	 * If the Logical Sector # is requested, then compute it:
	 */
	if (sect != NULL) {
		*sect = (ulong_t)blk >> blk2sect;
	}

	/*
	 * If the Sector Offset is requested, then compute it:
	 * - Use 'blk2sect' to generate a bit-mask for the low-order
	 *	 bit of the block #.  The low-order bits represent the
	 *	 number of complete blocks that are contained within the
	 *	 Logical Sector.
	 * - Convert the # of complete blocks to a byte value.
	 * - Add the computed block offset.
	 */
	if (sectoff != NULL) {
		*sectoff =
			((blk & ((1 << blk2sect) - 1)) << CDFS_BLKSHFT(vfs)) + blkoff;
	}

	return(RET_OK);
}



/*
 * Compare the name(s) of a Dir Rec with the specified name.
 * If any of the Dir Rec names match the specified name,
 * return RET_TRUE, otherwise return RET_FALSE.
 *
 * Note: The name of the Dir Rec is returned to the caller.
 */
int
cdfs_CmpDrecName(vfs, drec_buf, namep, pname)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
uchar_t				*namep;					/* Name to compare with			*/
struct pathname		*pname;					/* Matched (or ISO) Dir Rec name*/
{
	/*
	 * Get the name of this Dir Rec:
	 * - If RRIP is active, then use the RRIP name (if it exists).
	 * - Otherwise, use the XCDR-converted ISO name.
	 *
	 * Note: This algorithm MUST compliment the algorithm in
	 * cdfs_readdir() used to generate the directory entry names
	 * that are returned to the caller.
	 *
	 */
	if (((CDFS_FLAGS(vfs) & CDFS_RRIP_ACTIVE) == 0) ||
		(cdfs_GetRripName(vfs, drec_buf, pname) != RET_OK) ||
		(pname->pn_pathlen == 0)) {

		if ((cdfs_GetIsoName(vfs, drec_buf, pname) != RET_OK) ||
			(cdfs_XcdrName(vfs, (uchar_t *)pname->pn_buf,
					pname->pn_pathlen, pname) != RET_OK) ||
			(pname->pn_pathlen == 0)) {

			return(RET_ERR);
		}
	}

	/*
	 * Compare the name of this Dir Rec with the desired name.
	 *
	 * Note: In addition to a byte-to-byte comparision, we
	 * need to explicitly verify that the lengths of the two
	 * strings also match.
	 */
	if ((strncmp(pname->pn_buf, (char *)namep, pname->pn_pathlen) == 0) &&
		(namep[pname->pn_pathlen] == '\0')) {
		return(RET_TRUE);
	} else {
		return(RET_FALSE);
	}
}




/*
 * Check the specified Dir Rec to see if it should be hidden
 * from the user.
 */
int
cdfs_HiddenDrec(vfs, drec_buf)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
{
	union media_drec	*drec;				/* Dir Rec template				*/
	uchar_t				*name;				/* Addr of ISO name of file/dir */
	uint_t				namelen;			/* Len of ISO name of file/dir	*/
	boolean_t			hidden;				/* TRUE = File is to be hidden	*/
	int					retval;				/* Return value of called procs	*/

/*
 * XXX - 
 * This following block of code examines the EXISTENCE bit
 * (ISO-9660 Section 9.1.6) to determine if "the existence of
 * the file need not be made known to the user".
 * Since, this "feature" really should to be configurable via the
 * 'space.c' file and mount(1M) command, it is better to leave
 * it disable until the user can selectively disable it.
 */
#ifdef CDFS_HIDDEN_FEATURE_CONFIGURABLE
	drec = (union media_drec *)drec_buf->sb_ptr;

	/*
	 * Note: The SENSE of the ISO_DREC_EXIST flags is intuitively
	 * backwards, i.e. When set to 0, the file's existence shall
	 * be made known to the user.  When set to 1, the file's
	 * existence may be hidden from the user.
	 */
	hidden = B_FALSE;

	if (CDFS_HIDDEN_FEATURE_ENABLED == B_TRUE) {
		switch (CDFS_TYPE(vfs)) {
			case CDFS_ISO_9660: {
				if ((drec->Iso.drec_Flags & ISO_DREC_EXIST) != 0) {
					hidden = B_TRUE;
				}
				name = &drec->Iso.drec_VarData;
				namelen = drec->Iso.drec_FileIDSz;
				break;
			}
			case CDFS_HIGH_SIERRA: {
				if ((drec->Hs.drec_Flags & ISO_DREC_EXIST) != 0) {
					hidden = B_TRUE;
				}
				name = &drec->Hs.drec_VarData;
				namelen = drec->Hs.drec_FileIDSz;
				break;
			}
			default: {
				cmn_err(CE_WARN,
					"cdfs_HiddenDrec(): Invalid CDFS type: 0x%x\n",
					CDFS_TYPE(vfs));
				return(RET_ERR);
			}
		}
	}
	
	/*
	 * Determine whether or not to hide the file from the caller.
	 * 
	 * NOTE: Some discs (e.g. Discovery System's Sampler CD-ROM)
	 * are recorded with the 'EXISTENCE' flag asserted (i.e.
	 * file is to be hidden) for the CDFS_DOT and CDFS_DOTDOT
	 * (Current and Parent) directory entries of each directory.
	 * In order accommodate these discs, we ignore the
	 * "hiddenness" of these directory entries so that internal
	 * lookups of DOT and DOTDOT succeed and so that the user
	 * sees '.' and '..' entries via 'ls -a'.
	 */
	if (hidden == B_TRUE) {
		if ((strncmp((caddr_t)name, (caddr_t)CDFS_DOT, namelen) == 0) &&
			(CDFS_DOT[namelen] == '\0')) {
			hidden = B_FALSE;
		} else
		if ((strncmp((caddr_t)name, (caddr_t)CDFS_DOTDOT, namelen) == 0) &&
			(CDFS_DOTDOT[namelen] == '\0')) {
			hidden = B_FALSE;
		}
		if (hidden == B_TRUE) {
			return(B_TRUE);
		}
	}
#endif

	/*
	 * If RRIP extensions are enabled and present, check the
	 * various SUFs to see if the entry should be hidden.
	 */
	if ((CDFS_FLAGS(vfs) & CDFS_RRIP_ACTIVE) != 0) {
		retval = cdfs_HiddenRrip(vfs, drec_buf);
		return(retval);
	}

	return(RET_FALSE);
}



/*
 * Get the ISO-9660/High-Sierra name associated with the
 * specified Directory Record.
 */
int
cdfs_GetIsoName(vfs, drec_buf, pname)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*drec_buf;				/* Dir Rec buffer structure		*/
struct pathname		*pname;					/* Pathname struct to put name	*/
{
	union media_drec	*drec;				/* Dir Rec template				*/
	uchar_t				*name;				/* Addr of name within Dir Rec	*/ 
	uint_t				namelen;			/* Length of name				*/

	drec = (union media_drec *)drec_buf->sb_ptr;

	/*
	 * Locate the ISO-9660/High-Sierra name i.e. File ID string.
	 */
	switch (CDFS_TYPE(vfs)) {
		case CDFS_ISO_9660: {
			name = (uchar_t *)&drec->Iso.drec_VarData;
			namelen = drec->Iso.drec_FileIDSz;
			break;
		}
		case CDFS_HIGH_SIERRA: {
			name = (uchar_t *)&drec->Hs.drec_VarData;
			namelen = drec->Hs.drec_FileIDSz;
			break;
		}
		default: {
			cmn_err(CE_WARN,
				"cdfs_GetIsoName(): Invalid CDFS type: 0x%x\n",
				CDFS_TYPE(vfs));
			return(RET_ERR);
		}
	}

	if (namelen == 0) {
		cmn_err(CE_NOTE,
			"cdfs_GetIsoName(): Invalid File ID Length: %d\n", namelen);
		return(RET_ERR);
	}	

	cdfs_pn_set(pname, name, namelen);
	return(RET_OK);
}



/*
 * Convert the File-releative byte offset to the
 * corresponding Media-relative physical block #/offset. 
 * Note: The last byte of each File Section need not
 * end on a block boundry.  Thus, for multi-extent files,
 * a File-relative logical block boundry do not necessarily
 * correspond to a physical block boundry.
 */
int
cdfs_bmap(vfs, ip, offset, blkno, blkoff, lastcont)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_inode	*ip;					/* Inode structure for file		*/
ulong_t				offset;					/* File-relative byte offset	*/
daddr_t				*blkno;					/* Media-relative physcal blk # */
uint_t				*blkoff;				/* Offset within physical blk	*/
uint_t				*lastcont;				/* Last byte of continuous data	*/
{
	struct cdfs_drec	*drec;				/* Dir Rec structure			*/
	uint_t				bytecnt; 			/* Cummulative byte count		*/
	uint_t				blkcnt;				/* Cummulative blk cnt in D-Rec	*/
	uint_t				unit_size;			/* # bytes in a file unit		*/
	uint_t				cmpl_units;			/* # of complete File Units		*/
	uint_t				end; 				/* End of continuous file data	*/

	/*
	 * Locate the Dir Rec that contains the desired location.
	 */
	if (offset >= ip->i_Size) {
		return(RET_EOF);
	}

	drec = ip->i_DirRec;
	ASSERT(drec != NULL);

	bytecnt = 0;
	for (;;) {
		if (bytecnt + drec->drec_DataLen > offset) {
			break;
		}

		bytecnt += drec->drec_DataLen;
		drec = drec->drec_NextDR;

		if (drec == ip->i_DirRec) {
			return(RET_EOF);
		}
	}

	/*
	 * We've found the Dir Rec that contains the desired data.
	 * - Initialize the block counter to the # of blocks in the
	 *	 Extent preceeding the file data.
	 * - Initialize the ending file offset to the end of the
	 *	 continuous area that contains the file data, i.e. the
	 *	 end of this File Section.
	 */
	blkcnt = drec->drec_XarLen;
	end = bytecnt + drec->drec_DataLen;

	/*
	 * If this is an interleaved Extent, locate the File Unit that
	 * contains the desired location.
	 * - Compute the # of bytes in a File Unit.
	 * - Compute the # of complete File Units to skip.
	 * - Increment the byte count by the # bytes in the complete/full
	 *	 File Units.  Note: Interleave blocks do not contain file data.
	 * - Recompute the File Offset of the end of the continuous area
	 *	 that contains the file data.  This continuous area will be
	 *	 limited by the end of the current File Unit.
	 * - Increment the block count by the # of blocks consumed by the
	 *	 complete/full File Units, including the interleave blocks.
	 */
	if (drec->drec_UnitSz != 0) {
		unit_size = drec->drec_UnitSz << CDFS_BLKSHFT(vfs);
		cmpl_units = (offset - bytecnt) / unit_size;
		bytecnt += (cmpl_units * unit_size);
		end = MIN(end, bytecnt + unit_size);
		blkcnt += ((drec->drec_UnitSz + drec->drec_Interleave) * cmpl_units);
	}

	/*
	 * Now that we've located a continuous set of physical blocks
	 * that contains the desired offset, the desired physical block #
	 * and offset are straigh-forward:
	 * - Compute the physical block #.
	 * - Since the current byte counter is pointing to the beginning
	 *	 of a block, the final block offset is simply the non-block
	 *	 portion of the remaining # of byte.
	 */
	if (blkno != NULL) {
		*blkno = drec->drec_ExtLoc + blkcnt +
			((offset - bytecnt) >> CDFS_BLKSHFT(vfs));
	}

	if (blkoff != NULL) {
		*blkoff = ((offset - bytecnt) & CDFS_BLKMASK(vfs));
	}

	if (lastcont != NULL) {
		*lastcont = end;
	}

	return(RET_OK);
}



/*
 * Read the Logical Sector that contains the desired media data.
 * - CDFS_FBUFIO - The desired location is specified by 'sb_offset'.
 * - CDFS_BUFIO: The desired location is specified by 'sb_sect'.
 */
int
cdfs_ReadSect(vfs, tbuf)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*tbuf;					/* Temp. I/O buffer structure	*/
{
	uint_t		sectoff;					/* Sector offset of the location*/
	uint_t		lastcont;					/* Last byte of continuous data	*/
	int			retval;						/* Return value of called procs	*/

	/*
	 * Release the current data and read in the new data.
	 */
	switch (tbuf->sb_type) {
		case CDFS_FBUFIO: {
			/*
			 * Read in the desired data based as specified by
			 * the current file offset:
			 * - Release current I/O buffer, if known.
			 * - Compute the sector # and sector offset of the
			 *	 desired location.
			 * - Compute the file offset of the beginning of the sector.
			 * - Read in the Logical Sector.
			 * - Initialize the remaining buffer parameters.
			 */
			if (tbuf->sb_fbp != NULL) {
				fbrelse(tbuf->sb_fbp);
				tbuf->sb_fbp = NULL;
				tbuf->sb_start = NULL;
				tbuf->sb_end = NULL;
				tbuf->sb_ptr = NULL;
			}

			retval = cdfs_SectNum(vfs, VTOI(tbuf->sb_vp), tbuf->sb_offset,
				&(tbuf->sb_sect), &sectoff, &lastcont);
			if (retval != RET_OK) {
				return(retval);
			}

			tbuf->sb_sectoff = tbuf->sb_offset - sectoff;

			retval = fbread(tbuf->sb_vp, tbuf->sb_sectoff,
				CDFS_SECTSZ(vfs), S_OTHER, &(tbuf->sb_fbp));
			if (retval != 0) {
				return(EIO);
			}

			/*
			 * Update the iobuf fields.
			 * Note: Buffer end is limited by the sector size
			 * or the amount of valid data within the sector.
			 */
			tbuf->sb_start = (uchar_t *)((tbuf->sb_fbp)->fb_addr);
			tbuf->sb_end = tbuf->sb_start +
				MIN(lastcont - tbuf->sb_sectoff, CDFS_SECTSZ(vfs));
			tbuf->sb_ptr = tbuf->sb_start + sectoff;

			break;
		}
		case CDFS_BUFIO: {
			/*
			 * Read in the desired data based as specified by
			 * the current file offset:
			 * - Release current I/O buffer, if known.
			 * - Read in the specified Logical Sector.
			 * - Compute the file offset of the beginning of the sector.
			 * - Read in the Logical Sector.
			 * - Initialize the remaining buffer parameters.
			 */
			if (tbuf->sb_bp != NULL) {
				brelse(tbuf->sb_bp);
				tbuf->sb_bp = NULL;
				tbuf->sb_start = NULL;
				tbuf->sb_end = NULL;
				tbuf->sb_ptr = NULL;
			}

			 if (tbuf->sb_offset >= CDFS_SECTSZ(vfs)) {
				tbuf->sb_sect +=
					(tbuf->sb_offset >> CDFS_SECTSHFT(vfs));
				tbuf->sb_offset &= CDFS_SECTMASK(vfs);
			 }

			 tbuf->sb_bp = bread(tbuf->sb_dev, tbuf->sb_sect,
				CDFS_SECTSZ(vfs));

			if (((tbuf->sb_bp)->b_error & B_ERROR) != 0) {
				brelse(tbuf->sb_bp);
				tbuf->sb_bp = NULL;
				return(EIO);
			}
			
			tbuf->sb_start = (uchar_t *)((tbuf->sb_bp)->b_un.b_addr);
			tbuf->sb_end = tbuf->sb_start + CDFS_SECTSZ(vfs);
			tbuf->sb_ptr = (uchar_t *)tbuf->sb_start + tbuf->sb_offset;
			break;
		}
		/* NOTREACHED */
		default: {
			cmn_err(CE_WARN, "cdfs_ReadSect(): Unknown buffer type: 0x%x\n",
				tbuf->sb_type);
			return(RET_ERR); 
		}
	}

	return(RET_OK);
}



/*
 * Fill the I/O buffer with data from the device.
 * Support for spanning "logically adjacent" sectors is transparent.
 *
 * - CDFS_FBUFIO:
 * - CDFS_BUFIO: The next "logically adjacent" sector is 
 *	specified by the 'sb_nextsect' field.
 */
int
cdfs_FillBuf(vfs, tbuf)
struct vfs			*vfs;					/* File system's VFS structure	*/
struct cdfs_iobuf	*tbuf;					/* Temp. I/O buffer structure	*/
{
	uint_t		sect;						/* Temp var for current sect #	*/
	uint_t		sectoff;					/* Temp var for cur. sect offset*/
	uint_t		offset;						/* Temp var for cur. offset		*/

	uchar_t		*start;						/* Starting addr of byte copy	*/
	uint_t		cnt;						/* # of bytes to copy			*/
	int			retval;						/* Return value of called procs	*/

	/*
	 * Determine if we're currently using the temporary buffer.
	 * This means that the previous record crossed a sector boundry.
	 */
	if ((tbuf->sb_tmpbuf != NULL) &&
		(tbuf->sb_start == tbuf->sb_tmpbuf)) {
		/*
		 * We're currently using the temp buffer.
		 * See if the desired record begins in that portion
		 * of the temp buffer that belongs to the "earlier sector".
		 */
		if ((tbuf->sb_ptr >= tbuf->sb_start) &&
			(tbuf->sb_ptr < tbuf->sb_split)) {
			/*
			 * The desired record begins in the "earlier sector"
			 * portion of the temp buffer.  All that is needed
			 * is to make sure there's enough data from the 
			 * "latter sector" to represent the entire desired record.
			 */
			if (tbuf->sb_ptr + tbuf->sb_reclen > tbuf->sb_end) {
				/*
				 * Only the first portion of the desired record is
				 * in the buffer.  So, copy the "missing" data from
				 * the "latter sector" buffer into the temp buf.
				 * - Compute the # of bytes missing.
				 * - Compute start addr within the "latter sector" buf,
				 *	 i.e. The location that the previous copy ended.
				 * - Verify that there is enough data in the sector
				 *	 and that there is enough space in the temp buffer.
				 * - Copy the data to the temp buffer.
				 * - Update the new end of the valid temp buffer data. 
				 */
				cnt = (tbuf->sb_ptr + tbuf->sb_reclen) - tbuf->sb_end;
				start = tbuf->sb_nextptr + (tbuf->sb_end - tbuf->sb_split);

				if ((cnt > (tbuf->sb_nextend - start)) ||
					(cnt > ((tbuf->sb_tmpbuf+cdfs_TmpBufSz)-tbuf->sb_end))) {
					/*
					 * The requested record exceeds either the "latter"
					 * sector or the temp buffer storage.  Either the
					 * size of the temp buffer needs to be increased,
					 * or we need to add support for a single record
					 * spanning more than two sectors.
					 */
					cmn_err(CE_WARN,
						"cdfs_FillBuf(): Unable to 'hold' entire data record:");
					cmn_err(CE_CONT,
						"Sect %d,  Sect offset %d,  Record len %d\n",
						tbuf->sb_sect, (tbuf->sb_offset - tbuf->sb_sectoff),
						tbuf->sb_reclen);

					if (cnt > (tbuf->sb_nextend - start)) {
						cmn_err(CE_CONT,
							"Record overflows secondary sector (len=%d).\n\n",
							tbuf->sb_nextend - tbuf->sb_nextptr);
					} else {
						cmn_err(CE_CONT,
							"CDFS Temp buffer too small (size=%d).\n\n",
							cdfs_TmpBufSz);
					}

					return(RET_ERR);
				}

				bcopy((caddr_t)start, (caddr_t)tbuf->sb_end, cnt);
				tbuf->sb_end += cnt;
			}
			return(RET_OK);
		}

		/*
		 * We're currently using the temp buffer, and 
		 * the desired record does not begin in that portion
		 * of the temp buffer belonging to the "earlier" sector.
		 * Therefore, the contents of the temp buf are no longer
		 * needed.
		 *
		 * If the desired record begins within the 
		 * "latter sector", then fixup the various buffer pointers
		 * to point to the sector buffer of the "latter sector".
		 *
		 * Otherwise, the "latter sector" is no longer needed,
		 * so no "fixup" needs to happen.  All of the pointers
		 * will be reset when the needed sector is read.
		 */
		if ((tbuf->sb_ptr >= tbuf->sb_split) &&
			(tbuf->sb_ptr <
				tbuf->sb_split + (tbuf->sb_nextend - tbuf->sb_nextptr))) {
			/*
			 * The desire record beings within the "latter sector".
			 * So, fixup the pointers to point to the real sector buffer: 
			 * - Fixup "current" sector num.
			 * - Fixup offset of "current" setctor.
			 * - Fixup start and end pointers.
			 * - Fixup roving pointer.
			 * - Fixup the roving offset.
			 */
			tbuf->sb_sect = tbuf->sb_nextsect;
			tbuf->sb_sectoff = tbuf->sb_nextsectoff;
			tbuf->sb_start = tbuf->sb_nextstart;
			tbuf->sb_end = tbuf->sb_nextend;
			tbuf->sb_ptr = tbuf->sb_nextptr + 
				(tbuf->sb_ptr - tbuf->sb_split);
			tbuf->sb_offset = tbuf->sb_sectoff +
				(tbuf->sb_ptr - tbuf->sb_start);
		}
	}

	/*
	 * The temp buffer is not (no longer) being used.
	 * If the desired record does not begin within the
	 * current sector then read in the needed sector.
	 */
	if ((tbuf->sb_ptr < tbuf->sb_start) ||
		(tbuf->sb_ptr >= tbuf->sb_end)) {
		retval = cdfs_ReadSect(vfs, tbuf);
		if (retval != RET_OK) {
			return(retval);
		}
	}
		
	/*
	 * If the desired record extends beyond the boundry of the
	 * current sector, then read in the next sect and allocate
	 * a temp buffer to create a temporary contiguous area to
	 * store the entire record.
	 */
	if (tbuf->sb_ptr + tbuf->sb_reclen > tbuf->sb_end) {
		/*
		 * The current record spans a sector boundry.  So,
		 * allocate a temp buffer and fill it with the
		 * entire record, some data from each sector buffer.
		 */
		if (tbuf->sb_tmpbuf == NULL) {
			tbuf->sb_tmpbuf = (uchar_t *)kmem_fast_alloc(&cdfs_TmpBufPool,
				cdfs_TmpBufSz, 1, KM_SLEEP);
			if (tbuf->sb_tmpbuf == NULL) {
				cmn_err(CE_WARN,
					"cdfs_FillBuf(): Unable to allocate a temporary buffer.\n");
				return(ENOMEM);
			}
		}

		/*
		 * Copy the first portion of the record from the
		 * current sector buffer to the temp buffer.
		 */
		cnt = tbuf->sb_end - tbuf->sb_ptr;
		bcopy((caddr_t)tbuf->sb_ptr, (caddr_t)tbuf->sb_tmpbuf, cnt);
		tbuf->sb_split = tbuf->sb_tmpbuf + cnt;

		/*
		 * Save the relevent data of the current buffer.
		 */
		sect = tbuf->sb_sect;
		sectoff = tbuf->sb_sectoff;
		offset = tbuf->sb_offset;

		/*
		 * Read in the next sector.
		 */
		switch (tbuf->sb_type) {
			case CDFS_FBUFIO: {
				tbuf->sb_offset += cnt;
				break;
			}
			case CDFS_BUFIO: {
				tbuf->sb_sect = tbuf->sb_nextsect;
				tbuf->sb_offset = tbuf->sb_nextoffset;
				break;
			}
			default: {
				cmn_err(CE_WARN, "cdfs_FillBuf(): Invalid IOBUF type: 0x%x\n",
					tbuf->sb_type);
				return(RET_ERR);
			}
		}

		retval = cdfs_ReadSect(vfs, tbuf);
		if (retval != RET_OK) {
			return(retval);
		}

		/*
		 * Compute the # of bytes to be copied from the
		 * new (latter) sector.  Also, verify that we
		 * now have enough data and storage space to
		 * hold the entire record.
		 */
		cnt = tbuf->sb_reclen - cnt;

		if ((cnt > (tbuf->sb_end - tbuf->sb_ptr)) ||
			(cnt > ((tbuf->sb_tmpbuf + cdfs_TmpBufSz) - tbuf->sb_split))) {
			/*
			 * The requested record exceeds either the "latter"
			 * sector or the temp buffer storage.  Either the
			 * size of the temp buffer needs to be increased,
			 * or we need to add support for a single record
			 * spanning more than two sectors.
			 */
			cmn_err(CE_WARN,
				"cdfs_FillBuf(): Unable to 'hold' entire data record");
			cmn_err(CE_CONT,
				"Sector=%d,  Sect offset=%d,  Record len=%d\n",
				sect, offset - sectoff, tbuf->sb_reclen);

			if (cnt > (tbuf->sb_end - tbuf->sb_ptr)) {
				cmn_err(CE_CONT,
					"Record overflows secondary sector (len=%d).\n\n",
					tbuf->sb_end - tbuf->sb_ptr);
			} else {
				cmn_err(CE_CONT,
					"CDFS Temporary buffer too small (size=%d).\n\n",
					cdfs_TmpBufSz);
			}

			return(RET_ERR);
		}

		bcopy((caddr_t)tbuf->sb_ptr, (caddr_t)tbuf->sb_split, cnt);

		/*
		 * Save the data describing the new ("latter") sector
		 * so that it can be restored when the temp buffer
		 * is no longer needed.
		 */
		tbuf->sb_nextsect = tbuf->sb_sect;
		tbuf->sb_nextsectoff = tbuf->sb_sectoff;
		tbuf->sb_nextstart = tbuf->sb_start;
		tbuf->sb_nextend = tbuf->sb_end;
		tbuf->sb_nextptr = tbuf->sb_ptr;

		/*
		 * Fix up the various pointers to point to the
		 * desired record as stored in the temp buffer.
		 */
		tbuf->sb_sect = sect;
		tbuf->sb_sectoff = sectoff;
		tbuf->sb_start = tbuf->sb_tmpbuf;
		tbuf->sb_end = tbuf->sb_tmpbuf + tbuf->sb_reclen;
		tbuf->sb_ptr = tbuf->sb_tmpbuf;
		tbuf->sb_offset = offset;
	}

	return(RET_OK); 
}
