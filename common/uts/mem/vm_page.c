/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)uts-comm:mem/vm_page.c	1.1.4.13"
#ident	"$Header: $"

/*
 * VM - physical page management.
 */

#include <fs/buf.h>
#include <fs/vnode.h>
#include <mem/anon.h>
#include <mem/page.h>
#include <mem/swap.h>
#include <mem/trace.h>
#include <mem/tuneable.h>
#include <mem/vmmac.h>
#include <mem/vmmeter.h>
#include <mem/vmsystm.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/mp.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

#if RESTRICTED_DMA
#include <mem/rdma.h>
#endif

#if defined(__STDC__)
void page_add(page_t **, page_t *);
STATIC int page_addmem(int);
STATIC int page_delmem(int);
STATIC void page_print(page_t *);
STATIC void page_unfree(page_t *);
STATIC void page_reuse(page_t *);
#else
void page_add();
STATIC int page_addmem();
STATIC int page_delmem();
STATIC void page_print();
STATIC void page_unfree();
STATIC void page_reuse();
#endif

STATIC int nopageage = 1;

STATIC u_int max_page_get[NPAGETYPE];	/* max page_get request size in pages */

uint_t mem_freemem[NPAGETYPE];		/* freemem per page type */

STATIC u_int freemem_wait;	/* someone (local) waiting for freemem */

u_int ext_freemem_wait;		/* someone external waiting for freemem */

u_int pages_pp_locked = 0;	/* physical pages actually locked */
u_int pages_pp_kernel = 0;	/* physical page locks by kernel */

extern void	cleanup();
STATIC void	call_debug();

int availrmem;
int availsmem;

#ifdef DEBUG
#define PAGE_DEBUG 1
#endif 

#ifdef PAGE_DEBUG
STATIC int do_checks = 0;
STATIC int do_check_vp = 1;
STATIC int do_check_free = 1;
STATIC int do_check_list = 1;
STATIC int do_check_pp = 1;

STATIC void page_vp_check();
STATIC void page_free_check();
STATIC void page_list_check();
STATIC void page_pp_check();

#define	CHECK(vp)	if (do_checks && do_check_vp) page_vp_check(vp)
#define	CHECKFREE()	if (do_checks && do_check_free) page_free_check()
#define	CHECKLIST(pp)	if (do_checks && do_check_list) page_list_check(pp)
#define	CHECKPP(pp)	if (do_checks && do_check_pp) page_pp_check(pp)

#else /* PAGE_DEBUG */

#define	CHECK(vp)
#define	CHECKFREE()
#define	CHECKLIST(pp)
#define	CHECKPP(pp)

#endif /* PAGE_DEBUG */


/*
 * Set to non-zero to avoid reclaiming pages which are
 * busy being paged back until the IO and completed.
 */
int nopagereclaim = 0;

/*
 * The logical page free list is maintained as two physical lists.
 * The free list contains those pages that should be reused first.
 * The cache list contains those pages that should remain unused as
 * long as possible so that they might be reclaimed.
 */
STATIC page_t *page_freelist[NPAGETYPE];	/* free list of pages */
STATIC page_t *page_cachelist[NPAGETYPE];	/* cache list of free pages */
STATIC int page_freelist_size[NPAGETYPE];	/* size of free list */
STATIC int page_cachelist_size[NPAGETYPE];	/* size of cache list */

mon_t	page_mplock;			/* lock for manipulating page links */

STATIC	mon_t	page_freelock;		/* lock for manipulating free list */

struct pp_chunk	pagepool[MAX_MEM_CHUNK];

STATIC struct pp_chunk *pp_first;	/* 1st pagepool chunk in search order */
STATIC u_int n_pp_chunk;		/* # chunks in pagepool table */

page_t *mem_pages[NPAGETYPE];		/* 1st pagepool page per type */
page_t *mem_epages[NPAGETYPE];		/* last pagepool page per type (+1) */

#define OK_PAGE_GET	0x50		/* min ok value for max_page_get */
#define PAGE_GET_RESV	0x20		/* min(pagepool size - max_page_get) */


STATIC	mon_t page_locklock;	/* mutex on locking variables */

#define	PAGE_LOCK_MAXIMUM \
	((1 << (sizeof (((page_t *)0)->p_lckcnt) * NBBY)) - 1)

STATIC struct page_tcnt {
	int	pc_free_cache;		/* free's into cache list */
	int	pc_free_dontneed;	/* free's with dontneed */
	int	pc_free_pageout;	/* free's from pageout */
	int	pc_free_free;		/* free's into free list */
	int	pc_get_cache;		/* get's from cache list */
	int	pc_get_free;		/* get's from free list */
	int	pc_reclaim;		/* reclaim's */
	int	pc_abortfree;		/* abort's of free pages */
	int	pc_find_hit;		/* find's that find page */
	int	pc_find_miss;		/* find's that don't find page */
#define	PC_HASH_CNT	(2*PAGE_HASHAVELEN)
	int	pc_find_hashlen[PC_HASH_CNT+1];
} pagecnt;

#ifdef	DEBUG
#define	BUMPPGCOUNT(x)	++(x)
#else
#define	BUMPPGCOUNT(x) 
#endif


#if RESTRICTED_DMA
/*
 * Must be called from page_init() before the page free list creation,
 * so it can separate the page pool into dmaable and non-dmaable pools.
 */
STATIC void
rdma_page_init()
{
	page_t *pp;
	uint_t pfn;
	uint_t ndmapages;

	ASSERT(rdma_enabled == B_FALSE);
	if (tune.t_dmalimit <= tune.t_dmabase)
		return;

	pp = mem_pages[STD_PAGE];
	while (pp != mem_epages[STD_PAGE]) {
		pfn = page_pptonum(pp);
		if (DMA_PFN(pfn))
			break;
		pp++;
	}
	mem_pages[DMA_PAGE] = pp;
	while (pp != mem_epages[STD_PAGE]) {
		pfn = page_pptonum(pp);
		if (!DMA_PFN(pfn))
			break;
		(pp++)->p_dma = 1;
	}
	mem_epages[DMA_PAGE] = pp;
	ndmapages = mem_epages[DMA_PAGE] - mem_pages[DMA_PAGE];

	if (!tune.t_devnondma && ndmapages == epages - pages)
		return;

	rdma_enabled = B_TRUE;

	/*
	 * Restrict P_DMA page_get() to 1/2 the total # of dmaable pages.
	 * If that's too small, adjust it, using the page_init() heuristic.
	 */
	max_page_get[DMA_PAGE] = ndmapages >> 1;
	if (max_page_get[DMA_PAGE] < OK_PAGE_GET &&
	    ndmapages - PAGE_GET_RESV > max_page_get[DMA_PAGE]) {
		max_page_get[DMA_PAGE] = ndmapages - PAGE_GET_RESV;
		if (max_page_get[DMA_PAGE] > OK_PAGE_GET)
			max_page_get[DMA_PAGE] = OK_PAGE_GET;
	}
}
#endif /* RESTRICTED_DMA */


/*
 * Initialize the physical page structures.
 * Since we cannot call the dynamic memory allocator yet,
 * we have startup() allocate memory for the page
 * structs and the hash tables for us.
 * This can be called multiple times for discontiguous memory chunks.
 */
void
page_init_chunk(pp, num_pp, base)
	page_t *pp;
	u_int num_pp, base;
{
	struct pp_chunk	**chunkpp;

	if (n_pp_chunk >= MAX_MEM_CHUNK)
		cmn_err(CE_PANIC, "page_init: too many memory chunks");

	pagepool[n_pp_chunk].pp_pfn = base;
	pagepool[n_pp_chunk].pp_epfn = base + num_pp * (PAGESIZE/MMU_PAGESIZE);
	pagepool[n_pp_chunk].pp_page = pp;
	pagepool[n_pp_chunk].pp_epage = pp + num_pp;

	/*
	 * Link this chunk into the search order list.  Larger chunks
	 * go to the front of the list.
	 * This is done with the hope that when scanning for a particular
	 * page structure/page frame number we'll be more likely to find
	 * the one we're looking for quicker.
	 */
	chunkpp = &pp_first;
	while (*chunkpp != NULL) {
		if ((*chunkpp)->pp_epage - (*chunkpp)->pp_page >= num_pp)
			break;
		chunkpp = &(*chunkpp)->pp_next;
	}
	pagepool[n_pp_chunk].pp_next = *chunkpp;
	*chunkpp = &pagepool[n_pp_chunk++];

	/*
	 * Add these pages to the count of available real and swappable memory.
	 */
	availrmem += num_pp;
	availsmem += num_pp;
}

/*
 * This performs the final initialization of the page system.
 * It should be called after all calls to page_init_chunk(),
 * of which there will be at least one.
 */
