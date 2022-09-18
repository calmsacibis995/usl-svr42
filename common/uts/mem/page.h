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

#ifndef _MEM_PAGE_H	/* wrapper symbol for kernel use */
#define _MEM_PAGE_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/page.h	1.1.4.10"
#ident	"$Header: $"

/*
 * VM - RAM pages.
*/

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h>		/* REQUIRED */
#endif

#ifndef _MEM_VM_HAT_H
#include <mem/vm_hat.h>		/* REQUIRED */
#endif

#ifndef _MEM_VMPARAM_H
#include <mem/vmparam.h>	/* REQUIRED */
#endif

#ifndef _UTIL_MP_H
#include <util/mp.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <vm/vm_hat.h>		/* REQUIRED */
#include <sys/vmparam.h>	/* REQUIRED */
#include <vm/mp.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Each physical page has a page structure, which is used to maintain
 * these pages as a cache.  A page can be found via a hashed lookup
 * based on the [vp, offset].  If a page has an [vp, offset] identity,
 * then it is entered on a doubly linked circular list off the
 * vnode using the p_vpnext/p_vpprev pointers.  If the p_free bit
 * is on, then the page is also on a doubly linked circular free
 * list using p_next/p_prev pointers.  If the p_intrans bit is on,
 * then the page is currently being read in or written back.
 * In this case, the p_next/p_prev pointers are used to link the
 * pages together for a consecutive IO request.  If the page
 * is in transit and the page is coming in (pagein), then you
 * must wait for the IO to complete before you can attach to the page.
 * 
 * phat and phat2 are the hat-specific parts
 * of a page structure, defined in vm_hat.h.
 *
 * phat is for those fields that cannot be overloaded; i.e., they must be
 * valid for all pages.
 *
 * phat2 contains the hat-specific parts of the page structure that can
 * be overloaded with p_dblist[] (a list of disk addresses for the page).
 * This may be the case for (e.g.) special pages such as page tables.
 */
typedef struct page {
	union pg_u {
		struct {
			uint	P_lock : 1;	/* locked */
			uint	P_want : 1;	/* wanted */
			uint	P_free : 1;	/* on free list */
			uint	P_intrans : 1;	/* data in transit */
			uint	P_gone : 1;	/* page has been released */
			uint	P_mod : 1;	/* software copy of mod bit */
			uint	P_ref : 1;	/* software copy of ref bit */
			uint	P_pagein : 1;	/* being paged in */
			uint	P_nc : 1;	/* do not cache page */
			uint	P_age : 1;	/* on page_freelist */
			uint	P_dma : 1;	/* page is DMAable
						 * (only used if RESTRICTED_DMA
						 * and rdma_enabled) */
			uint	P_nio : 5;	/* # of I/O reqs needed */
			ushort	P_keepcnt;	/* # of page "keeps" */
		} P_fields;
		struct {
			uint	P_flags : 16;
			ushort	P_keepcnt;
		} P_bits;
		uint	P_firstw;
	} pgbits;

	struct	vnode	*p_vnode;	/* logical vnode this page is from */
	uint		p_offset;	/* offset into vnode for this page */
	struct page	*p_hash;	/* hash by [vnode, offset] */
	struct page	*p_next;	/* next page in free/intrans lists */
	struct page	*p_prev;	/* prev page in free/intrans lists */
	struct page	*p_vpnext;	/* next page in vnode list */
	struct page	*p_vpprev;	/* prev page in vnode list */
	struct phat	p_hat;		/* hat-specific data */

	union {
		struct {
			ushort	P_lckcnt;	/* # of locks on page data */
			ushort	P_cowcnt;	/* # of copy on write locks */
		} P_lckcow;
		uint	P_counts;
	} pgcounts;

	union {
	  daddr_t P_dblist[PAGESIZE/NBPSCTR];	/* disk storage for the page */
	  struct phat2  P_hat2;			/* more hat-specific data */
	} pgdb;

	struct proc	*p_uown;	/* process owning it as u-page */
	clock_t		p_timestmp;	/* dreg, remove at next opportunity */
	lid_t		p_lid;		/* process level faulting page */
} page_t;

