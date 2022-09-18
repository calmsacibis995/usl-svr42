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

#ident	"@(#)uts-comm:fs/cdfs/cdfs_ioctl.c	1.8"
#ident	"$Header: $"

static char cdfs_copyright[] = "Copyright 1991, 1992 Intel Corp. 469252";

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_ioctl.h>
#include <fs/cdfs/cdrom.h>
#include <fs/cdfs/iso9660.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/ddi.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
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
#include <util/types.h>

#include "cdfs.h"

/*
 * CD-ROM file system (CDFS) ioctl routine
 *
 * This ioctl interface is used by the library functions that are specified
 * by the X/Open XCDR and RockRidge Group RRIP APIs.  Any application use
 * of this functionality should be through those libraries, not directly
 * through the ioctl interface.
 *
 * These ioctls are:
 *		CDFS_GETXAR		- Get eXtended Attribute Record from a file
 *		CDFS_GETDREC	- Get Directory Record for a file/directory
 *		CDFS_GETPTREC	- Get Path Table Record for a directory
 *		CDFS_GETSUF		- Get System Use Field from DREC System Use Area
 *		CDFS_GETTYPE	- Get the file system standard of a CD-ROM
 *		CDFS_DODEFS		- Get/set default uid, gid, and perms of a file/dir
 *		CDFS_DOIDMAP	- Get/set uid/gid mappings for the entire CD-ROM
 *		CDFS_DONMCONV	- Get/set file/dir name conversion options
 *		CDFS_SETDEVMAP	- Set device number mapping
 *		CDFS_GETDEVMAP	- Get device number mapping
 *
 * (All files/directories operated on are members of the CD-ROM file system.)
 *
 * The only functionality in these APIs that is not implemented by an
 * ioctl is that of the cd_pvd() function.  Since that function should
 * work even when the CD-ROM is not mounted, it would not be able to use an
 * ioctl.  It is implemented in a library, where it is available for a
 * variety of utilities to use.  Again, any use of this functionality
 * should be through the API only.
 *
 * These ioctls are only defined by the APIs to work with ISO-9660 discs.
 * However, these ioctls (except for CDFS_GETSUF - the implementation of
 * the cd_suf() function) also support High Sierra discs.
 *
 * Refer to the XCDR, RRIP, and RockRidge SUSP specifications for more
 * information about any of the functionality being implemented here.
 */



extern unsigned int		strlen (const char *);
extern int				vfs_getvfsswind (const char *);
extern int				maxminor;


/*
 * cdfs ioctl routine.
 */

/* ARGSUSED 5 */
int
cdfs_ioctl (vp, cmd, datap, flags, cr, rvalp)
	struct vnode		*vp;				/* Vnode pointer				*/
	int					cmd;				/* ioctl to be done				*/
	char				*datap;				/* User-supplied data			*/
	int					flags;				/* ioctl flags (not used)		*/
	struct cred			*cr;				/* Credential info about caller	*/
	int					*rvalp;				/* Return value pointer			*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct pathname		KPath;				/* Working path in kernel space	*/
	uchar_t				*UPath;				/* Path in user space			*/
	caddr_t				UArgPtr;			/* Argument struct in user space*/
	struct vnode		*RootVp;			/* Root vnode Ptr				*/
	struct vnode		*ActualVp = NULL;	/* Vnode of actual path			*/

	if ((vp == NULL) || (datap == NULL) || (cr == NULL) || (rvalp == NULL)) {
		return (EFAULT);
	}

	/*
	 * Some of the ioctls act on the whole file system, so they use the
	 * root vnode pointer.  Others act on individual files/directories,
	 * so they work with the individual vnode, or take the root vnode
	 * and the path of the file/directory.
	 *
	 * Regardless, all ioctls are to be invoked on the file descriptor of
	 * the file system root.  (See the comment in cdfs_ioctl.h for details.)
	 * Abort if this is not the root vnode.
	 */
	if (vp != (ITOV (CDFS_ROOT_INODE (vp->v_vfsp)))) {
		return (EBADFD);
	}

	RootVp = vp;

	RetVal = cdfs_GetIocArgs (datap, &KPath, &UPath, &UArgPtr, &ActualVp,
				cr);

	if (RetVal == 0) {
		switch (cmd) {
			/* Get an eXtended Attribute Record from the CD-ROM. */
			case CDFS_GETXAR: {
				RetVal = cdfs_ioc_GetXAR (ActualVp, UArgPtr);
				break;
			}

			/* Get a Directory Record from the CD-ROM. */
			case CDFS_GETDREC: {
				RetVal = cdfs_ioc_GetDREC (ActualVp, UArgPtr);
				break;
			}

			/* Get a Path Table Record from the CD-ROM. */
			case CDFS_GETPTREC: {
				RetVal = cdfs_ioc_GetPTREC (ActualVp, UArgPtr);
				break;
			}

			/*
			 * Get a System Use Field out of a Directory Record's System Use
			 * Area.
			 */
			case CDFS_GETSUF: {
				RetVal = cdfs_ioc_GetSUF (ActualVp, UArgPtr);
				break;
			}

			/* Get the standard that the CD-ROM is recorded to. */
			case CDFS_GETTYPE: {
				RetVal = cdfs_ioc_GetType (RootVp, UArgPtr);
				break;
			}

			/*
			 * Set or get default user ID, group ID, file permissions,
			 * directory permissions, and directory search permission
			 * interpretation.  These will then apply to the entire CD-ROM.
			 */
			case CDFS_DODEFS: {
				RetVal = cdfs_ioc_DoDefs (RootVp, UArgPtr, cr);
				break;
			}

			/* Set or get a single user or group ID mapping. */
			case CDFS_DOIDMAP: {
				RetVal = cdfs_ioc_DoIDMap (RootVp, UArgPtr, cr);
				break;
			}

			/* Set or get file/directory name conversion options. */
			case CDFS_DONMCONV: {
				RetVal = cdfs_ioc_DoNmConv (RootVp, UArgPtr, cr);
				break;
			}

			/* Set a single device number mapping entry. */
			case CDFS_SETDEVMAP: {
				RetVal = cdfs_ioc_SetDevMap (ActualVp,
							(uchar_t *) KPath.pn_path, UArgPtr, cr);
				break;
			}

			/* Get a single device number mapping entry. */
			case CDFS_GETDEVMAP: {
				RetVal = cdfs_ioc_GetDevMap (ActualVp, UPath, UArgPtr);
				break;
			}

			/* Unknown ioctl. */
			default: {
				RetVal = EINVAL;
				break;
			}
		}
	}

	(void) cdfs_ioc_Cleanup (ActualVp, &KPath);

	*rvalp = RetVal;
	return (RetVal);
}





/*
 * Collect the real arguments from the ioctl argument pointer.  Leave the
 * ioctl-specific data structure untouched, for the individual ioctls to
 * copyin.
 */
