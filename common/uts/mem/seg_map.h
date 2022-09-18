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

#ifndef _MEM_SEG_MAP_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_MAP_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/seg_map.h	1.1.2.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/param.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Argument structure for segmap_create().
 */
struct segmap_crargs {
	u_int	prot;	/* segment protections, e.g. (PROT_READ|PROT_WRITE) */
};

/*
 * The following computes the number of pages covered by an smap (see below).
 * Also, PGARRAYSIZE is computed, which is the length of an array of shorts
 * needed to hold enough bits to use one bit per page.
 */
#define PGPERSMAP	(MAXBSIZE / PAGESIZE)
#define NBPSMAP		(NBBY * 2 /* sizeof(u_short) */)
#define PGARRAYSIZE	((PGPERSMAP + NBPSMAP - 1) / NBPSMAP)

/*
 * Each smap struct represents a MAXBSIZE sized mapping to the
 * <sm_vp, sm_off> given in the structure.  The location of
 * the structure in the array gives the virtual address of the
 * mapping.
 */
struct smap {
	struct vnode	*sm_vp;		/* vnode pointer (if mapped) */
	u_int		sm_off;		/* file offset for mapping */
	u_short		sm_refcnt;	/* reference count for uses */
	u_short	sm_pgflag[PGARRAYSIZE];	/* per-page flags */
	/*
	 * These next 3 entries could be coded as
	 * u_shorts if we are tight on memory.
	 */
	struct smap	*sm_hash;	/* hash pointer */
	struct smap	*sm_next;	/* next pointer */
	struct smap	*sm_prev;	/* previous pointer */
	u_int	sm_pgowner;	/* owner (proc ptr) of uninitialized pages */
	u_short	sm_pgowncnt;	/* nesting count for getmaps while owned */
};

/*
 * Given a page number within an smap, compute the index within the
 * smap's sm_pgflag array and the bit offset within that array entry
 * of the uninitialized-page flag bit corresponding to the given page.
 */
#if PGARRAYSIZE > 1
#define SMPGIDX(pg)	((pg) / NBPSMAP)
#define SMPGOFF(pg)	((pg) % NBPSMAP)
#else
#define SMPGIDX(pg)	0
#define SMPGOFF(pg)	(pg)
#endif

/*
 * Macros to manipulate the uninitialized-page flag bit corresponding to
 * a given page number within a given smap.
 */
#define SM_PG_UNINIT(sm, pg)	  /* get uninitialized-page flag */ \
		((sm)->sm_pgflag[SMPGIDX(pg)] & (1 << SMPGOFF(pg)))

#define SM_SET_PG_UNINIT(sm, pg)  /* set uninitialized-page flag */ \
		((sm)->sm_pgflag[SMPGIDX(pg)] |= (1 << SMPGOFF(pg)))

#define SM_CLR_PG_UNINIT(sm, pg)  /* clear uninitialized-page flag */ \
		((sm)->sm_pgflag[SMPGIDX(pg)] &= ~(1 << SMPGOFF(pg)))

/*
 * (Semi) private data maintained by the segmap driver per SEGMENT mapping.
 */
struct segmap_data {
	struct smap    *smd_sm;		/* array of smap structures */
	struct smap    *smd_free;	/* free list head pointer */
	u_char		smd_prot;	/* protections for all smap's */
	u_char		smd_want;	/* smap want flag */
	u_int		smd_hashsz;	/* power-of-two hash table size */
	struct smap   **smd_hash;	/* pointer to hash table */
};

/*
 * These are flags used on release.  Some of these might get handled
 * by segment operations needed for msync (when we figure them out).
 * SM_ASYNC modifies SM_WRITE.  SM_DONTNEED modifies SM_FREE.
 * SM_FREE and SM_INVAL are mutually exclusive.
 */
#define	SM_WRITE	0x01		/* write back the pages upon release */
#define	SM_ASYNC	0x02		/* do the write asynchronously */
#define	SM_FREE		0x04		/* put pages back on free list */
#define	SM_INVAL	0x08		/* invalidate page (no caching) */
#define	SM_DONTNEED	0x10		/* less likely to be needed soon */

#ifdef _KERNEL

extern struct seg *segkmap;		/* the kernel generic mapping segment */

#if defined(__STDC__)

extern int	segmap_create(struct seg *, void *);
extern void	segmap_pagecreate(struct seg *, addr_t, u_int, int);
extern addr_t	segmap_getmap(struct seg *, struct vnode *, u_int);
extern int	segmap_release(struct seg *, addr_t, u_int);

#else

extern int	segmap_create();
extern void	segmap_pagecreate();
extern addr_t	segmap_getmap();
extern int	segmap_release();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _MEM_SEG_MAP_H */
