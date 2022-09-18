/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/fbio.c	1.5.3.4"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/fbuf.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/seg_map.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Pseudo-bio routines which use a segmap mapping to address file data.
 */

/*
 * Variables for maintaining the free list of fbuf structures.
 */
static struct fbuf *fb_free;
static int nfb_incr = 0x10;

/*
 * Return a pointer to locked kernel virtual address for
 * the given <vp, off> for len bytes.  It is not allowed to
 * have the offset cross a MAXBSIZE boundary over len bytes.
 */
int
fbread(vp, off, len, rw, fbpp)
	struct vnode *vp;
	register off_t off;
	uint len;
	enum seg_rw rw;
	struct fbuf **fbpp;
{
	register addr_t addr;
	register u_int o;
	register struct fbuf *fbp;
	faultcode_t err;

	o = off & MAXBOFFSET;
	if (o + len > MAXBSIZE)
		cmn_err(CE_PANIC, "fbread");
	addr = segmap_getmap(segkmap, vp, off & MAXBMASK);
	err = as_fault(&kas, addr + o, len, F_SOFTLOCK, rw);
	if (err) {
		(void) segmap_release(segkmap, addr, 0);
		if (FC_CODE(err) == FC_OBJERR)
			return FC_ERRNO(err);
		else
			return EIO;
	}
	fbp = (struct fbuf *)kmem_fast_alloc((caddr_t *)&fb_free,
	  sizeof (*fb_free), nfb_incr, KM_SLEEP);
	fbp->fb_addr = addr + o;
	fbp->fb_count = len;
	*fbpp = fbp;
	return 0;
}

/*
 * Similar to fbread() but we call segmap_pagecreate instead of using
 * as_fault for SOFTLOCK to create the pages without using VOP_GETPAGE
 * and then we zero up to the length rounded to a page boundary.
 * XXX - this won't work right when bsize < PAGESIZE!!!
 */
void
fbzero(vp, off, len, fbpp)
	struct vnode *vp;
	off_t off;
	uint len;
	struct fbuf **fbpp;
{
	addr_t addr;
	register uint o, zlen;

	o = off & MAXBOFFSET;
	ASSERT(o + len <= MAXBSIZE);
	addr = segmap_getmap(segkmap, vp, off & MAXBMASK) + o;

	*fbpp = (struct fbuf *)kmem_fast_alloc((caddr_t *)&fb_free,
	  sizeof (*fb_free), nfb_incr, KM_SLEEP);
	(*fbpp)->fb_addr = addr;
	(*fbpp)->fb_count = len;

	segmap_pagecreate(segkmap, addr, len, 1);

	/*
	 * Now we zero all the memory in the mapping we are interested in.
	 */
	zlen = (addr_t)ptob(btopr(len + addr)) - addr;
	ASSERT(zlen >= len && o + zlen <= MAXBSIZE);
	bzero(addr, zlen);
}

/*
 * Release the fbp using the rw mode specified.
 */
void
fbrelse(fbp, rw)
	register struct fbuf *fbp;
	enum seg_rw rw;
{
	addr_t addr;

	(void) as_fault(&kas, fbp->fb_addr, fbp->fb_count, F_SOFTUNLOCK, rw);
	addr = (addr_t)((uint)fbp->fb_addr & MAXBMASK);
	(void) segmap_release(segkmap, addr, 0);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fbp);
}

/*
 * Variant of fbrelse() that invalidates the pages upon releasing.
 *
 * Though the code of fbrelsei() and fbrelse() are nearly identical,
 * this function is kept as a separate routine for performance, and
 * the fact that the fbrelse() interface may not change for backward
 * compatibility.
 */
void
fbrelsei(fbp, rw)
	register struct fbuf *fbp;
	enum seg_rw rw;
{
	addr_t addr;

	(void) as_fault(&kas, fbp->fb_addr, fbp->fb_count, F_SOFTUNLOCK, rw);
	addr = (addr_t)((uint)fbp->fb_addr & MAXBMASK);
	(void) segmap_release(segkmap, addr, SM_INVAL);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fbp);
}

/*
 * Perform a direct write using segmap_release and the mapping
 * information contained in the inode.  Upon return the fbp is
 * invalid.
 */