STATIC int
cdfs_GetIocArgs (dataptr, kpath, upath, uargptr, vpp, cr)
	const caddr_t		dataptr;			/* Ioctl routine data pointer	*/
	struct pathname		*kpath;				/* Working path in kernel space	*/
	uchar_t				**upath;			/* Working path in user space	*/
	caddr_t				*uargptr;			/* Ptr to user's argument struct*/
	struct vnode		**vpp;				/* Vnode of real argument struct*/
	struct cred			*cr;				/* Credential info about caller	*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct cdfs_IocArgs	IocArgs;			/* Ioctl argument pointer		*/

	if (copyin ((caddr_t) (struct cdfs_IocArgs *) dataptr, (caddr_t) &IocArgs,
				sizeof (struct cdfs_IocArgs)) != 0 ) {
		kpath->pn_buf = NULL;
		return (EFAULT);
	}

	if ((IocArgs.PathName == NULL) || (IocArgs.ArgPtr == NULL)) {
		kpath->pn_buf = NULL;
		return (EFAULT);
	}

	*upath = IocArgs.PathName;
	*uargptr = IocArgs.ArgPtr;

	if (pn_get ((caddr_t) *upath, UIO_USERSPACE, kpath) != 0) {
		kpath->pn_buf = NULL;
		return (EFAULT);
	}

	/*
	 * Get the vnode of the real file.
	 */
	RetVal = cdfs_IocGetVnode (kpath, vpp, cr);
	if (RetVal != 0) {
		return (RetVal);
	}

	switch (CDFS_TYPE ((*vpp)->v_vfsp)) {
		case CDFS_ISO_9660:
			/* FALLTHROUGH */
		case CDFS_HIGH_SIERRA: {
			break;
		}
		default: {
			cmn_err (CE_WARN,
						"cdfs_ioctl(): Unsupported CD-ROM file system type");
			return (EINVAL);
			/* NOTREACHED */
			break;
		}
	}

	return (0);
}





/*
 * Do any necessary cleanup activities before exiting ioctl code.
 */
STATIC int
cdfs_ioc_Cleanup (vp, path)
	struct vnode		*vp;				/* Vnode pointer				*/
	struct pathname		*path;				/* Path struct pointer			*/
{
	if (vp != NULL) {
		VN_RELE (vp);
	}

	if (path->pn_buf != NULL) {
		pn_free (path);
	}

	return (0);
}





/*
 * Turn a path into a vnode.  Note that it's not as simple as just
 * calling lookupname(), since that might return a vnode that belongs
 * to another fs type, like specfs.  It's enough to get the inode of
 * the parent, and use cdfs_DirLookup() to get the cdfs inode of the
 * path.
 *
 * Note that cdfs_DirLookup() will match names depending on
 * whether XCDR name conversion is turned on or not, and whether the
 * name given is a RRIP name or not.
 *
 * So, we need do no more with path name conversion here.
 */
STATIC int
cdfs_IocGetVnode (path, vpp, cr)
	const struct pathname *path;			/* Working path					*/
	struct vnode		**vpp;				/* Vnode of real argument struct*/
	struct cred			*cr;				/* Credential info for caller	*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct vnode		*pvp;				/* Parent's vnode ptr			*/
	struct cdfs_inode	*ip;				/* Inode of path				*/
	struct pathname		parent;				/* Actual path to work on		*/
	struct pathname		child;				/* File/dir to work on			*/
	cnt_t				count;				/* General purpose counter		*/
	int					cdfs_index;			/* Index of cdfs in vfssw[] tbl	*/
	boolean_t			FoundSlash = B_FALSE;	/* Did we find a slash?		*/

	/*
	 * Get path of parent.
	 */
	pn_alloc (&parent);
	for (count = (cnt_t) (path->pn_pathlen - 1); count >= 0; count--) {
		if (path->pn_path[count] == '/') {
			FoundSlash = B_TRUE;
			break;
		}
	}

	if (count > 0) {
		RetVal = pn_set (&parent, path->pn_path);
		parent.pn_path[count] = '\0';
		parent.pn_pathlen = count;
	} else {
		if (FoundSlash) {
			RetVal = pn_set (&parent, "/");
		} else {
			RetVal = pn_set (&parent, ".");
		}
	}

	if (RetVal != 0) {
		pn_free (&parent);
		return (RetVal);
	}
	
	/*
	 * Get vnode of parent.
	 */
	RetVal = lookupname (parent.pn_path, UIO_SYSSPACE, FOLLOW, NULLVPP, &pvp);
	pn_free (&parent);
	if (RetVal != 0) {
		if (pvp != NULL) {
			VN_RELE (pvp);
		}
		return (RetVal);
	}

	/*
	 * If the parent's vnode is not in cdfs, just get the vnode pointer
	 * directly with lookupname (since this must be a mount point).  Otherwise, 
	 * find the child pathname, and use cdfs_DirLookup to get the cdfs
	 * version of this path's vnode.
	 */
	cdfs_index = vfs_getvfsswind (CDFS_ID);
	if (cdfs_index != pvp->v_vfsp->vfs_fstype) {
		VN_RELE (pvp);
		RetVal = lookupname (path->pn_path, UIO_SYSSPACE, FOLLOW, NULLVPP, vpp);
		if (RetVal != 0) {
			if (*vpp != NULL) {
				VN_RELE (*vpp);
			}
			return (RetVal);
		}
	} else {
		pn_alloc (&child);
		RetVal = pn_set (&child, path->pn_path);
		pn_setlast (&child);

		if (RetVal != 0) {
			pn_free (&child);
			VN_RELE (pvp);
			return (RetVal);
		}

		RetVal = cdfs_DirLookup (pvp->v_vfsp, VTOI (pvp),
					(uchar_t *) child.pn_path, &ip, cr);
		pn_free (&child);
		VN_RELE (pvp);
		if (RetVal != 0) {
			return (RetVal);
		}
		if (ip == NULL) {
			return (EFAULT);
		}

		cdfs_UnlockInode (ip);
		*vpp = ITOV (ip);
	}

	/*
	 * Abort and warn the user if this vnode is not a cdfs vnode.
	 */
	if (cdfs_index != (*vpp)->v_vfsp->vfs_fstype) {
		cmn_err (CE_WARN, "cdfs_ioctl(): Not in cdfs file system: %s",
					path->pn_path);
		return (ENOSYS);
	}

	return (0);
}





/*
 * Ioctl to get the eXtended Attribute Record field from a given file.
 */
STATIC int
cdfs_ioc_GetXAR (vp, dataptr)
	const struct vnode	*vp;				/* Vnode we're working on		*/
	caddr_t				dataptr;			/* User space data pointer		*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct cdfs_XarArgs	Args;				/* Argument structure			*/
	struct cdfs_drec	*DRecInfo;			/* Pointer to Dir Record		*/

	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0 ) {
		return (EFAULT);
	}

	/*
	 * Acceptable argument ranges:
	 *		file section:	n > 1 (use file section #n), n == -1 (use last file
	 *							section)
	 *		XAR length:		n == -1 (value not specified), n >= 0 (get n bytes
	 *							of the XAR)
	 *		App Use length:	n == -1 (value not specified), n >= 0 (see below)
	 *		Esc Seq length:	n == -1 (value not specified), n >= 0 (see below)
	 *							(get the fixed length part of the XAR plus
	 *							the specified portion of the App Use and Esc
	 *							Seq parts of the XAR.)
	 *
	 * Either (XAR length) or (App Use length and Esc Seq length) may be given,
	 * but not all.
	 */
	if ((Args.fsec == 0) || (Args.fsec < -1)) {
		return (ENXIO);
	}
	if ((Args.xarlen < -1) || (Args.applen < -1) || (Args.esclen < -1)) {
		return (EINVAL);
	}
	if ((Args.xarlen > -1) && ((Args.applen > -1) || (Args.esclen > -1))) {
		return (EINVAL);
	}
	if (Args.Xar == NULL) {
		return (EFAULT);
	}

	/*
	 * Find the right DREC, and make sure this drec exists and has an
	 * XAR.
	 */
	DRecInfo = cdfs_FindDRec (vp, Args.fsec);
	if (DRecInfo == NULL) {
		return (ENXIO);
	}
	if(DRecInfo->drec_XarLen == 0) {
		return (ENOMATCH);
	}

	/*
	 * Get the XAR, and copy it out if successful.
	 */
	RetVal = cdfs_GetAndCopyXar (vp, DRecInfo, Args.xarlen, Args.applen,
				Args.esclen, (caddr_t) Args.Xar);

	return (RetVal);
}





