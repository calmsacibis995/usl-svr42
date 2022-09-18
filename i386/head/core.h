/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _CORE_H
#define _CORE_H

#ident	"@(#)sgs-head:core.h	1.9.4.2"

/* machine dependent stuff for core files */

#if defined(__STDC__)

#if #machine(i386)
#define TXTRNDSIZ 0x400000
#define stacktop(siz)	(0x80000000L - siz)
#define stackbas(siz) 	0x80000000L

#elif #machine(m68k) || #machine(m88k)
#define TXTRNDSIZ	512L
#define stacktop(siz)	(0x1000000L)
#define stackbas(siz)	(0x1000000L - siz)

#elif #machine(sparc)
/* The stack is not at a constant position on sparc machine */

#else
#define TXTRNDSIZ 2048L
#define stacktop(siz) (0xF00000 + siz)
#define stackbas(siz) 0xF00000
#endif

#else
#if M32
#define TXTRNDSIZ 2048L
#define stacktop(siz) (0xF00000 + siz)
#define stackbas(siz) 0xF00000
#endif

#if i386
#define TXTRNDSIZ 0x400000
#define stacktop(siz)	(0x80000000L - siz)
#define stackbas(siz) 	0x80000000L
#endif

#if m68k || m88k
#define TXTRNDSIZ	512L
#define stacktop(siz)	(0x1000000L)
#define stackbas(siz)	(0x1000000L - siz)
#endif

#endif /* __STDC__ */

#endif /* _CORE_H */
