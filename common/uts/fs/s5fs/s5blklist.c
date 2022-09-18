/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/s5fs/s5blklist.c	1.5.2.1"
#ident	"$Header: $"

#include <util/types.h>
#include <fs/buf.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <util/param.h>
#include <fs/stat.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <util/var.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/mode.h>
#include <proc/user.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <proc/proc.h>	/* XXX -- needed for user-context kludge in ILOCK */
#include <proc/disp.h>	/* XXX */
#include <fs/s5fs/s5param.h>
#include <fs/s5fs/s5fblk.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5ino.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/fs_subr.h>

STATIC int	s5bldblklst();
STATIC int	s5bldindr();

/*
 * Allocate and build the block address map.
 */
int
s5allocmap(ip)
	register struct inode *ip;
{
	register int	*bnptr;
	register int	bsize;
	register int	nblks;
	register int	npblks;
	register struct vnode *vp = ITOV(ip);
	int err = 0;

	if (ip->i_map) 
		return err;

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
		npblks = nblks + bpp - 1;
		npblks /= bpp;
		npblks *= bpp;
	} else
		npblks = nblks;

	bnptr = (int *)kmem_alloc(sizeof(int)*npblks, KM_NOSLEEP);
	if (bnptr == NULL)
		return ENOMEM;

	/*
	 * Build the actual list of block numbers for the file.
	 */
	if ((err= s5bldblklst(bnptr, ip, nblks)) == 0) {
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
STATIC
int
s5bldblklst(lp, ip, nblks)
	int	*lp;
	register struct inode	*ip;
	register int		nblks;
{
	register int	lim;
	register int	*eptr;
	register int	i;
	register struct vnode *vp = ITOV(ip);
	int err;
	dev_t	 dev;

	/*
	 * Get the block numbers from the direct blocks first.
	 */
	eptr = &lp[nblks];
	lim = (nblks < NADDR-3) ? nblks : NADDR-3;
	
	for (i = 0; i < lim; i++)
		*lp++ = ip->i_addr[i];
	
	if (lp >= eptr)
		return 0;
	
	dev = vp->v_vfsp->vfs_dev;
	while (lp < eptr) {
		err = s5bldindr(ip, &lp, eptr, dev, ip->i_addr[i], i-(NADDR-3));
		if (err)
			return err;
		i++;
	}
	return 0;
}

STATIC
int 
s5bldindr(ip, lp, eptr, dev, blknbr, indlvl)
	struct inode 		*ip;
	register int		**lp;
	register int		*eptr;
	register dev_t	dev;
	int			blknbr;
	int			indlvl;
{
	register struct buf *bp;
	register int	*bnptr;
	int		cnt;
	struct s5vfs	*s5vfsp;
	int err = 0;
	int 		bsize, sksize;
	struct vnode	*vp = ITOV(ip);

	bsize = VBSIZE(vp);
	if (blknbr == 0){
		sksize = 1;
		for (cnt=0; cnt < (indlvl + 1); cnt++)
			sksize *= (bsize/sizeof(int));

                if (eptr - *lp < sksize)
                        sksize = eptr - *lp;

		for (cnt=0; cnt < sksize; cnt++)
			*(*lp)++ = 0;
		return 0;
	}

	bp = bread(dev, blknbr, bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return ENXIO;
	}
	bnptr = bp->b_un.b_words;
	s5vfsp = S5VFS(vp->v_vfsp);
	cnt = s5vfsp->vfs_nindir;
	
	ASSERT(indlvl >= 0);
	while (cnt-- && *lp < eptr) {
		if (indlvl == 0)
			*(*lp)++ = *bnptr++;
		else {
			err = s5bldindr(ip, lp, eptr, dev, *bnptr++, indlvl-1);
			if (err) 
				break;
		}
	}

	brelse(bp);
	return err;
}

/*
 * Free the block list attached to an inode.
 */
s5freemap(ip)
	struct inode	*ip;
{
	register int	nblks;
	register	bsize;
	register struct vnode *vp = ITOV(ip);
	register int	*bnptr;

	ASSERT(ip->i_flag & ILOCKED);

	if (vp->v_type != VREG || ip->i_map == NULL)
		return 0;

	bsize = VBSIZE(vp);
	nblks = (ip->i_size + bsize - 1)/bsize;
	if (PAGESIZE > bsize) {
		int bpp;

		bpp = PAGESIZE/bsize;
		nblks += bpp - 1;
		nblks /= bpp;
		nblks *= bpp;
	}

	bnptr = ip->i_map;
	ip->i_map = NULL;
	kmem_free((caddr_t)bnptr, nblks*sizeof(int));
	return 0;
}