/*
 * Find the linked list entry for the DREC of a given file section.
 */
STATIC struct cdfs_drec *
cdfs_FindDRec (vp, fsec)
	const struct vnode	*vp;				/* Vnode pointer				*/
	const int			fsec;				/* File section					*/
{
	cnt_t				count;				/* General purpose counter		*/
	struct cdfs_drec	*DRecInfo = NULL;	/* Pointer to Dir Record		*/

	DRecInfo = (VTOI (vp))->i_DirRec;
	if (DRecInfo == NULL) {
		return (NULL);
	}

	/*
	 * Step through file sections.  If we fall through the loop
	 * without ever finding the specified file section, that
	 * number does not exist.
	 */
	count = 1;
	do {
	 	/*
		 * Args.fsec will be either:
		 *		the file section number to get the XAR from, or
		 *		-1 to get the XAR from the last file section.
		 */
		if ((count == fsec) || ((fsec == -1) &&
					(DRecInfo->drec_NextDR == (VTOI (vp))->i_DirRec))) {
			return (DRecInfo);
		}
		DRecInfo = DRecInfo->drec_NextDR;
		if (DRecInfo == NULL || DRecInfo == (VTOI (vp))->i_DirRec) {
			return (NULL);
		}
		count++;
	} while (DRecInfo != (VTOI (vp))->i_DirRec);

	return (NULL);
}





/*
 * Retrieve the given XAR from the disc.  Copy it out in chunks, depending
 * on what the user has asked for.
 *
 * Note that only xarlen or the pair applen/esclen may be specified, not
 * all three!
 */
STATIC int
cdfs_GetAndCopyXar (vp, drec_ptr, xarlen, applen, esclen, dest)
	const struct vnode	*vp;				/* Vnode pointer				*/
	const struct cdfs_drec *drec_ptr;		/* DREC info structure			*/
	int					xarlen;				/* Amount of XAR user wants		*/
	int					applen;				/* Amount of App Use user wants	*/
	int					esclen;				/* Amount of Esc Seq user wants	*/
	caddr_t				dest;				/* Destination addr in user space*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	uint_t				XARSize;			/* Size of XAR (bytes)			*/
	struct cdfs_iobuf	xar_buf;			/* I/O buffer structure			*/
	int					resid;				/* How much of the XAR is left	*/
	int					RealEscLen;			/* Esc Seq length from media	*/
	int					RealAppLen;			/* App Use length from media	*/
	uint_t				SectSize;			/* Logical sector size			*/
	uint_t				BigOffset;			/* Total current offset in XAR	*/

	/*
	 * Set some commonly used variables.  Note that BigOffset is slightly
	 * different than sb_offset, since sb_offset is always forced to be < 1
	 * sector by cdfs_ReadSect.
	 */
	XARSize = drec_ptr->drec_XarLen << CDFS_BLKSHFT (vp->v_vfsp);
	SectSize = CDFS_SECTSZ (vp->v_vfsp);
	BigOffset = 0;

	/*
	 * Initialize I/O buffer structure.
	 *
	 * Note that the "predictable" contents of an XAR differ between ISO9660
	 * and High Sierra, in that the HS XAR also contains a copy of the DREC.
	 *
	 * Also note that the ISO_XAR_FIXEDLEN value is correct for both
	 * High Sierra AND ISO9660.
	 */
	CDFS_SETUP_IOBUF (&xar_buf, CDFS_BUFIO);
	xar_buf.sb_dev = CDFS_DEV (vp->v_vfsp);
	xar_buf.sb_sect = drec_ptr->drec_ExtLoc;
	switch (CDFS_TYPE (vp->v_vfsp)) {
		case CDFS_ISO_9660: {
			if (xarlen > -1) {
				xar_buf.sb_reclen = (xarlen < ISO_XAR_FIXEDLEN) ? xarlen :
							ISO_XAR_FIXEDLEN;
			} else {
				xar_buf.sb_reclen = ISO_XAR_FIXEDLEN;
			}
			break;
		}
		case CDFS_HIGH_SIERRA: {
			if (xarlen > -1) {
				xar_buf.sb_reclen =
							(xarlen < (ISO_XAR_FIXEDLEN + drec_ptr->drec_Len)) ?
							xarlen : (ISO_XAR_FIXEDLEN + drec_ptr->drec_Len);
			} else {
				xar_buf.sb_reclen = ISO_XAR_FIXEDLEN + drec_ptr->drec_Len;
			}
			break;
		}
	}

	/*
	 * Get the fixed length part of the XAR, and copy out the part the
	 * user wants.
	 */
	RetVal = cdfs_ReadSect (vp->v_vfsp, &xar_buf);
	if (RetVal != 0) {
		CDFS_RELEASE_IOBUF (&xar_buf);
		return (RetVal);
	}

	RetVal = copyout ((caddr_t) xar_buf.sb_ptr, dest, xar_buf.sb_reclen);
	if (RetVal != 0) {
		CDFS_RELEASE_IOBUF (&xar_buf);
		return (RetVal);
	}

	/*
	 * Are we already finished with this request?
	 */
	if (xarlen <= xar_buf.sb_reclen) {
		CDFS_RELEASE_IOBUF (&xar_buf);
		return (0);
	}

	/*
	 * Note the size of the variable length fields in this XAR.
	 *
	 * There is no Escape Sequence field in a High Sierra XAR.
	 * Setting RealEscLen to 0 will cause the code below that
	 * gets the Esc Seq field to do nothing.
	 */
	switch (CDFS_TYPE (vp->v_vfsp)) {
		case CDFS_ISO_9660: {
			RealEscLen = (int) ((union media_xar *)
						xar_buf.sb_start)->Iso.xar_EscSeqLen;
			RealAppLen = (int) ((union media_xar *)
						xar_buf.sb_start)->Iso.xar_ApplUseLen;
			break;
		}
		case CDFS_HIGH_SIERRA: {
			/*
			 * High Sierra does not have the Escape Sequence field.
			 */
			RealEscLen = 0;
			RealAppLen = (int) ((union media_xar *)
						xar_buf.sb_start)->Hs.xar_ApplUseLen;
			break;
		}
	}

	/*
	 * Get the remainder of the XAR, up to xarlen, or in pieces
	 * of size applen and esclen.
	 */
	if (xarlen > 0) {
		resid = ((xarlen < XARSize) ? xarlen : XARSize) - xar_buf.sb_reclen;
	} else {
		if (applen > 0) {
			resid = (applen < RealAppLen) ? applen : RealAppLen;
		}
	}

	xar_buf.sb_ptr += xar_buf.sb_reclen;
	xar_buf.sb_offset += xar_buf.sb_reclen;
	BigOffset += xar_buf.sb_reclen;
	xar_buf.sb_reclen = ((SectSize - xar_buf.sb_reclen) < resid) ?
				(SectSize - xar_buf.sb_reclen) : resid;

	/*
	 * Get either the rest of the XAR up to xarlen bytes, or exactly
	 * applen more bytes.
	 */
	while (resid > 0) {
		if ((xar_buf.sb_start == NULL) ||
					(xar_buf.sb_ptr < xar_buf.sb_start) ||
					(xar_buf.sb_ptr + xar_buf.sb_reclen > xar_buf.sb_end)) {
			RetVal = cdfs_ReadSect (vp->v_vfsp, &xar_buf);
			if (RetVal != 0) {
				CDFS_RELEASE_IOBUF (&xar_buf);
				return (RetVal);
			}
		}
		RetVal = copyout ((caddr_t) xar_buf.sb_ptr, dest + BigOffset,
					xar_buf.sb_reclen);
		if (RetVal != 0) {
			CDFS_RELEASE_IOBUF (&xar_buf);
			return (RetVal);
		}
		xar_buf.sb_ptr += xar_buf.sb_reclen;
		xar_buf.sb_offset += xar_buf.sb_reclen;
		BigOffset += xar_buf.sb_reclen;
		resid -= xar_buf.sb_reclen;
		xar_buf.sb_reclen = (SectSize < resid) ? SectSize : resid;
	}

	if (esclen > 0) {
		resid = (esclen < RealEscLen) ? esclen : RealEscLen;
		xar_buf.sb_ptr += RealAppLen;
		xar_buf.sb_offset += RealAppLen;
		BigOffset += RealAppLen;
		xar_buf.sb_reclen = (resid <
					(SectSize - (xar_buf.sb_offset % SectSize))) ? resid :
					(SectSize - (xar_buf.sb_offset % SectSize));

		while (resid > 0) {
			if ((xar_buf.sb_start == NULL) ||
						(xar_buf.sb_ptr < xar_buf.sb_start) ||
						(xar_buf.sb_ptr + xar_buf.sb_reclen > xar_buf.sb_end)) {
				RetVal = cdfs_ReadSect (vp->v_vfsp, &xar_buf);
				if (RetVal != 0) {
					CDFS_RELEASE_IOBUF (&xar_buf);
					return (RetVal);
				}
			}

			RetVal = copyout ((caddr_t) xar_buf.sb_ptr, dest + BigOffset,
						xar_buf.sb_reclen);
			if (RetVal != 0) {
				CDFS_RELEASE_IOBUF (&xar_buf);
				return (RetVal);
			}
			xar_buf.sb_ptr += xar_buf.sb_reclen;
			xar_buf.sb_offset += xar_buf.sb_reclen;
			BigOffset += xar_buf.sb_reclen;
			resid -= xar_buf.sb_reclen;
			xar_buf.sb_reclen = SectSize < resid ? SectSize : resid;
		}
	}

	CDFS_RELEASE_IOBUF (&xar_buf);
	return (RetVal);
}