int
fbwrite(fbp)
	register struct fbuf *fbp;
{
	int err;
	addr_t addr;

	(void) as_fault(&kas, fbp->fb_addr, fbp->fb_count,
	  F_SOFTUNLOCK, S_WRITE);
	addr = (addr_t)((uint)fbp->fb_addr & MAXBMASK);
	err = segmap_release(segkmap, addr, SM_WRITE);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fbp);
	return err;
}
/*
 * Perform an asynchronous write using segmap_release and the mapping
 * information contained in the inode.  Upon return the fbp is
 * invalid.
 *
 * Though the code of fbawrite() and fbwrite() are nearly identical,
 * this function is kept as a separate routine for performance, and
 * the fact that the fbwrite() interface may not change for backward
 * compatibility.
 */
int
fbawrite(fbp)
	register struct fbuf *fbp;
{
	int err;
	addr_t addr;

	(void) as_fault(&kas, fbp->fb_addr, fbp->fb_count,
	  F_SOFTUNLOCK, S_WRITE);
	addr = (addr_t)((uint)fbp->fb_addr & MAXBMASK);
	err = segmap_release(segkmap, addr, SM_WRITE | SM_ASYNC);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fbp);
	return err;
}

/*
 * Variant of fbwrite() that invalidates the pages upon releasing.
 *
 * Though the code of fbwritei() and fbwrite() are nearly identical,
 * this function is kept as a separate routine for performance, and
 * the fact that the fbwrite() interface may not change for backward
 * compatibility.
 */
int
fbwritei(fbp)
	register struct fbuf *fbp;
{
	int err;
	addr_t addr;

	(void) as_fault(&kas, fbp->fb_addr, fbp->fb_count,
	  F_SOFTUNLOCK, S_WRITE);
	addr = (addr_t)((uint)fbp->fb_addr & MAXBMASK);
	err = segmap_release(segkmap, addr, SM_WRITE | SM_INVAL);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fbp);
	return err;
}

/*
 * Perform a synchronous indirect write of the given block number
 * on the given device, using the given fbuf.  Upon return the fbp
 * is invalid.
 */
int
fbiwrite(fbp, devvp, bn, bsize)
	register struct fbuf *fbp;
	register struct vnode *devvp;
	daddr_t bn;
	int bsize;
{
	register struct buf *bp;
	int error;
	addr_t addr, eaddr;
	page_t *pp, *plist;

	/*
	 * Allocate a temp bp using pageio_setup, then use it
	 * to do I/O from the area mapped by fbuf which is currently
	 * all locked down in place.
	 *
	 * XXX - need to have a generalized bp header facility
	 * which we build up pageio_setup on top of.  Other places
	 * (like here and in device drivers for the raw I/O case)
	 * could then use these new facilities in a more straight
	 * forward fashion instead of playing all these games.
	 */
	/*
	 * Create the I/O list.
	 */
	plist = NULL;
	addr = (addr_t)((uint_t)fbp->fb_addr & PAGEMASK);
	eaddr = fbp->fb_addr + fbp->fb_count;
	while (addr < eaddr) {
		pp = page_numtopp(kvtopfn(addr));
		/*
		 * Inline the relevant portions of pvn_getdirty(),
		 * to get the page in the right state for I/O.
		 */
		ASSERT(pp->p_keepcnt != 0);
		ASSERT(!pp->p_intrans);
		ASSERT(!pp->p_free);
		page_lock(pp);
		ASSERT(PAGE_HAS_MAPPINGS(pp));
		hat_pagesync(pp);
		pp->p_mod = pp->p_ref = 0;
		pp->p_intrans = 1;
		pp->p_pagein = 0;
		PAGE_HOLD(pp);
		/*
		 * Add the page to our I/O list.
		 */
		page_sortadd(&plist, pp);
		addr += PAGESIZE;
	}
	bp = pageio_setup(plist, fbp->fb_count, devvp, B_WRITE);
	bp->b_un.b_addr = (caddr_t)((uint_t)fbp->fb_addr & PAGEOFFSET);

	bp->b_blkno = bn * btod(bsize);
	bp->b_dev = cmpdev(devvp->v_rdev);	/* store in old dev format */
	bp->b_edev = devvp->v_rdev;
	bp->b_proc = NULL;			/* i.e. the kernel */

	(*bdevsw[getmajor(devvp->v_rdev)].d_strategy)(bp);

	error = biowait(bp);
	pageio_done(bp);

	(void) as_fault(&kas, fbp->fb_addr, fbp->fb_count,
	  F_SOFTUNLOCK, S_OTHER);
	addr = (addr_t)((uint)fbp->fb_addr & MAXBMASK);
	if (error == 0)
		error = segmap_release(segkmap, addr, 0);
	else
		(void) segmap_release(segkmap, addr, 0);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fbp);

	return error;
}
