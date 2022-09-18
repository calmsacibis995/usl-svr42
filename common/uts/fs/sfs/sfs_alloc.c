/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_alloc.c	1.19.3.4"
#ident	"$Header: $"

#include <acc/mac/covert.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/sfs/sfs_tables.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>

/* control structure for covert channel limiter */
STATIC ccevent_t cc_re_db = { CC_RE_DB, CCBITS_RE_DB };
STATIC ccevent_t cc_re_inode = { CC_RE_INODE, CCBITS_RE_INODE };

STATIC int	sfs_hashalloc();
STATIC daddr_t	sfs_fragextend();
STATIC int	sfs_alloccg();
STATIC int	sfs_alloccgblk();
STATIC int	sfs_ialloccg();
STATIC int	sfs_mapsearch();
STATIC int	sfs_badblock();
STATIC int	sfs_isblock();
STATIC void	sfs_clrblock();
STATIC void	sfs_setblock();
STATIC void	sfs_fragacct();
STATIC int	sfs_scanc();
STATIC int	sfs_skpc();

int sfs_ifree();


/*
 * Allocate a block in the file system.
 *
 * The size of the requested block is given, which must be some
 * multiple of fs_fsize and <= fs_bsize.
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate a block:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate a block in the same cylinder group.
 *   4) quadratically rehash into other cylinder groups, until an
 *      available block is located.
 *   5) brute force search for a free block.
 * If no block preference is given the following hierarchy is used
 * to allocate a block:
 *   1) allocate a block in the cylinder group that contains the
 *      inode for the file.
 *   2) quadratically rehash into other cylinder groups, until an
 *      available block is located.
 *   3) brute force search for a free block.
 */
int
sfs_alloc(ip, bpref, size, bnp, cr)
	register struct inode *ip;
	daddr_t bpref;
	int size;
	daddr_t *bnp;
	struct cred *cr;
{
	register struct fs *fs;
	daddr_t bno;
	int cg;
	int err;

	ASSERT((ip->i_flag & ILOCKED) && (ip->i_owner == curproc->p_slot));
	ASSERT(((ip->i_number & 1) == 0) || (UFSIP(ip)));
	fs = ip->i_fs;
	if (size == 0)
		return(EINVAL);
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		cmn_err(CE_WARN,
		"sfs_alloc: bad size, dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}
	if (size == fs->fs_bsize && fs->fs_cstotal.cs_nbfree == 0)
		goto nospace;
	/*
	 * SFS does not support the minimum free space feature
	 * on block allocation.  It would require too many commands
	 * to have P_FILESYS as an inheritable privilege.
	 */
	if (UFSIP(ip)
	&&  freespace(fs, fs->fs_minfree) <= 0
	&&  pm_denied(cr, P_FILESYS))
		goto nospace;
#ifdef QUOTA
	err = sfs_chkdq(ip, (long)btodb(size), 0, cr);
	if (err)
		return (err);
#endif QUOTA
	if (bpref >= fs->fs_size)
		bpref = 0;
	if (bpref == 0)
		cg = (int)itog(fs, ip->i_number);
	else
		cg = dtog(fs, bpref);
	err = sfs_hashalloc(ip, cg, (long)bpref, size, &bno, sfs_alloccg,
		NULL);
	/* check if allocation is successful */
	if (bno > 0) {
		*bnp = bno;
		return (0);
	}
	/* check if operation is in error */
	if (err)
		return (err);
	/* there are no blocks available */
nospace:
	delay(5*HZ);	/* sleep an arbitrary 5 seconds */
	cmn_err(CE_NOTE, "%s: file system full\n", fs->fs_fsmnt);
	cc_limiter(&cc_re_db, cr);
	return (ENOSPC);
}

/*
 * Reallocate a fragment to a bigger size
 *
 * The number and size of the old block is given, and a preference
 * and new size is also specified.  The allocator attempts to extend
 * the original block.  Failing that, the regular block allocator is
 * invoked to get an appropriate block.
 */
int
sfs_realloccg(ip, bprev, bpref, osize, nsize, bnp, cr)
	register struct inode *ip;
	daddr_t bprev, bpref;
	int osize, nsize;
	daddr_t *bnp;
	struct cred *cr;
{
	daddr_t bno;
	register struct fs *fs;
	int cg, request;
	int err = 0;

