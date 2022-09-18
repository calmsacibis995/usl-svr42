/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/bfs/bfs_compact.c	1.5.3.2"
#ident	"$Header: $"

#include <fs/bfs/bfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/types.h>

#define	BFSBUFSIZE 8192

STATIC char bfs_buffer[BFSBUFSIZE];
STATIC int bfs_debugcomp;

static int bfs_shiftfile();
static off_t bfs_getnxtcblock();

/*
 * Compact file system by closing any gaps between files.
 */
int
bfs_compact(bs, cr)
	struct bsuper *bs;
	struct cred *cr;
{
	daddr_t eblock;
	long gapsize = 0;
	int cnt = 0;
	off_t off;
	struct bfs_dirent dir;

	BFS_LOCK(bs);	/* No I/O while compacting */

	cmn_err(CE_CONT,"Compacting BFS filesystem\n");

	off = bfs_getnxtcblock(bs, 0, &dir, cr);	/* Get first file */
	eblock = (bs->bsup_start / BFS_BSIZE);

	do {
		/*
		 * If this file is not the next block after the previous file,
		 * there is a gap.  The global gapsize must be increased and the
		 * file must be shifted.
		 */
		if (dir.d_sblock != eblock + 1 && dir.d_sblock != eblock) {

			gapsize =  (dir.d_sblock - eblock) - 1;
#ifdef DEBUG
			if (bfs_debugcomp)
			    cmn_err(CE_CONT,"Found a gap.  New gapsize is %d\n",
			      gapsize);
#endif
			bfs_shiftfile(bs, &dir, gapsize, off, cr);
			cnt++;
		}
		eblock = dir.d_eblock;
		/*
		 * Get the next file.
		 */
		off = bfs_getnxtcblock(bs, eblock, &dir, cr);
	} while (off);

	bs->bsup_compacted = BFS_YES;

	if (cnt)
		cmn_err(CE_CONT, "Compaction of BFS filesystem completed\n");
	else
		cmn_err(CE_CONT, "BFS filesystem was already compacted\n");

	bfs_unlock(bs);
	return 0;
}

/*
 * Function to get the block number and dirent of the first file after
 * "curblock."  Returns offset of the dirent.
 */
static off_t
bfs_getnxtcblock(bs, curblock, drent, cr)
	struct bsuper *bs;
	daddr_t curblock;
	struct bfs_dirent *drent;
	struct cred *cr;
{
	struct bfs_dirent dir;
	off_t off = 0;
	register int i;

	drent->d_sblock = (bs->bsup_end + 1) / BFS_BSIZE;

	/*
	 * Loop through all of the files until the start block is the lowest
	 * number greater than curblock.
	 */
	for (i = BFS_DIRSTART; i < bs->bsup_start;
	     i += sizeof(struct bfs_dirent)) {
		BFS_GETINODE(bs->bsup_devnode, i, &dir, cr);

		if (dir.d_ino == 0)
			continue;

		if (dir.d_sblock > curblock && dir.d_sblock < drent->d_sblock) {
#ifdef DEBUG
		    if (bfs_debugcomp)
			cmn_err(CE_CONT,"nxt: fnd sblk %d, gt than %d, lt %d\n",
			  dir.d_sblock, curblock, drent->d_sblock);
#endif
			*drent = dir;
			off = i;
		}
	}
	return off;	/* Return the dirent offset */
}

/*
 * Shift the file described by dirent "dir", "gapsize" blocks.  "offset"
 * describes the location on the disk of the dirent.
 */

#define BFS_CCT_READ(bvp, offset, len, buf,cr) \
	vn_rdwr(UIO_READ, bvp, (caddr_t)buf, len, offset, \
				UIO_SYSSPACE, 0, 0, cr, 0)

#define BFS_CCT_WRITE(bvp, offset, len, buf,cr) \
	vn_rdwr(UIO_WRITE, bvp, (caddr_t)buf, len, offset, \
				UIO_SYSSPACE, IO_SYNC, BFS_ULT, cr, (int *)0)

