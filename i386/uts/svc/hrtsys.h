/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_HRTSYS_H	/* wrapper symbol for kernel use */
#define _SVC_HRTSYS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:svc/hrtsys.h	1.5"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *	Kernel structure for keeping track of sleep and alarm requests.
 */


typedef struct timer {
	struct proc	*hrt_proc;	/* Ptr to proc table entry.	*/
	int		hrt_clk;	/* Which clock to use		*/
	unsigned long	hrt_time;	/* Remaining time to alarm in	*/
					/* HZ.				*/
	unsigned long	hrt_int;	/* Base interval for repeating	*/
					/* alarms.			*/
	unsigned long	hrt_rem;	/* Value of remainder for	*/
					/* repeat alarm.		*/
	unsigned long	hrt_crem;	/* Cummulative remainder for	*/
					/* repeating alarm.		*/
	unsigned long	hrt_res;	/* User specified resolution.	*/
	ushort		hrt_cmd;	/* The user specified command.	*/
	struct vnode	*hrt_vp;	/* Ptr to vnode for event queue	*/
					/* to post to.			*/
	int	(*hrt_fn)();		/* function to call		*/	
	struct timer	*hrt_next;	/* Next on list.		*/
	struct timer	*hrt_prev;	/* Previous on list.		*/
} timer_t;

#ifdef _KERNEL

extern timer_t	hrt_active;
extern timer_t	hrt_avail;
extern timer_t	it_avail;

extern int	hrt_cancel_proc();
extern void	hrt_dequeue();
extern void	hrt_free();
extern void	hrt_timeout();
extern void	hrtinit();
extern void	itimer_dequeue();
extern void	itimer_timeout();
extern void	itinit();

#endif /* _KERNEL */

#endif	/* _SVC_HRTSYS_H */