	ASSERT((ip->i_flag & ILOCKED) && (ip->i_owner == curproc->p_slot));
	ASSERT(((ip->i_number & 1) == 0) || (UFSIP(ip)));
	fs = ip->i_fs;
	if ((unsigned)osize > fs->fs_bsize || fragoff(fs, osize) != 0 ||
	    (unsigned)nsize > fs->fs_bsize || fragoff(fs, nsize) != 0) {
		cmn_err(CE_WARN,
		"sfs_realloccg: bad size, dev = 0x%x, bsize = %d, osize = %d, nsize = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, osize, nsize, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}
	/*
	 * SFS does not support the minimum free space feature
	 * on block allocation.  It would require too many commands
	 * to have P_FILESYS as an inheritable privilege.
	 */
	if (UFSIP(ip)
	&&  freespace(fs, fs->fs_minfree) <= 0
	&&  pm_denied(cr, P_FILESYS))
		goto nospace;
	if (bprev == 0) {
		cmn_err(CE_WARN,
		"sfs_realloccg: bad bprev, dev = 0x%x, bsize = %d, bprev = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, bprev, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}
#ifdef QUOTA
	err = sfs_chkdq(ip, (long)btodb(nsize - osize), 0, cr);
	if (err)
		return (err);
#endif QUOTA
	cg = dtog(fs, bprev);
	bno = sfs_fragextend(ip, cg, (long)bprev, osize, nsize);
	if (bno != 0) {
		*bnp = bno;
		return (0);
	}
	if (bpref >= fs->fs_size)
		bpref = 0;
	switch ((int)fs->fs_optim) {
	case FS_OPTSPACE:
		/*
		 * Allocate an exact sized fragment. Although this makes 
		 * best use of space, we will waste time relocating it if 
		 * the file continues to grow. If the minimum free reserve
		 * has been set to less than 5%, or if the fragmentation
		 * is less than half of the minimum free reserve, we choose
		 * to begin optimizing for time.
		 */
		request = nsize;
		if (fs->fs_minfree < 5 ||
		    fs->fs_cstotal.cs_nffree >
		    fs->fs_dsize * fs->fs_minfree / (2 * 100))
			break;
		cmn_err(CE_NOTE, "%s: optimization changed from SPACE to TIME\n",
			fs->fs_fsmnt);
		fs->fs_optim = FS_OPTTIME;
		break;
	default:
		/*
		 * Old file systems.
		 */
		cmn_err(CE_NOTE, "%s: bad optimization, defaulting to TIME\n",
			fs->fs_fsmnt);
		fs->fs_optim = FS_OPTTIME;
		/* fall through */
	case FS_OPTTIME:
		/*
		 * At this point we have discovered a file that is trying
		 * to grow a small fragment to a larger fragment. To save
		 * time, we allocate a full sized block, then free the 
		 * unused portion. If the file continues to grow, the 
		 * `sfs_fragextend' call above will be able to grow it in place
		 * without further copying. If aberrant programs cause
		 * disk fragmentation to grow within 2% of the free reserve,
		 * we choose to begin optimizing for space.
		 */
		request = fs->fs_bsize;
		if (fs->fs_cstotal.cs_nffree <
		    fs->fs_dsize * (fs->fs_minfree - 2) / 100)
			break;
		cmn_err(CE_NOTE, "%s: optimization changed from TIME to SPACE\n",
			fs->fs_fsmnt);
		fs->fs_optim = FS_OPTSPACE;
		break;
	}
	err = sfs_hashalloc(ip, cg, (long)bpref, request, &bno, sfs_alloccg,
		NULL);
	/* check if allocation is successful */
	if (bno > 0) {
		if (nsize < request) {
			err = sfs_free(ip, bno + numfrags(fs, nsize),
				(off_t)(request - nsize));
			if (err)
				return (err);
		}
		*bnp = bno;
		return (0);
	}
	/* check if operation is in error */
	if (err)
		return (err);
	/* there are no blocks available */
nospace:
	delay(5*HZ);	/* sleep an arbitrary 5 seconds */
	cmn_err(CE_NOTE, "%s: file system full\n", fs->fs_fsmnt);
	cc_limiter(&cc_re_db, cr);
	return (ENOSPC);
}

/*
 * Allocate an inode in the file system.
 * 
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate an inode:
 *   1) allocate the requested inode.
 *   2) allocate an inode in the same cylinder group.
 *   3) quadratically rehash into other cylinder groups, until an
 *      available inode is located.
 *   4) brute force search for a free inode.
 * If no inode preference is given the following hierarchy is used
 * to allocate an inode:
 *   1) allocate an inode in cylinder group 0.
 *   2) quadratically rehash into other cylinder groups, until an
 *      available inode is located.
 *   3) brute force search for a free inode.
 */
int
sfs_ialloc(pip, ipref, mode, ipp, cr, iskipped)
	register struct inode *pip;
	ino_t ipref;
	mode_t mode;
	struct inode **ipp;
	struct cred *cr;
	int *iskipped;		/* returns # inodes skipped to find a free one */
{
	register struct inode *ip;
	register struct fs *fs;
	int cg;
	ino_t ino;
	int err;

	ASSERT((pip->i_flag & ILOCKED) && (pip->i_owner == curproc->p_slot));
	fs = pip->i_fs;
loop:
	if (fs->fs_cstotal.cs_nifree == 0)
		goto noinodes;
#ifdef QUOTA
	if (UFSIP(pip)) {
		err = sfs_chkiq((struct sfs_vfs *)(ITOV(pip))->v_vfsp->vfs_data,
			(struct inode *)NULL, cr->cr_uid, 0, cr);
		if (err)
			return (err);
	}
#endif
	if (ipref >= (u_long)(fs->fs_ncg * fs->fs_ipg))
		ipref = 0;
	cg = itog(fs, ipref);
	/*
	 * Allocate inode.
	 */
	err = sfs_hashalloc(pip, cg, (long)ipref, mode, &ino, sfs_ialloccg,
		iskipped);

	if (err)
		return (err);

	ASSERT(((ino & 1) == 0) || (UFSIP(pip)));
	if (ino == 0)
		goto noinodes;
	/*
	 * Associate an incore inode.
	 */
	err = sfs_iget((ITOV(pip))->v_vfsp, fs, ino, ipp, cr);
	if (err) {
		sfs_ifree(pip, ino, 0);
		return (err);
	}
	ip = *ipp;
	if (ip->i_mode) {
		cmn_err(CE_NOTE, "mode = 0%o, inum = %d, fs = %s\n",
		    ip->i_mode, ip->i_number, fs->fs_fsmnt);
		sfs_iupdat(ip, IUP_SYNC);
		sfs_iput(ip);
		goto loop;			
	}
	/*
	 * Start with zero blocks.  If there were blocks associated
	 * with the inode, they are recovered on the next file system
	 * check (fsck).
	 */
	if (ip->i_blocks) {
		cmn_err(CE_NOTE, "free inode %s/%d had %d blocks\n",
		    fs->fs_fsmnt, ino, ip->i_blocks);
		ip->i_blocks = 0;
	}
	return (0);
noinodes:
	cmn_err(CE_NOTE, "%s: out of inodes\n", fs->fs_fsmnt);
	cc_limiter(&cc_re_inode, cr);
	return (ENOSPC);
}

/*
 * Free a block or fragment.
 *
 * The specified block or fragment is placed back in the
 * free map. If a fragment is deallocated, a possible 
 * block reassembly is checked.
 */
int
sfs_free(ip, bno, size)
	register struct inode *ip;
	daddr_t bno;
	off_t size;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int cg, blk, frags, bbase;
	register int i;
	struct vfs *vfsp = ITOV(ip)->v_vfsp;
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	daddr_t newbno = bno;

	ASSERT((ip->i_flag & ILOCKED) && (ip->i_owner == curproc->p_slot));
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return (EIO);

	fs = ip->i_fs;
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		cmn_err(CE_WARN, 
		    "sfs_free: bad size, dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		sfs_fsinvalid(vfsp);
		return (EIO);
	}
	cg = dtog(fs, newbno);
	if (sfs_badblock(fs, newbno)) {
		cmn_err(CE_WARN, "bad block %d, ino %d\n", 
			newbno, ip->i_number);
		sfs_fsinvalid(vfsp);
		return (EIO);
	}
	bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);

	cgp = (struct cg *)bp->b_un.b_addr;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (0);
	}
	cgp->cg_time = hrestime.tv_sec;
	newbno = dtogd(fs, newbno);
	if (size == fs->fs_bsize) {
		if (sfs_isblock(fs, cgp->cg_free, (daddr_t)fragstoblks(fs, newbno))) {
			cmn_err(CE_WARN, "freeing free blk, block = %d, fs = %s, cg = %d\n",
			   newbno, fs->fs_fsmnt, cg);
			brelse(bp);
			sfs_fsinvalid(vfsp);
			return (EIO);
		}
		sfs_setblock(fs, cgp->cg_free, (daddr_t)fragstoblks(fs, newbno));
		cgp->cg_cs.cs_nbfree++;
		fs->fs_cstotal.cs_nbfree++;
		fs->fs_cs(fs, cg).cs_nbfree++;
		i = cbtocylno(fs, newbno);
		cgp->cg_b[i][cbtorpos(fs, newbno)]++;
		cgp->cg_btot[i]++;
	} else {
		bbase = newbno - fragnum(fs, newbno);
		/*
		 * Decrement the counts associated with the old frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		sfs_fragacct(fs, blk, cgp->cg_frsum, -1);
		/*
		 * Deallocate the fragment
		 */
		frags = numfrags(fs, size);
		for (i = 0; i < frags; i++) {
			if (isset(cgp->cg_free, newbno + i)) {
				cmn_err(CE_WARN,
				"freeing free frag, block = %d, fs = %s, cg = %d\n",
				   newbno + i, fs->fs_fsmnt, cg); 
				brelse(bp);
				sfs_fsinvalid(vfsp);
				return (EIO);
			}
			setbit(cgp->cg_free, newbno + i);
		}
		cgp->cg_cs.cs_nffree += i;
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		/*
		 * Add back in counts associated with the new frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		sfs_fragacct(fs, blk, cgp->cg_frsum, 1);
		/*
		 * If a complete block has been reassembled, account for it
		 */
		if (sfs_isblock(fs, cgp->cg_free, (daddr_t)fragstoblks(fs,bbase))) {
			cgp->cg_cs.cs_nffree -= fs->fs_frag;
			fs->fs_cstotal.cs_nffree -= fs->fs_frag;
			fs->fs_cs(fs, cg).cs_nffree -= fs->fs_frag;
			cgp->cg_cs.cs_nbfree++;
			fs->fs_cstotal.cs_nbfree++;
			fs->fs_cs(fs, cg).cs_nbfree++;
			i = cbtocylno(fs, bbase);
			cgp->cg_b[i][cbtorpos(fs, bbase)]++;
			cgp->cg_btot[i]++;
		}
	}
	fs->fs_fmod++;
	bdwrite(bp);
	return (0);
}

/*
 * Free an inode.
 *
 * The specified inode is cleared from the inuse inode map.
 */
int
sfs_ifree(ip, ino, mode)
	struct inode *ip;
	ino_t ino;
	mode_t mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	ino_t inot;
	int cg;
	ino_t newino = ino;