/*
 * Get the Directory RECord of a given section of a file/directory.
 */
STATIC int
cdfs_ioc_GetDREC (vp, dataptr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	caddr_t				dataptr;			/* User-specified data structure*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct cdfs_DRecArgs Args;				/* Argument structure			*/
	struct cdfs_drec	*DRecInfo;			/* Pointer to Dir Record		*/
	struct cdfs_iobuf	drec_buf;			/* DREC I/O structure			*/

	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0) {
		return (EFAULT);
	}
	if ((Args.fsec == 0) || (Args.fsec < -1)) {
		return (ENXIO);
	}
	if (Args.DRec == NULL) {
		return (EFAULT);
	}

	/*
	 * Find the specified DREC.
	 */
	DRecInfo = cdfs_FindDRec (vp, Args.fsec);
	if (DRecInfo == NULL) {
		return (ENXIO);
	}

	/*
	 * Read in this DREC and copy it out.
	 */
	RetVal = cdfs_RetrieveDRec (vp, DRecInfo, &drec_buf);
	if (RetVal == 0) {
		RetVal = copyout ((caddr_t) drec_buf.sb_ptr, (caddr_t) Args.DRec,
					drec_buf.sb_reclen);
	}
	CDFS_RELEASE_IOBUF (&drec_buf);

	return (RetVal);
}





/*
 * Get the given DREC, and copy it out to user space.
 *
 * Note that this assigns values into the cdfs_iobuf struct that
 * we were given, but that that struct must be freed by the caller.
 */
STATIC int
cdfs_RetrieveDRec (vp, drec_info, drec_buf)
	const struct vnode	*vp;				/* Vnode pointer				*/
	const struct cdfs_drec *drec_info;		/* DREC info structure			*/
	struct cdfs_iobuf	*drec_buf;			/* DREC I/O structure pointer	*/
{
	int 				RetVal;				/* Return value holder			*/

	CDFS_SETUP_IOBUF (drec_buf, CDFS_BUFIO);
	drec_buf->sb_sect = drec_info->drec_Loc;
	drec_buf->sb_offset = drec_info->drec_Offset;
	drec_buf->sb_dev = CDFS_DEV (vp->v_vfsp);
	drec_buf->sb_reclen = drec_info->drec_Len;

	RetVal = cdfs_ReadSect (vp->v_vfsp, drec_buf);

	return (RetVal);
}





/*
 * Get the Path Table RECord for the given directory.
 */
STATIC int
cdfs_ioc_GetPTREC (vp, dataptr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	caddr_t				dataptr;			/* User space data pointer		*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct cdfs_iobuf	ptrec_buf;			/* I/O buffer structure			*/
	int					resid;				/* Residual bytes left in PTREC	*/
	boolean_t			IsRoot = B_FALSE;	/* Is this the root vnode?		*/

	/*
	 * There will only be Path Table entries for directories, so fail
	 * if this is not a directory.
	 */
	if (vp->v_type != VDIR) {
		return (ENOTDIR);
	}

	if (ITOV (CDFS_ROOT_INODE (vp->v_vfsp)) == vp) {
		IsRoot = B_TRUE;
	}

	/*
	 * Initialize I/O buffer parameters.
	 */
	CDFS_SETUP_IOBUF (&ptrec_buf, CDFS_BUFIO);
	ptrec_buf.sb_sect = (CDFS_FS (vp->v_vfsp))->cdfs_PathTabLoc;
	ptrec_buf.sb_dev = CDFS_DEV (vp->v_vfsp);

	resid = (int) (CDFS_FS (vp->v_vfsp))->cdfs_PathTabSz;

	while ((RetVal = cdfs_GetNextPTRec (vp->v_vfsp, &ptrec_buf,
				&resid)) == 0) {
		/*
		 * Handle the special case where the path we were given is of
		 * the root of this file system.  We know that the root PTREC
		 * is the first one.
		 */
		if (IsRoot) {
			RetVal = 0;
			break;
		}

		/*
		 * Note that a return value of 0 means success, <0 means the values
		 * do not match.
		 */
		if ((RetVal = cdfs_CheckPTRec (vp, 
					(union media_ptrec *) ptrec_buf.sb_ptr, 
					CDFS_TYPE (vp->v_vfsp))) >= 0) {
			break;
		}
	}

	if (RetVal < 0) {
		RetVal = ENOMATCH;
	}

	if (RetVal == 0) {
		if (copyout ((caddr_t) ptrec_buf.sb_ptr, dataptr,
					ptrec_buf.sb_reclen) != 0) {
			RetVal = EFAULT;
		}
	}

	CDFS_RELEASE_IOBUF(&ptrec_buf);
	return (RetVal);
}





/*
 * Get the next path table record.  If there isn't enough of the PT
 * left for this record, about the only thing we can do is abort.
 *
 * Note that PTRECs may span sector boundaries.  This is magically
 * handled by cdfs_FillBuf.
 */
