/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:mem/rdma.c	1.12"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/rdma.h>
#include <mem/tuneable.h>
#include <proc/proc.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <util/types.h>

#if RESTRICTED_DMA

/*
 *	This file is responsible for handling arbitrary I/O requests whose
 *	buffers may include pages of memory the underlying I/O device cannot
 *	access.  In general it does this by selecting pages the I/O device
 *	can access, doing the I/O, and moving the data as needed to/from the
 *	original pages.
 *
 * Control flags in devflag:
 *	D_OLD			- The driver is old style (assume D_DMA).
 *	D_DMA			- The driver needs buffers to be DMAable.
 */

extern int bdevswsz;	/* XXX - should be declared in io/conf.h */

extern int nodev();

boolean_t rdma_enabled;

STATIC struct rdma_bdevsw {
	int	(*d_strategy)();
} *rdma_bdevsw;

#define	VALID_PP(pp)	((pp) >= pages && (pp) < epages)

STATIC void dma_pages();
STATIC int rdma_strategy();
#ifdef DEBUG
STATIC int rdma_stub_strategy();
#endif


/*
 *	Fix-up a bdevsw entry, if necessary.
 *	Called from rdma_fix_swtbls(), below, and from mod_drv_install().
 */
void
rdma_fix_bswtbl(int major)
{
	ASSERT(rdma_enabled);

	if (bdevsw[major].d_strategy == nodev) {
#ifdef DEBUG
		rdma_bdevsw[major].d_strategy = rdma_stub_strategy;
#endif
		return;
	}

	ASSERT(bdevsw[major].d_flag != NULL);

	/* Handle old-style drivers. */
	if (*bdevsw[major].d_flag & D_OLD) {
		/* Can't tell if it's a DMA driver, so assume it is. */
		*bdevsw[major].d_flag |= D_DMA;
	}

	/* If it's not a DMA driver, we don't have to do anything. */
	if (!(*bdevsw[major].d_flag & D_DMA)) {
#ifdef DEBUG
		rdma_bdevsw[major].d_strategy = rdma_stub_strategy;
#endif
		return;
	}

	rdma_bdevsw[major].d_strategy = bdevsw[major].d_strategy;
	bdevsw[major].d_strategy = rdma_strategy;
}

/*
 *	Set up rdma_bdevsw[] table.  Called at boot time.
 */
void
rdma_fix_swtbls()
{
	register int major;

	ASSERT(rdma_enabled);

	rdma_bdevsw = (struct rdma_bdevsw *)
		kmem_zalloc(bdevswsz * sizeof(struct rdma_bdevsw), KM_SLEEP);
	ASSERT(KADDR((uint_t)rdma_bdevsw));

	for (major = bdevcnt; major-- != 0;)
		rdma_fix_bswtbl(major);
}


/*
 *	Start of strategy (block I/O) section.
 */


/*
 * The rdma_priv structure is used to save b_iodone/b_private
 * when we have to use our own iodone routine.
 */
struct rdma_priv {
	void	(*priv_iodone)();
	char	*priv_private;
};

STATIC void
rdma_iodone(bp)
	buf_t	*bp;
{
	page_t	*pp, *nextpp, *origpp;
	uint_t	off, count;

	ASSERT(bp->b_flags & B_PAGEIO);
	ASSERT(bp->b_private != NULL);

	/* Put back saved b_iodone/b_private */
	bp->b_iodone = ((struct rdma_priv *)bp->b_private)->priv_iodone;
	bp->b_private = ((struct rdma_priv *)bp->b_private)->priv_private;

	/*
	 * Find the pages we substituted, and put the originals back.
	 */
	pp = bp->b_pages;
	off = ((uint_t)bp->b_un.b_addr & PAGEOFFSET);
		/*
		 * Note: when we compute the offset, we have to mask out
		 * the high order bits, in case the driver remapped the
		 * buffer with bp_mapin(), so that b_addr now holds the
		 * virtual address, not just the page offset.
		 */
	for (count = 0; count < bp->b_bufsize; count += PAGESIZE - off, off = 0,
					       pp = nextpp) {
		nextpp = pp->p_next;
		/*
		 * Substituted pages were marked as vnode == NULL, offset != 0.
		 */
		if (pp->p_vnode || pp->p_offset == 0)
			continue;
		origpp = (page_t *)pp->p_offset;
		/*
		 * Unlink the substituted page from the b_pages list, and
		 * link the original one back in its place.
		 */
		if (pp != nextpp) {
			pp->p_prev->p_next = nextpp->p_prev = origpp;
			origpp->p_prev = pp->p_prev;
			origpp->p_next = nextpp;
			pp->p_prev = pp->p_next = pp;
		}
		if (bp->b_pages == pp)
			bp->b_pages = origpp;
		/*
		 * If we're reading, copy the data now.
		 */
		if (bp->b_flags & B_READ) {
			uint_t	len;

			if (off + (len = bp->b_bufsize - count) > PAGESIZE)
				len = PAGESIZE - off;
			ppcopyrange(pp, origpp, off, len);
		}
		/*
		 * Free the substitution page.
		 */
		pp->p_offset = 0;
		PAGE_RELE(pp);
	}

	/* Now do regular iodone processing */
	biodone(bp);
}

