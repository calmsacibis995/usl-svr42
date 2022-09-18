/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_ext.h	1.2 01 Mar 1992 19:27:50 -  */
#ident	"@(#)uts-comm:fs/vxfs/vx_ext.h	1.3"

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

#ifndef	_FS_VXFS_VX_EXT_H
#define	_FS_VXFS_VX_EXT_H

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *  structures and flags used for extent allocation
 */

struct extalloc {
	struct inode	*ext_inode;	/* inode allocating space for */
	daddr_t		ext_space;	/* total amount of space needed */
	int		ext_flags;	/* flags for allocation */
	daddr_t		ext_indir;	/* addr of indir if indir data ext */
	daddr_t		ext_prevstart;	/* start of previous extent */
	long		ext_prevsize;	/* size of previous extent */
	int		ext_alloctype;	/* type of allocated extent */
	daddr_t		ext_start;	/* start of allocated extent */
	long		ext_size;	/* size of allocated extent */
};

#define	EXT_MASK	0x001f	/* mask for calls to vx_extalloc */
#define	EXT_FIXED	0x0001	/* fixed size required */
#define	EXT_ALIGN	0x0002	/* aligned extent required */
#define	EXT_PREALLOC	0x0004	/* this is preallocation */
#define	EXT_REALLOC	0x0008	/* reallocation is allowed */
#define	EXT_EXTEND	0x0010	/* extension is allowed */

#define	EXT_FINDFIX	0x1000	/* don't search for larger extent to breakup */
#define	EXT_PREV	0x2000	/* find nearest to previous */
#define	EXT_FORWARD	0x4000	/* search forward through au */
#define	EXT_BACKWARD	0x8000	/* search backward through au */

#define	REORG_SEARCH	1	/* search range for extents */
#define	REORG_PREALLOC	2	/* use normal extent search */

#define	VX_MAX_REALLOC	5	/* max realloc rules for extreorg */
#define	VX_MAX_NEXTENTS	10	/* max nextents for extreorg */

/*
 * These defines are the commands used in the ext_cmd field of
 * a vx_realloc structure.
 */

#define	VX_EXTREALLOC	1	/* do reallocation */
#define	VX_EXTSWAP	2	/* swap a data extent */
#define	VX_EXTINDSWAP1	3	/* swap an indirect address extent */
#define	VX_EXTINDSWAP2	4	/* swap a double indirect address extent */

struct vx_realloc {
	int	ext_cmd;	/* type of reallocation */
	int	ext_flag;	/* search direction */
	daddr_t	ext_start;	/* start of search */
	daddr_t	ext_end;	/* end of search */
	daddr_t	ext_nextents;	/* max number of extents to allocate */
};

struct vx_extreorg {
	int		ext_op;		/* type of reorg operation */
	ino_t		ext_inum;	/* inode to reallocate */
	long		ext_igen;	/* generation count for file */
	long		ext_isize;	/* size in blocks of file */
	long		ext_iblocks;	/* blocks held by file */
	off_t		ext_offset;	/* offset to realloc from */
	long		ext_size;	/* number of blocks to realloc */
	struct vx_realloc ext_alloc[VX_MAX_REALLOC];	/* rules for realloc */
	int		ext_nalloc;	/* number of valid entries in array */
	struct dinode	*ext_inode;	/* return area for new inode */
};

#define	VX_MAXDEFEXT	2048	/* max extent allocated by default policy */
#define	VX_BACKSIZE	128	/* size to search backwards for */

#define	VX_MAPSZ	512	/* bytes per map sector */
#define	VX_MAPSHFT	9	/* log2(VX_MAPSZ) */
#define	VX_MAPBLKS	2048	/* blocks mapped by one map sector (bits / 2) */
#define	VX_MAPBLKMASK	0x7ff	/* mask for VX_MAPBLKS */
#define	VX_MAPBLKSHFT	11	/* shift for VX_MAPBLKS */
#define	VX_MAPBYTOBL	2	/* shift for bytes per sector to blocks */

/*
 * Is x a power of 2
 */

#define	VX_ISPOWER2(x)	(!((x) & ((x) - 1)))

/*
 * Offset from high order bit of the leftmost bit in a byte.
 */

#define	VX_LEFTBIT(x)	(vx_firstbit[(x)] >> 4)

/*
 * Offset from low order bit of the rightmost bit in a byte.
 */

#define	VX_RIGHTBIT(x)	(vx_firstbit[(x)] & 0xf)

/*
 * Macro that computes the log base 2 of x.
 */

#define	VX_LOG2(x)							  \
	(((unsigned)(x) < 256) ?  (7 - VX_LEFTBIT((unsigned)(x))) :	  \
       ((unsigned)(x) < 65536) ? (15 - VX_LEFTBIT((unsigned)(x) >> 8)) :  \
    ((unsigned)(x) < 16777216) ? (23 - VX_LEFTBIT((unsigned)(x) >> 16)) : \
				 (31 - VX_LEFTBIT((unsigned)(x) >> 24)))

#ifdef	_KERNEL
extern char	vx_firstbit[];
extern char	vx_mapfree[];
#endif

#endif	/*_FS_VXFS_VX_EXT_H*/
