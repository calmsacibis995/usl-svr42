/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOT_S51KCONF_H	/* wrapper symbol for kernel use */
#define _BOOT_S51KCONF_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:boot/s51kconf.h	1.5"
#ident  "$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif	/* _UTIL_TYPES_H */

extern short bnami();
extern short biget();
extern long  breadi();

extern int	s5blksiz;
extern int	s5inopb;
extern int	s5nindir;

#define BSIZE		s5blksiz	/* size of logical block size		*/
#define	NINDIR		s5nindir	/* BSIZE/sizeof(daddr_t)*/
#define S51K_RDCNT	1	/* read block count 			*/

#define S5_BLK_GBUF     (GBUFSZ / BSIZE)    	/* max blk in global buf*/

/*
 * inode information:
 * space for direct-pointers & 1 indirect block
 * plus MAXDINDR double indirect blocks.
 */
#define NADDR        13      /* number of pointers in a disk inode 	*/
#define NDCTADDR     (NADDR-3)/* # of direct pointers in a disk inode 	*/
#define MAXDINDR     32      /* max no of double indirect blk 		*/

/* inumber to disk address 						*/
#define	itod(x)	(daddr_t)(((unsigned)(x)+(2*s5inopb-1))/s5inopb)
/* inumber to disk offset 						*/
#define	itoo(x)	(int)(((unsigned)(x)+(2*s5inopb-1))&(s5inopb-1))

#define S5_OFF2BLK(x)	((x) / BSIZE)	/* disk offset to blk # 	*/
#define S5_BUFOFF(x)	((x) & (BSIZE - 1))	/* buffer offset	*/

/* this inode struct is optimized for bootstrap only
 * it is a minimal subset of s5 inode structure to save memory
 * expansion is allowed at the end for future bootstrap
 */
typedef	struct	binode
{
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	ushort	i_ftype;	/* otherwise known as mode		*/
	off_t	i_size;		/* size of the file			*/
	time_t	i_mtime;	/* last modification time 		*/
} binode_t;

/*	in-core logical block buffer pool				*/
/*	an account for logical blocks that are physically contiguous	*/
struct	blk_pool {
	int	bp_cache;	/* cache #: -1 direct, 0 indrt	*/
	int	bp_cnt;		/* cache count			*/
	/* The following get malloc'ed in s5_init() */
	long	*bp_blkno;	/* starting physical block #	*/
	int	*bp_blkcnt;	/* total contiguous phy blocks	*/
};

#endif	/* _BOOT_S51KCONF_H */