	fs = ip->i_fs;
	if ((unsigned)newino >= fs->fs_ipg*fs->fs_ncg) {
		cmn_err(CE_WARN, "sfs_ifree: range, dev = 0x%x, ino = %d, fs = %s\n",
		    ip->i_dev, newino, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}
	cg = itog(fs, newino);
	bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);

	cgp = (struct cg *)bp->b_un.b_addr;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (0);
	}
	cgp->cg_time = hrestime.tv_sec;
	inot = newino % (u_long)fs->fs_ipg;
	if (isclr(cgp->cg_iused, inot)) {
		cmn_err(CE_NOTE, "freeing free inode, mode= %o, ino = %d, fs = %s, cg = %d\n",
		   ip->i_mode, newino, fs->fs_fsmnt, cg);
		brelse(bp);
		return (0);
	}
	clrbit(cgp->cg_iused, inot);
	if (inot < (u_long)cgp->cg_irotor)
		cgp->cg_irotor = inot;
	cgp->cg_cs.cs_nifree++;
	fs->fs_cstotal.cs_nifree++;
	fs->fs_cs(fs, cg).cs_nifree++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir--;
		fs->fs_cstotal.cs_ndir--;
		fs->fs_cs(fs, cg).cs_ndir--;
	}
	fs->fs_fmod++;
	bdwrite(bp);
	return (0);
}


