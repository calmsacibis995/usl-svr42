/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_blklst.c	1.4.3.2"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/disp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * This file contains the functions for manipulating an inode's block list
 * for mapped files.
 */

STATIC int	*sfs_bldindr();
STATIC int	sfs_bldblklst();

/*
 *  Allocate and build the block address map
 */

sfs_allocmap(ip)
register struct inode *ip;
{
	register int	*bnptr;
	register int	bsize;
	register int	nblks;
	register int	npblks;
	register struct vnode *vp;
	int 		err = 0;

	vp = ITOV(ip);
	if (ip->i_map) 
		return(err);

	/*
	 * Get number of blocks to be mapped.
	 */

	ASSERT(ip->i_map == 0);
	bsize = VBSIZE(vp);
	nblks = (ip->i_size + bsize - 1)/bsize;
	if (nblks == 0)
		return 0;

	if (PAGESIZE > bsize) {
		int bpp;

		bpp = PAGESIZE/bsize;
		npblks = ((ip->i_size + PAGESIZE) >> PAGESHIFT) * bpp;
		ASSERT(npblks >= nblks);
	} else
		npblks = nblks;


	bnptr = (int *)kmem_alloc(sizeof(int)*npblks, KM_NOSLEEP);
	if (bnptr == NULL)
		return ENOMEM;

	/*
	 * Build the actual list of block numbers
	 * for the file.
	 */

	if ((err = sfs_bldblklst(bnptr, ip, nblks)) == 0) {
		/*
		 * If the size is not an integral number of
		 * pages long, then the last few block
		 * number up to the next page boundary are
		 * made zero so that no one will try to
		 * read them in.
		 */
		while (nblks < npblks)
			bnptr[nblks++] = 0;
		ip->i_map = bnptr;
	} else
		kmem_free(bnptr, sizeof(int) * npblks);

	return err;
}

/*
 * Build the list of block numbers for a file.  This is used
 * for mapped files.
 */

STATIC int
sfs_bldblklst(lp, ip, nblks)
register int		*lp;
register struct inode	*ip;
register int		nblks;
{
	register int	lim;
	register int	*eptr;
	register int	i;
	register struct vnode *vp;
	dev_t	 dev;

	/*
	 * Get the block numbers from the direct blocks first.
	 */

	vp = ITOV(ip);
	eptr = &lp[nblks];
	if (nblks < NDADDR)
		lim = nblks;
	else
		lim = NDADDR;
	
	for (i = 0  ;  i < lim  ;  i++)
		*lp++ = ip->i_db[i];
	
	if (lp >= eptr)
		return(0);
	
	/*
	 * Handle the indirect blocks.
	 */

	dev = vp->v_vfsp->vfs_dev;
	i = 0;
	while (lp < eptr) {
		lp = sfs_bldindr(ip, lp, eptr, dev, ip->i_ib[i], i);
		if (lp == 0)
			return(1);
		i++;
	}
	return(0);
}

/*
 * Build the list of block numbers for the indirect blocks.
 * This is a recursive function.
 */

STATIC int  *
sfs_bldindr(ip, lp, eptr, dev, blknbr, indlvl)
struct inode 		*ip;
register int		*lp;
register int		*eptr;
register dev_t		dev;
int			blknbr;
int			indlvl;
{
	register struct buf *bp;
	register int	*bnptr;
	int		cnt;
	struct buf 	*bread();
	int 		bsize;
	struct vnode	*vp;
	struct fs *fs;
	int		sksize;

	vp = ITOV(ip);
	bsize = VBSIZE(vp);
	if (blknbr == 0) {
		sksize = 1;
		for (cnt = 0; cnt <= indlvl; cnt++)
			sksize *= (bsize/sizeof(int));
		if (eptr - lp < sksize)
			sksize = eptr - lp;
		for (cnt = 0; cnt < sksize; cnt++)
			*lp++ = 0;
		return lp;
	}
	fs = getfs(vp->v_vfsp);
	bp = bread(dev, fragstoblks(fs, blknbr), bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return((int *) 0);
	}
	bnptr = bp->b_un.b_words;
	cnt = NINDIR(getfs(vp->v_vfsp));
	
	ASSERT(indlvl >= 0);
	while (cnt--  &&  lp < eptr) {
		if (indlvl == 0) {
			*lp++ = *bnptr++;
		} else {
			lp = sfs_bldindr(ip, lp, eptr, dev, *bnptr++, indlvl-1);
			if (lp == 0) {
				brelse(bp);
				return((int *) 0);
			}
		}
	}

	brelse(bp);
	return(lp);
}

/*
 * Free the block list attached to an inode.
 */

void
sfs_freemap(ip)
struct inode	*ip;
{
	register int	nblks;
	register int	npblks;
	register	bsize;
	register struct vnode *vp;
	register int	type;
	register int	*bnptr;

	vp = ITOV(ip);
	ASSERT(ip->i_flag & ILOCKED);
	
	type = ip->i_mode & IFMT;
	if (type != IFREG || ip->i_map == NULL)
		return;

	bsize = VBSIZE(vp);
	nblks = (ip->i_size + bsize - 1)/bsize;


	if (PAGESIZE > bsize) {
		int bpp;

		bpp = PAGESIZE/bsize;
		nblks = ((ip->i_size + PAGEOFFSET) >> PAGESHIFT) * bpp;
	} else
		nblks = (ip->i_size + bsize -1)/bsize;

	bnptr = ip->i_map;
	ip->i_map = NULL;
	kmem_free((caddr_t)bnptr, nblks*sizeof(int));

	return;
}
