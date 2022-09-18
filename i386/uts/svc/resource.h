/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_RESOURCE_H	/* wrapper symbol for kernel use */
#define _SVC_RESOURCE_H	/* subject to change without notice */

#ident	"@(#)uts-x86:svc/resource.h	1.4"
#ident	"$Header: $"

/*
 * Process priority specifications
 */

#define	PRIO_PROCESS	0
#define	PRIO_PGRP	1
#define	PRIO_USER	2


/*
 * Resource limits
 */

#define	RLIMIT_CPU	0		/* cpu time in seconds */
#define	RLIMIT_FSIZE	1		/* maximum file size */
#define	RLIMIT_DATA	2		/* data size */
#define	RLIMIT_STACK	3		/* stack size */
#define	RLIMIT_CORE	4		/* core file size */
#define RLIMIT_NOFILE	5		/* file descriptors */
#define RLIMIT_VMEM	6		/* maximum mapped memory */
#define RLIMIT_AS	RLIMIT_VMEM

#define	RLIM_NLIMITS	7		/* number of resource limits */

#define	RLIM_INFINITY	0x7fffffff

typedef unsigned long rlim_t;

struct rlimit {
	rlim_t	rlim_cur;		/* current limit */
	rlim_t	rlim_max;		/* maximum value for rlim_cur */
};

#ifdef _KERNEL

extern struct rlimit rlimits[];

#if defined(__STDC__)

extern int rlimit(int, rlim_t, rlim_t);

#else

extern int rlimit();

#endif	/* __STDC__ */

#else  /* _KERNEL */

#if defined(__STDC__)

extern int getrlimit(int, struct rlimit *);
extern int setrlimit(int, const struct rlimit *);

#else

extern int getrlimit();
extern int setrlimit();
#endif  /* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _SVC_RESOURCE_H */
