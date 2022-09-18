/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FLOCK_H	/* wrapper symbol for kernel use */
#define _FS_FLOCK_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/flock.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _FS_FCNTL_H
#include <fs/fcntl.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/fcntl.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	INOFLCK		1	/* vnode is locked when reclock() is called */
#define	SETFLCK		2	/* set a file lock */
#define	SLPFLCK		4	/* wait if blocked */
#define	RCMDLCK		8	/* RGETLK/RSETLK/RSETLKW specified */

#define IGN_PID		(-1)	/* ignore epid when cleaning locks */

/* file locking structure (connected to vnode) */

#define l_end 		l_len
#define MAXEND  	017777777777

typedef struct filock {
	struct	flock set;	/* contains type, start, and end */
	union	{
		int wakeflg;	/* for locks sleeping on this one */
		struct {
			long sysid;
			pid_t pid;
		} blk;			/* for sleeping locks only */
	}	stat;
	struct	filock *prev;
	struct	filock *next;
} filock_t;

/*
 * File and record locking configuration structure;
 * record use total may overflow.
 */
struct flckinfo {
	long reccnt;	/* number of records currently in use */
	long rectot;	/* number of records used since system boot */
};

#if defined(__STDC__)

struct vnode;	/* to eliminate warning from function prototype */
int reclock(struct vnode *, struct flock *, int, int, off_t);

#else

int reclock();

#endif

#endif	/* _FS_FLOCK_H */