static int
bfs_shiftfile(bs, dir, gapsize, offset, cr)
	struct bsuper *bs;
	struct bfs_dirent *dir;
	long gapsize;
	off_t offset;
	struct cred *cr;
{
	long maxshift;
	long filesize;
	long w4fsck[2];
	struct bfs_sanity sw;

	sw.fromblock = dir->d_sblock;
	sw.toblock = dir->d_sblock - gapsize;

#ifdef DEBUG
	if (bfs_debugcomp)
		cmn_err(CE_CONT,"Shifting a file  inode %d   from %d to %d\n",
		  dir->d_ino, sw.fromblock, sw.toblock);
#endif

	maxshift = min(BFSBUFSIZE, gapsize*BFS_BSIZE);

	/*
	 * Write sanity words for fsck to denote compaction is in progress.
	 */
	sw.bfromblock = sw.fromblock;
	sw.btoblock = sw.toblock;
	BFS_CCT_WRITE(bs->bsup_devnode,BFS_SANITYWSTART,
		      sizeof(struct bfs_sanity),&sw,cr);

	/*
	 * Calculate the new EOF.
	 */
	if (dir->d_eoffset / BFS_BSIZE == dir->d_eblock &&
	    dir->d_eblock >= sw.fromblock)
		dir->d_eoffset =
			(dir->d_eoffset - (dir->d_sblock * BFS_BSIZE)) +
			(sw.toblock * BFS_BSIZE);

	w4fsck[0] = -1;
	w4fsck[1] = -1;
	filesize = (dir->d_eblock - dir->d_sblock +1) * BFS_BSIZE;

	/*
	 * Write as many sectors of the file at a time.
	 */
	while (sw.fromblock != (dir->d_eblock + 1)) {
		/*
		 * Must recalculate "maxshift" each time.
		 */
		maxshift = min(maxshift,
			(dir->d_eblock-sw.fromblock+1)*BFS_BSIZE);

		/*
		 * If gapsize is less than file size, must write words for fsck
		 * to denote that compaction is in progress (i.e which blocks
		 * are being shifted.)
		 * Otherwise, there is no need to write sanity words. If the
		 * system crashes during compaction, fsck can take it from 
		 * the top without data lost. 
		 */
		if (gapsize*BFS_BSIZE < filesize) {
			sw.bfromblock = sw.fromblock;
			sw.btoblock = sw.toblock;
			BFS_CCT_WRITE(bs->bsup_devnode,BFS_SANITYWSTART,
			  sizeof(struct bfs_sanity), &sw, cr);
		}

		BFS_CCT_READ(bs->bsup_devnode, sw.fromblock*BFS_BSIZE,
		  maxshift, bfs_buffer, cr);

		BFS_CCT_WRITE(bs->bsup_devnode, sw.toblock*BFS_BSIZE,
		  maxshift, bfs_buffer, cr);

		sw.fromblock+= (maxshift / BFS_BSIZE);
		sw.toblock+= (maxshift / BFS_BSIZE);

		/*
		 * If gapsize is less than file size, must write a "-1" to
		 * the first 2 sanity words to let fsck know where compaction
		 * is.
		 */
		if (gapsize*BFS_BSIZE < filesize)
			BFS_CCT_WRITE(bs->bsup_devnode,BFS_SANITYWSTART,
			  sizeof(w4fsck), w4fsck, cr);
	}

	/*
	 * Calculate the new dirent and write it to disk.
	 */
	dir->d_sblock -= gapsize;
	dir->d_eblock -= gapsize;
	BFS_PUTINODE(bs->bsup_devnode, offset, dir,cr);

	/*
	 * Must write "-1" to all 4 sanity words for fsck to denote that
	 * compaction is not in progress.
	 */
	sw.fromblock = -1;
	sw.toblock = -1;
	sw.bfromblock = -1;
	sw.btoblock = -1;
	BFS_CCT_WRITE(bs->bsup_devnode,BFS_SANITYWSTART,
	  sizeof(struct bfs_sanity), &sw, cr);
#ifdef DEBUG
	if (bfs_debugcomp)
		cmn_err(CE_CONT,"File shifted\n");
#endif
	return 0;
}
