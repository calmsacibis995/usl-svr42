/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 *       Copyrighted as an unpublished work.
 *       (c) Copyright 1989 INTERACTIVE Systems Corporation
 *       All rights reserved.
 *
 *       RESTRICTED RIGHTS
 *
 *       These programs are supplied under a license.  They may be used,
 *       disclosed, and/or copied only as permitted under such license
 *       agreement.  Any copy must contain the above copyright notice and
 *       this restricted rights notice.  Use, copying, and/or disclosure
 *       of the programs is strictly prohibited unless otherwise provided
 *       in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/bootlib/s5fs.c	1.2"
#ident  "$Header: $"

#include "util/types.h"
#include "util/param.h"
#include "util/sysmacros.h"
#include "fs/vnode.h"
#include "fs/s5fs/s5inode.h"
#include "fs/s5fs/s5ino.h"
#include "fs/s5fs/s5dir.h"
#include "fs/s5fs/s5filsys.h"
#include "io/vtoc.h"
#include "svc/bootinfo.h"

#include "boot/boot.h"
#include "boot/s51kconf.h"

/*
 * s5 filesystem routines.
 */


/* boot_delta is set in the disk initialization routine 			*/
extern long	boot_delta;		/* sector offset to filesystem 	*/

/* global data buffer 							*/
char	gbuf[GBUFSZ];			/* global disk buffer - boot	*/
daddr_t	iaddr[NADDR];			/* inode addresses		*/

struct	binode	in;			/* use bootstrap version 	*/

/* cache for indirect + maximum doubly indirect inode addresses		*/
long indaddr[MAXDINDR+1];

/* in-core logical block to physical block map	
 * logical blocks that are physically contiguous are merged into an
 * array of starting physical block number and total count.
 * Even the file resides in a s5 file system, this translation
 * allows contiguous physical blocks to be read together.
 */
struct	blk_pool blkpool;		

/*
	Checks if root is s5 and initializes variables.
*/

/*
** The following new variables are being introduced to support
** booting off floppies with 512-byte filesystems.  These
** variables are replacing hard coded values originally #defined
** in sys/s5fs.h.
*/

int	s5blksiz;
int	s5inopb;
int	s5nindir;

int
s5_init(ptnstart)
off_t	ptnstart;
{
	register off_t	supblkst;
	static struct	 filsys	supblk;	
	register	ushort	ds;

	ulong	altmap();


	/* get start of s5 superblock on disk */
	supblkst = ptnstart + SUPERBOFF/512;

	/* read s5 superblock */	
	disk( supblkst, physaddr(&supblk), (short)1 );

	/* check if root is s5 type */
	if ( supblk.s_magic != FsMAGIC ) {
		debug(printf("s5_init: not an s5 filesystem\n"));
		return -1;
	}

	switch ( supblk.s_type) {
	case Fs1b:
		s5blksiz=512;
		break;
	case Fs2b:
		s5blksiz=1024;
		break;
	case Fs4b:
		s5blksiz=2048;
		break;
	}

	s5inopb=s5blksiz/sizeof(struct dinode); 
	s5nindir=s5blksiz/sizeof(daddr_t);

	blkpool.bp_blkno = (long *)bt_malloc( s5nindir * sizeof( long ));
	blkpool.bp_blkcnt = (int *)bt_malloc( s5nindir * sizeof( int ));

	debug(printf("s5_init: s5 filesystem, %d bytes\n", s5blksiz));
	return 0;

}


long breadi();
char *getcomp();

extern	short	bps;			/* bytes per sector		*/
extern	struct	bootenv bootenv;

/*
 * bnami(): 	Convert path name to inode #; inum is cwd.
 * 		Returns inode number (0 if not found).
 */

