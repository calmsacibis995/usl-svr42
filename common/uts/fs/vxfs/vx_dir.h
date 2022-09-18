/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_dir.h	1.2 01 Mar 1992 19:27:24 -  */
#ident	"@(#)uts-comm:fs/vxfs/vx_dir.h	1.3"

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

#ifndef	_FS_VXFS_VX_DIR_H
#define	_FS_VXFS_VX_DIR_H

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *
 * Each directory block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has
 * a struct direct at the front of it, containing its inode number,
 * the length of the entry, the length of the name contained in
 * the entry, and a hashing linkage.
 * These are followed by the name padded to a 4 byte boundary
 * with null bytes.  Names are NOT guaranteed null terminated.
 * The maximum length of a name in a directory is VX_MAXNAMLEN.
 *
 * The macro VX_DIRSIZ(len) gives the amount of space required to represent
 * a directory entry.  Free space in a directory is represented by
 * entries which have dp->d_reclen > VX_DIRSIZ(dp->d_namlen).  All data bytes
 * in a directory block are claimed by the directory entries.  This
 * usually results in the last entry in a directory having a large
 * dp->d_reclen.  When entries are deleted from a directory, the
 * space is returned to the previous entry in the same directory
 * block by increasing its dp->d_reclen.  If the first entry of
 * a directory block is free, then its dp->d_ino is set to 0.
 * Entries other than the first in a directory do not normally have
 * dp->d_ino set to 0.
 *
 * Directory names are hashed into 32 buckets and linked into hash chains
 * At the beginning of each directory block is the anchor of the hash
 * chains, and a free space summary. The hash chain anchor contains the
 * offset in the block of the first directory entry whose name hashes to the
 * corresponding value. 
 *
 * If the entire directory can fit in the immediate data area of an inode
 * the hash chains are not used and d_nhash is 0.
 */

#define	VX_MINNAMLEN	2
#define	VX_MAXNAMLEN	256
#define	VX_NHASHSHIFT	5

struct	direct {
	u_long	d_ino;			/* inode number of entry */
	u_short	d_reclen;		/* length of this record */
	u_short	d_namlen;		/* length of string in d_name */
	u_short	d_hashnext;		/* offset in block of next hash entry */
	char	d_name[VX_MAXNAMLEN];	/* name */
};

struct	mindirect {
	u_long	d_ino;			/* inode number of entry */
	u_short	d_reclen;		/* length of this record */
	u_short	d_namlen;		/* length of string in d_name */
	u_short	d_hashnext;		/* offset in block of next hash entry */
	char	d_name[VX_MINNAMLEN];	/* name */
};

struct	dirblk {
	u_short	d_tfree;		/* total free space in block */
	u_short	d_nhash;		/* number of hash chains */
	u_short	d_hash[1];		/* hash chain head */
};

struct	immed_dirblk {
	u_short	d_tfree;		/* total free space in block */
	u_short	d_nhash;		/* number of hash chains */
};

#define	VX_DIRPAD ((int)((struct direct *)0)->d_name)

/*
 * The VX_DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name
 * rounded up to a 4 byte boundary.
 */

#define	VX_DIRSIZ(len) 	(((VX_DIRPAD + 3) + (len)) & (~3))

/*
 * block overhead of variable length block
 * (number of hash chains + 2) shorts
 */

#define	VX_DIRBLKOVER(db) (((db)->d_nhash + 2) * sizeof (short))

/*
 * check that directory entry is aligned and fits inside the given block
 */

#define	VX_DIROK(dbp, boff, blen, dp) \
	(((boff) & 3) == 0 && (boff) >= VX_DIRBLKOVER((dbp)) && \
	 (boff) < ((blen) - VX_DIRPAD) && \
	 (dp)->d_namlen < (dp)->d_reclen && \
	 ((boff) + (off_t)(dp)->d_reclen) <= (blen))

/*
 * maximum number of directory entries that can be in a block
 */

#define	VX_MAXDIRENT(blen)	((blen) >> 3)

/*
 * directory operations
 */

#define	DR_REMOVE	1		/* rm or unlink */
#define	DR_RMDIR	2		/* rmdir */
#define	DR_RENAME	3		/* rename */

#endif	/* _FS_VXFS_VX_DIR_H */
