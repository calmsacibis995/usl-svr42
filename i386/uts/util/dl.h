/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_DL_H	/* wrapper symbol for kernel use */
#define _UTIL_DL_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/dl.h	1.5"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

/* double long structure */
typedef	struct dl {
	ulong	dl_lop;
	long	dl_hop;
} dl_t;

/* bits in double long */
#define DL_BITS	64 	

/* 
 * Functions that perform double long arithmetic.
 */
extern dl_t	ladd();
extern dl_t	lsub();
extern dl_t	lmul();
extern dl_t	ldivide();

/*
 * Logical shift function.
 * 
 * If the second argument is positive, shift to the left.
 * If it is negative, shift to the right.
 */
extern dl_t	lshiftl();	

#ifndef _KERNEL

extern dl_t	llog10();
extern dl_t	lexp10();

extern dl_t	lzero;
extern dl_t	lone;
extern dl_t	lten;

#endif /* _KERNEL */

#endif	/* _UTIL_DL_H */