void
page_init(page_t *strtexcludepage, uint_t numexcludepage)
{
	page_t *pp, *endexcludepage;

	ASSERT(n_pp_chunk > 0 && pp_first != NULL);

	pages = pagepool[0].pp_page;
	epages = pagepool[n_pp_chunk - 1].pp_epage;
	ASSERT(epages - pages == availrmem);

	/*
	 * Arbitrarily limit the max page_get request
	 * to 1/2 of the page structs we have.
	 *
	 * If this value is < OK_PAGE_GET, then we set max_page_get to
	 * num - KEEP_FREE.  If this number is less 1/2 of memory,
	 * use 1/2 of mem.  If it's greater than OK_PAGE_GET, use OK_PAGE_GET.
	 *
	 * All of this is just an attempt to run even if very little memory
	 * is available.  There are no guarantees!  The system will probably
	 * die later with insufficient memory even though we get by here.
	 */
	max_page_get[STD_PAGE] = (epages - pages) >> 1;
	if (max_page_get[STD_PAGE] < OK_PAGE_GET &&
	    (epages - pages) - PAGE_GET_RESV > max_page_get[STD_PAGE]) {
		max_page_get[STD_PAGE] = (epages - pages) - PAGE_GET_RESV;
		if (max_page_get[STD_PAGE] > OK_PAGE_GET)
			max_page_get[STD_PAGE] = OK_PAGE_GET;
	}

	/*
	 * Determine the number of pages that can be pplocked.  This
	 * is the number of page frames less the maximum that can be
	 * taken for buffers less another percentage.  The percentage should
	 * be a tunable parameter, and in SVR4 should be one of the "tune"
	 * structures.
	 */
	if (pages_pp_maximum <= (tune.t_minarmem + 20) ||
	    pages_pp_maximum > (epages - pages))
		pages_pp_maximum = (epages - pages) / 10;

	/*
	 * Verify that the hashing stuff has been initialized.
	 */
	if (page_hash == NULL || page_hashsz == 0)
		cmn_err(CE_PANIC, "page_init");

#if RESTRICTED_DMA
	rdma_page_init();
#endif

	/*
	 * The physical space for the page struct array has already been
	 * allocated.  Mark all the pages as locked, and call page_free()
	 * to make the pages available.
	 * Exclude those pages in range of strtexcludepage, numexcludepage.
	 * Those pages should not be used until after the info in them (the
	 * dynamic symbol table) has been copied elsewhere.
	 */
	if (strtexcludepage == NULL)
		strtexcludepage = endexcludepage = epages;
	else
		endexcludepage = strtexcludepage + numexcludepage;
	for (pp = pages; pp < strtexcludepage; pp++) {
		pp->p_lock = 1;
		page_free(pp, 1);
	}
	for (; pp < endexcludepage; pp++) {
		pp->p_lock = 1;
		pp->p_free = 0;
		PAGE_HOLD(pp);
	}
	for (; pp < epages; pp++) {
		pp->p_lock = 1;
		page_free(pp, 1);
	}

#ifdef lint
	page_print(pp);
#endif /* lint */
}

/*
 * Use cv_wait() to wait for the given page.  It is assumed that
 * the page_mplock is already locked upon entry and this lock
 * will continue to be held upon return.
 *
 * NOTE:  This routine must be called at splvm() and the caller must
 *	  re-verify the page identity.
 */
void
page_cv_wait(pp)
	register page_t *pp;
{
	register int s;

	/*
	 * Protect against someone clearing the
	 * want bit before we get to sleep.
	 */
	s = splvm();
	if (bclnlist == NULL) {
		/* page may be done except for cleanup if
		 * bclnlist != NULL.
		 * during startup, in particular, this
		 * would cause deadlock.
		 * Later, it causes an unnecessary delay
		 * unless that case is handled.
		 */
		pp->p_want = 1;
		cv_wait(&page_mplock, (char *)pp);
	}
	(void) splx(s);

	/*
	 * We may have been awakened from swdone,
	 * in which case we must clean up the i/o
	 * list before being able to use the page.
	 */
	mon_exit(&page_mplock);
	if (bclnlist != NULL) {
		s = spl0();
		cleanup();
		(void) splx(s);
	}
	mon_enter(&page_mplock);
}

/*
 * Reclaim the given page from the free list to vp list.
 */
void
page_reclaim(pp)
	register page_t *pp;
{
	register int s;
	register struct anon *ap;

	ASSERT(pp >= pages && pp < epages);
	s = splvm();
	mon_enter(&page_freelock);

	if (pp->p_free) {
#ifdef	TRACE
		register int age = pp->p_age;

		ap = NULL;
#endif	/* TRACE */
		page_unfree(pp);
		BUMPPGCOUNT(pagecnt.pc_reclaim);
		if (pp->p_vnode) {
			cnt.v_pgrec++;
			cnt.v_pgfrec++;
			vminfo.v_pgrec++;

			if (ap = swap_anon(pp->p_vnode, pp->p_offset)) {
				if (ap->un.an_page == NULL && ap->an_refcnt > 0)
					ap->un.an_page = pp;
				cnt.v_xsfrec++;
				vminfo.v_xsfrec++;
			} else {
				cnt.v_xifrec++;
				vminfo.v_xifrec++;
			}
			CHECK(pp->p_vnode);
		}

		trace6(TR_PG_RECLAIM, pp, pp->p_vnode, pp->p_offset,
			ap, age, freemem);
	}

	mon_exit(&page_freelock);
	(void) splx(s);
}

#ifdef	DEBUG

/*
 * Search the hash list for a page with the specified <vp, off> and
 * then reclaim it if found on the free list.
 */
page_t *
page_find(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int len = 0;
	register int s;

	s = splvm();
	mon_enter(&page_mplock);
	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off && pp->p_gone == 0)
			break;
	if (pp != NULL) {
		pagecnt.pc_find_hit++;
		if (pp->p_free)
			page_reclaim(pp);
	} else
		pagecnt.pc_find_miss++;
	if (len > PC_HASH_CNT)
		len = PC_HASH_CNT;
	pagecnt.pc_find_hashlen[len]++;
	mon_exit(&page_mplock);
	(void) splx(s);
	return (pp);
}

/*
 * Quick page lookup to merely find if a named page exists
 * somewhere w/o having to worry about which list it is on.
 */
page_t *
page_exists(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int len = 0;
	register int s;

	s = splvm();
	mon_enter(&page_mplock);
	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off && pp->p_gone == 0)
			break;
	if (pp)
		pagecnt.pc_find_hit++;
	else
		pagecnt.pc_find_miss++;
	if (len > PC_HASH_CNT)
		len = PC_HASH_CNT;
	pagecnt.pc_find_hashlen[len]++;
	mon_exit(&page_mplock);
	(void) splx(s);
	return (pp);
}


#else

/*
 * Search the hash list for a page with the specified <vp, off> and
 * then reclaim it if found on the free list.
 */
page_t *
page_find(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int s;
	s = splvm();
	mon_enter(&page_mplock);
	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash) {
		if (pp->p_vnode == vp && pp->p_offset == off && 
			pp->p_gone == 0) {
				if (pp->p_free) page_reclaim(pp);
				break;
		}
	}
	mon_exit(&page_mplock);
	splx(s);
	return(pp);
}

/*
 * Quick page lookup to merely find if a named page exists
 * somewhere w/o having to worry about which list it is on.
 */
page_t *
page_exists(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int s;

	s = splvm();
	mon_enter(&page_mplock);
	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash)
		if (pp->p_vnode == vp && pp->p_offset == off && pp->p_gone == 0)
			break;
	mon_exit(&page_mplock);
	(void) splx(s);
	return (pp);
}


#endif

/*
 * Find a page representing the specified <vp, offset>.
 * If we find the page but it is intransit coming in,
 * we wait for the IO to complete and then reclaim the
 * page if it was found on the free list.
 */
page_t *
page_lookup(vp, off)
	struct vnode *vp;
	u_int off;
{
	register page_t *pp;
	register int s;

again:
	pp = page_find(vp, off);
	if (pp != NULL) {
		ASSERT(pp >= pages && pp < epages);
		/*
		 * Try calling cleanup here to reap the
		 * async buffers queued up for processing.
		 */
		if (pp->p_intrans && pp->p_pagein && bclnlist) {
			cleanup();
		}

		s = splvm();
		mon_enter(&page_mplock);
		while (pp->p_lock && pp->p_intrans && pp->p_vnode == vp &&
		    pp->p_offset == off && !pp->p_gone &&
		    (pp->p_pagein || nopagereclaim)) {
			cnt.v_intrans++;
			page_cv_wait(pp);
		}

		/*
		 * If we still have the right page and it is now
		 * on the free list, get it back via page_reclaim.
		 * Note that when a page is on the free list, it
		 * maybe ripped away at interrupt level.  After
		 * we reclaim the page, it cannot not be taken away
		 * from us at interrupt level anymore.
		 */
		if (pp->p_vnode == vp && pp->p_offset == off && !pp->p_gone) {
			if (pp->p_free)
				page_reclaim(pp);
		} else {
			mon_exit(&page_mplock);
			(void) splx(s);
			goto again;
		}
		mon_exit(&page_mplock);
		(void) splx(s);
	}
	return (pp);
}

