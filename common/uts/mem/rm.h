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

#ifndef _MEM_RM_H	/* wrapper symbol for kernel use */
#define _MEM_RM_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/rm.h	1.1.2.3"
#ident	"$Header: $"

/*
 * VM - Resource Management.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef DEBUG

extern struct page *	rm_allocpage();
extern struct page *	rm_allocpage_aligned();
extern size_t		rm_asrss();

#else

#define rm_allocpage(seg, addr, len, flags) \
	(struct page *) (page_get((uint)(len), (uint)(flags)))

#define rm_allocpage_aligned(seg, addr, len, mask, val, flags) \
	(struct page *) (page_get_aligned(len, (u_int)(mask), (u_int)(val), \
					  (u_int)(flags)))

/* yield the memory claim requirement for an address space */
#define rm_asrss(as)	((as) == NULL? 0 : (as)->a_rss)

#endif /* DEBUG */

extern void		rm_outofanon();
extern void		rm_outofhat();
extern size_t		rm_assize();

#endif /* _MEM_RM_H */
