/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_LOCK_H	/* wrapper symbol for kernel use */
#define _PROC_LOCK_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/lock.h	1.2"
#ident	"$Header: $"
/*
 * flags for locking procs and texts
 */
#define	UNLOCK	 0
#define	PROCLOCK 1
#define	TXTLOCK	 2
#define	DATLOCK	 4

#ifdef _KERNEL

#define	MEMLOCK	 8

#if defined(__STDC__)
int punlock(void);
#else
int punlock();
#endif	/* __STDC__ */

#else

#if defined(__STDC__)
int proclock(int);
#else
int proclock();
#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _PROC_LOCK_H */
