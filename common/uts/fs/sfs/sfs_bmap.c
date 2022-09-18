/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_bmap.c	1.5.3.3"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/fbuf.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/seg.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

STATIC daddr_t	sfs_blkpref();

/*
 * sfs_bmap defines the structure of file system storage by mapping
 * a logical block number in a file to a physical block number
 * on the device.  It should be called with a locked inode when
 * allocation is to be done.
 *
 * sfs_bmap translates logical block number lbn to a physical block
 * number and returns it in *bnp, possibly along with a read-ahead
 * block number in *rabnp.  bnp and rabnp can be NULL if the
 * information is not required.  rw specifies whether the mapping
 * is for read or write.  If for write, the block must be at least
 * size bytes and will be extended or allocated as needed.  If
 * alloc_only is set, sfs_bmap may not create any in-core pages
 * that correspond to the new disk allocation.  Otherwise, the in-core
 * pages will be created and initialized as needed.
 *
 * Returns 0 on success, or a non-zero errno if an error occurs.
 */
int
sfs_bmap(ip, lbn, bnp, rabnp, size, rw, alloc_only, cr)
	register struct inode *ip;	/* file to be mapped */
	daddr_t lbn;		/* logical block number */
	daddr_t *bnp;		/* mapped block number */
	daddr_t *rabnp;		/* read-ahead block */
	int size;		/* block size */
	enum seg_rw rw;		/* S_READ, S_WRITE, or S_OTHER */
	int alloc_only;		/* allocate disk blocks but create no pages */
	struct cred *cr;	/* user's credentials */
{
	register struct fs *fs;
	register struct buf *bp;
	register int i;
	struct buf *nbp;
	int j, sh;
	daddr_t ob, nb, pref, llbn, tbn, *bap;
	struct vnode *vp = ITOV(ip);
	long bsize = VBSIZE(vp);
	long osize, nsize;
	int issync, isdir;
	int err;
	dev_t dev;
	struct vfs *vfsp = vp->v_vfsp;
	struct vnode *devvp = ((struct sfs_vfs *)vfsp->vfs_data)->vfs_devvp;
	struct fbuf *fbp;
	int alloc_init = 0;

	ASSERT(rw != S_WRITE ||
		((ip->i_flag & ILOCKED) && ip->i_owner == curproc->p_slot));


	if (lbn < 0)
		return (EFBIG);

	if (ip->i_map) {
		int nblks;

		if (PAGESIZE > bsize)
			nblks = ((ip->i_size + PAGEOFFSET) >> PAGESHIFT)
				* (PAGESIZE/bsize);
		else
			nblks = (ip->i_size + bsize - 1)/bsize;
		/*
		 * In the S_WRITE case, we ignore the last block in the i_map,
		 * since it may contain fragments which need to be reallocated.
		 */
		if (lbn < (rw == S_WRITE ? nblks - 1 : nblks))
			tbn = ip->i_map[lbn];
		else
			tbn = 0;
		if (tbn == 0) {
			/*
			 * If file is to be extended, and the request
			 * is not S_WRITE, return a hole.  If the
			 * request is S_WRITE, free the existing
			 * mapping, and just go allocate.
			 */
			if (rw == S_WRITE) {
				sfs_freemap(ip);
				goto lbmap;
			}
			if (bnp)
				*bnp = SFS_HOLE;
			if (rabnp)
				*rabnp = SFS_HOLE;
			return 0;
		}
		if (bnp)
			*bnp = tbn;
		if (rabnp) {
			if ((lbn + 1) >= nblks) {
				*rabnp = SFS_HOLE;
			} else {
				*rabnp = ip->i_map[lbn+1];
				if (*rabnp == 0)
					*rabnp = SFS_HOLE;
			}
		}
		return 0;
	}
lbmap:
	fs = ip->i_fs;
	llbn = lblkno(fs, ip->i_size - 1);
	isdir = ((ip->i_mode & IFMT) == IFDIR);
	issync = ((ip->i_flag & ISYNC) != 0);
	if (isdir || issync || alloc_only == 0)
		alloc_init++;

	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 */
	if (rw == S_WRITE && llbn < NDADDR && llbn < lbn &&
	    (ob = ip->i_db[llbn]) != 0) {
		osize = blksize(fs, ip, llbn);
		if (osize < bsize && osize > 0) {
			addr_t zptr;
			/*
			 * Make sure we have all needed pages setup correctly.
			 */
			err = fbread(ITOV(ip), (u_int)(llbn << fs->fs_bshift),
			    bsize, S_OTHER, &fbp);
			if (err)
				return (err);
			zptr = fbp->fb_addr + osize;	/* zero extended frag(s) */
			bzero(zptr, bsize-osize);

			pref = sfs_blkpref(ip, llbn, (int)llbn, &ip->i_db[0]);
			err = sfs_realloccg(ip, ob, pref, osize, bsize, &nb, cr);
			if (err) {
				if(fbp)
					fbrelse(fbp, S_OTHER);
				cmn_err(CE_NOTE, "sfs_bmap: sfs_realloccg failed\n");
				return (err);
			}
			/*
			 * Don't check isdir here, directories won't do this
			 */
			if (issync)
				(void) fbiwrite(fbp, devvp, nb, fs->fs_fsize);
			else
				fbrelse(fbp, S_WRITE);

			ip->i_size = (llbn + 1) << fs->fs_bshift;
			ip->i_db[llbn] = nb;
			ip->i_flag |= IUPD | ICHG;
			ip->i_blocks += btodb(bsize - osize);

			if (nb != ob) {
				err = sfs_free(ip, ob, (off_t)osize);
				if (err != 0)
					return (err);
			}
		}
	}

