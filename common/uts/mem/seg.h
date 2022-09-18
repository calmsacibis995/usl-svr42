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

#ifndef _MEM_SEG_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/seg.h	1.1.2.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _UTIL_MP_H
#include <util/mp.h>		/* REQUIRED */
#endif

#ifndef _MEM_FAULTCODE_H
#include <mem/faultcode.h>	/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <vm/mp.h>		/* REQUIRED */
#include <vm/faultcode.h>	/* SVR4.0COMPAT */

#else

#include <vm/mp.h>		/* SVR4.0COMPAT */
#include <vm/faultcode.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * VM - Segments.
 *
 * An address space contains a set of segments, managed by drivers.
 * Drivers support mapped devices, sharing, copy-on-write, etc.
 *
 * After a segment is created, faults may occur on pages of the segment.
 * When a fault occurs, the fault handling code must get the desired
 * object and set up the hardware translation to the object.  For some
 * objects, the fault handling code also implements copy-on-write.
 *
 * When the hardware address translation (hat) layer wants to unload
 * a translation, it can call the unload routine which is responsible
 * for processing reference and modify bits.  (The unload facility is
 * currently unused in this implementation.)
 */

/*
 * Fault information passed to the seg fault handling routine.
 * F_SOFTLOCK and F_SOFTUNLOCK are used by software
 * to lock and unlock pages for physical I/O.
 */
enum fault_type {
	F_INVAL,		/* invalid page */
	F_PROT,			/* protection fault */
	F_SOFTLOCK,		/* software requested locking */
	F_SOFTUNLOCK		/* software requested unlocking */
};

/*
 * seg_rw gives the access type for a fault operation.
 */
enum seg_rw {
	S_OTHER,		/* unknown or not touched */
	S_READ,			/* read access attempted */
	S_WRITE,		/* write access attempted */
	S_EXEC 			/* execution access attempted */
};

/*
 * Segment operations.
 * Each segment driver defines a seg_ops vector, pointed to by
 * the seg structures of segments of its particular type.
 * Not all of the generic segment operations make sense for
 * every type of segment, so a driver may supply stubs for some
 * operations.
 */
#if defined(__STDC__)

struct seg;
struct vnode;

struct seg_ops {
	int (*dup)(struct seg *, struct seg *);
	int (*unmap)(struct seg *, addr_t, u_int);
	void (*free)(struct seg *);
	faultcode_t (*fault)
		(struct seg *, addr_t, u_int, enum fault_type, enum seg_rw);
	faultcode_t (*faulta)(struct seg *, addr_t);
	void (*unload)(struct seg *, addr_t, u_int, u_int);
	int (*setprot)(struct seg *, addr_t, u_int, u_int);
	int (*checkprot)(struct seg *, addr_t, u_int, u_int);
	int (*kluster)(struct seg *, addr_t, int);
	u_int (*swapout)(struct seg *);
	int (*sync)(struct seg *, addr_t, u_int, int, u_int);
	int (*incore)(struct seg *, addr_t, u_int, char *);
	int (*lockop)(struct seg *, addr_t, u_int, int, int, ulong *, u_int);
	int (*getprot)(struct seg *, addr_t, u_int, u_int *);
	off_t (*getoffset)(struct seg *, addr_t);
	int (*gettype)(struct seg *, addr_t);
	int (*getvp)(struct seg *, addr_t, struct vnode **);		
};

#else

struct seg_ops {
	int		(*dup)();	/* duplicate segment */
	int		(*unmap)();	/* unmap a range of addresses */
	void		(*free)();	/* free segment */
	faultcode_t	(*fault)();	/* handle faults */
	faultcode_t	(*faulta)();	/* read in pages asynchronously */
	void		(*unload)();	/* called when unloading translation */
	int		(*setprot)();	/* set specified access protection */
	int		(*checkprot)();	/* check access protection */
	int		(*kluster)();	/* see if kluster op makes sense */
	u_int		(*swapout)();	/* swap pages to secondary storage */
	int		(*sync)();	/* sync pages with mapped object */
	int		(*incore)();	/* see if pages are in memory */
	int		(*lockop)();	/* lock/unlock mapped pages */
	int		(*getprot)();	/* get vector of page protections */
	off_t		(*getoffset)();	/* offset into mapped object */
	int		(*gettype)();	/* sharing type (MAP_SHARED/PRIVATE) */
	int		(*getvp)();	/* associated vnode pointer */
};

#endif	/* __STDC__ */

/*
 * The seg structure contains a lock to prevent races, the base virtual
 * address and size of the segment, a back pointer to the containing
 * address space, pointers to maintain a circularly doubly linked list
 * of segments in the same address space, and procedure and data hooks
 * for the driver.  The seg list of an address space is sorted by
 * ascending base address and overlapping segments are not allowed.
 */
struct seg {
	mon_t	s_lock;			/* lock, currently unused */
	addr_t	s_base;			/* base virtual address */
	u_int	s_size;			/* size in bytes */
	struct	as *s_as;		/* containing address space */
	struct	seg *s_next;		/* next seg in this address space */
	struct	seg *s_prev;		/* prev seg in this address space */
	struct	seg_ops *s_ops;		/* operations vector */
	_VOID	*s_data;		/* private data for instance */
};

#ifdef _KERNEL

/*
 * Generic segment operations.
 */

#ifdef DEBUG

u_int	seg_page();
u_int	seg_pages();

#else

/* translate addr into page number within segment */
#define seg_page(seg, addr) \
 	((u_int)(((addr) - (seg)->s_base) >> PAGESHIFT))

/* return number of pages in segment */
#define seg_pages(seg) \
	((u_int)(((seg)->s_size + PAGEOFFSET) >> PAGESHIFT))

#endif	/* DEBUG */

#if defined(__STDC__)

extern struct seg *seg_alloc(struct as *, addr_t, u_int);
extern int	seg_attach(struct as *, addr_t, u_int, struct seg *);
extern void	seg_free(struct seg *);
extern void	seg_unmap(struct seg *);

#else

extern struct seg *seg_alloc();
extern int	seg_attach();
extern void	seg_free();
extern void	seg_unmap();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _MEM_SEG_H */
