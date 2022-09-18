/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ident	"@(#)uts-x86at:boot/bootlib/bfs.c	1.3"
#ident  "$Header: $"

#include	"util/types.h"
#include	"fs/vnode.h"
#include	"fs/vfs.h"
#include	"fs/bfs/bfs.h"
#include 	"util/sysmacros.h"
#include 	"svc/bootinfo.h"

#include 	"boot/libfm.h"
#include	"boot/boot.h"
#include	"boot/bfsconf.h"


int	bfs_rdblk_cnt;		/* runtime determined total block read count */
off_t 	fd;			/* global bfs file descriptor - file offset  */
off_t 	eof;			/* global end of file offset of fd           */
struct	bfsroot bfs_root;	/* in-core bfs root inode		     */
struct	bfs_dirent bfs_file;	/* in-core bfs file inode		     */
struct	bfsflst	bfs_flst;	/* in-core bfs root directory file list	     */

extern	struct bootenv bootenv;

/*
 * BFS - read routine
 *       convert file offset into logical block
 *	 taking care of file offset that may begin or end in
 *	 non-block boundary
 */
bfsread(foffset,mem,rdbytes)
off_t foffset;
char *mem;
int rdbytes;
{
	off_t	off_srt;
	off_t	off_end;
	register ulong blk_srt;
	ulong	blk_end;
	register ulong blkcnt;
	int	blk_xfer;
	int	buf_off;
	int 	nbytes;
	register int byte_xfer;

	off_srt = fd + foffset;

	if ( off_srt >= eof )
		return 0;

	if ( off_srt + rdbytes > eof )
		rdbytes = eof - off_srt;

	nbytes = rdbytes;

/*	setup all the block count					*/
	off_end = off_srt + nbytes - 1;
	blk_srt = OFF2BLK(off_srt);
	blk_end = OFF2BLK(off_end);
	blkcnt  = blk_end - blk_srt + 1;

#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & LOADDBG)
		printf("bfsrd: off_srt= 0x%x off_end= 0x%x blk_srt= 0x%x blk_end= 0x%x blkcnt= 0x%x\n",
			off_srt, off_end, blk_srt, blk_end, blkcnt);
#endif

/*	
 *	read in terms of block number
 *	taken care of non-block boundary file offset 
 *	before copying to the destination memory
 */
	buf_off = (int) BUFOFF(off_srt);
	for (;blkcnt>0;) {
/*	set block transfer to allocated buffer size			*/
		blk_xfer=bfs_rdblk_cnt;
		if (blkcnt < blk_xfer)
			blk_xfer = blkcnt;
/*	dread returns the actual number of blocks transferred 
 *	taking care of track boundary condition
 */
		blk_xfer = dread(blk_srt,blk_xfer);
		byte_xfer = blk_xfer * BFS_BSIZE;
		byte_xfer -= buf_off;
		if (nbytes <= byte_xfer) 
			byte_xfer = nbytes;
		memcpy(physaddr(mem),physaddr(&gbuf[buf_off]),byte_xfer);
		mem += byte_xfer;
		blk_srt += blk_xfer;
		buf_off = 0;
		nbytes -= byte_xfer;
		blkcnt -=blk_xfer;
	}

#ifdef BOOT_DEBUG
	if (bootenv.db_flag & LOADDBG)
		if (nbytes != 0)
			printf("bfsread: Incomplete read\n");
#endif
	return(rdbytes);
}

/*
 * BFS - initialization routine
 *	 cache in the root inode
 */
bfsinit()
{
	off_t	offset;
	int	rdcnt;
	int	nflst;
	struct	bfsflst *bflp = &bfs_flst;
	struct	bfsroot *brtp = &bfs_root;

/*	set fd to zero for directories read				*/
	fd = 0;

/*	get root inode - directory entry				*/
	offset = BFS_INO2OFF(BFSROOTINO);
/*	Need to hard code eof before first call to bfsread()            */
	eof = offset + BFS_DIRSZ;
	rdcnt  = bfsread(offset, &BRTINOP, BFS_DIRSZ);

/*	find # of directory entries					*/
	nflst = ((BRTINOP.d_eblock - BRTINOP.d_sblock) * BFS_BSIZE) +
		BUFOFF(BRTINOP.d_eoffset);
/*	+1 since d_eoffset points to last byte rather than following byte */
	eof = BRTINOP.d_eoffset + 1;
	brtp->brt_nflst = nflst / BFS_FLSTSZ;

/*	initialize the file list cache					*/
	bflp->bfl_cnt = -1;

#ifdef BOOT_DEBUG
	if (bootenv.db_flag & LOADDBG) {
		printf("bfsinit: rtino offset= 0x%x DIR sblk= 0x%x eblk= 0x%x",
			offset, BRTINOP.d_sblock, BRTINOP.d_eblock);
		printf(" nflst= %d\n", brtp->brt_nflst);
	}
#endif
}

/*
 * BFS - open routine
 */
bfsopen(fname)
char *fname;
{
	char	*fcomp;
	off_t	offset;
	int	rdcnt;
	int	dscan_cnt;
	int	nflst;
	int	fidx;
	struct	bfsroot *brtp = &bfs_root;
	struct	bfsflst *bflp = &bfs_flst;
	int	i;

/* 	get last component of the filename 				*/
	for (fcomp = fname; *fname != '\0';) {
		if (*fname++ == '/')
			fcomp = fname;
	}

/*	set fd to zero for directories read				*/
	fd = 0;

/*	scan through the file list cache first				*/
	if (bflp->bfl_cnt != -1) 
		fidx=ld_search(fcomp, bflp);
	else
		fidx = -1;

/*	search for file name in the root directory			*/
	if (fidx == -1) {
		dscan_cnt = NBFSFLST;
		offset    = BRTINOP.d_sblock * BFS_BSIZE;
		eof = BRTINOP.d_eoffset + 1;

		for (nflst=brtp->brt_nflst; nflst > 0; nflst-=dscan_cnt) {
			if (nflst < dscan_cnt)
				dscan_cnt = nflst;
			rdcnt = bfsread(offset,&BFLSTP,dscan_cnt*BFS_FLSTSZ);
			bflp->bfl_cnt = dscan_cnt;
			fidx=ld_search(fcomp, bflp);
			if (fidx != -1)
				break;
			offset += rdcnt;
		}
		if (fidx == -1)
			return(-1);
	}

/*	get search file inode - directory entry				*/
	offset = BFS_INO2OFF(BFLSTP[fidx].l_ino);
	eof = offset + BFS_DIRSZ;
	rdcnt  = bfsread(offset, &bfs_file, BFS_DIRSZ);
/*	set beginning file offset from the file system			*/
	fd = bfs_file.d_sblock * BFS_BSIZE;
	eof = bfs_file.d_eoffset + 1;

#ifdef BOOT_DEBUG
	if (bootenv.db_flag & LOADDBG)
		printf("bfsopen: fd= 0x%x fidx= %d\n", fd, fidx);
#endif

	return(fd);
}

/*
 *	search routine - locate filename in the bfs file list structure
 *	return -1 if no filename match
 */
ld_search(fcomp, bflp)
char	*fcomp;
struct	bfsflst *bflp;
{
	int	i;

	for (i=0; i< bflp->bfl_cnt; i++) {
		if ( BFLSTP[i].l_ino != 0 && strncmp(fcomp,
					BFLSTP[i].l_name, BFS_MAXFNLEN) == 0)
			return (i);
	}
	return (-1);
}