	/*
	 * The first NDADDR blocks are direct blocks.
	 */
	if (lbn < NDADDR) {
		nb = ip->i_db[lbn];
		if (rw != S_WRITE)
			goto gotit;

		if (nb == 0 || ip->i_size < (lbn + 1) << fs->fs_bshift) {
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, blkoff(fs, ip->i_size));
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;

				/* need to allocate a block or frag */
				ob = nb;
				pref = sfs_blkpref(ip, lbn, (int)lbn, &ip->i_db[0]);
				err = sfs_realloccg(ip, ob, pref, osize, nsize,
				    &nb, cr);
				if (err)
					return (err);
			} else {
				/* need to allocate a block or frag */
				osize = 0;
				if (ip->i_size < (lbn + 1) << fs->fs_bshift)
					nsize = fragroundup(fs, size);
				else
					nsize = bsize;
				pref = sfs_blkpref(ip, lbn, (int)lbn, &ip->i_db[0]);
				err = sfs_alloc(ip, pref, nsize, &nb, cr);
				if (err)
					return (err);
				ob = nb;
			}

			/*
			 * Read old/create new zero pages
			 */
			fbp = NULL;
			if (osize != 0 || alloc_init) {
				err = fbread(ITOV(ip),
				    (long)(lbn << fs->fs_bshift),
				    nsize, S_OTHER, &fbp);
				if (err) {
					int err1;
					if (nb != ob) {
						err1 = sfs_free(ip, nb, 
							(off_t)nsize);
						if (err1 != 0)
							return (err1);
					} else {
						err1 = sfs_free(ip,
						    ob + numfrags(fs, osize),
						    (off_t)(nsize - osize));
						if (err1 != 0)
							return (err1);
					}
#ifdef QUOTA
					(void) sfs_chkdq(ip,
					    -(long)btodb(nsize - osize), 0, cr);
#endif QUOTA
					return (err);
				}
			}

			/*
			 * Write directory blocks synchronously so that they
			 * never appear with garbage in them on the disk.
			 */
			if (isdir)
				(void) fbiwrite(fbp, devvp, nb, fs->fs_fsize);
			else if (fbp)
				fbrelse(fbp, S_WRITE);
			ip->i_db[lbn] = nb;
			ip->i_blocks += btodb(nsize - osize);
			ip->i_flag |= IUPD | ICHG;

			if (nb != ob) {
				err = sfs_free(ip, ob, (off_t)osize);
				if (err != 0)
					return (err);
			}
		}