/*
 * Enter page ``pp'' in the hash chains and
 * vnode page list as referring to <vp, offset>.
 */
int
page_enter(pp, vp, offset)
	page_t *pp;
	struct vnode *vp;
	u_int offset;
{
	register int v;

	mon_enter(&page_mplock);

	if (page_exists(vp, offset) != NULL) {
		/* already entered? */
		v = -1;
	} else {
		page_hashin(pp, vp, offset, 1);
		CHECK(vp);

		v = 0;
	}

	mon_exit(&page_mplock);

	trace4(TR_PG_ENTER, pp, vp, offset, v);

	return (v);
}

/*
 * page_abort will cause a page to lose its
 * identity and to go (back) to the free list.
 */
void
page_abort(pp)
	register page_t *pp;
{
	register struct vnode *vp;
	register int s;
	register int type;

	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_free == 0);

	if ((vp = pp->p_vnode) != NULL) 
		pp->p_gone = 1;
	if (pp->p_keepcnt != 0) {
		/*
		 * We cannot do anything with the page now.
		 * page_free() will be called later when
		 * the keep count goes back to zero.
		 */
		trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 1);
		return;
	}
	if (pp->p_intrans) {
		/*
		 * Since the page is already `gone', we can
		 * just let pvn_done() worry about freeing
		 * this page later when the IO finishes.
		 */
		trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 2);
		return;
	}

	if (pp->p_counts) {
		mon_enter(&page_locklock);
		pages_pp_locked--;
		if (pp->p_lckcnt) 
			availrmem++;
		availrmem += pp->p_cowcnt;
		pp->p_counts = 0;
		mon_exit(&page_locklock);
	}

	if (PAGE_HAS_MAPPINGS(pp)) {
		hat_pageunload(pp);
	}

	if (vp != NULL) {
		if (IS_SWAPVP(vp)) {
			struct anon *ap;
			if ((ap = swap_anon(vp, pp->p_offset)) &&
				(ap->an_refcnt > 0))
					ap->un.an_page = NULL;
		}
		page_hashout(pp);
	}

	pp->p_ref = pp->p_mod = 0;
	trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 0);

	/*
	 * Ordinarily page_free() may be called to do the actual work.
	 * Performance considerations drive doing the work here itself,
	 * since we know the page cannot go the cachelist.       
	 */

	s = splvm();
	mon_enter(&page_freelock);
	page_unlock(pp);
	pp->p_free = 1;
	pp->p_age = 1;
	freemem++;
#if RESTRICTED_DMA
	if (rdma_enabled && DMA_PP(pp))
		mem_freemem[type = DMA_PAGE]++;
	else
#endif
		type = STD_PAGE;
	page_freelist_size[type]++;
	page_add(&page_freelist[type], pp);
	BUMPPGCOUNT(pagecnt.pc_free_free);
	mon_exit(&page_freelock);
	CHECKFREE();
	if (freemem_wait) {
		freemem_wait = 0;
		wakeprocs((caddr_t)&freemem, PRMPT);
	}
	if (ext_freemem_wait) {
		ext_freemem_wait = 0;
		wakeprocs((caddr_t)&ext_freemem_wait, PRMPT);
	}
	(void) splx(s);
}

/*
 * Put page on the "free" list.  The free list is really two circular lists
 * with page_freelist and page_cachelist pointers into the middle of the lists.
 */
void
page_free(pp, dontneed)
	register page_t *pp;
	int dontneed;
{
	register struct vnode *vp;
	struct anon *ap;
	register int s;
	int type;

	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_free == 0);
	ASSERT(pp->p_uown == NULL);

	vp = pp->p_vnode;
	CHECK(vp);

	/*
	 * If we are a swap page, get rid of corresponding
	 * page hint pointer in the anon vector (since it is
	 * easy to do right now) so that we have to find this
	 * page via a page_lookup to force a reclaim.
	 *
	 * Page frees occur also as a result of page_abort and
	 * free_vp_pages, and changes to this routine should be
	 * checked to determine applicability there.
	 */

	/* XXX : This may go away eventually */
	if (vp && IS_SWAPVP(vp)) {
		if ((ap = swap_anon(pp->p_vnode, pp->p_offset)) &&
			(ap->an_refcnt > 0))
				ap->un.an_page = NULL;
	}

	if (pp->p_gone) {
		if (pp->p_intrans || pp->p_keepcnt != 0) {
			/*
			 * This page will be freed later from pvn_done
			 * (intrans) or the segment unlock routine.
			 * For now, the page will continue to exist,
			 * but with the "gone" bit on.
			 */
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 0);
			return;
		}
		if (vp)
			page_hashout(pp);
		vp = NULL;
	}
	ASSERT(pp->p_intrans == 0);

	if ((u_int)(pp->p_keepcnt) 
		| (u_int)(PAGE_HAS_MAPPINGS(pp)) | (u_int)(pp->p_counts))
			cmn_err(CE_PANIC, "page_free");

	s = splvm();
	mon_enter(&page_freelock);

	/*
	 * Unlock the page before inserting it on the free list.
	 */
	page_unlock(pp);

	/*
	 * Now we add the page to the head of the free list.
	 * But if this page is associated with a paged vnode
	 * then we adjust the head forward so that the page is
	 * effectively at the end of the list.
	 */
	pp->p_free = 1;
	pp->p_ref = pp->p_mod = 0;
	freemem++;
#if RESTRICTED_DMA
	if (rdma_enabled && DMA_PP(pp))
		mem_freemem[type = DMA_PAGE]++;
	else
#endif
		type = STD_PAGE;
	if (vp == NULL) {
		/* page has no identity, put it on the front of the free list */
		pp->p_age = 1;
		page_freelist_size[type]++;
		page_add(&page_freelist[type], pp);
		BUMPPGCOUNT(pagecnt.pc_free_free);
		trace6(TR_PG_FREE, pp, vp, pp->p_offset, dontneed, freemem, 1);
	} else {
		page_cachelist_size[type]++;
		page_add(&page_cachelist[type], pp);
		if (!dontneed || nopageage) {
			/* move it to the tail of the list */
			page_cachelist[type] = page_cachelist[type]->p_next;
			BUMPPGCOUNT(pagecnt.pc_free_cache);
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 2);
		} else {
			BUMPPGCOUNT(pagecnt.pc_free_dontneed);
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 3);
		}
	}

	mon_exit(&page_freelock);

	CHECK(vp);
	CHECKFREE();

	if (freemem_wait) {
		freemem_wait = 0;
		wakeprocs((caddr_t)&freemem, PRMPT);
	}
	if (ext_freemem_wait) {
		ext_freemem_wait = 0;
		wakeprocs((caddr_t)&ext_freemem_wait, PRMPT);
	}
	(void) splx(s);
}




STATIC int free_pages = 0;

void
free_vp_pages(vp, off, len)
	register struct vnode *vp;
	register u_int off;
	u_int len;
{
	register page_t *pp, *epp;
	register u_int eoff;
	int type;
	int s;

	eoff = off + len;

	if (free_pages == 0)
		return;
	CHECK(vp);
	/* free_vp_page may take some time so PREEMPT() */
	PREEMPT();
	s = splvm();
	if ((pp = epp = vp->v_pages) != 0) {
		do {
			if (pp->p_vnode != vp)
				continue; /* must be a marker page */
			if (pp->p_offset < off || pp->p_offset >= eoff)
				continue;
			ASSERT(!pp->p_intrans || pp->p_keepcnt);
			if (pp->p_mod ) /* XXX somebody needs to handle these */
				continue;
			if (pp->p_keepcnt || PAGE_HAS_MAPPINGS(pp) || pp->p_free ||
				pp->p_lckcnt || pp->p_cowcnt)
				continue;
			ASSERT(pp >= pages && pp < epages);
			ASSERT(!pp->p_gone);	/* p_keepcnt would be up */
			mon_enter(&page_freelock);
			page_unlock(pp);
			freemem++;
			ASSERT(pp->p_free == 0);
			ASSERT(pp->p_intrans == 0);
			ASSERT(pp->p_keepcnt == 0);
			pp->p_free = 1;
			pp->p_ref = 0;
#if RESTRICTED_DMA
			if (rdma_enabled && DMA_PP(pp))
				mem_freemem[type = DMA_PAGE]++;
			else
#endif
				type = STD_PAGE;
			page_cachelist_size[type]++;
			page_add(&page_cachelist[type], pp);
			page_cachelist[type] = page_cachelist[type]->p_next;
			BUMPPGCOUNT(pagecnt.pc_free_cache);
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 3);
			mon_exit(&page_freelock);
			CHECK(vp);
			CHECKFREE();
		} while ((pp = pp->p_vpnext) != epp);
#if RESTRICTED_DMA
		if (freemem_wait && (page_cachelist[STD_PAGE] != NULL ||
			(rdma_enabled && page_cachelist[DMA_PAGE] != NULL)))
#else
		if (freemem_wait && page_cachelist[STD_PAGE] != NULL)
#endif
		{
			freemem_wait = 0;
			wakeprocs((caddr_t)&freemem, PRMPT);
		}
#if RESTRICTED_DMA
		if (ext_freemem_wait && (page_cachelist[STD_PAGE] != NULL ||
			(rdma_enabled && page_cachelist[DMA_PAGE] != NULL)))
#else
		if (ext_freemem_wait && page_cachelist[STD_PAGE] != NULL)
#endif
		{
			ext_freemem_wait = 0;
			wakeprocs((caddr_t)&ext_freemem_wait, PRMPT);
		}
	}
	splx(s);
}