/*
 * Implement the cylinder overflow algorithm.
 *
 * The policy implemented by this algorithm is:
 *   1) allocate the block in its requested cylinder group.
 *   2) quadratically rehash on the cylinder group number.
 *   3) brute force search for a free block.
 */
STATIC int
sfs_hashalloc(ip, cg, pref, size, allocp, allocator, alloccost)
	struct inode *ip;
	int cg;
	long pref;
	int size;	/* size for data blocks, mode for inodes */
	u_long *allocp; /* ino or bno */
	int (*allocator)();
	int *alloccost;	/* cost of allocating an inode */
{
	register struct fs *fs;
	register int i;
	u_long alloc = 0;	/* !!!crucial initialization!!! */
	int error;
	int icg = cg;

	/*
	 * The objective of alloccost is to return to the caller
	 * the number of inodes skipped to get to a free one.
	 * This cost is incorporated by the generic limiter to
	 * treat a covert channel involving inode allocation.
	 * On block allocation, alloccost is NULL.
	 */
	if (alloccost)
		*alloccost = 0;

	fs = ip->i_fs;
	/*
	 * 1: preferred cylinder group
	 */
	error = (*allocator)(ip, cg, pref, size, &alloc, alloccost);
	/*
	 * Done if either the allocation was successful
	 * or the operation was in error.
	 */
	if (alloc || error)
		goto done;
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->fs_ncg)
			cg -= fs->fs_ncg;
		error = (*allocator)(ip, cg, 0, size, &alloc, alloccost);
		/*
		 * Done if either the allocation was successful
		 * or the operation was in error.
		 */
		if (alloc || error)
			goto done;
	}
	/*
	 * 3: brute force search
	 * Note that we start at i == 2, since 0 was checked initially,
	 * and 1 is always checked in the quadratic rehash.
	 */
	cg = (icg + 2) % fs->fs_ncg;
	for (i = 2; i < fs->fs_ncg; i++) {
		error = (*allocator)(ip, cg, 0, size, &alloc, alloccost);
		/*
		 * Done if either the allocation was successful
		 * or the operation was in error.
		 */
		if (alloc || error)
			goto done;
		cg++;
		if (cg == fs->fs_ncg)
			cg = 0;
	}