#define p_lock		pgbits.P_fields.P_lock
#define p_want		pgbits.P_fields.P_want
#define p_free		pgbits.P_fields.P_free
#define p_intrans	pgbits.P_fields.P_intrans
#define p_gone		pgbits.P_fields.P_gone
#define p_mod		pgbits.P_fields.P_mod
#define p_ref		pgbits.P_fields.P_ref
#define p_pagein	pgbits.P_fields.P_pagein
#define p_nc		pgbits.P_fields.P_nc
#define p_age		pgbits.P_fields.P_age
#define p_dma		pgbits.P_fields.P_dma
#define p_nio		pgbits.P_fields.P_nio
#define p_keepcnt	pgbits.P_fields.P_keepcnt
#define p_counts	pgcounts.P_counts
#define p_lckcnt	pgcounts.P_lckcow.P_lckcnt
#define p_cowcnt	pgcounts.P_lckcow.P_cowcnt
#define p_firstw	pgbits.P_firstw
#define p_dblist	pgdb.P_dblist
#define p_hat2		pgdb.P_hat2

/*
 * PAGE_HAS_MAPPINGS macro defined in vm_hat.h is used to tell if the page has
 * any mappings to the page.
 */

#ifdef _KERNEL

#define PAGE_HOLD(pp)	(pp)->p_keepcnt++
#define PAGE_RELE(pp)	page_rele(pp)

/*
 * Macro for building the mask against which the first word
 * in page structure is compared, to evaluate quickly whether
 * (pp->p_free||pp->p_lock||pp->p_intrans||pp->p_keepcnt > 0)
 * is true.
 * We do this by turning off the bits that are not of interest.
 */
#define build_mask(x) {							\
	(x) = ~0;							\
	((union pg_u *)(&x))->P_fields.P_want = 0;			\
	((union pg_u *)(&x))->P_fields.P_gone = 0;			\
	((union pg_u *)(&x))->P_fields.P_mod = 0;			\
	((union pg_u *)(&x))->P_fields.P_ref = 0;			\
	((union pg_u *)(&x))->P_fields.P_pagein = 0;			\
	((union pg_u *)(&x))->P_fields.P_nc = 0;			\
	((union pg_u *)(&x))->P_fields.P_age = 0;			\
	((union pg_u *)(&x))->P_fields.P_nio = 0;			\
	((union pg_u *)(&x))->P_fields.P_dma = 0;			\
}

/*
 * page_get() request flags.
 */
#define P_NOSLEEP	0x0000		/* don't sleep in allocation */
#define P_CANWAIT	0x0001		/* OK to sleep if necessary */
#define P_PHYSCONTIG	0x0002		/* need physically contiguous pages */
#define P_DMA		0x0004		/* need DMA-able pages */
					/* (ignored if not RESTRICTED_DMA) */
#define P_NORESOURCELIM	0x0008		/* ignore usual resource limit */

#endif	/* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Page types.  If RESTRICTED_DMA support is enabled, pages will be divided
 * into standard (STD_PAGE) and DMAable (DMA_PAGE) pages; in this case,
 * standard pages are non-DMAable.  Otherwise all pages are standard pages,
 * and are all DMAable (if DMA is supported at all on the platform).
 */

#define STD_PAGE	0
#if RESTRICTED_DMA
#define DMA_PAGE	1
#define NPAGETYPE	2
#else
#define NPAGETYPE	1
#endif

/*
 * Page accounting structures that allow for a non-contiguous memory layout.
 * For contiguous memory architectures, set MAX_MEM_CHUNK to 1 (see vmparam.h).
 */
struct pp_chunk {		/* page pool accounting structure */
	uint	pp_pfn;		/* page frame number of first page in chunk */
	uint	pp_epfn;	/* pfn of first page after chunk */
	struct page *pp_page;	/* pointer to first page structure */
	struct page *pp_epage;	/* pp_page + (pp_epfn - pp_pfn) */
	struct pp_chunk *pp_next; /* next page pool chunk in search order */
};

#endif	/* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern struct pp_chunk pagepool[];

/* page structure address for 1st page pool page (of a type) in the system */
extern page_t	*mem_pages[NPAGETYPE];
#define pages	(mem_pages[STD_PAGE])	/* == pagepool[0].pp_page */

/* one past last page pool page */
extern page_t	*mem_epages[NPAGETYPE];
#define epages	(mem_epages[STD_PAGE])	/* == pagepool[n].pp_epage */

extern uint_t mem_freemem[NPAGETYPE];	/* # free pages per type; however,
					 * mem_freemem[STD_PAGE] is special:
					 * it include all pages, not just
					 * STD_PAGE pages.
					 */
#define freemem	(mem_freemem[STD_PAGE])