/*
 * Remove the page from the free list.
 * The caller is responsible for calling this
 * routine at splvm().
 */
STATIC void
page_unfree(pp)
	register page_t *pp;
{
	int type;

	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_intrans == 0);
	ASSERT(pp->p_free != 0);

	freemem--;
#if RESTRICTED_DMA
	if (rdma_enabled && DMA_PP(pp))
		mem_freemem[type = DMA_PAGE]--;
	else
#endif
		type = STD_PAGE;
	if (pp->p_age) {
		page_sub(&page_freelist[type], pp);
		page_freelist_size[type]--;
	} else {
		page_sub(&page_cachelist[type], pp);
		page_cachelist_size[type]--;
	}
	pp->p_free = pp->p_age = 0;
}

/*
 * Allocate enough pages for bytes of data.
 * Return a doubly linked, circular list of pages.
 * Must spl around entire routine to prevent races from
 * pages being allocated at interrupt level.
 */
page_t *
page_get(bytes, flags)
	u_int bytes;
	u_int flags;
{
	register page_t *pp;
	page_t *plist = NULL;
	register int npages;
	register int physcontig;
	register int reqfree;
	int type;
	int s;

#if RESTRICTED_DMA
	if (rdma_enabled && (flags & P_DMA))
		type = DMA_PAGE;
	else
#endif
		type = STD_PAGE;

	/*
	 * Try to see whether request is too large to *ever* be
	 * satisfied, in order to prevent deadlock.  We arbitrarily
	 * decide to limit maximum size requests to max_page_get.
	 */
	if ((npages = btopr(bytes)) >= max_page_get[type]) {
		trace4(TR_PAGE_GET, bytes, flags, freemem, 1);
		return (plist);
	}

	physcontig = ((flags & P_PHYSCONTIG) && (npages > 1));

	/*
	 * Never subject sched to the resource limit.
	 * We use curproc instead of u.u_procp because segu_get() is called
	 * at startup before "u." structure is mapped in.
	 * Note that although curproc is not necessarily valid at interrupt
	 * level (since pswtch() can be interrupted), the only time we should
	 * get here at interrupt level is for kmem_alloc(), in which case,
	 * P_NORESOURCELIM will already be set.
	 */
	if (curproc == proc_sched)
		flags |= P_NORESOURCELIM;

	reqfree = (flags & P_NORESOURCELIM) ? npages : npages + minpagefree;

	/*
	 * If possible, wait until there are enough
	 * free pages to satisfy our entire request.
	 *
	 * XXX:	Before waiting, we try to arrange to get more pages by
	 *	processing the i/o completion list and prodding the
	 *	pageout daemon.  However, there's nothing to guarantee
	 *	that these actions will provide enough pages to satisfy
	 *	the request.  In particular, the pageout daemon stops
	 *	running when freemem > lotsfree, so if npages > lotsfree
	 *	there's nothing going on that will bring freemem up to
	 *	a value large enough to satisfy the request.
	 */
	s = splvm();
	while (mem_freemem[type] < reqfree) {
try_again:
		if (!(flags & P_CANWAIT)) {
			trace4(TR_PAGE_GET, bytes, flags, freemem, 2);
			(void) splx(s);
			return (plist);
		}
		/*
		 * Given that we can wait, call cleanup directly to give
		 * it a chance to add pages to the free list.  This strategy
		 * avoids the cost of context switching to the pageout
		 * daemon unless it's really necessary.
		 */
		if (bclnlist != NULL) {
			(void) splx(s);
			cleanup();
			s = splvm();
			continue;
		}
		/*
		 * There's nothing immediate waiting to become available.
		 * Turn the pageout daemon loose to find something.
		 */
		trace1(TR_PAGEOUT_CALL, 0);
		wakeprocs((caddr_t)proc_pageout, PRMPT);
		freemem_wait++;
		trace4(TR_PAGE_GET_SLEEP, bytes, flags, freemem, 0);
		(void) sleep((caddr_t)&freemem, PSWP+2);
		trace4(TR_PAGE_GET_SLEEP, bytes, flags, freemem, 1);
	}

	mon_enter(&page_freelock);

	if (physcontig) {
		register int numpages;
		register struct pp_chunk *chp;
		register page_t *epage;

		for (chp = pp_first; chp != NULL; chp = chp->pp_next) {
			epage = chp->pp_epage - (npages - 1);
			for (pp = chp->pp_page; pp < epage; ++pp) {
				if (!pp->p_free)
					continue;
#if RESTRICTED_DMA
				if (type == DMA_PAGE && !DMA_PP(pp))
					continue;
#endif
				numpages = npages;
				do {
					++pp;
					if (--numpages == 0)
						goto found_pages;
#if RESTRICTED_DMA
					if (type == DMA_PAGE && !DMA_PP(pp))
						break;
#endif
				} while (pp->p_free);
			}
		}

		mon_exit(&page_freelock);
		goto try_again;
	}

found_pages:
	freemem -= npages;

	trace4(TR_PAGE_GET, bytes, flags, freemem, 0);

	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back. The
	 * first clause of the test prevents waking up the pageout
	 * daemon in situations where it would decide that there's
	 * nothing to do.  (However, it also keeps bclnlist from
	 * being processed when it otherwise would.)
	 *
	 * XXX: Check against lotsfree rather than desfree?
	 */
	if (nscan < desscan && freemem < desfree) {
		trace1(TR_PAGEOUT_CALL, 1);
		wakeprocs((caddr_t)proc_pageout, PRMPT);
	}

	/*
	 * Pull the pages off the free list and build the return list.
	 */
	while (npages--) {

		if (physcontig) {
			--pp;
#if RESTRICTED_DMA
			if (rdma_enabled && DMA_PP(pp))
				mem_freemem[DMA_PAGE]--;
#endif
		}
#if RESTRICTED_DMA
		else if (mem_freemem[DMA_PAGE] > lotsfree) {
			ASSERT(rdma_enabled);
			if ((pp = page_freelist[DMA_PAGE]) != NULL) {
				ASSERT(pp->p_age != 0);
			} else if ((pp = page_cachelist[DMA_PAGE]) != NULL) {
				ASSERT(pp->p_age == 0);
			} else {
				cmn_err(CE_PANIC, "page_get: freemem error");
			}
			mem_freemem[DMA_PAGE]--;
		}
#endif /* RESTRICTED_DMA */
		else if ((pp = page_freelist[type]) != NULL) {
			ASSERT(pp->p_age != 0);
			if (type != STD_PAGE)
				mem_freemem[type]--;
		} else if ((pp = page_cachelist[type]) != NULL) {
			ASSERT(pp->p_age == 0);
			if (type != STD_PAGE)
				mem_freemem[type]--;
#if RESTRICTED_DMA
		} else if ((pp = page_freelist[DMA_PAGE]) != NULL) {
			ASSERT(rdma_enabled);
			ASSERT(pp->p_age != 0);
			mem_freemem[DMA_PAGE]--;
		} else if ((pp = page_cachelist[DMA_PAGE]) != NULL) {
			ASSERT(rdma_enabled);
			ASSERT(pp->p_age == 0);
			mem_freemem[DMA_PAGE]--;
#endif /* RESTRICTED_DMA */
		} else {
			cmn_err(CE_PANIC, "page_get: freemem error");
		}

		ASSERT(pp->p_age == 0 || pp->p_vnode == NULL);
#if RESTRICTED_DMA
		ASSERT(type != DMA_PAGE || DMA_PP(pp));
#endif

		page_reuse(pp);
		page_add(&plist, pp);
	}
	mon_exit(&page_freelock);
	CHECKFREE();
	(void) splx(s);
	return (plist);
}


