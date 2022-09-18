/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_UMEM_H
#define _SVC_UMEM_H

#ident	"@(#)uts-x86:svc/umem.h	1.1"
#ident	"$Header: $"

/* umem.h - User memory allocation */

/* Flags for umem_fee() to indicate where memory was allocated */
#define UMEM_NEWSEG	1	/* Allocated wherever the system picked */
#define UMEM_STACK	2	/* Allocated on user stack */

#if defined(__STDC__)

extern _VOID	*umem_alloc(size_t, int*);
extern void	umem_free(_VOID*, size_t, int);

#else

extern _VOID	*umem_alloc();
extern void	umem_free();

#endif

#endif	/* _SVC_UMEM_H */