gotit:
		if (bnp != NULL)
			*bnp = (nb == 0)? SFS_HOLE : nb;
		if (rabnp != NULL) {
			nb = ip->i_db[lbn + 1];
			*rabnp = (nb == 0 || lbn >= NDADDR - 1) ?
			  SFS_HOLE : nb;
		}
		return (0);
	}

	/*
	 * Determine how many levels of indirection.
	 */
	pref = 0;
	sh = 1;
	tbn = lbn - NDADDR;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(fs);
		if (tbn < sh)
			break;
		tbn -= sh;
	}

	if (j == 0)
		return (EFBIG);

	/*
	 * Fetch the first indirect block.
	 */
	dev = ip->i_dev;
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		if (rw != S_WRITE) {
			if (bnp != NULL)
				*bnp = SFS_HOLE;
			if (rabnp != NULL)
				*rabnp = SFS_HOLE;
			return (0);
		}
		/*
		 * Need to allocate an indirect block.
		 */
		pref = sfs_blkpref(ip, lbn, 0, (daddr_t *)0);
		err = sfs_alloc(ip, pref, bsize, &nb, cr);
		if (err)
			return (err);
		/*
		 * Write zero block synchronously so that
		 * indirect blocks never point at garbage.
		 */
		bp = getblk(dev, fragstoblks(fs, nb), bsize);

		clrbuf(bp);
		bwrite(bp);

		ip->i_ib[NIADDR - j] = nb;
		ip->i_blocks += btodb(bsize);
		ip->i_flag |= IUPD | ICHG;

		/*
		 * In the ISYNC case, sfs_writei() will notice that the
		 * block count on the inode has changed and will be sure
		 * to sfs_iupdat the inode at the end of sfs_writei().
		 */
	}

	/*
	 * Fetch through the indirect blocks.
	 */
	for (; j <= NIADDR; j++) {
		ob = nb;
		if ( (((vp)->v_flag & VISSWAP) != 0) && 
		     ((curproc->p_flag & SSWLOCKS) == 0) ) {
			curproc->p_swlocks++;
			curproc->p_flag |= SSWLOCKS;
			bp = bread(ip->i_dev, fragstoblks(fs, ob), bsize);
			if (--curproc->p_swlocks == 0)
				curproc->p_flag &= ~SSWLOCKS;
		}
		else 
			bp = bread(ip->i_dev, fragstoblks(fs, ob), bsize);

		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (tbn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			if (rw != S_WRITE) {
				brelse(bp);
				if (bnp != NULL)
					*bnp = SFS_HOLE;
				if (rabnp != NULL)
					*rabnp = SFS_HOLE;
				return (0);
			}
			if (pref == 0) {
				if (j < NIADDR) {
					/* Indirect block */
					pref = sfs_blkpref(ip, lbn, 0,
						(daddr_t *)0);
				} else {
					/* Data block */
					pref = sfs_blkpref(ip, lbn, i, &bap[0]);
				}
			}

			err = sfs_alloc(ip, pref, bsize, &nb, cr);
			if (err) {
				brelse(bp);
				return (err);
			}

			if (j < NIADDR) {
				/*
				 * Write synchronously so indirect
				 * blocks never point at garbage.
				 */
				nbp = getblk(dev, fragstoblks(fs,nb), bsize);

				clrbuf(nbp);
				bwrite(nbp);
			} else if (alloc_init) {
				/*
				 * To avoid deadlocking if the pageout
				 * daemon decides to push a page for this
				 * inode while we are sleeping holding the
				 * bp but waiting more pages for fbzero,
				 * we give up the bp now.
				 *
				 * XXX - need to avoid having the pageout
				 * daemon get in this situation to begin with!
				 */
				brelse(bp);
                                err = fbread(ITOV(ip),
                                    (long)(lbn << fs->fs_bshift),
                                    bsize, S_OTHER, &fbp);
                                if (err) {
					int err1;
					err1 = sfs_free(ip, nb, (off_t)bsize);
					if (err1 != 0)
						return (err1);
#ifdef QUOTA
                                        (void) sfs_chkdq(ip, -(long)bsize, 0);
#endif /* QUOTA */
                                        return (err);
                                }


				/*
				 * Cases which we need to do a synchronous
				 * write of the zeroed data pages:
				 *
				 * 1) If we are writing a directory then we
				 * want to write synchronously so blocks in
				 * directories never contain garbage.
				 *
				 * 2) If we are filling in a hole and the
				 * indirect block is going to be synchronously
				 * written back below we need to make sure
				 * that the zeroes are written here before
				 * the indirect block is updated so that if
				 * we crash before the real data is pushed
				 * we will not end up with random data is
				 * the middle of the file.
				 *
				 * 3) If the request is not to allocate
				 * only, to cover cases where pages may
				 * not be zero'ed, e.g., when bsize > PAGESIZE.
				 */
				if (isdir
				||  (issync && lbn < llbn)
				||  alloc_only == 0)
					(void) fbiwrite(fbp, devvp, nb,
						fs->fs_fsize);
				else
					fbrelse(fbp, S_WRITE);

				/*
				 * Now get the bp back
				 */
				if ( (((vp)->v_flag & VISSWAP) != 0) && 
				     ((curproc->p_flag & SSWLOCKS) == 0) ) {
					curproc->p_swlocks++;
					curproc->p_flag |= SSWLOCKS;
					bp = bread(ip->i_dev, fragstoblks(fs, ob), bsize);
					if (--curproc->p_swlocks == 0)
						curproc->p_flag &= ~SSWLOCKS;
				}
				else 
					bp = bread(ip->i_dev, fragstoblks(fs, ob), bsize);

				err = geterror(bp);
				if (err) {
					(void) sfs_free(ip, nb, (off_t)bsize);
#ifdef QUOTA
					(void) sfs_chkdq(ip, -(long)btodb(bsize),
					    0, cr);
#endif QUOTA
					brelse(bp);
					return (err);
				}
				bap = bp->b_un.b_daddr;
			}

			bap[i] = nb;
			ip->i_blocks += btodb(bsize);
			ip->i_flag |= IUPD | ICHG;

			if (issync)
				bwrite(bp);
			else
				bdwrite(bp);
		} else {
			brelse(bp);
		}
	}
	if (bnp != NULL)
		*bnp = nb;
	if (rabnp != NULL) {
		if (i < NINDIR(fs) - 1) {
			nb = bap[i + 1];
			*rabnp = (nb == 0) ? SFS_HOLE : nb;
		} else {
			*rabnp = SFS_HOLE;
		}
	}
	return (0);
}