done:
	*allocp = alloc;
	return (error);
}

/*
 * Determine whether a fragment can be extended.
 *
 * Check to see if the necessary fragments are available, and 
 * if they are, allocate them.
 */
STATIC daddr_t
sfs_fragextend(ip, cg, bprev, osize, nsize)
	struct inode *ip;
	int cg;
	long bprev;
	int osize, nsize;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	long bno;
	int frags, bbase;
	int i;

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nffree < numfrags(fs, nsize - osize))
		return (NULL);
	frags = numfrags(fs, nsize);
	bbase = fragnum(fs, bprev);
	if (bbase > fragnum(fs, (bprev + frags - 1))) {
		/* cannot extend across a block boundary */
		return (NULL);
	}
	bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);

	cgp = (struct cg *)bp->b_un.b_addr;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = hrestime.tv_sec;
	bno = dtogd(fs, bprev);
	for (i = numfrags(fs, osize); i < frags; i++)
		if (isclr(cgp->cg_free, bno + i)) {
			brelse(bp);
			return (NULL);
		}
	/*
	 * The current fragment can be extended.
	 * Deduct the count on fragment being extended into.
	 * Increase the count on the remaining fragment (if any).
	 * Allocate the extended piece.
	 */
	for (i = frags; i < fs->fs_frag - bbase; i++)
		if (isclr(cgp->cg_free, bno + i))
			break;
	cgp->cg_frsum[i - numfrags(fs, osize)]--;
	if (i != frags)
		cgp->cg_frsum[i - frags]++;
	for (i = numfrags(fs, osize); i < frags; i++) {
		clrbit(cgp->cg_free, bno + i);
		cgp->cg_cs.cs_nffree--;
		fs->fs_cs(fs, cg).cs_nffree--;
		fs->fs_cstotal.cs_nffree--;
	}
	fs->fs_fmod++;
	bdwrite(bp);
	return (bprev);
}

/*
 * Determine whether a block can be allocated
 * in a cylinder group.
 *
 * Check to see if a block of the appropriate size
 * is available, and if it is, allocate it.
 */
