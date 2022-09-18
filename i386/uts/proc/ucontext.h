/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_UCONTEXT_H	/* wrapper symbol for kernel use */
#define _PROC_UCONTEXT_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/ucontext.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _PROC_REGSET_H
#include <proc/regset.h>	/* REQUIRED */
#endif

#ifndef _PROC_SIGNAL_H
#include <proc/signal.h>	/* REQUIRED */
#endif

#else

#include <sys/types.h> 	/* SVR4.0COMPAT */
#include <sys/regset.h> /* SVR4.0COMPAT */
#include <sys/signal.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

typedef struct {
	gregset_t	gregs;	/* general register set */
	fpregset_t 	fpregs;	/* floating point register set */
} mcontext_t;

typedef struct ucontext {
	u_long		uc_flags;
	struct ucontext	*uc_link;	/* previous user context */
	sigset_t   	uc_sigmask;	/* signal mask */
	stack_t 	uc_stack;	/* stack state */
	mcontext_t 	uc_mcontext;	/* machine context */
	long		uc_filler[5];	/* pad the structure to 512 bytes */
} ucontext_t;

#define GETCONTEXT	0
#define SETCONTEXT	1

/* 
 * values for uc_flags
 * these are implementation dependent flags, that should be hidden
 * from the user interface, defining which elements of ucontext
 * are valid, and should be restored on call to setcontext
 */

#define	UC_SIGMASK	001
#define	UC_STACK	002
#define	UC_CPU		004
#define	UC_FP		010

#ifdef WEITEK
#define UC_WEITEK	020
#endif /* WEITEK */

#ifdef WEITEK
#define UC_MCONTEXT (UC_CPU|UC_FP|UC_WEITEK)
#else
#define UC_MCONTEXT (UC_CPU|UC_FP)
#endif /* WEITEK */

/* 
 * UC_ALL specifies the default context
 */

#define UC_ALL		(UC_SIGMASK|UC_STACK|UC_MCONTEXT)

#ifdef _KERNEL

#if defined(__STDC__)
void savecontext(ucontext_t *, k_sigset_t);
void restorecontext(ucontext_t *);
#else
void savecontext();
void restorecontext();
#endif

#endif
#endif /* _PROC_UCONTEXT_H */
