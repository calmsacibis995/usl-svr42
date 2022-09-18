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

#ifndef _MEM_AS_H	/* wrapper symbol for kernel use */
#define _MEM_AS_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/as.h	1.1.3.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _MEM_FAULTCODE_H
#include <mem/faultcode.h>	/* SVR4.0COMPAT */
#endif

#ifndef _MEM_VM_HAT_H
#include <mem/vm_hat.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <vm/faultcode.h>	/* SVR4.0COMPAT */
#include <vm/vm_hat.h>		/* REQUIRED */

#else

#include <vm/vm_hat.h>		/* SVR4.0COMPAT */
#include <vm/faultcode.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */


/*
 * VM - Address spaces.
 */

/*
 * Each address space consists of a list of sorted segments
 * and machine dependent address translation information.
 *
 * All the hard work is in the segment drivers and the
 * hardware address translation code.
 */
struct as {
	uint	a_lock: 1;	/* someone has this as structure locked */
	uint	a_want: 1;	/* someone wants this as structure */
	uint	a_paglck: 1;	/* lock pages in memory */
	uint	: 13;		/* unused bits */
	ushort	a_keepcnt;	/* number of `keeps' */
	struct	seg *a_segs;	/* segments in this address space */
	struct	seg *a_seglast;	/* last segment hit on the address space */
	size_t	a_size;		/* size of address space */
	size_t	a_rss;		/* memory claim for this address space */
	struct	hat a_hat;	/* hardware address translation */
};

#ifdef _KERNEL

/*
 * Flags for as_gap().
 */
#define AH_DIR		0x1	/* direction flag mask */
#define AH_LO		0x0	/* find lowest hole */
#define AH_HI		0x1	/* find highest hole */
#define AH_CONTAIN	0x2	/* hole must contain `addr' */

extern struct seg *	as_segat();
extern struct as *	as_alloc();
extern void		as_free();
extern struct as *	as_dup();
extern int		as_addseg();
extern faultcode_t	as_fault();
extern faultcode_t	as_faulta();
extern int		as_setprot();
extern int		as_checkprot();
extern int		as_unmap();
extern int		as_map();
extern int		as_gap();
extern int		as_memory();
extern uint		as_swapout();
extern int		as_incore();
extern int		as_ctl();
extern uint		as_getprot();

#endif /* _KERNEL */

#endif	/* _MEM_AS_H */
