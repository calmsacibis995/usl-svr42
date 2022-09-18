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

#ifndef _MEM_SEG_U_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_U_H	/* subject to change without notice */

#ident	"@(#)uts-x86:mem/seg_u.h	1.5"
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
 * VM - u-area segment management.
 *
 * This file contains definitions related to the u-area segment type.
 *
 * In its most general form, this segment type provides an interface
 * for managing stacks that are protected by red zones, with the size
 * of each stack independently specifiable.  The current implementation
 * is restricted in the following way.
 * 1)	It assumes that all stacks are the same size.  In particular,
 *	it assumes that the stacks it manages are actually traditional
 *	u-areas, each containing a stack at one end.
 *
 * The segment driver manages a contiguous chunk of virtual space,
 * carving it up into individual stack instances as required, and
 * associating physical storage, MMU mappings, and swap space with
 * each individual stack instance.
 *
 * As a matter of nomenclature, the individual allocation units are
 * referred to as "slots".
 */

/*
 * The number of pages covered by a single seg_u slot.
 *
 * This value is the number of (software) pages in the u-area
 * (including the stack in the u-area) plus an additional page
 * for a stack red zone.  If the seg_u implementation is ever
 * generalized to allow variable-size stack allocation, this
 * define will have to change.
 *
 * On the x86, ublocks are variable length.  Each process can have a maximum of
 * SEGU_PAGES (MAXUSIZE) pages allocated for it.  Since MAXUSIZE is fairly
 * large, this uses up a lot of kernel virtual address space, but as long as
 * no physical memory is wasted this is alright.
 *
 * The x86 implementation does not need an extra redzone page.
 */
#define SEGU_PAGES	MAXUSIZE

/*
 * Macros for converting between the base address of a slot
 * and the base address of its accessible portion.
 * In the absence of a redzone page, the conversions are 1-1.
 */
#define segu_stom(v)	(v)		/* slot to mapped address */
#define segu_mtos(v)	(v)		/* mapped address to slot */

/*
 * Private information per overall segu segment (as opposed
 * to per slot within segment).
 *
 * XXX:	We may wish to modify the free list to handle it as a queue
 *	instead of a stack; this possibly could reduce the frequency
 *	of cache flushes.  If so, we would need a list tail pointer
 *	as well as a list head pointer.
 */
struct segu_segdata {
	/*
	 * info needed:
	 *	- slot vacancy info
	 *	- a way of getting to state info for each slot
	 */
	struct segu_data *usd_slots;	/* array of structs, one per slot */
	struct segu_data *usd_free;	/* slot free list head */
};

/*
 * Private per-slot information.
 */
struct segu_data {
	struct segu_data *su_next;	/* free list link */
	struct anon *su_swaddr[SEGU_PAGES]; /* disk addr of u area when swapped */
	u_int su_flags;			/* state info: see below */
	struct proc *su_proc;		/* owner of the slot */
};

/*
 * Flag bits.
 *
 * When the SEGU_LOCKED bit is set, all the resources associated with the
 * corresponding slot are locked in place, so that referencing addresses
 * in the slot's range will not cause a fault.  Clients using this driver
 * to manage a u-area lock down the slot when the corresponding process
 * becomes runnable and unlock it when the process is swapped out.
 */
#define SEGU_ALLOCATED	0x01		/* slot is in use */
#define SEGU_LOCKED	0x02		/* slot's resources locked */

#ifdef	_KERNEL

extern struct seg	*segu;		/* kernel's u-area mapping seg */

/*
 * Public routine declarations not part of the seg_ops vector go here.
 */

#if defined(__STDC__)

extern int	segu_create(struct seg *, void *);
extern addr_t	segu_get(struct proc *, int);
extern void	segu_release(struct proc *);
extern int	segu_expand(u_int);
extern u_int	segu_shrink(u_int);

#else

extern int	segu_create();
extern addr_t	segu_get();
extern void	segu_release();
extern int	segu_expand();
extern u_int	segu_shrink();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _MEM_SEG_U_H */