/*
 * Variables controlling locking of physical memory.
 */
extern uint	pages_pp_locked;	/* physical pages actually locked */
extern uint	pages_pp_kernel;	/* physical page locks by kernel */
extern uint	pages_pp_maximum;	/* tuning: lock + claim <= max */

/*
 * Page frame operations.
 */
extern int	 page_addclaim();
extern int	 page_enter();
extern int	 page_pp_lock();
extern page_t	*page_exists();
extern page_t	*page_find();
extern page_t	*page_get();
extern page_t	*page_get_aligned();
extern page_t	*page_lookup();
extern page_t	*page_numtookpp();
extern void	 page_abort();
extern void	 page_free();
extern void	 page_hashin();
extern void	 page_hashout();
extern void	 page_init();
extern void	 page_init_chunk();
extern void	 page_pp_unlock();
extern void	 page_pp_useclaim();
extern void	 page_reclaim();
extern void	 page_sortadd();
extern void	 page_sub();
extern void	 page_subclaim();
extern void	 page_wait();

#if defined(DEBUG) || MAX_MEM_CHUNK > 1

extern uint	 page_pptonum();
extern page_t	*page_numtopp();

#else

#define page_pptonum(pp)						\
	(((uint)((pp) - pages) *					\
		(PAGESIZE/MMU_PAGESIZE)) + pagepool[0].pp_pfn)

#define page_numtopp(pfnum)						\
	(((pfnum) < pagepool[0].pp_pfn					\
		|| (pfnum) >= pagepool[0].pp_epfn) ?			\
		(page_t *)NULL :					\
		&pages[(uint)((pfnum) - pagepool[0].pp_pfn) /		\
			(PAGESIZE/MMU_PAGESIZE)])

#endif	/* DEBUG || MAX_MEM_CHUNK > 1 */

#ifdef DEBUG

extern void	 page_rele();
extern void	 page_lock();
extern void	 page_unlock();

#else

extern mon_t page_mplock;	/* lock for manipulating page links */

#define page_rele(pp) {							\
	mon_enter(&page_mplock);					\
	if (--((struct page *)(pp))->p_keepcnt == 0) {			\
		while (((struct page *)(pp))->p_want) {			\
			((struct page *)(pp))->p_want = 0;		\
			cv_broadcast(&page_mplock, (char *)(pp));	\
		}							\
	}								\
	mon_exit(&page_mplock);						\
	if (((struct page *)(pp))->p_keepcnt == 0			\
		&& (((struct page *)(pp))->p_gone			\
		|| ((struct page *)(pp))->p_vnode == NULL))		\
		page_abort(pp);						\
}

#define page_lock(pp) {							\
	mon_enter(&page_mplock);					\
	while (pp->p_lock)						\
		page_cv_wait(pp);					\
	pp->p_lock = 1;							\
	mon_exit(&page_mplock);						\
}

#define page_unlock(pp) {						\
	mon_enter(&page_mplock);					\
	((struct page *)(pp))->p_lock = 0;				\
	while (((struct page *)(pp))->p_want) {				\
		((struct page *)(pp))->p_want = 0;			\
		cv_broadcast(&page_mplock, (char *)(pp));		\
	}								\
	mon_exit(&page_mplock);						\
}

#endif	/* DEBUG */

/*
 * Page hash table is a power-of-two in size, externally chained
 * through the hash field.  PAGE_HASHAVELEN is the average length
 * desired for this chain, from which the size of the page_hash
 * table is derived at boot time and stored in the kernel variable
 * page_hashsz.  PAGE_HASHVPSHIFT is defined so that
 * 1 << PAGE_HASHVPSHIFT is the approximate size of a vnode struct.
 */

extern page_t	**page_hash;	/* page hash table */
extern int	page_hashsz;	/* size of page hash table */

#define PAGE_HASHAVELEN		4
#define PAGE_HASHVPSHIFT	6

#define PAGE_HASHFUNC(vp, off)						\
	((((off) >> PAGESHIFT) + ((int)(vp) >> PAGE_HASHVPSHIFT)) &	\
		(page_hashsz - 1))

#ifdef __STDC__

extern void page_cv_wait(page_t *);
extern void ppcopy(page_t *, page_t *);
extern void ppcopyrange(page_t *, page_t *, uint_t, uint_t);

#else

extern void page_cv_wait();
extern void ppcopy();
extern void ppcopyrange();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _MEM_PAGE_H */