STATIC int
cdfs_GetNextPTRec (vfs, pt_buf, resid)
	struct vfs			*vfs;				/* Pointer to file system data	*/
	struct cdfs_iobuf	*pt_buf;			/* Working buffer				*/
	int					*resid;				/* Residual bytes left in PTREC	*/
{
	int 				RetVal = 0;			/* Return value holder			*/

	/*
	 * If we're not at the start of the first buffer, set sb_ptr to point
	 * at the start of the next PTREC.
	 */
	if (pt_buf->sb_start != NULL) {
		pt_buf->sb_ptr += pt_buf->sb_reclen;
		pt_buf->sb_offset += pt_buf->sb_reclen;
	}

	/*
	 * Get the fixed-length part of the PTREC, so that we know we have
	 * the length field.
	 */
	pt_buf->sb_reclen = ISO_PT_FIXEDLEN;
	if (*resid < pt_buf->sb_reclen) {
		return (-1);
	}

	if ((pt_buf->sb_start == NULL) || (pt_buf->sb_ptr < pt_buf->sb_start) ||
				(pt_buf->sb_ptr + pt_buf->sb_reclen > pt_buf->sb_end)) {
		pt_buf->sb_nextsect = pt_buf->sb_sect + 1;
		RetVal = cdfs_FillBuf (vfs, pt_buf);
		if (RetVal != 0) {
			return (RetVal);
		}
	}

	/* 
	 * Get the full PTREC, now that we know we have the size field in
	 * the working copy.  Make sure to account for a pad byte, if any.
	 */
	switch (CDFS_TYPE (vfs)) {
		case CDFS_ISO_9660: {
			pt_buf->sb_reclen += ((union media_ptrec *)
						pt_buf->sb_ptr)->Iso.pt_DirIDSz;
			break;
		}
		case CDFS_HIGH_SIERRA: {
			pt_buf->sb_reclen += ((union media_ptrec *)
						pt_buf->sb_ptr)->Hs.pt_DirIDSz;
			break;
		}
	}

	if ((pt_buf->sb_reclen % 2) != 0) {
		pt_buf->sb_reclen++;
	}

	if (*resid < pt_buf->sb_reclen) {
		return (-1);
	}

	if ((pt_buf->sb_ptr < pt_buf->sb_start) ||
				(pt_buf->sb_ptr + pt_buf->sb_reclen > pt_buf->sb_end)) {
		pt_buf->sb_nextsect = pt_buf->sb_sect + 1;
		RetVal = cdfs_FillBuf (vfs, pt_buf);
	}

	*resid -= pt_buf->sb_reclen;

	return (RetVal);
}





/*
 * Check the current Path Table Record to see if it's the one we're
 * looking for.
 *
 * Success is measured by having the same directory extent location.
 * This will always be unique.
 */
STATIC int
cdfs_CheckPTRec (vp, pt, type)
	const struct vnode	*vp;				/* Vnode pointer				*/
	const union media_ptrec *pt;			/* Pointer to this PTREC		*/
	const enum cdfs_type type;				/* CD file system standard		*/
{
	int 				RetVal = -1;		/* Return value holder			*/

	switch (type) {
		case CDFS_ISO_9660: {
			if (pt->Iso.pt_ExtendLoc ==
						(VTOI (vp))->i_DirRec->drec_ExtLoc) {
				RetVal = 0;
			}
			break;
		}
		case CDFS_HIGH_SIERRA: {
			if (pt->Hs.pt_ExtendLoc ==
						(VTOI (vp))->i_DirRec->drec_ExtLoc) {
				RetVal = 0;
			}
			break;
		}
	}

	return (RetVal);
}





/*
 * Get a System Use Field for a given file/dir.  This SUF is either in the
 * System Use Area (SUA) of the DREC or in a continuation (CE) area.
 */
STATIC int
cdfs_ioc_GetSUF (vp, dataptr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	caddr_t				dataptr;			/* User space data pointer		*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct cdfs_SUFArgs Args;				/* Argument structure			*/
	struct cdfs_drec	*DRecInfo;			/* Ptr to DREC linked list item	*/
	struct cdfs_iobuf	DRecBuf;			/* DREC I/O buffer				*/
	struct cdfs_iobuf	SufBuf;				/* SUF I/O buffer				*/
	struct susp_suf		*sua;				/* Pointer to SUA				*/
	struct susp_ce		ce_suf;				/* Continuation area SUF		*/
	uint_t				sualen;				/* SUA length					*/
	uint_t				suflen;				/* SUF length					*/
	char				sig[2];				/* Signature the user specified	*/
	uint_t				sig_symbol;			/* Signature converted to symbol*/

	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0) {
		return (EFAULT);
	}
	if (Args.SUF == NULL) {
		return (EFAULT);
	}
	if ((Args.fsec == 0) || (Args.fsec < -1)) {
		return (ENXIO);
	}
	if ((Args.index < 1) || (Args.length < 0)) {
		return (EINVAL);
	}
	if ((Args.signature != NULL) &&
				(copyin (Args.signature, (caddr_t) sig, sizeof (sig)) != 0)) {
		return (EFAULT);
	}
	if (Args.length == 0) {
		return (0);
	}

	/*
	 * Just bail out if we're ignoring RRIP - the code that reads SUFs will
	 * not behave itself well if we are ignoring RRIP.
	 */
	if ((CDFS_FLAGS (vp->v_vfsp) & CDFS_RRIP_ACTIVE) != CDFS_RRIP_ACTIVE) {
		return (ENOMATCH);
	}

	/*
	 * Find the specified DREC.
	 */
	DRecInfo = cdfs_FindDRec (vp, Args.fsec);
	if (DRecInfo == NULL) {
		return (ENXIO);
	}

	/*
	 * Read in the block holding this DREC (and thus, the SUF).
	 */
	RetVal = cdfs_RetrieveDRec (vp, DRecInfo, &DRecBuf);
	if (RetVal != 0) {
		CDFS_RELEASE_IOBUF (&DRecBuf);
		return (RetVal);
	}

	/*
	 * Find the start of the SUA, if there is one.
	 */
	RetVal = cdfs_LocSusp (vp->v_vfsp, &DRecBuf, &sua, &sualen);
	if (RetVal != 0) {
		if (RetVal < 0) {
			RetVal = EINVAL;
		}
		CDFS_RELEASE_IOBUF (&DRecBuf);
		return (RetVal);
	}

	if (sualen == 0) {
		CDFS_RELEASE_IOBUF (&DRecBuf);
		return (ENOMATCH);
	}

	/*
	 * Initialize SUF I/O structure.
	 */
	CDFS_SETUP_IOBUF(&SufBuf, CDFS_BUFIO);
	SufBuf.sb_dev = CDFS_DEV(vp->v_vfsp);
	SufBuf.sb_sect = DRecBuf.sb_sect;
	SufBuf.sb_sectoff = DRecBuf.sb_sectoff;
	SufBuf.sb_start = (uchar_t *) sua;
	SufBuf.sb_ptr = SufBuf.sb_start;
	SufBuf.sb_end = SufBuf.sb_start + sualen;
	SufBuf.sb_offset = DRecBuf.sb_offset + (SufBuf.sb_start - DRecBuf.sb_ptr);

	bzero ((caddr_t) &ce_suf, sizeof (ce_suf));

	/*
	 * Set the signature to look for.
	 */
	if (Args.signature == NULL) {
		sig_symbol = CDFS_SUFID_NULL;
	} else {
		sig_symbol = CDFS_SUFID (sig[0], sig[1]);
	}

	RetVal = cdfs_ReadSUF(vp->v_vfsp, &SufBuf, sig_symbol, Args.index,
				&ce_suf); 
	if (RetVal != RET_OK) {
		if ((RetVal == RET_EOF) || (RetVal == RET_NOT_FOUND)) {
			RetVal = ENOMATCH;
		} else {
			RetVal = EINVAL;
		}
	} else {
		suflen = ((struct susp_suf *) SufBuf.sb_ptr)->suf_Len;
		if (copyout ((caddr_t) SufBuf.sb_ptr, (caddr_t) Args.SUF,
					((suflen > Args.length) ? Args.length : suflen)) != 0) {
			RetVal = EFAULT;
		}
	}
	
	CDFS_RELEASE_IOBUF (&DRecBuf);
	CDFS_RELEASE_IOBUF (&SufBuf);

	return (RetVal);
}