STATIC void
rdma_blockio(strat, bp)
	void	(*strat)();
	buf_t	*bp;
{
	caddr_t	addr;
	page_t	*pp, *nextpp, *dmapp;
	uint_t	off, count;

	ASSERT(rdma_enabled);
	ASSERT(!(bp->b_flags & B_PHYS));

	if (!(bp->b_flags & B_PAGEIO)) {
		/*
		 * Non-B_PAGEIO buffers would have been kmem_alloc'ed, and
		 * therefore should be entirely DMAable.
		 */
#ifdef DEBUG
		addr = bp->b_un.b_addr; 
		for (count = 0; count < bp->b_bcount; count += PAGESIZE) {
			ASSERT(DMA_BYTE(kvtophys(addr)));
			addr += PAGESIZE;
		}
#endif
		(*strat) (bp);
		return;
	}

	ASSERT(bp->b_bufsize == bp->b_bcount);

	/*
	 * Check each page on the b_pages list to see if it's DMAable.
	 * If not, substitute a DMAable page and mark it so we put it back
	 * when we're done.
	 */
	dmapp = NULL;
	pp = bp->b_pages;
	off = (uint_t)bp->b_un.b_addr;
	for (count = 0; count < bp->b_bufsize; count += PAGESIZE - off, off = 0,
					       pp = nextpp) {
		nextpp = pp->p_next;
		if (DMA_PP(pp))
			continue;
		dmapp = page_get(PAGESIZE, P_DMA|P_CANWAIT);
		ASSERT(dmapp != NULL);
		ASSERT(dmapp->p_prev == dmapp);
		ASSERT(dmapp->p_next == dmapp);
		ASSERT(DMA_PP(dmapp));
		/*
		 * Unlink the original page from the b_pages list, and
		 * link the new one in its place.
		 */
		if (pp != nextpp) {
			pp->p_prev->p_next = nextpp->p_prev = dmapp;
			dmapp->p_prev = pp->p_prev;
			dmapp->p_next = nextpp;
			pp->p_prev = pp->p_next = pp;
		}
		if (bp->b_pages == pp)
			bp->b_pages = dmapp;
		/*
		 * If we're writing, copy the data now.
		 */
		if (!(bp->b_flags & B_READ)) {
			uint_t	len;

			if (off + (len = bp->b_bufsize - count) > PAGESIZE)
				len = PAGESIZE - off;
			ppcopyrange(pp, dmapp, off, len);
		}
		/*
		 * We need to mark the substituted page in such a way
		 * that we can tell it's been substituted.  We know it has
		 * no identity, so p_vnode == NULL.  In that case, p_offset
		 * is not used, so we'll use it; non-zero will indicate a
		 * substituted page.  We also make p_offset serve double duty
		 * as the back pointer to the original page.
		 */
		ASSERT(dmapp->p_vnode == NULL);
		ASSERT(dmapp->p_offset == 0);
		*(page_t **)&dmapp->p_offset = pp;
	}

	/*
	 * If we found any non-DMAable pages, we have to set up an iodone
	 * routine, to put everything back to normal after the I/O.
	 */
	if (dmapp != NULL) {
		/*
		 * We need to save the original b_iodone and b_private fields.
		 * Instead of kmem_alloc'ing a structure for them, we stuff it
		 * into the "private data" (dblist space) of one of the
		 * substituted pages.
		 */
		struct rdma_priv *privp;

		privp = (struct rdma_priv *)dmapp->p_dblist;
		privp->priv_iodone = bp->b_iodone;
		privp->priv_private = bp->b_private;
		bp->b_private = (char *)privp;
		bp->b_iodone = rdma_iodone;
	}

	(*strat) (bp);
}