STATIC int
sfs_alloccg(ip, cg, bpref, size, bnop, skipped)
	struct inode *ip;
	int cg;
	daddr_t bpref;
	int size;
	daddr_t *bnop;
	int *skipped;	/* unused; used by inode allocator */
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	struct vfs *vfsp = ITOV(ip)->v_vfsp;
	int bno, frags;
	int allocsiz;
	register int i;
	int error = 0;

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nbfree == 0 && size == fs->fs_bsize)
		return (0);
	bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);
	cgp = (struct cg *)bp->b_un.b_addr;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC ||
	    (cgp->cg_cs.cs_nbfree == 0 && size == fs->fs_bsize)) {
		brelse(bp);
		return (0);
	}
	cgp->cg_time = hrestime.tv_sec;
	if (size == fs->fs_bsize) {
		error = sfs_alloccgblk(fs, cgp, bpref, vfsp, &bno);
		if (error == 0)
			*bnop = bno;
		bdwrite(bp);
		return (error);
	}
	/*
	 * Check to see if any fragments are already available
	 * allocsiz is the size which will be allocated, hacking
	 * it down to a smaller size if necessary.
	 */
	frags = numfrags(fs, size);
	for (allocsiz = frags; allocsiz < fs->fs_frag; allocsiz++)
		if (cgp->cg_frsum[allocsiz] != 0)
			break;
	if (allocsiz == fs->fs_frag) {
		/*
		 * No fragments were available, so a block
		 * will be allocated and hacked up.
		 */
		if (cgp->cg_cs.cs_nbfree == 0) {
			brelse(bp);
			return (0);
		}
		error = sfs_alloccgblk(fs, cgp, bpref, vfsp, &bno);
		if (error != 0) {
			brelse(bp);
			return (error);
		}
		bpref = dtogd(fs, bno);
		/*
		 * Return remaining fragments to the free map.
		 */
		for (i = frags; i < fs->fs_frag; i++)
			setbit(cgp->cg_free, bpref + i);
		i = fs->fs_frag - frags;
		cgp->cg_cs.cs_nffree += i;
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		fs->fs_fmod++;
		cgp->cg_frsum[i]++;
		bdwrite(bp);
		*bnop = bno;
		return (0);
	}
	/*
	 * The bno returned from sfs_mapsearch() is relative to a
	 * cylinder group, so prior to returning success the block
	 * number needs to be adjusted.
	 */
	error = sfs_mapsearch(fs, cgp, bpref, allocsiz, vfsp, &bno);
	if (error != 0) {
		brelse(bp);
		return (error);
	}
	/*
	 * Remove the necessary fragments from free map.
	 * Update the count on the remaining fragments (if any).
	 */
	for (i = 0; i < frags; i++)
		clrbit(cgp->cg_free, bno + i);
	cgp->cg_cs.cs_nffree -= frags;
	fs->fs_cstotal.cs_nffree -= frags;
	fs->fs_cs(fs, cg).cs_nffree -= frags;
	fs->fs_fmod++;
	cgp->cg_frsum[allocsiz]--;
	if (frags != allocsiz)
		cgp->cg_frsum[allocsiz - frags]++;
	*bnop = cg * fs->fs_fpg + bno;
	bdwrite(bp);
	return (0);
}

/*
 * Allocate a block in a cylinder group.
 *
 * This algorithm implements the following policy:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate the next available block on the block rotor for the
 *      specified cylinder group.
 * Note that this routine only allocates fs_bsize blocks; these
 * blocks may be fragmented by the routine that allocates them.
 */
STATIC int
sfs_alloccgblk(fs, cgp, bpref, vfsp, bnop)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
	struct vfs *vfsp;
	daddr_t *bnop;
{
	daddr_t bno;
	int cylno, pos, delta;
	short *cylbp;
	register int i;
	int error;

	/*
	 * If no preference is specified, start search with the
	 * last allocated block.
	 */
	if (bpref == 0) {
		bpref = cgp->cg_rotor;
		goto norot;
	}
	bpref = blknum(fs, bpref);
	bpref = dtogd(fs, bpref);
	/*
	 * If the requested block is available, use it.
	 */
	if (sfs_isblock(fs, cgp->cg_free, (daddr_t)fragstoblks(fs, bpref))) {
		bno = bpref;
		goto gotit;
	}
	/*
	 * Check for a block available on the same cylinder.
	 */
	cylno = cbtocylno(fs, bpref);
	if (cgp->cg_btot[cylno] == 0)
		goto norot;
	if (fs->fs_cpc == 0) {
		/*
		 * Block layout info is not available, so just
		 * have to take any block in this cylinder.
		 */
		bpref = howmany(fs->fs_spc * cylno, NSPF(fs));
		goto norot;
	}
	/*
	 * Check the summary information to see if a block is 
	 * available in the requested cylinder starting at the
	 * requested rotational position and proceeding around.
	 */
	cylbp = cgp->cg_b[cylno];
	pos = cbtorpos(fs, bpref);
	for (i = pos; i < NRPOS; i++)
		if (cylbp[i] > 0)
			break;
	if (i == NRPOS)
		for (i = 0; i < pos; i++)
			if (cylbp[i] > 0)
				break;
	if (cylbp[i] > 0) {
		/*
		 * Found a rotational position, now find the actual
		 * block.  Invalidate file system if none is actually there.
		 */
		pos = cylno % fs->fs_cpc;
		bno = (cylno - pos) * fs->fs_spc / NSPB(fs);
		if (fs->fs_postbl[pos][i] == -1) {
			cmn_err(CE_WARN,"sfs_alloccgblk: cyl groups corrupted,pos = %d, i = %d, fs = %s\n",
			    pos, i, fs->fs_fsmnt);
			sfs_fsinvalid(vfsp);
			return (EIO);
		}
		for (i = fs->fs_postbl[pos][i];; ) {
			if (sfs_isblock(fs, cgp->cg_free, (daddr_t)(bno + i))) {
				bno = blkstofrags(fs, (bno + i));
				goto gotit;
			}
			delta = fs->fs_rotbl[i];
			if (delta <= 0 || delta > MAXBPC - i)
				break;
			i += delta;
		}
		cmn_err(CE_WARN, "sfs_alloccgblk: can't find blk in cyl, pos = %d, i = %d, fs = %s\n",
		pos, i, fs->fs_fsmnt);
		sfs_fsinvalid(vfsp);
		return (EIO);
	}
norot:
	/*
	 * No blocks in the requested cylinder, so take
	 * next available one in this cylinder group.
	 *
	 * The bno returned from sfs_mapsearch() is relative to a
	 * cylinder group, so prior to returning success the block
	 * number needs to be adjusted.
	 */
	error = sfs_mapsearch(fs, cgp, bpref, (int)fs->fs_frag, vfsp, &bno);
	if (error != 0)
		return (error);
	cgp->cg_rotor = bno;
gotit:
	sfs_clrblock(fs, cgp->cg_free, (long)fragstoblks(fs, bno));
	cgp->cg_cs.cs_nbfree--;
	fs->fs_cstotal.cs_nbfree--;
	fs->fs_cs(fs, cgp->cg_cgx).cs_nbfree--;
	cylno = cbtocylno(fs, bno);
	cgp->cg_b[cylno][cbtorpos(fs, bno)]--;
	cgp->cg_btot[cylno]--;
	fs->fs_fmod++;
	*bnop = cgp->cg_cgx * fs->fs_fpg + bno;
	return (0);
}