short
bnami(inum, path)
short		inum;
register char	*path;
{
	struct direct	comp, dir;
	long		count;
	long		offset;

	/*
	 * Loop, scanning path.
	 */

	for(;;) {

		while(*path == '/')			/* skip leading /'s */
			path++;

		/*
		 * If null path, found it!
		 */

		if (*path == '\0')
			return(inum);

		/*
		 * Get inode, find entry in directory.
		 * It must be a directory.
		 */

		if (biget(inum) <= 0) {
#ifdef BOOT_DEBUG
			if (bootenv.db_flag & LOADDBG )
				printf("s5fs: biget can't find inode %d\n",
						inum);
#endif
			return(0);		/* didn't find */
		}

		if ((in.i_ftype & IFMT) != IFDIR) {
#ifdef BOOT_DEBUG
			if (bootenv.db_flag & LOADDBG )
				printf("s5fs: inode not a directory %d\n",
						inum);
#endif
			return(0);		/* not a directory */
		}

		 /* Loop thru directory, looking for name. */
		
		path = getcomp(path, comp.d_name);	/* get component */

		offset = 0;

		while(count = breadi(offset,physaddr(&dir),(long)sizeof(dir))){
			if (dir.d_ino != 0)
				if ( strncmp( comp.d_name, dir.d_name, DIRSIZ) == 0 ) {
					inum = dir.d_ino;
					break;
				}
			offset += count;
		}

		if ( count == 0 ) {
#ifdef BOOT_DEBUG
			if (bootenv.db_flag & LOADDBG )
				printf("s5fs: ran out of directory\n");
#endif
			return(0);	/* ran out of directory */
		}
	}
}


/*
 * biget():	input disk-version of inode; sets global inode struct 'in'.
 *		Returns 0 if bad inum, -1 if file too big, inum on success.
 */
 
short
biget(inum)
short	inum;
{
	register char	*p1, *p2;
	struct dinode	*dp;
	register int	i;
	int		j;

	if (inum <= 0)
		return(0);			/* Sorry! */

	dread(itod(inum),S51K_RDCNT);
	dp = (struct dinode *)gbuf;
	dp += itoo(inum);

	in.i_ftype = dp->di_mode;
	in.i_size = dp->di_size;

	/*
	 * Get address pointers.
	 * Our long words are held low[0],low[1],high[0],high[1]
	 * The 3 bytes in the dinode are low[0],low[1],high[0].
	 */

	p1 = (char *)iaddr;
	p2 = (char *)dp->di_addr;
	for(i = 0; i < NADDR; i++) {
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = 0;
	}

/* 	check for indirect block 				*/
	if ((indaddr[0] = iaddr[NADDR-3]) != 0) {

/* 		if file is "very long" get 2nd indirect block 	*/
		if (iaddr[NADDR-2] != 0)
		{
			dread(iaddr[NADDR-2],S51K_RDCNT);

			j = 0;
			while (((long *)gbuf)[j] != 0  && j < MAXDINDR) {
				indaddr[j+1] = ((long *)gbuf)[j];
				j++;
			}

			if (j == MAXDINDR) {
				printf("biget: file too large to read\n");
				return (-1);
			}
		}
	}

/*	block merge the direct access blocks				*/
	blk_merge(iaddr, NDCTADDR, &blkpool);
	blkpool.bp_cache = -1;
#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & LOADDBG ) 
		printf(" cache= 0x%x\n", blkpool.bp_cache);
#endif
	return (inum);		
}


/*
 * breadi():	Read the current file pointed to by inode 'in', 
 *		starting from given file offset to target memory location.
 *		Returns the read count.
 */

