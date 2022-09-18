/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_bitmaps.h	1.3 09 Jun 1992 22:50:40 - Copyright (c) 1991, 1992 VERITAS Software, Inc */
#ident	"@(#)uts-comm:fs/vxfs/vx_bitmaps.h	1.4"

/*
 * Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054
 */

#ifndef	_FS_VXFS_VX_BITMAPS_H
#define	_FS_VXFS_VX_BITMAPS_H

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * defines and definitions for inode, inode extended operation, and
 * extent bitmaps.
 */

/*
 * map types
 */

#define	VX_EMAP		1		/* free extent map */
#define	VX_IMAP		2		/* free inode map */
#define	VX_IEMAP	3		/* extended inode operations map */

/*
 * map clone structure
 */
struct	vx_mclone {
	struct vx_mlinkhd fmc_mlink;	/* transaction dependency q */
	struct buf	*fmc_bp;	/* cloned copy buffer */
	char		fmc_clonedone;	/* completion status */
};
	
/*
 * map control structure
 * There is one of these for each map on a file system
 */

struct	vx_map {
	struct vx_mlinkhd fm_mlink;	/* transaction dependency q */
	struct vx_mlinkhd fm_holdlink;	/* holding subfunction q */
	struct vx_mclone *fm_clonep;	/* clone structure */
	int		fm_holdcnt;	/* # trans holding map */
	int		fm_chgcnt;	/* # changes since last write */
	struct fs	*fm_fs;		/* map buffer */
	struct buf	*fm_bp;		/* map buffer */
	char		fm_lock;	/* access lock */
	char		fm_flag;	/* flags */
	char		fm_type;	/* type of map */
	int		fm_aun;		/* au number */
	int		fm_mapsz;	/* size of map */
	daddr_t		fm_fblkno;	/* first block of map */
};

/*
 * values for fm_flag and fmc_clonedone
 */

#define	VX_MAPBAD	0x1			/* map is bad */
#define	VX_MAPBADWRITE	0x2			/* map write failed */
#define	VX_CLONEDONE	0x4			/* clone write done */
#define	VX_MAPBADMSG	0x8			/* map bad message issued */

#endif	/* _FS_VXFS_VX_BITMAPS_H */
