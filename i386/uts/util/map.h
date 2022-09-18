/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MAP_H	/* wrapper symbol for kernel use */
#define _UTIL_MAP_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/map.h	1.4"
#ident	"$Header: $"

/*
 *	struct map	X[]	.m_size			.m_addr
 *			---	------------		-----------
 *			[0]	mapsize(X)		mapwant(X)
 *				num. of X[] unused	sleep value
 *
 *	  mapstart(X)->	[1]	num. of units		unit number
 *			 :	    :		  	:
 *			[ ]	    0
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

struct map
{
	unsigned long m_size;	/* number of units available */
	unsigned long m_addr;	/* address of first available unit */
};

extern struct map sptmap[];	/* map for system virtual space */

#define	mapstart(X)	&X[1]		/* start of map array */

#define	mapwant(X)	X[0].m_addr	/* processes waiting on this map */

#define	mapsize(X)	X[0].m_size	/* number of empty slots left in map */

#define	mapdata(X) {(X)-2, 0} , {0, 0}

#define	mapinit(X, Y)	X[0].m_size = (Y)-2 /* First and last entries are used
					     * differently. See above. */


#if defined(__STDC__)
extern void rmfree(struct map *, size_t, u_long);
extern void mfree(struct map *, size_t, u_long);
extern u_long rmalloc(struct map *, size_t);
extern u_long malloc(struct map *, size_t);
#else
extern void rmfree();
extern void mfree();
extern u_long malloc();
extern u_long rmalloc();
#endif

#endif	/* _UTIL_MAP_H */