/* 
 * Determine the type of the CD-ROM we have mounted.
 */
STATIC int
cdfs_ioc_GetType (vp, dataptr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	caddr_t				dataptr;			/* User space data pointer		*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	enum cdfs_type		Type;				/* CD type						*/
	/*
	 * Uncomment this to provide some info on unsupported fs types.
	 * See below.
	 *		uint_t				UndefTypeVal = CDFS_UNDEF_FS_TYPE;
	 */

	Type = CDFS_TYPE (vp->v_vfsp);

	switch (Type) {
		case CDFS_ISO_9660:
			/* FALLTHROUGH */
		case CDFS_HIGH_SIERRA: {
			if (copyout ((caddr_t) &Type, dataptr, sizeof (Type)) != 0) {
				RetVal = EFAULT;
			}
			break;
		}

		/*
		 * Uncomment this, if the cdfs_GetIocArgs() code changes to allow
		 * ioctls on unsupported fs types.
		 *
		 *	default: {
		 *		if (copyout ((caddr_t) &UndefTypeVal, dataptr, sizeof (Type)) !=
		 *					0) {
		 *			RetVal = EFAULT;
		 *		}
		 *		break;
		 *	}
		 */
	}

	return (RetVal);
}





/*
 * Set or get the default file attributes.  These attributes will be
 * applied for all files that don't have them specified in another way
 * (like permissions in an XAR or in an RRIP "PX" field).
 */
STATIC int
cdfs_ioc_DoDefs (vp, dataptr, cr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	caddr_t				dataptr;			/* User space data pointer		*/
	struct cred			*cr;				/* User credential struct ptr	*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct cdfs			*CdfsData;			/* FS private data struct		*/
	struct cdfs_DefArgs	Args;				/* Argument structure			*/
	uint_t				PermMask;			/* Mask to use for permissions	*/

	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0) {
		return (EFAULT);
	}
	if ((Args.DefCmd == CD_SETDEFS) &&
				(((Args.defs.def_uid < 0) || (Args.defs.def_uid > MAXUID) ||
				(Args.defs.def_gid < 0) || (Args.defs.def_gid > MAXUID) ||
				(Args.defs.def_fperm > MODEMASK) ||
				(Args.defs.def_dperm > MODEMASK)))) {
		return (EINVAL);
	}

	CdfsData = CDFS_FS (vp->v_vfsp);

	/*
	 * Note that we have no mechanism for acting on a subset of the
	 * possible arguments - we always act on all of them.
	 */
	switch (Args.DefCmd) {
		/*
		 * Setting defaults.
		 */
		case CD_SETDEFS: {
			/*
			 * Only valid if this is a privileged user.
			 */
			if (pm_denied (cr, P_FILESYS)) {
				return (EPERM);
			}
			CdfsData->cdfs_Dflts.def_uid = Args.defs.def_uid;
			CdfsData->cdfs_Dflts.def_gid = Args.defs.def_gid;

			/*
			 * Mask off any write bits in these permissions.
			 */
			PermMask = MODEMASK & ~(IWRITE_USER | IWRITE_GROUP | IWRITE_OTHER);
			CdfsData->cdfs_Dflts.def_fperm = Args.defs.def_fperm & PermMask;
			CdfsData->cdfs_Dflts.def_dperm = Args.defs.def_dperm & PermMask;

			switch (Args.defs.dirsperm) {
				case CD_DIRXAR:
					/* FALLTHROUGH */
				case CD_DIRRX: {
					 CdfsData->cdfs_Dflts.dirsperm = Args.defs.dirsperm;
					 break;
				}

				default: {
					RetVal = EINVAL;
					break;
				}
			}
			break;
		}

		/*
		 * Getting defaults.
		 */
		case CD_GETDEFS: {
			Args.defs.def_uid = CdfsData->cdfs_Dflts.def_uid;
			Args.defs.def_gid = CdfsData->cdfs_Dflts.def_gid;
			Args.defs.def_fperm = CdfsData->cdfs_Dflts.def_fperm;
			Args.defs.def_dperm = CdfsData->cdfs_Dflts.def_dperm;
			Args.defs.dirsperm = CdfsData->cdfs_Dflts.dirsperm;

			if (copyout ((caddr_t) &Args, dataptr, sizeof (Args)) != 0) {
				RetVal = EFAULT;
			}
			break;
		}

		default: {
			RetVal = EINVAL;
			break;
		}
	}

	return (RetVal);
}





/*
 * Set or get the ID mappings of files/directories.  An ID mapping
 * causes the effective owner/group to be different from how they
 * are recorded on the CD-ROM.
 */
