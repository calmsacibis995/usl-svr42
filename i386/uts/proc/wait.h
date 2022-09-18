/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_WAIT_H	/* wrapper symbol for kernel use */
#define _PROC_WAIT_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/wait.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _PROC_SIGINFO_H
#include <proc/siginfo.h> /* SVR4.0COMPAT */
#endif

#if !defined(_POSIX_SOURCE) && !defined(_PROC_PROCSET_H)
#include <proc/procset.h> /* SVR4.0COMPAT */
#endif /* !defined(_POSIX_SOURCE) && !defined(_PROC_PROCSET_H) */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/siginfo.h>	/* SVR4.0COMPAT */

#if !defined(_POSIX_SOURCE) 
#include <sys/procset.h>	/* SVR4.0COMPAT */
#endif /* !defined(_POSIX_SOURCE) */

#endif /* _KERNEL_HEADERS */


/*
 * arguments to wait functions
 */

#if !defined(_POSIX_SOURCE) 
#define WEXITED		0001	/* wait for processes that have exited	*/
#define WTRAPPED	0002	/* wait for processes stopped while tracing */
#define WSTOPPED	0004	/* wait for processes stopped by signals */
#define WCONTINUED	0010	/* wait for processes continued */
#endif /* !defined(_POSIX_SOURCE) */

#define WUNTRACED	0004	/* for POSIX */

#define WNOHANG		0100	/* non blocking form of wait	*/

#if !defined(_POSIX_SOURCE) 
#define WNOWAIT		0200	/* non destructive form of wait */

#define WOPTMASK	(WEXITED|WTRAPPED|WSTOPPED|WCONTINUED|WNOHANG|WNOWAIT)

/*
 * macros for stat return from wait functions
 */

#define WCONTFLG		0177777
#define WCOREFLG		0200

#define WWORD(stat)		((int)((stat))&0177777)
#endif /* !defined(_POSIX_SOURCE) */

#if !defined(_POSIX_SOURCE) 
#define WSTOPFLG		0177
#define WSIGMASK		0177
#define WLOBYTE(stat)		((int)((stat)&0377))
#define WHIBYTE(stat)		((int)(((stat)>>8)&0377))
#endif /* !defined(_POSIX_SOURCE) */ 

#define WIFEXITED(stat)		(((int)((stat)&0377))==0)
#define WIFSIGNALED(stat)	(((int)((stat)&0377))>0&&((int)(((stat)>>8)&0377))==0)
#define WIFSTOPPED(stat)	(((int)((stat)&0377))==0177&&((int)(((stat)>>8)&0377))!=0)

#if !defined(_POSIX_SOURCE) 
#define WIFCONTINUED(stat)	(WWORD(stat)==WCONTFLG)
#endif /* !defined(_POSIX_SOURCE) */

#define WEXITSTATUS(stat)	((int)(((stat)>>8)&0377))
#define WTERMSIG(stat)		(((int)((stat)&0377))&0177)
#define WSTOPSIG(stat)		((int)(((stat)>>8)&0377))

#if !defined(_POSIX_SOURCE) 
#define WCOREDUMP(stat)		((stat)&WCOREFLG)
#endif /* !defined(_POSIX_SOURCE) */



#if !defined(_KERNEL)
#if defined(__STDC__)

extern pid_t wait(int *);
extern pid_t waitpid(pid_t, int *, int);

#if !defined(_POSIX_SOURCE) 
extern int waitid(idtype_t, id_t, siginfo_t *, int);
#endif /* !defined(_POSIX_SOURCE) */

#else

extern pid_t wait();
extern pid_t waitpid();

#if !defined(_POSIX_SOURCE) 
extern int waitid();
#endif /* !defined(_POSIX_SOURCE) */

#endif	/* __STDC__ */
#endif	/* _KERNEL */

#endif	/* _PROC_WAIT_H */