long
breadi(foffset, mem, rdbytes)
off_t	foffset;	/* offset in file 				*/
char	*mem;		/* physical address of destination 		*/
int	rdbytes;	/* amount to fetch 				*/
{
	off_t		off_srt;
	off_t		off_end;
	ulong		blk_srt;
	ulong		blk_end;
	ulong		blkcnt;
	register ulong	blk_xfer;
	int		buf_off;
	int		nbytes = rdbytes;
	int		byte_xfer;
	int		blk_cache;
	register int	bpl_idx;	/* index to the block pool	*/
	register int	bpl_totscan;	/* total blocks scanned in bp	*/
	ulong		bpl_srt;	/* starting logical block #	*/
	int		bpl_ce_off;	/* offset into contiguous entry	*/

#ifdef BOOT_DEBUG
	if (mem > (char *)0x10000L)
		printf("breadi: memaddr= 0x%x count= 0x%x offset= 0x%x in.size= 0x%x\n",
			mem, rdbytes, foffset, in.i_size);
#endif
	/* 
	 * Restrict count, if it would go beyond EOF. 
	 * Return 0 already at EOF. (defensive programming -
	 * should not happen in real practice.)
	 */

	if (foffset + rdbytes > in.i_size)
		rdbytes = in.i_size - foffset;

	if ( rdbytes <= 0 )
		return(0);

	off_srt = foffset;
	off_end = foffset + rdbytes - 1;
	blk_srt = S5_OFF2BLK(off_srt);
	blk_end = S5_OFF2BLK(off_end);
	blkcnt  = blk_end - blk_srt + 1;

#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & LOADDBG)
		printf("breadi: off_srt= 0x%x off_end= 0x%x blk_srt= 0x%x blk_end= 0x%x blkcnt= 0x%x\n",
			off_srt, off_end, blk_srt, blk_end, blkcnt);
#endif

	buf_off = (int) S5_BUFOFF(off_srt);
	for (bpl_idx=0, bpl_totscan=0;blkcnt>0;) {
		blk_xfer = S5_BLK_GBUF;
		if (blkcnt < blk_xfer) 
			blk_xfer = blkcnt;
/*		get cached block pool and block offset into the cache	*/
		if (blk_srt < NDCTADDR) {	/* direct inode		*/
			bpl_srt = blk_srt;
			blk_cache = -1;
		} else {			/* indirect inode	*/
			bpl_srt = blk_srt - NDCTADDR;	
			blk_cache = bpl_srt / NINDIR;	
			bpl_srt = bpl_srt - (blk_cache * NINDIR);
		}
		if (blk_cache != blkpool.bp_cache) {
			if (blk_cache == -1) {
/*				block merge the direct access blocks	*/
				blk_merge(iaddr, NDCTADDR, &blkpool);
			} else {
				dread(indaddr[blk_cache],S51K_RDCNT);
/*				block merge the data blocks #		*/
				blk_merge(gbuf, NINDIR, &blkpool);
			}
/*			reset indices					*/
			blkpool.bp_cache = blk_cache;
			bpl_totscan = bpl_idx = 0;

#ifdef BOOT_DEBUG2
			if (bootenv.db_flag & LOADDBG) {
				printf("breadi: REFREASH cache blk_cache= 0x%x blk_srt= 0x%x\n", blk_cache, blk_srt);
			}
#endif

		}
/*		locate index into the cached block pool
 *		continue on from the last search
 */
		for (; bpl_idx<blkpool.bp_cnt; bpl_idx++) {
			bpl_ce_off=bpl_srt - bpl_totscan;
			if (bpl_ce_off < blkpool.bp_blkcnt[bpl_idx])
				break;
			bpl_totscan += blkpool.bp_blkcnt[bpl_idx];
		}
/*		check for insufficient cached block data		*/
		if (bpl_idx >= blkpool.bp_cnt)
			break;
/*		check for max contiguous blocks				*/
		if ((blkpool.bp_blkcnt[bpl_idx]-bpl_ce_off) < blk_xfer) 
			blk_xfer = blkpool.bp_blkcnt[bpl_idx] - bpl_ce_off;
		blk_xfer = dread((blkpool.bp_blkno[bpl_idx]+bpl_ce_off), blk_xfer);
		byte_xfer = blk_xfer * BSIZE;
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
			printf("breadi: Incomplete read, blkcnt= 0x%x\n", 
				blkcnt);
#endif
	return (rdbytes);
}



/*
 * getcomp():	Get next path component from a string.
 *	 	Return next position in string as value.
 */

char *
getcomp(path, comp)
register char *path;
register char *comp;
{
	register i;
	char	c;

	for(i = 0; *path && *path != '/'; i++) {
		c = *path++;
		if (i < DIRSIZ)
			*comp++ = c;
	}
	while(i++ < DIRSIZ)
		*comp++ = 0;

	return(path);
}

/*
 *	All logical block numbers that are physically contiguous 
 *	are kept as a pair of starting logical block # and count
 */
blk_merge(vec, cnt, bplp)
long	vec[];
int	cnt;
struct	blk_pool *bplp;
{
	int	i;
	int	j;
	long	vecno;

	if (cnt <= 0) {
		bplp->bp_cnt = 0;
		return;
	}

	bplp->bp_cnt = 1;
	j = 0;
	bplp->bp_blkno[j]  = vec[0];
	bplp->bp_blkcnt[j] = 1;

	for (i=1; i<cnt; i++){
/*		if two neighbouring blocks are contiguous then merge	*/
		if (vec[i-1] == vec[i]-1) 
			bplp->bp_blkcnt[j]++;
		else {	
			j++;
			bplp->bp_blkno[j]  = vec[i];
			bplp->bp_blkcnt[j] = 1;
			bplp->bp_cnt++;
		}
	}
#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & LOADDBG ) {
		for (j=0;(j<5 && j<bplp->bp_cnt);j++) {
			printf("[%d] blkno= 0x%x cnt= 0x%x ", 
				j, bplp->bp_blkno[j], bplp->bp_blkcnt[j]);
		}
	}
#endif
}
