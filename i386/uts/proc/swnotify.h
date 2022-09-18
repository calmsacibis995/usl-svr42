/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_SWNOTIFY_H	/* wrapper symbol for kernel use */
#define _PROC_SWNOTIFY_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/swnotify.h	1.1"
#ident	"$Header: $"

/*
 * Support for the swtch_notify() function, which can be used by drivers
 * to receive notification whenever an execution context is switched in.
 * This can be used to control mapping registers which need to be set
 * appropriately for direct user access to I/O devices which share bus
 * addresses.
 *
 * The driver allocates an swnotify_t structure, usually as part of its
 * per-device information structure, and initializes the swn_func field
 * to point to the driver function to be called for notification.  It then
 * calls swtch_notify() in the desired context, passing a pointer to this
 * structure and the SWN_REGISTER command.  Subsequently, every time a
 * context switch is made (back) to that context, the function will be
 * called, with a pointer to the swnotify_t structure as the single argument.
 * The driver can derive a pointer to its per-device information structure
 * from this pointer.
 *
 * When the driver no longer requires notification, it calls swtch_notify()
 * again, this time with the SWN_CANCEL command.  The SWN_CANCEL command
 * can be used out-of-context.
 *
 * The driver should not access any swnotify_t field other than swn_func.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct swnotify {
	struct swnotify	*swn_next;
	void		(*swn_func)();
	_VOID		*swn_contextp;
} swnotify_t;

#endif /* _KERNEL || _KMEMUSER */

#if defined(_KERNEL)

typedef enum swnotify_cmd {
	SWN_REGISTER,	/* register for notification of context-switch in */
	SWN_CANCEL	/* cancel notification of context-switch in */
} swnotify_cmd_t;

#if defined(__STDC__)

extern int swtch_notify(swnotify_t *, swnotify_cmd_t);

#else

extern int swtch_notify();

#endif /* __STDC__ */

#endif /* _KERNEL */

#endif /* _PROC_SWNOTIFY_H */
