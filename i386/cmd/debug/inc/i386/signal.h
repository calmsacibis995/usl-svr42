/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SIGNAL_H
#define _SIGNAL_H
#ident	"@(#)debugger:inc/i386/signal.h	1.1"

/*LOCAL COPY*/
#ifndef __SIG_TYP
#define __SIG_TYP
typedef void SIG_FUNC_TYP(int);
typedef SIG_FUNC_TYP *SIG_TYP;
#endif

typedef int 	sig_atomic_t;

extern char *_sys_siglist[];
extern int _sys_nsig;

#include <sys/signal.h>

#ifdef __cplusplus
extern "C" {
#endif
extern SIG_TYP signal(int, SIG_TYP);
extern int raise(int);
#ifdef __cplusplus
}
#endif

#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif
extern int kill(pid_t, int);
extern int sigaction(int, const struct sigaction *, struct sigaction *);
extern int sigaddset(sigset_t *, int);
extern int sigdelset(sigset_t *, int);
extern int sigemptyset(sigset_t *);
extern int sigfillset(sigset_t *);
extern int sigismember(const sigset_t *, int);
extern int sigpending(sigset_t *);
extern int sigprocmask(int, const sigset_t *, sigset_t *);
extern int sigsuspend(sigset_t *);
#ifdef __cplusplus
}
#endif

#include <sys/procset.h>
#ifdef __cplusplus
extern "C" {
#endif
extern int gsignal(int);
extern SIG_TYP sigset(int, SIG_TYP);
extern int sighold(int);
extern int sigrelse(int);
extern int sigignore(int);
extern int sigpause(int);
extern int sigaltstack(const stack_t *, stack_t *);
extern int sigsend(idtype_t, id_t, int);
extern int sigsendset(const procset_t *, int);
#ifdef __cplusplus
}
#endif

#undef SIG_ERR
#undef SIG_DFL
#undef SIG_IGN
#undef SIG_HOLD
#define SIG_ERR	(SIG_TYP)-1
#define	SIG_DFL	(SIG_TYP)0
#define	SIG_IGN	(SIG_TYP)1
#define SIG_HOLD (SIG_TYP)2

#endif 	/* _SIGNAL_H */