/*
 * All block device DMA I/O are vectored through here for restricted DMA checks.
 */
STATIC int
rdma_strategy(bp)
	buf_t	*bp;
{
	major_t	index;
	int	(*strat)();

	ASSERT(rdma_enabled);
	ASSERT(!(bp->b_flags & B_PHYS));

	index = getmajor(bp->b_edev);		/* index back into bdevsw[] */

	ASSERT(bdevsw[index].d_strategy == rdma_strategy);

	strat = rdma_bdevsw[index].d_strategy; /* The actual strategy routine */

	rdma_blockio((void (*)())strat, bp);

	return 0;
}

#ifdef DEBUG
/*
 *	Should never be invoked.
 */
STATIC int
rdma_stub_strategy(bp)
	buf_t *bp;
{
	cmn_err(CE_PANIC,"rdma_stub_strategy: bp %x edev %x\n", bp, bp->b_edev);
	/* NOTREACHED */
}
#endif

/*
 *	End of strategy (block I/O) section.
 */


#define MAXCOPYSZ	64

/*
 *	This routine is involved with obtaining DMAable memory during raw I/O.
 */
int
rdma_physio(strat, bp, rw)
	int		(*strat)();
	register buf_t	*bp;
	int		rw;
{
	uint_t			total_pages;
	uint_t			non_dmaable_pages;
	uint_t			pages_possible;
	register int		i;
	caddr_t			kerneladdr;
	register uint_t		iocount;
	uint_t			bcount;
	register uint_t		count;
	register caddr_t	addr;
	caddr_t			saveaddr;
	uint_t			savecount;
	daddr_t			blkno;
	int			err;

	ASSERT(rdma_enabled);
	ASSERT(!(bp->b_flags & B_ASYNC));
	ASSERT(!(bp->b_flags & B_PAGEIO));
	ASSERT(bp->b_flags & B_PHYS);

	saveaddr = bp->b_un.b_addr;
	savecount = bp->b_bcount;

	dma_pages(bp, &total_pages, &non_dmaable_pages);

	if (non_dmaable_pages == 0) {
		PHYSIO_STRAT(strat, bp);
		err = biowait(bp);
		PHYSIO_DONE(bp, saveaddr, savecount);
		return err;
	}

#ifdef RDMA_VERBOSE
	cmn_err(CE_CONT,
		"rdma_physio: non dmaable pages %d  total pages %d\n",
		non_dmaable_pages, total_pages);
#endif
	ASSERT(non_dmaable_pages <= total_pages);

	/*
	 * XXX - The following reasoning is misguided, since the aligned case
	 * can be expected to be statistically much more frequent than the 511
	 * unaligned cases.  Therefore, this *should* be recoded to optimize
	 * the aligned case.
	 *
	 *	511 out of every 512 times on average, the I/O request
	 *	will not be aligned on a sector boundry so the
	 *	following code is correct.  On the rare case that the
	 *	request is sector aligned, the following code could be
	 *	made more efficent by not copying it, but we're
	 *	leaving it as is for now. The original fix was:
	 *
	 *		if ( NotSectorAligned )
	 *			All the code that follows
	 *		else
	 *			Do it the old way
	 *
	 *	The old code which ONLY worked for sector aligned requests,
	 *	(hence the need for the new code) was deleted because it
	 *	was not very efficent and it was not worth 200 lines of
	 *	extra code to handle a rare case. The original comment follows:
	 *
	 *	Since the I/O job is not on a sector boundary during DMA
	 *	we have no other option but to copy job.
	 *	This is so because whole sectors must be read/written.
	 *	We will pass down a 'page aligned' as well as
	 *	'sector aligned' job.
	 *	If 'dma_breakup()' gets invoked this will not again
	 *	remap the job since the boundary conditions are already met,
	 *	but it will do the I/O in units of pages which is expected.
	 *	At the end of each transfer we do the copying.
	 */

	pages_possible = MIN(MAXCOPYSZ, total_pages);

#ifdef RDMA_VERBOSE
	cmn_err(CE_CONT, "pages_possible: %d addr: %x count: %x\n",
			 pages_possible, bp->b_un.b_addr, bp->b_bcount);
#endif

	kerneladdr = kmem_alloc(ptob(pages_possible), KM_SLEEP);

	bp->b_flags &= ~B_PHYS;

	addr = saveaddr;
	bcount = savecount;
	count = pages_possible * PAGESIZE;

	blkno = bp->b_blkno;

	while (bcount > 0) {

		/*
		 * We need to reset b_addr each time through the loop because
		 * we can't be assured it is not changed during call to the
		 * strategy routine below.
		 */

		bp->b_un.b_addr = kerneladdr;	/* Sector and Page aligned */
		bp->b_bcount = iocount = MIN(bcount, count);

		bioreset(bp);

		if (rw != B_READ)	/* write job */
			bcopy(addr, kerneladdr, iocount);

		(*strat)(bp);

		err = biowait(bp);

		ASSERT(bp->b_resid <= iocount);
		iocount -= bp->b_resid;

		if (rw == B_READ && iocount)	/* read job */
			bcopy(kerneladdr, addr, iocount);

		bcount -= iocount;
		addr += iocount;

		if (err || bp->b_resid)
			break;

		/*
		 * We can't add directly to b_blkno, because it may have been
		 * changed during call to strategy routine above.
		 */

		bp->b_blkno = (blkno += btod(iocount));
	}

	kmem_free(kerneladdr, ptob(pages_possible));

	bp->b_flags |= B_PHYS;
	bp->b_un.b_addr = saveaddr;
	bp->b_bcount = savecount;
	bp->b_resid = bcount;

	return err;
}