STATIC void
page_reuse(pp)
	register page_t	*pp;
{
	register u_int	i;
	int type;

#if RESTRICTED_DMA
	if (rdma_enabled && DMA_PP(pp))
		type = DMA_PAGE;
	else
#endif
		type = STD_PAGE;

	if (pp->p_age) {
		trace5(TR_PG_ALLOC, pp, pp->p_vnode, pp->p_offset,
			0, 0);
		BUMPPGCOUNT(pagecnt.pc_get_free);
		page_sub(&page_freelist[type], pp);
		page_freelist_size[type]--;
	} else {
		trace5(TR_PG_ALLOC, pp, pp->p_vnode, pp->p_offset,
			pp->p_age, 1);
		BUMPPGCOUNT(pagecnt.pc_get_cache);
		page_sub(&page_cachelist[type], pp);
		page_cachelist_size[type]--;
		if (pp->p_vnode) {
			/* destroy old vnode association */
			CHECK(pp->p_vnode);
			page_hashout(pp);
		}
	}

	ASSERT(!PAGE_HAS_MAPPINGS(pp));
	ASSERT(pp->p_free);
	ASSERT(pp->p_intrans == 0);
	ASSERT(pp->p_keepcnt == 0);

	/*
	 * Initialize the p_dblist[] fields.
	 */
	for (i=0; i<(PAGESIZE/NBPSCTR); i++)
		pp->p_dblist[i] = -1;

	pp->p_free = pp->p_mod = pp->p_nc = pp->p_age = 0;
	pp->p_lock = pp->p_intrans = pp->p_pagein = 0;
	pp->p_ref = 1;		/* protect against immediate pageout */
	pp->p_keepcnt = 1;	/* mark the page as `kept' */
}


/*
 * Special version of page_get() which enforces specified alignment constraints
 * on the physical address.
 * The first page is guaranteed to have (addr & align_mask) == align_val.
 * The remaining pages will be physically contigous.
 */

#define ALIGNED_PP(pp)	((page_pptonum(pp) & align_mask) == align_val)

page_t *
page_get_aligned(bytes, align_mask, align_val, flags)
	u_int bytes;
	u_int align_mask, align_val;
	u_int flags;
{
	register page_t 	*pp;
	register int		npages, numpages;
	register struct pp_chunk *chp;
	register page_t		*epage;
	page_t			*plist = NULL;
	int			s;
	register int		reqfree;

	ASSERT(!(flags & P_DMA));

	ASSERT((align_mask & MMU_PAGEOFFSET) == 0);
	ASSERT((align_val & MMU_PAGEOFFSET) == 0);
	align_mask = (align_mask >> MMU_PAGESHIFT);
	align_val = (align_val >> MMU_PAGESHIFT);

	npages = btopr(bytes);

	/*
	 * Try to see whether request is too large to *ever* be
	 * satisfied, in order to prevent deadlock.  We arbitrarily
	 * decide to limit maximum size requests to max_page_get.
	 */
	if (npages >= max_page_get[STD_PAGE]) {
		trace4(TR_PAGE_GET, bytes, flags, freemem, 1);
		return (plist);
	}

	/*
	 * Never subject sched to the resource limit.
	 * We use curproc instead of u.u_procp because segu_get() is called
	 * at startup before "u." structure is mapped in.
	 * Note that although curproc is not necessarily valid at interrupt
	 * level (since pswtch() can be interrupted), the only time we should
	 * get here at interrupt level is for kmem_alloc(), in which case,
	 * P_NORESOURCELIM will already be set.
	 */
	if (curproc == proc_sched)
		flags |= P_NORESOURCELIM;

	reqfree = (flags & P_NORESOURCELIM) ? npages : npages + minpagefree;

	/*
	 * If possible, wait until there are enough
	 * free pages to satisfy our entire request.
	 *
	 * XXX:	Before waiting, we try to arrange to get more pages by
	 *	processing the i/o completion list and prodding the
	 *	pageout daemon.  However, there's nothing to guarantee
	 *	that these actions will provide enough pages to satisfy
	 *	the request.  In particular, the pageout daemon stops
	 *	running when freemem > lotsfree, so if npages > lotsfree
	 *	there's nothing going on that will bring freemem up to
	 *	a value large enough to satisfy the request.
	 */
	s = splvm();

	while (freemem < reqfree) {
try_again:
		if (!(flags & P_CANWAIT)) {
			trace4(TR_PAGE_GET, bytes, flags, freemem, 2);
			(void) splx(s);
			return(plist);
		}
		/*
		 * Given that we can wait, call cleanup directly to give
		 * it a chance to add pages to the free list.  This strategy
		 * avoids the cost of context switching to the pageout
		 * daemon unless it's really necessary.
		 */
		if (bclnlist != NULL) {
			(void) splx(s);
			cleanup();
			s = splvm();
			continue;
		}
		/*
		 * There's nothing immediate waiting to become available.
		 * Turn the pageout daemon loose to find something.
		 */
		trace1(TR_PAGEOUT_CALL, 0);
		wakeprocs((caddr_t)proc_pageout, PRMPT);
		freemem_wait++;
		trace3(TR_PAGE_GET_SLEEP, flags, freemem, 0);
		(void) sleep((caddr_t)&freemem, PSWP+2);
		trace3(TR_PAGE_GET_SLEEP, flags, freemem, 1);
	}

	/*
	 * Scan pages looking for a free page.  If found, check to see
	 * if it has the proper alignment (according to the arguments).
	 *
	 * Keep going until we find a contiguous chunk of pages that
	 * meet the constraints.
	 */

	for (chp = pp_first; chp != NULL; chp = chp->pp_next) {
		epage = chp->pp_epage - (npages - 1);
		for (pp = chp->pp_page; pp < epage; ++pp) {
			if (!pp->p_free || !ALIGNED_PP(pp))
				continue;
			numpages = npages;
			do {
				++pp;
				if (--numpages == 0)
					goto found_pages;
			} while (pp->p_free);
		}
	}

	mon_exit(&page_freelock);
	goto try_again;

found_pages:
	freemem -= npages;

	trace4(TR_PAGE_GET, bytes, flags, freemem, 0);

	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back. The
	 * first clause of the test prevents waking up the pageout
	 * daemon in situations where it would decide that there's
	 * nothing to do.  (However, it also keeps bclnlist from
	 * being processed when it otherwise would.)
	 *
	 * XXX: Check against lotsfree rather than desfree?
	 */
	if (nscan < desscan && freemem < desfree) {
		trace1(TR_PAGEOUT_CALL, 1);
		wakeprocs((caddr_t)proc_pageout, PRMPT);
	}

	/*
	 * Pull the pages off the free list and build the return list.
	 */
	while (npages--) {
		page_reuse(--pp);
		page_add(&plist, pp);
	}

	mon_exit(&page_freelock);
	CHECKFREE();
	(void) splx(s);
	return pp;
}

#ifdef DEBUG
/*
 * XXX - need to fix up all this page rot!
 */

/*
 * Release a keep count on the page and handle aborting the page if the
 * page is no longer held by anyone and the page has lost its identity.
 */
void
page_rele(pp)
	page_t *pp;
{

	mon_enter(&page_mplock);

	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_free == 0);

	if (pp->p_keepcnt == 0)			/* sanity check */
		cmn_err(CE_PANIC, "page_rele");
	if (--pp->p_keepcnt == 0) {
		ASSERT(pp->p_intrans == 0);
		while (pp->p_want) {
			pp->p_want = 0;
			cv_broadcast(&page_mplock, (char *)pp);
		}
		ASSERT(pp->p_intrans == 0);
	}

	mon_exit(&page_mplock);

	if (pp->p_keepcnt == 0 && (pp->p_gone || pp->p_vnode == NULL))
		page_abort(pp);			/* yuck */
}

/*
 * Lock a page.
 */
void
page_lock(pp)
	page_t *pp;
{

	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_free == 0);
	mon_enter(&page_mplock);
	while (pp->p_lock)
		page_cv_wait(pp);
	pp->p_lock = 1;
	mon_exit(&page_mplock);
}

/*
 * Unlock a page.
 */
void
page_unlock(pp)
	page_t *pp;
{

	ASSERT(pp >= pages && pp < epages);
	ASSERT(!pp->p_intrans);

	mon_enter(&page_mplock);
	pp->p_lock = 0;
	while (pp->p_want) {
		pp->p_want = 0;
		cv_broadcast(&page_mplock, (char *)pp);
	}
	mon_exit(&page_mplock);
}
#endif

/*
 * Add page ``pp'' to the hash/vp chains for <vp, offset>.
 */