STATIC int
cdfs_ioc_DoIDMap (vp, dataptr, cr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	caddr_t				dataptr;			/* User space data pointer		*/
	struct cred			*cr;				/* User credential struct ptr	*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	cnt_t				count;				/* General purpose counter		*/
	cnt_t				count2;				/* General purpose counter #2	*/
	struct cdfs			*CdfsData;			/* FS private data struct		*/
	struct cdfs_IDMapArgs Args;				/* Argument structure			*/
	uint_t				*Cap;				/* Cap variable on loops		*/
	caddr_t				TmpPtr = NULL;		/* Working pointer to found map	*/
	boolean_t			DoingUmap;			/* Are we doing user mapping?	*/
	boolean_t			Setting;			/* Are we setting a mapping?	*/

	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0) {
		return (EFAULT);
	}
	if (((Args.IDMapCmd == CD_SETUMAP) || (Args.IDMapCmd == CD_SETGMAP)) &&
				(Args.idmap.from_id > (ushort_t) MAXUID)) {
		return (EINVAL);
	}
	if (((Args.IDMapCmd == CD_SETUMAP) && (Args.idmap.to_uid > MAXUID)) ||
				((Args.IDMapCmd == CD_SETGMAP) &&
				(Args.idmap.to_gid > MAXUID))) {
		return (EINVAL);
	}

	CdfsData = CDFS_FS (vp->v_vfsp);

	/*
	 * Set some info about what kind of mapping operation we're doing.
	 */
	switch (Args.IDMapCmd) {
		case CD_SETUMAP:
			/* FALLTHROUGH */
		case CD_SETGMAP: {
			/*
			 * Only valid if this is a privileged user.
			 */
			if (pm_denied (cr, P_FILESYS)) {
				return (EPERM);
			}
			Setting = B_TRUE;
			break;
		}
		case CD_GETUMAP:
			/* FALLTHROUGH */
		case CD_GETGMAP: {
			Setting = B_FALSE;
			break;
		}
		default: {
			return (EINVAL);
			/* NOTREACHED */
			break;
		}
	}

	switch (Args.IDMapCmd) {
		case CD_SETUMAP:
			/* FALLTHROUGH */
		case CD_GETUMAP: {
			Cap = &CdfsData->cdfs_UidMapCnt;
			if (Args.count > CD_MAXUMAP) {
				return (ENXIO);
			}
			DoingUmap = B_TRUE;
			break;
		}
		case CD_SETGMAP:
			/* FALLTHROUGH */
		case CD_GETGMAP: {
			Cap = &CdfsData->cdfs_GidMapCnt;
			if (Args.count > CD_MAXGMAP) {
				return (ENXIO);
			}
			DoingUmap = B_FALSE;
			break;
		}
	}

	/*
	 * Find this mapping if it's already in use.  If a specific mapping
	 * number was requested, find it if it exists, otherwise scan through
	 * the table, checking IDs.
	 */
	if (Args.count != 0) {
		if (DoingUmap) {
			if (CdfsData->cdfs_UidMap[Args.count - 1].from_uid !=
						CDFS_UNUSED_MAP_ENTRY) {
				TmpPtr = (caddr_t) &CdfsData->cdfs_UidMap[Args.count - 1];
			}
		} else {
			if (CdfsData->cdfs_GidMap[Args.count - 1].from_gid !=
						CDFS_UNUSED_MAP_ENTRY) {
				TmpPtr = (caddr_t) &CdfsData->cdfs_GidMap[Args.count - 1];
			}
		}
	} else {
		for (count = 0; count <= *Cap; count++) {
			if (DoingUmap) {
				if ((ulong_t) CdfsData->cdfs_UidMap[count].from_uid ==
							(uid_t) Args.idmap.from_id) {
					TmpPtr = (caddr_t) &CdfsData->cdfs_UidMap[count];
					break;
				}
			} else {
				if ((ulong_t) CdfsData->cdfs_GidMap[count].from_gid ==
							(gid_t) Args.idmap.from_id) {
					TmpPtr = (caddr_t) &CdfsData->cdfs_GidMap[count];
					break;
				}
			}
		}
	}

	/*
	 * Are we clearing this map entry?  (Clearing is indicated by to_?id
	 * values of CDFS_UNUSED_MAP_ENTRY.)  If so, move all the remaining
	 * map entries up by one, and we're done.  Otherwise, assign the
	 * mapping.
	 */
	if (((Setting) && (DoingUmap) &&
				(Args.idmap.to_uid == CDFS_UNUSED_MAP_ENTRY)) ||
				((Setting) && (!DoingUmap) &&
				(Args.idmap.to_gid == CDFS_UNUSED_MAP_ENTRY))) {
		if (TmpPtr == NULL) {
			return (ENOMATCH);
		}
		for (count2 = count; count2 <= (*Cap - 1); count2++) {
			if (DoingUmap) {
				CdfsData->cdfs_UidMap[count2].from_uid =
							CdfsData->cdfs_UidMap[count2 + 1].from_uid;
				CdfsData->cdfs_UidMap[count2].to_uid =
							CdfsData->cdfs_UidMap[count2 + 1].to_uid;
			} else {
				CdfsData->cdfs_GidMap[count2].from_gid =
							CdfsData->cdfs_GidMap[count2 + 1].from_gid;
				CdfsData->cdfs_GidMap[count2].to_gid =
							CdfsData->cdfs_GidMap[count2 + 1].to_gid;
			}
		}

		if (DoingUmap) {
			CdfsData->cdfs_UidMap[count2].from_uid = CDFS_UNUSED_MAP_ENTRY;
			CdfsData->cdfs_UidMap[count2].to_uid = CDFS_UNUSED_MAP_ENTRY;
		} else {
			CdfsData->cdfs_GidMap[count2].from_gid = CDFS_UNUSED_MAP_ENTRY;
			CdfsData->cdfs_GidMap[count2].to_gid = CDFS_UNUSED_MAP_ENTRY;
		}

		(*Cap)--;
		if (*Cap < 0) {
			cmn_err (CE_WARN, "cdfs_ioctl(): ID map count underflow, setting to 0");
			*Cap = 0;
		}
	} else {
		/*
		 * If we found this mapping in the list, set or get it.
		 */
		if (TmpPtr != NULL) {
			if (Setting) {
				if (DoingUmap) {
					((struct cd_uidmap *) TmpPtr)->to_uid = Args.idmap.to_uid;
				} else {
					((struct cd_gidmap *) TmpPtr)->to_gid = Args.idmap.to_gid;
				}
			} else {
				if (DoingUmap) {
					Args.idmap.from_id = (ushort_t)
								((struct cd_uidmap *) TmpPtr)->from_uid;
					Args.idmap.to_uid = ((struct cd_uidmap *) TmpPtr)->to_uid;
					Args.idmap.to_gid = CDFS_UNUSED_MAP_ENTRY;
				} else {
					Args.idmap.from_id = (ushort_t)
								((struct cd_gidmap *) TmpPtr)->from_gid;
					Args.idmap.to_gid = ((struct cd_gidmap *) TmpPtr)->to_gid;
					Args.idmap.to_uid = CDFS_UNUSED_MAP_ENTRY;
				}
			}
		} else {
			if (Setting) {
				/*
				 * Only do the assignment if there is room left.
				 */
				RetVal = ENXIO;
				if (DoingUmap) {
					if (*Cap < CD_MAXUMAP) {
						CdfsData->cdfs_UidMap[*Cap].from_uid =
									(uid_t) Args.idmap.from_id;
						CdfsData->cdfs_UidMap[*Cap].to_uid = Args.idmap.to_uid;
						(*Cap)++;
						RetVal = 0;
					}
				} else {
					if (*Cap < CD_MAXGMAP) {
						CdfsData->cdfs_GidMap[*Cap].from_gid =
									(gid_t) Args.idmap.from_id;
						CdfsData->cdfs_GidMap[*Cap].to_gid = Args.idmap.to_gid;
						(*Cap)++;
						RetVal = 0;
					}
				}
			} else {
				/*
				 * Err if we got this far and could not get the requested
				 * mapping.
				 */
				RetVal = ENOMATCH;
			}
		}
	}

	/*
	 * Copy out the results, if this was a CD_GET?MAP operation.
	 */
	if ((!Setting) && (RetVal == 0)) {
		if (copyout ((caddr_t) &Args, dataptr, sizeof (Args)) != 0) {
			RetVal = EFAULT;
		}
	}

	return (RetVal);
}





/*
 * Set or get name conversion options.
 */
STATIC int
cdfs_ioc_DoNmConv (vp, dataptr, cr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	caddr_t				dataptr;			/* User space data pointer		*/
	struct cred			*cr;				/* User credential struct ptr	*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	struct cdfs			*CdfsData;			/* FS private data struct		*/
	struct cdfs_NMConvArgs Args;			/* Argument structure			*/

	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0) {
		return (EFAULT);
	}

	CdfsData = CDFS_FS (vp->v_vfsp);

	switch (Args.ConvCmd) {
		/*
		 * Setting file/directory name conversion options.
		 */
		case CD_SETNMCONV: {
			/*
			 * Only valid if this is a privileged user.
			 */
			if (pm_denied (cr, P_FILESYS)) {
				RetVal = EPERM;
				break;
			}
			if (((Args.conv_flags & ~(CD_LOWER | CD_NOVERSION)) != 0) &&
						((Args.conv_flags & ~(CD_NOCONV)) != 0)) {
				RetVal = EINVAL;
				break;
			}
			CdfsData->cdfs_NameConv = Args.conv_flags;
			break;
		}

		/*
		 * Getting file/directory name conversion options.
		 */
		case CD_GETNMCONV: {
			Args.conv_flags = CdfsData->cdfs_NameConv;
			if (copyout ((caddr_t) &Args, dataptr, sizeof (Args)) != 0) {
				RetVal = EFAULT;
			}
			break;
		}

		default: {
			RetVal = EINVAL;
			break;
		}
	}

	return (RetVal);
}





/*
 * Set device mappings.
 */
