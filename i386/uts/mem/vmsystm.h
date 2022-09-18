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

#ifndef _MEM_VMSYSTM_H	/* wrapper symbol for kernel use */
#define _MEM_VMSYSTM_H	/* subject to change without notice */

#ident	"@(#)uts-x86:mem/vmsystm.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL

/*
 * Miscellaneous virtual memory subsystem variables and routines.
 *
 * This is the location of last resort for declarations
 * for which no more appropriate home can be found.
 * The temptation to add things to this file (particularly
 * global variable declarations) should be resisted.
 */

extern struct buf *bclnlist;	/* async I/Os needing synchronous cleanup */

extern int	deficit;	/* estimate of needs of new swapped in procs */
extern int	nscan;		/* number of scans in last second */
extern int	desscan;	/* desired pages scanned per second */

/* writable copies of tunables */
extern int	maxpgio;	/* max paging i/o per sec before start swaps */
extern int	lotsfree;	/* max free before clock freezes */
extern int	minfree;	/* minimum free pages before swapping begins */
extern int	desfree;	/* # of pages to try to keep free via daemon */
extern int	minpagefree;	/* non-critical uses can't go below this */

#ifdef __STDC__

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _SVC_SYSTM_H
#include <svc/systm.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/systm.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

extern int valid_va_range(addr_t *, u_int *, u_int, int);
extern int useracc(caddr_t, uint, int);
extern int page_deladd(int, int, rval_t *);
extern void map_addr(addr_t *, u_int, off_t, int);

#else

extern int valid_va_range();
extern int useracc();
extern int page_deladd();
extern void map_addr();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _MEM_VMSYSTM_H */