void
page_hashin(pp, vp, offset, lock)
	register page_t *pp;
	register struct vnode *vp;
	u_int offset, lock;
{
	register page_t **hpp;
	register int s;

	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_uown == NULL);

	pp->p_vnode = vp;
	pp->p_offset = offset;
	pp->p_lock = lock;

	/*
	 * Raise priority to splvm() since the hash list
	 * can be manipulated at interrupt level.
	 */
	s = splvm();

	hpp = &page_hash[PAGE_HASHFUNC(vp, offset)];
	pp->p_hash = *hpp;
	*hpp = pp;

	/* XXX this should never happen */
	if (vp == (struct vnode *)NULL) {
		(void) splx(s);
		return;			/* no real vnode */
	}

	/*
	 * Add the page to the end of the v_pages linked list.
	 * This is part of making pvn_vplist_dirty() et al.
	 * process all new pages added while it was sleeping.
	 */
	if (vp->v_pages == NULL) {
		vp->v_pages = pp->p_vpnext = pp->p_vpprev = pp;
	} else {
		pp->p_vpnext = vp->v_pages;
		pp->p_vpprev = vp->v_pages->p_vpprev;
		vp->v_pages->p_vpprev = pp;
		pp->p_vpprev->p_vpnext = pp;
	}
	CHECKPP(pp);
	(void) splx(s);
}

/*
 * Remove page ``pp'' from the hash and vp chains and remove vp association.
 */
void
page_hashout(pp)
	register page_t *pp;
{
	register page_t **hpp, *hp;
	register struct vnode *vp;
	register int s;

	ASSERT(pp >= pages && pp < epages);

	/*
	 * Raise priority to splvm() since the hash list
	 * can be manipulated at interrupt level.
	 */
	s = splvm();
	CHECKPP(pp);
	vp = pp->p_vnode;
	hpp = &page_hash[PAGE_HASHFUNC(vp, pp->p_offset)];
	for (;;) {
		hp = *hpp;
		if (hp == pp)
			break;
		if (hp == NULL)
			cmn_err(CE_PANIC, "page_hashout");
		hpp = &hp->p_hash;
	}
	*hpp = pp->p_hash;

	pp->p_hash = NULL;
	pp->p_vnode = NULL;
	pp->p_offset = 0;
	pp->p_gone = 0;

	/*
	 * Remove this page from the linked list of pages
	 * using p_vpnext/p_vpprev pointers for the list.
	 */
	CHECKPP(pp);
	if (vp->v_pages == pp)
		vp->v_pages = pp->p_vpnext;		/* go to next page */

	if (vp->v_pages == pp)
		vp->v_pages = NULL;			/* page list is gone */
	else {
		pp->p_vpprev->p_vpnext = pp->p_vpnext;
		pp->p_vpnext->p_vpprev = pp->p_vpprev;
	}
	pp->p_vpprev = pp->p_vpnext = pp;	/* make pp a list of one */
	(void) splx(s);
}

/*
 * Add the page to the front of the linked list of pages
 * using p_next/p_prev pointers for the list.
 * The caller is responsible for protecting the list pointers.
 */
void
page_add(ppp, pp)
	register page_t **ppp, *pp;
{

	ASSERT(pp >= pages && pp < epages);
	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
	} else {
		pp->p_next = *ppp;
		pp->p_prev = (*ppp)->p_prev;
		(*ppp)->p_prev = pp;
		pp->p_prev->p_next = pp;
	}
	*ppp = pp;
	CHECKPP(pp);
}

/*
 * Remove this page from the linked list of pages
 * using p_next/p_prev pointers for the list.
 * The caller is responsible for protecting the list pointers.
 */
void
page_sub(ppp, pp)
	register page_t **ppp, *pp;
{

	ASSERT(pp >= pages && pp < epages);
	CHECKPP(pp);
	if (*ppp == NULL || pp == NULL)
		cmn_err(CE_PANIC, "page_sub");

	if (*ppp == pp)
		*ppp = pp->p_next;		/* go to next page */

	if (*ppp == pp)
		*ppp = NULL;			/* page list is gone */
	else {
		pp->p_prev->p_next = pp->p_next;
		pp->p_next->p_prev = pp->p_prev;
	}
	pp->p_prev = pp->p_next = pp;		/* make pp a list of one */
}

/*
 * Add this page to the list of pages, sorted by offset.
 * Assumes that the list given by *ppp is already sorted.
 * The caller is responsible for protecting the list pointers.
 */
void
page_sortadd(ppp, pp)
	register page_t **ppp, *pp;
{
	register page_t *p1;
	register u_int off;

	ASSERT(pp >= pages && pp < epages);
	CHECKLIST(*ppp);
	CHECKPP(pp);
	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
		*ppp = pp;
	} else {
		/*
		 * Figure out where to add the page to keep list sorted
		 */
		p1 = *ppp;
		if (pp->p_vnode != p1->p_vnode && p1->p_vnode != NULL &&
		    pp->p_vnode != NULL)
			cmn_err(CE_PANIC, "page_sortadd: bad vp");

		off = pp->p_offset;
		if (off < p1->p_prev->p_offset) {
			do {
				if (off == p1->p_offset && p1->p_gone)
					break;
				else

				if (off == p1->p_offset)
					cmn_err(CE_PANIC,
						"page_sortadd: same offset");
				if (off < p1->p_offset)
					break;
				p1 = p1->p_next;
			} while (p1 != *ppp);
		}

		/* link in pp before p1 */
		pp->p_next = p1;
		pp->p_prev = p1->p_prev;
		p1->p_prev = pp;
		pp->p_prev->p_next = pp;

		if (off < (*ppp)->p_offset)
			*ppp = pp;		/* pp is at front */
	}
	CHECKLIST(*ppp);
}

/*
 * Wait for page if kept and then reclaim the page if it is free.
 * Caller needs to verify page contents after calling this routine.
 *
 * NOTE:  The caller must ensure that the page is not on
 *	  the free list before calling this routine.
 */
void
page_wait(pp)
	register page_t *pp;
{
	register struct vnode *vp;
	register u_int offset;
	register int s;

	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_free == 0);
	CHECKPP(pp);
	vp = pp->p_vnode;
	offset = pp->p_offset;

	/*
	 * Reap any pages in the to be cleaned list.
	 * This might cause the page that we might
	 * have to wait for to become available.
	 */
	if (bclnlist != NULL) {
		cleanup();
		/*
		 * The page could have been freed by cleanup, so
		 * verify the identity after raising priority since
		 * it may be ripped away at interrupt level.
		 */
		s = splvm();
		if (pp->p_vnode != vp || pp->p_offset != offset) {
			(void) splx(s);
			return;
		}
	} else
		s = splvm();

	mon_enter(&page_mplock);
	while (pp->p_keepcnt != 0) {
		page_cv_wait(pp);
		/*
		 * Verify the identity of the page since it
		 * could have changed while we were sleeping.
		 */
		if (pp->p_vnode != vp || pp->p_offset != offset)
			break;
	}

	/*
	 * If the page is now on the free list and still has
	 * its original identity, get it back.  If the page
	 * has lost its old identity, the caller of page_wait
	 * is responsible for verifying the page contents.
	 */
	if (pp->p_vnode == vp && pp->p_offset == offset && pp->p_free) {
		page_reclaim(pp);
	}

	mon_exit(&page_mplock);
	(void) splx(s);
	CHECKPP(pp);
}

/*
 * Lock a physical page into memory "long term".  Used to support "lock
 * in memory" functions.  Accepts the page to be locked, and a cow variable
 * to indicate whether a the lock will travel to the new page during
 * a potential copy-on-write).  
 */

/* ARGSUSED */

int
page_pp_lock(pp, cow, kernel)
	page_t *pp;			/* page to be locked */
	int cow;			/* cow lock */
	int kernel;			/* must succeed -- ignore checking */
{
	int r = 0;			/* result -- assume failure */

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	page_lock(pp);
	mon_enter(&page_locklock);

	if (cow) {
		if ((availrmem - 1 ) >= pages_pp_maximum) {
		 	--availrmem;
			pages_pp_locked++;
			if (pp->p_cowcnt < (u_short) PAGE_LOCK_MAXIMUM)
				if (++pp->p_cowcnt == PAGE_LOCK_MAXIMUM)
					cmn_err(CE_WARN,
					"Page frame 0x%x locked permanently\n",
						page_pptonum(pp));
			r = 1;
		} 
	} else {
		if (pp->p_lckcnt) {
			if (pp->p_lckcnt < (u_short) PAGE_LOCK_MAXIMUM)
				if (++pp->p_lckcnt == PAGE_LOCK_MAXIMUM)
					cmn_err(CE_WARN,
					"Page frame 0x%x locked permanently\n",
						page_pptonum(pp));
			r = 1;
		} else {
			if (kernel) {
				/* availrmem accounting done by caller */
				pages_pp_kernel++;
				++pp->p_lckcnt;
				r = 1;
			} else if ((availrmem - 1) >= pages_pp_maximum) {
				pages_pp_locked++;
				--availrmem;
				++pp->p_lckcnt;
				r = 1;
			}
		}
	}
	mon_exit(&page_locklock);
	page_unlock(pp);
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	return (r);
}

