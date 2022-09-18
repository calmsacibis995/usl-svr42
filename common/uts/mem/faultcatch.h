/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_FAULTCATCH_H	/* wrapper symbol for kernel use */
#define _MEM_FAULTCATCH_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/faultcatch.h	1.1.2.4"
#ident	"$Header: $"

/*
 * This file defines a mechanism for catching kernel page fault errors.
 * Any access to a pageable address should be protected by this mechanism,
 * since the I/O may fail, or (in the case of a user-supplied address)
 * the address may be invalid.
 *
 * Usage:
 *		CATCH_FAULTS(flags)
 *			protected_statement
 *		errno = END_CATCH();
 *
 * The flags define the type of address to protect.  This includes user
 * addresses, seg_u addresses, and seg_map addresses.
 *
 * The value returned by END_CATCH() will be 0 if no fault error occurred,
 * or the errno returned from the fault handler (unless the error occurred
 * on a user address, in which case the fault handler's return value is
 * ignored and EFAULT is returned).
 *
 * Caveats:
 *
 * CATCH_FAULTS should not be used from interrupt routines, or
 * nested within another CATCH_FAULTS.
 *
 * The protected code must not do anything stateful, such as using spl's
 * or setting locks, since it may be aborted in midstream.
 */

#define CATCH_UFAULT		0x0001
#define CATCH_SEGMAP_FAULT	0x0002
#define CATCH_SEGU_FAULT	0x0004
#define CATCH_KPAGE_FAULT	0x0008
#define CATCH_BUS_TIMEOUT	0x4000
#define CATCH_ALL_FAULTS	0x8000

#define CATCH_KERNEL_FAULTS	(CATCH_SEGMAP_FAULT|CATCH_SEGU_FAULT|CATCH_KPAGE_FAULT)

#if !defined(LOCORE)

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The implementation of CATCH_FAULTS() uses
 * an instance of struct fault_catch in the u-area.
 */

typedef struct fault_catch {
	u_int	fc_flags;	/* type(s) of faults to catch, see above */
	int	fc_errno;	/* holds return value for END_CATCH */
	void	(*fc_func)();	/* fault handler, normally fc_jmpjmp() */
	label_t	fc_jmp;		/* save area for setjmp/longjmp */
} fault_catch_t;

#if defined(_KERNEL)

/*
 * NOTE: Although the implementation of CATCH_FAULTS() uses setjmp/longjmp,
 * the enclosed code MUST NOT do anything stateful, since it could be aborted
 * at any point.  This applies to multiprocessing locks as well, so this
 * particular use of setjmp/longjmp is safe in a multiprocessor context.
 */

#ifdef DEBUG

#define CATCH_FAULTS(flags)				\
	if (ASSERT(!servicing_interrupt()),		\
	    ASSERT(u.u_fault_catch.fc_flags == 0),	\
	    (u.u_fault_catch.fc_errno = 0),		\
	    (u.u_fault_catch.fc_flags = (flags)),	\
	    setjmp(&u.u_fault_catch.fc_jmp) == 0)

#else	/* !DEBUG */

#define CATCH_FAULTS(flags)				\
	if ((u.u_fault_catch.fc_errno = 0),		\
	    (u.u_fault_catch.fc_flags = (flags)),	\
	    setjmp(&u.u_fault_catch.fc_jmp) == 0)

#endif	/* DEBUG */

#define END_CATCH()					\
	((u.u_fault_catch.fc_flags = 0),		\
	 u.u_fault_catch.fc_errno)

#if defined(__STDC__)
extern void	fc_jmpjmp(void);
#else
extern void	fc_jmpjmp();
#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* !LOCORE */

#endif	/* _MEM_FAULTCATCH_H */