/*
 * Determine whether an inode can be allocated in a cylinder group.
 *
 * Check to see if an inode is available, and if it is,
 * allocate it using the following policy:
 *   1) allocate the requested inode.
 *   2) allocate the next available inode after the requested
 *      inode in the specified cylinder group.
 */
STATIC int
sfs_ialloccg(ip, cg, ipref, mode, inop, skipped)
	struct inode *ip;
	int cg;
	daddr_t ipref;
	int mode;
	ino_t *inop;
	int *skipped;	/* to contain number of inodes skipped */
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int start, len, loc, map, i;
	int nsearched;

	ASSERT(((ipref & 1) == 0) || (UFSIP(ip)));
	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nifree == 0) {
		if (skipped)
			*skipped += fs->fs_ipg;
		return (0);
	}
	bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);

	cgp = (struct cg *)bp->b_un.b_addr;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC || 
		cgp->cg_cs.cs_nifree == 0) {
		brelse(bp);
		return (0);
	}
	cgp->cg_time = hrestime.tv_sec;
	nsearched = 0;
	if (ipref) {
		ipref %= fs->fs_ipg;
		if (isclr(cgp->cg_iused, ipref))
			goto gotit;
	}
	start = cgp->cg_irotor / NBBY;
	len = howmany(fs->fs_ipg - cgp->cg_irotor, NBBY);
	loc = sfs_skpc(0xff, (u_int)len, &cgp->cg_iused[start]);
	nsearched = len - loc;
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = sfs_skpc(0xff, (u_int)len, &cgp->cg_iused[0]);
		nsearched += len - loc;
		if (loc == 0) {
			cmn_err(CE_WARN, "sfs_ialloccg: map corrupted, cg = %d, irotor = %d, fs = %s\n",
			    cg, cgp->cg_irotor, fs->fs_fsmnt);
			brelse(bp);
			sfs_fsinvalid(ITOV(ip)->v_vfsp);
			return (EIO);
		}
	}
	nsearched *= NBBY;
	i = start + len - loc;
	map = cgp->cg_iused[i];
	ipref = i * NBBY;
	for (i = 1; i < (1 << NBBY); i <<= 1, ipref++) {
		if ((map & i) == 0) {
			cgp->cg_irotor = ipref;
			goto gotit;
		}
		nsearched++;
	}

	cmn_err(CE_WARN, "sfs_ialloccg: block not in map fs = %s, cg = %d\n", 
	fs->fs_fsmnt, cg);
	brelse(bp);
	sfs_fsinvalid(ITOV(ip)->v_vfsp);
	return (EIO);
gotit:
	if (skipped && nsearched)
		*skipped += nsearched;
	setbit(cgp->cg_iused, ipref);
	cgp->cg_cs.cs_nifree--;
	fs->fs_cstotal.cs_nifree--;
	fs->fs_cs(fs, cg).cs_nifree--;
	fs->fs_fmod++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir++;
		fs->fs_cstotal.cs_ndir++;
		fs->fs_cs(fs, cg).cs_ndir++;
	}
	*inop = cg * fs->fs_ipg + ipref;
	bdwrite(bp);
	return (0);
}