/*
 *	Utility routines.
 */

STATIC void
dma_pages(bp, ptotal_pages, pnon_dma_pages)
	register buf_t	*bp;
	uint_t		*ptotal_pages;
	uint_t		*pnon_dma_pages;
{
	register caddr_t addr;
	register paddr_t userpage;

	ASSERT(rdma_enabled);

	if (bp->b_flags & B_PAGEIO) {
		register page_t	*pp;

		pp = bp->b_pages;
		*ptotal_pages = *pnon_dma_pages = 0;
		do {
			ASSERT(VALID_PP(pp));
			if (!DMA_PP(pp))
				(*pnon_dma_pages)++;
			(*ptotal_pages)++;
		} while ((pp = pp->p_next) != bp->b_pages);
		return;
	}

	*ptotal_pages = *pnon_dma_pages = 0;
	for (addr = (caddr_t)((uint_t)bp->b_un.b_addr & ~(NBPP - 1));
	     addr < bp->b_un.b_addr + bp->b_bcount; addr += PAGESIZE) {

		if ((userpage = vtop(addr, bp->b_proc)) == (paddr_t)0 &&
		    addr != (caddr_t)phystokv(0)) {
			cmn_err(CE_PANIC,
				"dma_pages: page not locked: addr %x proc %x",
				addr, bp->b_proc);
		}
		if (!DMA_BYTE(userpage))
			(*pnon_dma_pages)++;
		(*ptotal_pages)++;
	}
}

#endif /* RESTRICTED_DMA */


/*
 * void
 * rdma_filter(void (*strat)(), buf_t *bp)
 *	DDI/DKI routine to make a buffer DMA-able.
 */
/* ARGSUSED */
void
rdma_filter(strat, bp)
	void	(*strat)();
	buf_t	*bp;
{
#if RESTRICTED_DMA
	if (rdma_enabled) {
		if (bp->b_flags & B_PHYS)
			(void) rdma_physio((int (*)())strat, bp, bp->b_flags & B_READ);
		else
			rdma_blockio(strat, bp);
	} else
#endif /* RESTRICTED_DMA */
		(*strat) (bp);
}
