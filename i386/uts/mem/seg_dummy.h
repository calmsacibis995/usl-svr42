/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_DUMMY_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_DUMMY_H	/* subject to change without notice */

#ident	"@(#)uts-x86:mem/seg_dummy.h	1.2"
#ident	"$Header: $"

#ifdef _KERNEL

#if defined(__STDC__)
struct seg;
extern int segdummy_create(struct seg *, void *);
#else
extern int segdummy_create();
#endif

#endif /* _KERNEL */

#endif	/* _MEM_SEG_DUMMY_H */
