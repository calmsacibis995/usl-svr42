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

#ifndef _MEM_ANON_H	/* wrapper symbol for kernel use */
#define _MEM_ANON_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/anon.h	1.1.2.4"
#ident	"$Header: $"

/*
 * VM - Anonymous pages.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Each page that is anonymous, either in memory or in swap,
 * has an anon structure.  The structure's primary purpose is
 * to hold a reference count so that we can detect when the last
 * copy of a multiply-referenced copy-on-write page goes away.
 * When on the free list, un.an_next gives the next anon structure
 * in the list.  Otherwise, un.an_page is a "hint" that probably
 * points to the current page.  This must be explicitly checked
 * since the page can be moved underneath us.  This is simply
 * an optimization to avoid having to look up each page when
 * doing things like fork.
 */
struct anon {
	short	an_refcnt;		/* reference count */
	char	an_flag;		/* flags, see below */
#ifdef	DEBUG
	char	an_use;			/* for debugging code */
#endif
	union {
		struct	page *an_page;	/* "hint" to the real page */
		struct	anon *an_next;	/* free list pointer */
	} un;
};

/*
 * indirect anon structure
 */
struct ianon {
	struct	anon *an_bap;		/* pointer to anon structure */
};

/* an_flag values */
#define ALOCKED		0x1		/* someone has this anon locked */
#define AWANT		0x2		/* someone wants this anon */
#define	AINDIR		0x4		/* this anon has an ianon */

#ifdef DEBUG
/* an_use values */
#define AN_NONE		0
#define AN_DATA		1
#define AN_UPAGE	2
#endif

/*
 * The anoninfo structure is used to track
 * the availability of anon pages.
 */
struct anoninfo {
	uint	ani_max;		/* maximum anon pages available */
	uint	ani_free;		/* no. of anon pages currently free */
	uint	ani_resv;		/* no. of anon pages reserved */
};

#ifdef _KERNEL

extern struct anoninfo	anoninfo;	/* tracks availability of anon pages */

extern struct anon *	anon_alloc();
extern void		anon_dup();
extern void		anon_free();
extern int		anon_getpage();
extern struct page *	anon_private();
extern struct page *	anon_zero();
extern struct page *	anon_zero_aligned();
extern void		anon_unloadmap();
extern int		anon_resv();
extern void		anon_unresv();

#define ALOCK(ap) {					\
	while ((ap)->an_flag & ALOCKED) {		\
		(ap)->an_flag |= AWANT;			\
		(void) sleep((caddr_t)(ap), PINOD);	\
	}						\
	(ap)->an_flag |= ALOCKED;			\
}

#define AUNLOCK(ap) {					\
	ASSERT((ap)->an_flag & ALOCKED);		\
	(ap)->an_flag &= ~ALOCKED;			\
	if ((ap)->an_flag & AWANT) {			\
		(ap)->an_flag &= ~AWANT;		\
		wakeprocs((caddr_t)(ap), PRMPT);	\
	}						\
}

/* flags to anon_private() */
#define STEAL_PAGE 0x1			/* steal the page for copy-on-write */
#define LOCK_PAGE 0x2			/* transfer lock to new page */

#endif /* _KERNEL */

#endif	/* _MEM_ANON_H */