/*
 * Decommit a lock on a physical page frame.  Account for cow locks if
 * appropriate.
 */

/* ARGSUSED */

void
page_pp_unlock(pp, cow, kernel)
	page_t *pp;			/* page to be unlocked */
	int cow;			/* expect cow lock */
	int kernel;			/* this was a kernel lock */
{

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	page_lock(pp);
	mon_enter(&page_locklock);

	if (cow) {
		ASSERT(pp->p_cowcnt > 0);
		pp->p_cowcnt--;
		pages_pp_locked--;
		availrmem++;
	} else {
		ASSERT(pp->p_lckcnt > 0);
		if (--pp->p_lckcnt == 0) {
			if (kernel) {
				pages_pp_kernel--;
			} else {
				pages_pp_locked--;
				availrmem++;
			}
		}
	}
	mon_exit(&page_locklock);
	page_unlock(pp);
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
}

/*
 * Transfer a cow lock to a real lock on a physical page.  Used after a 
 * copy-on-write of a locked page has occurred.  
 */
void
page_pp_useclaim(opp, npp)
	page_t *opp;		/* original page frame losing lock */
	page_t *npp;		/* new page frame gaining lock */
{
	ASSERT((short)opp->p_lckcnt >= 0);
	ASSERT((short)opp->p_cowcnt >= 1);
	ASSERT((short)npp->p_lckcnt >= 0);
	ASSERT((short)npp->p_cowcnt >= 0);
	page_lock(npp);
	page_lock(opp);
	mon_enter(&page_locklock);
	opp->p_cowcnt--;
	npp->p_cowcnt++;
	mon_exit(&page_locklock);
	page_unlock(opp);
	page_unlock(npp);
}

/*
 * Simple claim adjust functions -- used to support to support changes in
 * claims due to changes in access permissions.  Used by segvn_setprot().
 */
int
page_addclaim(pp)
	page_t *pp;
{
	int r = 1;			/* result */

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	mon_enter(&page_locklock);
	ASSERT(pp->p_lckcnt > 0);
	if (--pp->p_lckcnt == 0) {
		pp->p_cowcnt++;
	} else {
		if ((availrmem - 1 ) >= pages_pp_maximum) {
		 	--availrmem;
			pages_pp_locked++;
			if (pp->p_cowcnt < (u_short) PAGE_LOCK_MAXIMUM)
				if (++pp->p_cowcnt == PAGE_LOCK_MAXIMUM)
					cmn_err(CE_WARN,
					"Page frame 0x%x locked permanently\n",
						page_pptonum(pp));
		} else {
			pp->p_lckcnt++;
			r = 0;
		}
	}
	mon_exit(&page_locklock);
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	return (r);
}

void
page_subclaim(pp)
	page_t *pp;
{

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	mon_enter(&page_locklock);

	ASSERT(pp->p_cowcnt > 0);
	pp->p_cowcnt--;
	if (pp->p_lckcnt) {
		availrmem++;
		pages_pp_locked--;
	}
	pp->p_lckcnt++;
	mon_exit(&page_locklock);
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
}

/*
 * Mark a page as read-only when the page
 * contains holes as a result of file size
 * change (truncate up or lseek and write).
 */
u_int
page_rdonly(pp)
	page_t *pp;
{
	return (hat_rdonly(pp));
}

/*
 * Functions to transform a page pointer to a
 * physical page number and vice versa.
 */

#if defined(DEBUG) || MAX_MEM_CHUNK > 1

u_int
page_pptonum(pp)
	page_t *pp;
{
	struct pp_chunk *chp = pp_first;

	ASSERT(pp >= pages && pp < epages);

#if MAX_MEM_CHUNK > 1
	while (pp < chp->pp_page || pp >= chp->pp_epage) {
		chp = chp->pp_next;
		ASSERT(chp != NULL);
	}
#endif
	return ((u_int)((pp - chp->pp_page) * (PAGESIZE/MMU_PAGESIZE)) +
		chp->pp_pfn);
}

page_t *
page_numtopp(pfn)
	register u_int pfn;
{
	struct pp_chunk *chp = pp_first;

	while (pfn < chp->pp_pfn || pfn >= chp->pp_epfn) {
#if MAX_MEM_CHUNK > 1
		chp = chp->pp_next;
		if (chp == NULL)
#endif
			return ((page_t *)NULL);
	}
	return (&chp->pp_page[(pfn - chp->pp_pfn) / (PAGESIZE/MMU_PAGESIZE)]);
}

#endif	/* DEBUG || MAX_MEM_CHUNK > 1 */

/*
 * This routine is like page_numtopp, but will only return page structs
 * for pages which are ok for loading into hardware using the page struct.
 */
page_t *
page_numtookpp(pfn)
	register u_int pfn;
{
	register page_t *pp;

	if ((pp = page_numtopp(pfn)) == (page_t *)NULL)
		return ((page_t *)NULL);
	if (pp->p_free || pp->p_gone)
		return ((page_t *)NULL);
	return (pp);
}

/*
 * This routine is like page_numtopp, but will only return page structs
 * for pages which are ok for loading into hardware using the page struct.
 * If not for the things like the window system lock page where we
 * want to make sure that the kernel and the user are exactly cache
 * consistent, we could just always return a NULL pointer here since
 * anyone mapping physical memory isn't guaranteed all that much
 * on a virtual address cached machine anyways.  The important thing
 * here is not to return page structures for things that are possibly
 * currently loaded in DVMA space, while having the window system lock
 * page still work correctly.
 */
page_t *
page_numtouserpp(pfn)
	register u_int pfn;
{
	register page_t *pp;

	if ((pp = page_numtopp(pfn)) == (page_t *)NULL)
		return ((page_t *)NULL);
	if (pp->p_free || pp->p_gone || pp->p_intrans || pp->p_lock ||
	    /* is this page possibly involved in indirect (raw) IO? */
	    (pp->p_keepcnt > 0 && pp->p_vnode != NULL))
		return ((page_t *)NULL);
	return (pp);
}

/*
 * Debugging routine only!
 * XXX - places calling this should be debugging routines
 * or remove the test altogether or call cmn_err(CE_PANIC).
 */

extern int	call_demon();
STATIC int call_demon_flag = 1;

STATIC void
call_debug(mess)
	char *mess;
{
 	cmn_err(CE_WARN, mess);
	if (call_demon_flag)
		call_demon();
}

#ifdef PAGE_DEBUG
/*
 * Debugging routine only!
 */
STATIC void
page_vp_check(vp)
	register struct vnode *vp;
{
	register page_t *pp;
	int count = 0;
	int err = 0;

	if (vp == NULL)
		return;

	if ((pp = vp->v_pages) == NULL) {
		/* random check to see if no pages on this vp exist */
		if ((pp = page_find(vp, 0)) != NULL) {
			cmn_err(CE_CONT, "page_vp_check: pp=%x on NULL vp list\n", vp);
			call_debug("page_vp_check");
		}
		return;
	}

	do {
		/*
		 * stext and sdata denotes starting of kernel text and
		 * data portion. If a vp lies in this range, then that page
		 * is a marker page.
		 */
		if (pp->p_vnode != vp
		 && ((char *)pp->p_vnode < (char *)stext
		 || (char *)pp->p_vnode >= (char *)sdata)) {
			cmn_err(CE_CONT, "pp=%x pp->p_vnode=%x, vp=%x\n",
			    pp, pp->p_vnode, vp);
			err++;
		}
		if (pp->p_vpnext->p_vpprev != pp) {
			cmn_err(CE_CONT, "pp=%x, p_vpnext=%x, p_vpnext->p_vpprev=%x\n",
			    pp, pp->p_vpnext, pp->p_vpnext->p_vpprev);
			err++;
		}
		if (++count > 10000) {
			cmn_err(CE_CONT, "vp loop\n");
			err++;
			break;
		}
		pp = pp->p_vpnext;
	} while (err == 0 && pp != vp->v_pages);

	if (err)
		call_debug("page_vp_check");
}

/*
 * Debugging routine only!
 */