/*
 * Select the desired position for the next block in a file.  The file is
 * logically divided into sections. The first section is composed of the
 * direct blocks. Each additional section contains fs_maxbpg blocks.
 * 
 * If no blocks have been allocated in the first section, the policy is to
 * request a block in the same cylinder group as the inode that describes
 * the file. If no blocks have been allocated in any other section, the
 * policy is to place the section in a cylinder group with a greater than
 * average number of free blocks.  An appropriate cylinder group is found
 * by using a rotor that sweeps the cylinder groups. When a new group of
 * blocks is needed, the sweep begins in the cylinder group following the
 * cylinder group from which the previous allocation was made. The sweep
 * continues until a cylinder group with greater than the average number
 * of free blocks is found. If the allocation is for the first block in an
 * indirect block, the information on the previous allocation is unavailable;
 * here a best guess is made based upon the logical block number being
 * allocated.
 * 
 * If a section is already partially allocated, the policy is to
 * contiguously allocate fs_maxcontig blocks.  The end of one of these
 * contiguous blocks and the beginning of the next is physically separated
 * so that the disk head will be in transit between them for at least
 * fs_rotdelay milliseconds.  This is to allow time for the processor to
 * schedule another I/O transfer.
 */
STATIC daddr_t
sfs_blkpref(ip, lbn, indx, bap)
	struct inode *ip;
	daddr_t lbn;
	int indx;
	daddr_t *bap;
{
	register struct fs *fs;
	register int cg;
	int avgbfree, startcg;
	daddr_t nextblk;

	fs = ip->i_fs;
	if (indx % fs->fs_maxbpg == 0 || bap[indx - 1] == 0) {
		if (lbn < NDADDR) {
			cg = itog(fs, ip->i_number);
			return (fs->fs_fpg * cg + fs->fs_frag);
		}
		/*
		 * Find a cylinder with greater than average
		 * number of unused data blocks.
		 */
		if (indx == 0 || bap[indx - 1] == 0)
			startcg = itog(fs, ip->i_number) + lbn / fs->fs_maxbpg;
		else
			startcg = dtog(fs, bap[indx - 1]) + 1;
		startcg %= fs->fs_ncg;
		avgbfree = fs->fs_cstotal.cs_nbfree / fs->fs_ncg;
		for (cg = startcg; cg < fs->fs_ncg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		for (cg = 0; cg <= startcg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		return (NULL);
	}
	/*
	 * One or more previous blocks have been laid out. If less
	 * than fs_maxcontig previous blocks are contiguous, the
	 * next block is requested contiguously, otherwise it is
	 * requested rotationally delayed by fs_rotdelay milliseconds.
	 */
	nextblk = bap[indx - 1] + fs->fs_frag;
	if (indx > fs->fs_maxcontig &&
	    bap[indx - fs->fs_maxcontig] + blkstofrags(fs, fs->fs_maxcontig)
	    != nextblk)
		return (nextblk);
	if (fs->fs_rotdelay != 0)
		/*
		 * Here we convert ms of delay to frags as:
		 * (frags) = (ms) * (rev/sec) * (sect/rev) /
		 *	((sect/frag) * (ms/sec))
		 * then round up to the next block.
		 */
		nextblk += roundup(fs->fs_rotdelay * fs->fs_rps * fs->fs_nsect /
		    (NSPF(fs) * 1000), fs->fs_frag);
	return (nextblk);
}
