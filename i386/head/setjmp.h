/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _JBLEN

#ident	"@(#)sgs-head:setjmp.h	1.9.5.2"

#if defined(__STDC__)

#if #machine(i386)
#define _SIGJBLEN 128	/* (sizeof(ucontext_t) / sizeof (int)) */
#elif #machine(i860)
#define _SIGJBLEN 137	/* (sizeof(ucontext_t) / sizeof (int)) */
#elif #machine(sparc)
#define _SIGJBLEN 19	/* (sizeof(ucontext_t) / sizeof (int)) */
#else
#define _SIGJBLEN 64	/* (sizeof(ucontext_t) / sizeof (int)) */
#endif

#if #machine(i860)
#define _JBLEN  22
#elif #machine(m68k)
#define _JBLEN  40
#elif #machine(m88k)
#define _JBLEN  24
#elif #machine(sparc)
#define _JBLEN  12
#else   
#define _JBLEN  10
#endif	/* #machine */

#else 

#if i386
#define _SIGJBLEN 128	/* (sizeof(ucontext_t) / sizeof (int)) */
#elif i860
#define _SIGJBLEN 137	/* (sizeof(ucontext_t) / sizeof (int)) */
#elif sparc
#define _SIGJBLEN 19	/* (sizeof(ucontext_t) / sizeof (int)) */
#else
#define _SIGJBLEN 64	/* (sizeof(ucontext_t) / sizeof (int)) */
#endif

#if i860
#define _JBLEN  22
#elif m68k
#define _JBLEN  40
#elif m88k
#define _JBLEN  24
#elif sparc
#define _JBLEN  12
#else   
#define _JBLEN  10
#endif

#endif	/* __STDC__ */

typedef int jmp_buf[_JBLEN];

#if defined(__STDC__)
extern int setjmp(jmp_buf);
extern void longjmp(jmp_buf, int);

#if __STDC__ == 0 || defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) /* non-ANSI standard compilation */
typedef int sigjmp_buf[_SIGJBLEN];

extern int sigsetjmp(sigjmp_buf, int);
extern void siglongjmp(sigjmp_buf, int);
#endif

#if __STDC__ != 0
#define setjmp(env)	setjmp(env)
#endif

#else
typedef int sigjmp_buf[_SIGJBLEN];

extern int setjmp();
extern void longjmp();
extern int sigsetjmp();
extern void siglongjmp();

#endif  /* __STDC__ */

#endif  /* _JBLEN */