STATIC void
page_free_check()
{
	boolean_t err = B_FALSE;
	int count[NPAGETYPE];
	int type;
	register page_t *pp;

	for (type = 0; type < NPAGETYPE; type++) {
		count[type] = 0;
		if (page_freelist[type] != NULL) {
			pp = page_freelist[type];
			do {
				if (pp->p_free == 0 || pp->p_age == 0) {
					err = B_TRUE;
					cmn_err(CE_CONT,
						"page_free_check: pp = %x\n",
						pp);
				}
				count[type]++;
				pp = pp->p_next;
			} while (pp != page_freelist[type]);
		}
		if (page_cachelist[type] != NULL) {
			pp = page_cachelist[type];
			do {
				if (pp->p_free == 0 || pp->p_age != 0) {
					err = B_TRUE;
					cmn_err(CE_CONT,
						"page_free_check: pp = %x\n",
						pp);
				}
				count[type]++;
				pp = pp->p_next;
			} while (pp != page_cachelist[type]);
		}
	}

	for (type = 0; type < NPAGETYPE; type++) {
		if (type != STD_PAGE)
			count[STD_PAGE] += count[type];
	}

	for (type = 0; type < NPAGETYPE; type++) {
		if (count[type] != mem_freemem[type])
			err = B_TRUE;
	}

	if (err) {
		cmn_err(CE_CONT, "page_free_check:\n");
		for (type = 0; type < NPAGETYPE; type++) {
			cmn_err(CE_CONT, "\tcount[%d] = %x, freemem[%d] = %x\n",
				type, count[type], type, mem_freemem[type]);
		}
		call_debug("page_free_check");
	}
}

/*
 * Debugging routine only!
 * Verify that the list is properly sorted by offset on same vp
 */
void
page_list_check(plist)
	page_t *plist;
{
	register page_t *pp = plist;

	if (pp == NULL)
		return;
	while (pp->p_next != plist) {
		if (pp->p_next->p_offset <= pp->p_offset ||
		    pp->p_vnode != pp->p_next->p_vnode) {
			cmn_err(CE_CONT, "pp = %x <%x, %x> pp next = %x <%x, %x>\n",
			    pp, pp->p_vnode, pp->p_offset, pp->p_next,
			    pp->p_next->p_vnode, pp->p_next->p_offset);
			call_debug("page_list_check");
		}
		pp = pp->p_next;
	}
}

/*
 * Debugging routine only!
 * Verify that pp is actually on vp page list.
 */
void
page_pp_check(pp)
	register page_t *pp;
{
	register page_t *p1;
	register struct vnode *vp;

	if ((vp = pp->p_vnode) == (struct vnode *)NULL)
		return;

	if ((p1 = vp->v_pages) == (page_t *)NULL) {
		cmn_err(CE_CONT, "pp = %x, vp = %x\n", pp, vp);
		call_debug("NULL vp page list");
		return;
	}

	do {
		if (p1 == pp)
			return;
	} while ((p1 = p1->p_vpnext) != vp->v_pages);

	cmn_err(CE_CONT, "page %x not on vp %x page list\n", pp, vp);
	call_debug("vp page list");
}
#endif /* PAGE_DEBUG */

/*
 * The following are used by the sys3b S3BDELMEM and S3BADDMEM
 * functions.
 */

STATIC page_t *Delmem;	/* Linked list of deleted pages. */
STATIC int Delmem_cnt;	/* Count of number of deleted pages. */

STATIC int
page_delmem(count)
	register int count;
{
	register page_t	*pp;
		
	if (freemem < count
	  || availrmem - count < tune.t_minarmem 
	  || availsmem - count < tune.t_minasmem) {
		return EINVAL;
	}

	while (count > 0) {
		page_t **ppl;

		ppl = &Delmem;

		pp = page_get(PAGESIZE, P_NOSLEEP|P_NORESOURCELIM);
		if (pp == NULL) 
			return EINVAL;
		page_add(ppl, pp);

		count--;
		Delmem_cnt++;
		availrmem--;
		availsmem--;
	}

	return(0);
}

STATIC int
page_addmem(count)
	register int count;
{
	register page_t	*pp;

	while (count > 0) {
		page_t **ppl;

		pp = *(ppl = &Delmem);
		if (pp == NULL) 
			return EINVAL;
		page_sub(ppl, pp);
		page_rele(pp);

		count--;
		Delmem_cnt--;
		availrmem++;
		availsmem++;
	}

	return(0);
}

int
page_deladd(add, count, rvp)
	register int count;
	rval_t *rvp;
{
	register int error;

	if (add)
		error = page_addmem(count);
	else
		error = page_delmem(count);
	if (error == 0) {
		if (add)
			rvp->r_val1 = freemem;
		else
			rvp->r_val1 = Delmem_cnt;
	}
	return(error);
}

/*
 * Debugging routines only!
 */

#ifdef DEBUG

void
page_print(pp)
	register page_t *pp;
{
	register struct vnode *vp;

        cmn_err(CE_CONT, "^mapping 0x%x nio %d keepcnt %d lck %d cow %d",
                pp->p_hat.mappings, pp->p_nio, pp->p_keepcnt,
                pp->p_lckcnt, pp->p_cowcnt);
	cmn_err(CE_CONT, "^%s%s%s%s%s%s%s%s%s\n", 
		(pp->p_lock)    ? " LOCK"    : "" ,
		(pp->p_want)    ? " WANT"    : "" ,
		(pp->p_free)    ? " FREE"    : "" ,
		(pp->p_intrans) ? " INTRANS" : "" ,
		(pp->p_gone)    ? " GONE"    : "" ,
		(pp->p_mod)     ? " MOD"     : "" ,
		(pp->p_ref)     ? " REF"     : "" ,
		(pp->p_pagein)  ? " PAGEIN"  : "" ,
		(pp->p_age)	? " AGE"  : "" );
        cmn_err(CE_CONT, "^vnode 0x%x, offset 0x%x",
                pp->p_vnode, pp->p_offset);
	if (swap_anon(pp->p_vnode, pp->p_offset))
		cmn_err(CE_CONT, "^  (ANON)");
	else if ((vp = pp->p_vnode) != 0
		 && ((char *)vp < (char *)stext
		 || (char *)vp >= (char *)sdata)) {
		cmn_err(CE_CONT, "^  v_flag 0x%x, v_count %d, v_type %d",
			vp->v_flag, vp->v_count, vp->v_type);
	}
	cmn_err(CE_CONT, "^\nnext 0x%x, prev 0x%x, vpnext 0x%x vpprev 0x%x\n",
	    pp->p_next, pp->p_prev, pp->p_vpnext, pp->p_vpprev);
}

void
phystopp(v)
{
	int pfn;
	page_t *pp;

	pfn = v >> PAGESHIFT;
	cmn_err(CE_CONT, "^pfn=0x%x, ", pfn);
	pp = page_numtopp(pfn);
	if (pp)
		cmn_err(CE_CONT, "^pp=0x%x\n", pp);
	else
		cmn_err(CE_CONT, "^pp=NULL\n");
}

void
pptophys(pp)
	page_t *pp;
{
	int pfn;

	pfn = page_pptonum(pp);
	cmn_err(CE_CONT, "^pfn=0x%x, ", pfn);
	cmn_err(CE_CONT, "^phys=0x%x\n", ctob(pfn));
}

xpage_find(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int len = 0;

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off) {
			if (pp->p_gone == 0)
				return 0;
		}
	return 1;
}

void
findpage(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int len = 0;
	register int found = 0;

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off) {
			if (found++)
				cmn_err(CE_CONT, "^\t\t\t\t\t\t      ");
			cmn_err(CE_CONT, "^%x %s%s%s%s%s%s%s%s%s %d %d %d\n", 
				pp,
				(pp->p_lock)    ? "L"    : " " ,
				(pp->p_want)    ? "W"    : " " ,
				(pp->p_free)    ? "F"    : " " ,
				(pp->p_intrans) ? "I" : " " ,
				(pp->p_gone)    ? "G"    : " " ,
				(pp->p_mod)     ? "M"     : " " ,
				(pp->p_ref)     ? "R"     : " " ,
				(pp->p_pagein)  ? "P"  : " " ,
				(pp->p_age)	? "A"  : "" ,
				pp->p_keepcnt, pp->p_lckcnt, pp->p_cowcnt);
		}
	if (found == 0)
		cmn_err(CE_CONT, "^not found\n");
}

void
print_pagepool()
{
	register struct pp_chunk *chp;

	cmn_err(CE_CONT, "^\nPagepool Chunk Table:\n");
	for (chp = pp_first; chp != NULL; chp = chp->pp_next) {
		cmn_err(CE_CONT, "^  (%d)  PFN %x-%x  PP %x-%x\n",
			chp - pagepool,
			chp->pp_pfn, chp->pp_epfn - 1,
			chp->pp_page, chp->pp_epage - 1);
	}
	cmn_err(CE_CONT, "^\n");
}

#endif	/* DEBUG */
