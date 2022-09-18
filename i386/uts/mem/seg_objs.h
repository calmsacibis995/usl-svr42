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

#ifndef _MEM_SEG_OBJS_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_OBJS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:mem/seg_objs.h	1.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Structure who's pointer is passed to the segvn_create routine
 */
struct segobjs_crargs {
	int	(*mapfunc)();	/* map function to call */
	u_int	offset;		/* starting offset */
	caddr_t	arg;
	u_char	prot;		/* protection */
	u_char	maxprot;	/* maximum protection */
};

/*
 * (Semi) private data maintained by the seg_objs driver per segment mapping
 */
struct	segobjs_data {
	int	(*mapfunc)();	/* really returns struct pte, not int */
	u_int	offset;		/* device offset for start of mapping */
	caddr_t	arg;		/* 1st argument to (*mapfunc)() */
	u_char	pageprot;	/* true if per page protections present */
	u_char	prot;		/* current segment prot if pageprot == 0 */
	u_char	maxprot;	/* maximum segment protections */
	struct	vpage *vpage;	/* per-page information, if needed */
};

#ifdef _KERNEL

#if defined(__STDC__)
extern int segobjs_create(struct seg *, void *);
#else
extern int segobjs_create();
#endif

#endif /* _KERNEL */

#endif	/* _MEM_SEG_OBJS_H */
