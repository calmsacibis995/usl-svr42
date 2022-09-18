/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_DEBUG_H	/* wrapper symbol for kernel use */
#define _UTIL_DEBUG_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/debug.h	1.4"
#ident	"$Header: $"

/*
 * Macro definitions needed when building a kernel
 * with debugging information.
 */

#define	YES 1
#define	NO  0

/*
 * ASSERT allows run-time checks to be performed at strategic
 * places throughout the kernel code.  If the expression in
 * the ASSERT() call is false, an error message will be printed
 * via a call to assfail().  After development and testing are
 * complete, assertions can be compiled out to improve performance.
 */

#if DEBUG == YES

#if defined(__STDC__)

extern int assfail(char *, char *, int);
#define ASSERT(EX) ((void)((EX) || assfail(#EX, __FILE__, __LINE__)))

#else

extern int assfail();
#define ASSERT(EX) ((void)((EX) || assfail("EX", __FILE__, __LINE__)))

#endif	/* __STDC__ */

#define DB_ISKV(A) ((unsigned)(A) >= KVBASE && (unsigned)(A) < UVUBLK)

#else

#define ASSERT(x)
#define DB_ISKV(A) 1

#endif	/* DEBUG */

/*
 * There are many kernel variables that could be file-scope static
 * (since they are defined and referenced in just one source file)
 * except that they would not then be visible in the symbol table
 * for access via a kernel debugger or a program reading /dev/kmem.
 * Such variables may be declared "STATIC" or "KSTATIC".  
 *
 *	STATIC  for variables file-scope static and
 *              are NOT referenced via the symbol table.
 *	KSTATIC	for variables file-scope static which
 *		ARE referenced via the symbol table by
 *		a program reading /dev/kmem.
 *
 * A compile-time definition of STATIC and KSTATIC as the null string 
 * renders such variables global, and thus visible in the symbol table.  
 * If no command line definition of STATIC is supplied at compile time, 
 * we use an appropriate value depending on whether or not a debugging 
 * version of the kernel is being built.
 * KSTATIC will always be defined as the null string, rendering variables
 * as global, unless otherwise defined at compile time.
 */

#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif
#endif

#ifndef KSTATIC
#define KSTATIC
#endif
#define Static STATIC
#define Public

#endif	/* _UTIL_DEBUG_H */
