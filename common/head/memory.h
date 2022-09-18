/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEMORY_H
#define _MEMORY_H

#ident	"@(#)sgs-head:memory.h	1.4.3.1"

/* from stddef.h */
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned        size_t;
#endif

#if defined(__STDC__)
extern void *memccpy(void *, const void *, int, size_t);
extern void *memchr(const void *, int, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
extern int memcmp(const void *, const void *, size_t);

#else
extern char
	*memccpy(),
	*memchr(),
	*memcpy(),
	*memset();
extern int memcmp();
#endif

#endif /* _MEMORY_H */