STATIC int
cdfs_ioc_SetDevMap (vp, path, dataptr, cr)
	const struct vnode	*vp;				/* Actual vnode we're working on*/
	const uchar_t		*path;				/* Path to map					*/
	caddr_t				dataptr;			/* Ioctl-specific data ptr		*/
	struct cred			*cr;				/* User credential structure	*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	cnt_t				count;				/* General purpose counter		*/
	cnt_t				save_count;			/* Saved counter				*/
	uint_t				size;				/* Size of string				*/
	struct cdfs			*CdfsData;			/* FS private data struct		*/
	struct cdfs_DevMapArgs Args;			/* Argument structure			*/
	struct cdfs_fid		TmpFid;				/* Temp file id					*/
	struct cd_devmap	*TmpMap = NULL;		/* Working device mapping		*/
	struct cdfs_inode	*ip;				/* Inode pointer				*/

	CdfsData = CDFS_FS (vp->v_vfsp);
	ip = VTOI (vp);

	/*
	 * Must be privileged user to do any dev map setting operations.
	 */
	if (pm_denied (cr, P_FILESYS)) {
		return (EPERM);
	}
	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0) {
		return (EFAULT);
	}

	if (((ip->i_Mode & IFMT) != IFBLK) && ((ip->i_Mode & IFMT) != IFCHR)) {
		return (ENOSYS);
	}

	if ((Args.DevCmd == CD_SETDMAP) && (Args.new_minor > maxminor)) {
		cmn_err (CE_WARN,
					"cdfs_ioc_SetDevMap(): Requested minor number %d, max %d, using max",
					Args.new_minor, maxminor);
		Args.new_minor = maxminor;
	}

	/*
	 * Get working copy of the fileid.
	 */
	TmpFid = ip->i_Fid;

	/*
	 * Find this device mapping, if there already is one.
	 */
	for (count = 0; count < CdfsData->cdfs_DevMapCnt; count++) {
		if ((CdfsData->cdfs_DevMap[count].to_num != NODEV) &&
					(CDFS_CMPFID ((&(CdfsData->cdfs_DevMap[count].fileid)),
					(&TmpFid)))) {
			TmpMap = &(CdfsData->cdfs_DevMap[count]);
			save_count = count;
			break;
		}
	}

	/*
	 * Set or unset the device mapping.  If it was not found, act at the end
	 * of the list.  Note that the list must have no gaps, and so is
	 * reordered if a mapping is unset.
	 */
	switch (Args.DevCmd) {
		case CD_SETDMAP: {
			if (TmpMap == NULL) {
				if (CdfsData->cdfs_DevMapCnt >= CD_MAXDMAP) {
					return (ENOMATCH);
				}
				save_count = CdfsData->cdfs_DevMapCnt;
				CdfsData->cdfs_DevMapCnt++;
			}

			if ((CdfsData->cdfs_DevMap[save_count].to_num =
					makedevice (Args.new_major, Args.new_minor)) == NODEV) {
				RetVal = EINVAL;
			} else {
				RetVal = 0;
				CdfsData->cdfs_DevMap[save_count].fileid = TmpFid;
				size = strlen ((char *) path);
				(void) strncpy ((caddr_t)CdfsData->cdfs_DevMap[save_count].path,
							(caddr_t) path, (size_t) size);
				CdfsData->cdfs_DevMap[save_count].path[size] = '\0';
			}
			break;
		}

		case CD_UNSETDMAP: {
			if (TmpMap == NULL) {
				RetVal = ENOMATCH;
				break;
			}

			Args.new_major = getmajor (ip->i_DevNum);
			Args.new_minor = getminor (ip->i_DevNum);
			if (copyout ((caddr_t) &Args, dataptr, sizeof (Args)) != 0) {
				RetVal = EFAULT;
			}

			/*
			 * Reorder the rest of the device mappings.
			 */
			for (count = save_count; count <= (CdfsData->cdfs_DevMapCnt - 1);
						count++) {
				CdfsData->cdfs_DevMap[count].to_num = 
							CdfsData->cdfs_DevMap[count + 1].to_num;
				CdfsData->cdfs_DevMap[count].fileid = 
							CdfsData->cdfs_DevMap[count + 1].fileid;
				size = strlen ((char *) CdfsData->cdfs_DevMap[count + 1].path);
				(void) strncpy ((caddr_t) CdfsData->cdfs_DevMap[count].path,
							(caddr_t) CdfsData->cdfs_DevMap[count + 1].path,
							(size_t) size);
				CdfsData->cdfs_DevMap[count].path[size] = '\0';
			}

			CdfsData->cdfs_DevMap[CdfsData->cdfs_DevMapCnt].to_num = NODEV;
			CdfsData->cdfs_DevMap[CdfsData->cdfs_DevMapCnt].fileid =
						CDFS_NULLFID;
			CdfsData->cdfs_DevMap[CdfsData->cdfs_DevMapCnt].path[0] = '\0';

			CdfsData->cdfs_DevMapCnt--;
			break;
		}

		default: {
			RetVal = EINVAL;
			break;
		}
	}

	return (RetVal);
}





/*
 * Get current device mapping, looking up either by index into device
 * mapping table or by the fileid associated with a given path.
 */
STATIC int
cdfs_ioc_GetDevMap (vp, upath, dataptr)
	const struct vnode	*vp;				/* Vnode pointer				*/
	uchar_t				*upath;				/* Path in user space			*/
	caddr_t				dataptr;			/* User space data pointer		*/
{
	int 				RetVal = 0;			/* Return value holder			*/
	cnt_t				count;				/* General purpose counter		*/
	struct cdfs			*CdfsData;			/* FS private data struct		*/
	struct cdfs_DevMapArgs Args;			/* Argument structure			*/
	struct cdfs_fid		TmpFid;				/* Temp file id					*/
	struct cd_devmap	*TmpPtr = NULL;		/* Ptr to working mapping		*/
	struct cdfs_inode	*ip;				/* Inode pointer				*/

	CdfsData = CDFS_FS (vp->v_vfsp);
	ip = VTOI (vp);

	if (copyin (dataptr, (caddr_t) &Args, sizeof (Args)) != 0) {
		return (EFAULT);
	}
	if (Args.index < 0) {
		return (EFAULT);
	}
	if (Args.index > CdfsData->cdfs_DevMapCnt) {
		return (ENOMATCH);
	}

	if (Args.index == 0) {
		/*
		 * Cycle through the device map table to find mapping.
		 *
		 * Note that this sets the index value for the user!
		 */
		TmpFid = ip->i_Fid;
		if (((ip->i_Mode & IFMT) != IFBLK) && ((ip->i_Mode & IFMT) != IFCHR)) {
			return (ENOSYS);
		}

		for (count = 0; count < CdfsData->cdfs_DevMapCnt; count++) {
			if (CDFS_CMPFID (&(CdfsData->cdfs_DevMap[count].fileid), &TmpFid)) {
				TmpPtr = &CdfsData->cdfs_DevMap[count];
				Args.index = count + 1;
				break;
			}
		}
	} else {
		TmpPtr = &CdfsData->cdfs_DevMap[Args.index - 1];
	}

	/*
	 * If we found a mapping, copy the results to user space.  Note that
	 * the retrieved path might not match what was asked for, if path is
	 * equal to the mount point.
	 *
	 * If we didn't find a mapping, give the user the major/minor numbers
	 * we've got, and don't trash the path we were given.
	 */
	if (TmpPtr != NULL) {
		Args.new_major = (int) getmajor (TmpPtr->to_num);
		Args.new_minor = (int) getminor (TmpPtr->to_num);
		if (copyout ((caddr_t) TmpPtr->path, (caddr_t) upath,
					strlen ((caddr_t) TmpPtr->path) + 1) != 0) {
			RetVal = EFAULT;
		}
	} else {
		Args.new_major = getmajor (ip->i_DevNum);
		Args.new_minor = getminor (ip->i_DevNum);
	}

	if (copyout ((caddr_t) &Args, dataptr, sizeof (Args)) != 0) {
		RetVal = EFAULT;
	}

	return (RetVal);
}