/*
 * Find a block of the specified size in the specified cylinder group.
 *
 * Invalidate file system if a request is made to find a block and none are
 * available.
 */
STATIC int
sfs_mapsearch(fs, cgp, bpref, allocsiz, vfsp, bnop)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
	int allocsiz;
	struct vfs *vfsp;
	daddr_t *bnop;
{
	daddr_t bno;
	int start, len, loc, i;
	int blk, field, subfield, pos;

	/*
	 * Find the fragment by searching through the
	 * free block map for an appropriate bit pattern.
	 * Refer to sfs_tables.h.
	 */
	if (bpref)
		start = dtogd(fs, bpref) / NBBY;
	else
		start = cgp->cg_frotor / NBBY;
	len = howmany(fs->fs_fpg, NBBY) - start;
	loc = sfs_scanc((unsigned)len, (u_char *)&cgp->cg_free[start],
	    (u_char *)sfs_fragtbl[fs->fs_frag],
	    (u_char)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = sfs_scanc((unsigned)len, (u_char *)&cgp->cg_free[0],
		    (u_char *)sfs_fragtbl[fs->fs_frag],
		    (u_char)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
		if (loc == 0) {
			cmn_err(CE_WARN, "sfs_mapsearch: map corrupted, start = %d, len = %d, fs = %s\n",
			    start, len, fs->fs_fsmnt);
			sfs_fsinvalid(vfsp);
			return (EIO);
		}
	}
	bno = (start + len - loc) * NBBY;
	cgp->cg_frotor = bno;
	/*
	 * Found the byte in the map, sift
	 * through the bits to find the selected frag.
	 * Refer to sfs_tables.h.
	 */
	for (i = bno + NBBY; bno < i; bno += fs->fs_frag) {
		blk = blkmap(fs, cgp->cg_free, bno);
		blk <<= 1;
		field = sfs_around[allocsiz];
		subfield = sfs_inside[allocsiz];
		for (pos = 0; pos <= fs->fs_frag - allocsiz; pos++) {
			if ((blk & field) == subfield) {
				*bnop = bno + pos;
				return (0);
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
	cmn_err(CE_WARN, "sfs_mapsearch: block not in map, bno = %d, fs = %s\n",
		bno, fs->fs_fsmnt);
	sfs_fsinvalid(vfsp);
	return (EIO);
}

/*
 * Block operations
 */

/*
 * Check that a specified block number is in range.
 */
STATIC int
sfs_badblock(fs, bn)
	register struct fs *fs;
	daddr_t bn;
{

	if ((unsigned)bn >= fs->fs_size) {
		cmn_err(CE_WARN, "bad block %d, %s: bad block\n", bn, fs->fs_fsmnt);
		return (1);
	}
	return (0);
}

/*
 * Check if a block is available
 */
STATIC int
sfs_isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{
	unsigned char mask;

	switch ((int)fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		ASSERT((fs->fs_frag != 0) || (fs->fs_frag == 0));
		return (NULL);
	}
}

/*
 * Take a block out of the map
 */
STATIC void
sfs_clrblock(fs, cp, h)
	struct fs *fs;
	u_char *cp;
	daddr_t h;
{

	switch ((int)fs->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		ASSERT((fs->fs_frag != 0) || (fs->fs_frag == 0));
	}
	return;
}

/*
 * Put a block into the map
 */
STATIC void
sfs_setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{

	switch ((int)fs->fs_frag) {

	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		ASSERT((fs->fs_frag != 0) || (fs->fs_frag == 0));
	}
	return;
}

/*
 * Update the frsum fields to reflect addition or deletion
 * of some frags.
 */
STATIC void
sfs_fragacct(fs, fragmap, fraglist, cnt)
	struct fs *fs;
	int fragmap;
	long fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(sfs_fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field = sfs_around[siz];
		subfield = sfs_inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
	return;
}

STATIC int
sfs_scanc(size, cp, table, mask)
	u_int size;
	register u_char *cp, table[];
	register u_char mask;
{
	register u_char *end = &cp[size];

	while (cp < end && (table[*cp] & mask) == 0)
		cp++;
	return (end - cp);
}

STATIC int
sfs_skpc(c, len, cp)
	register char c;
	register u_int len;
	register char *cp;
{

	if (len == 0)
		return (0);
	while (*cp++ == c && --len)
		;
	return (len);
}
