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

#ifndef _FS_CDFS_CDFS_IOCTL_H		/* Wrapper for multiple inclusions.		*/
#define _FS_CDFS_CDFS_IOCTL_H		/* Subject to change without notice.	*/

#ident	"@(#)uts-comm:fs/cdfs/cdfs_ioctl.h	1.9"
#ident	"$Header: $"


#ifdef _KERNEL_HEADERS

#ifndef _FS_CDFS_CDROM_H
#include <fs/cdfs/cdrom.h>	/* REQUIRED */
#endif

#ifndef _FS_CDFS_ISO9660_H
#include <fs/cdfs/iso9660.h>	/* REQUIRED */
#endif

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/cdrom.h>		/* REQUIRED */
#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/cdrom.h>
#include <sys/types.h>
#include <sys/fs/iso9660.h>

#endif /* _KERNEL_HEADERS */

/*
 * CDFS ioctls
 *
 * These implement the functionality needed by the XCDR and RRIP APIs.
 */
#define CIOC			('C' << 8)
#define	CDFS_GETXAR		(CIOC | 0x30)	/* Get eXtended Attr Rec of file/dir*/
#define	CDFS_GETDREC	(CIOC | 0x31)	/* Get Directory Record of file/dir	*/
#define	CDFS_GETPTREC	(CIOC | 0x32)	/* Get record out of Path Table		*/
#define	CDFS_GETSUF		(CIOC | 0x33)	/* Get System Use Field of file/dir	*/
#define	CDFS_GETTYPE	(CIOC | 0x34)	/* Get type of CD					*/
#define	CDFS_DODEFS		(CIOC | 0x35)	/* Get/set default UID/GID and perms*/
#define	CDFS_DOIDMAP	(CIOC | 0x36)	/* Get/set UID/GID mappings			*/
#define	CDFS_DONMCONV	(CIOC | 0x37)	/* Get/set file/dir name mappings	*/
#define	CDFS_SETDEVMAP	(CIOC | 0x38)	/* Set/unset device number mappings	*/
#define	CDFS_GETDEVMAP	(CIOC | 0x39)	/* Get device number mappings		*/



/*
 * Structures for passing arguments to and from CDFS ioctls.
 *
 * The first is the generic one, which holds a pathname pointer and a
 * pointer to the data for that ioctl.  That data will either be a single
 * data type or one of the structure types listed below.
 */

#pragma pack(1)
struct	cdfs_IocArgs {						/* Generic ioctl arguments		*/
	uchar_t					*PathName;		/* Real pathname to work on		*/
	void					*ArgPtr;		/* Ptr to the real arg structure*/
};
#pragma pack()


#pragma pack(1)
struct 	cdfs_XarArgs {
	int						fsec;			/* File section					*/
	int						applen;			/* Application Use length to use*/
	int						esclen;			/* Escape Sequence length to use*/
	int						xarlen;			/* Actual length to retrieve	*/
	union media_xar			*Xar;			/* Ptr to user's XAR buffer		*/
};
#pragma pack()

#pragma pack(1)
struct cdfs_DRecArgs {
	int						fsec;			/* File section					*/
	union media_drec		*DRec;			/* Ptr to user's DREC buffer	*/
};
#pragma pack()

#pragma pack(1)
struct cdfs_SUFArgs {
	struct susp_suf			*SUF;			/* Ptr to user's SUF buffer		*/
	int						length;			/* Length to get				*/
	int						fsec;			/* File section					*/
	char					*signature;		/* SUF signature to look for	*/
	int						index;			/* Which SUF to use?			*/
};
#pragma pack()

#pragma pack(1)
struct cdfs_DefArgs {
	int						DefCmd;			/* Command (get/set defaults)	*/
	struct cd_defs			defs;			/* Default structure to use/fill*/
};
#pragma pack()

#pragma pack(1)
struct cdfs_IDMapArgs {
	int						IDMapCmd;		/* Command (map/unmap uid/gid)	*/
	uint_t					count;			/* Which map?  0==use idmap vals*/
	struct cd_idmap			idmap;			/* Map structure to use/fill	*/
};
#pragma pack()

#pragma pack(1)
struct cdfs_NMConvArgs {
	int						ConvCmd;		/* Name conversion cmd (get/set)*/
	uint_t					conv_flags;		/* Conversion struct to use/fill*/
};
#pragma pack()

/*
 * Argument structure shared by CDFS_SETDEVMAP and CDFS_GETDEVMAP
 */
#pragma pack(1)
struct cdfs_DevMapArgs {
	int						DevCmd;			/* set/unset (SETDEVMAP only)	*/
	int						new_major;		/* New major number after map	*/
	int						new_minor;		/* New minor number after map	*/
	int						index;			/* Which mapping (SETDEVMAP only)*/
};
#pragma pack()

#endif	/* ! _FS_CDFS_CDFS_IOCTL_H */
