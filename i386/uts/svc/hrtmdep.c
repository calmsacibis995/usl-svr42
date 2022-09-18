/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/hrtmdep.c	1.4"
#ident	"$Header: $"

#include <proc/proc.h>
#include <proc/user.h>
#include <svc/hrtcntl.h>
#include <svc/hrtsys.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

#ifdef KPERF
#include <proc/disp.h>
#endif /* KPERF */

/*	This file contains the code that manages the hardware clocks and 
**	timers.  We must provide UNIX with a HZ resolution clock and give 
**	the user an interface to the timers through system calls.
*/

dl_t	tick;				/* computational convenience */
ulong	hr_lbolt;			/* high-resolution lightning bolt */


uint	ticks_til_clock;	/* Number of times the hardware	*/
					/* clock has to interrupt	*/
					/* before calling the UNIX	*/
					/* clock routine.		*/
uint	unix_tick;		/* Time remaining until the	*/
					/* next call to the UNIX clock	*/
					/* routine.			*/

clock_int(pc,cs,flags, oldipl)
caddr_t	pc;
int	cs;
int	flags;
int	oldipl;
{
	register timer_t	*hrp, *nhrp;
	register proc_t		*prp;
	register int		type;   /* type of virtual clock */
					/* 0 - CLK_USERVIRT	 */
					/* 1 - CLK_PROCVIRT	 */

#ifdef	KPERF
	/*
	** hr_lbolt is incremented at the start of this routine to allow
	**	correct time-stamping of Kernel PERFormance records.
	*/

	++hr_lbolt;

	if (kpftraceflg)
		kperf_write(KPT_INTR, clock_int, curproc);
#endif	/* KPERF */
	/*	If panic, stop clock
	*/

	if(panicstr){
		clkreld();
		return(0);
	}
	
	/*	Process the high-resolution timer list.
	*/

	if(hrt_active.hrt_next != &hrt_active){

		/*	There is an active high-resolution timer.
		**	Decrement it and see if it fires.
		*/

		hrp = hrt_active.hrt_next;
		if(--hrp->hrt_time == 0){

			/*	The timer fired.  If this was some
			**	kind of an alarm, then post the event.
			**	If it was a sleep, then do the wakeup.
			*/

			do{
				/*	Remove the entry from the list.
				*/

				hrt_dequeue(hrp);

				if(hrp->hrt_cmd == HRT_INTSLP){
					wakeup((caddr_t)hrp);
				} else {
					hrp->hrt_fn(hrp, 0, 0);
				}

				if(hrp->hrt_int){
				/* Repeative alarm */
					hrp->hrt_crem += hrp->hrt_rem;
					hrp->hrt_time = hrp->hrt_int;
					if(hrp->hrt_crem >= tick.dl_lop){
						hrp->hrt_time += 1;
						hrp->hrt_crem -= tick.dl_lop;
					};
					hrt_timeout(hrp, 0);
				} else {
					hrt_free(hrp, &hrt_avail);
				}

				hrp = hrt_active.hrt_next;
			} while(hrp != &hrt_active  &&  hrp->hrt_time == 0);
		}
	}

	/*
         *	We are using u_uservirt to implement the clock 
	 *	measuring the user process virtual time and
	 * 	u_procvirt for the process' virtual
	 *	time clock.
 	 *	Charge the time out based on the mode the cpu is in.
	 *	We assume that the current state has been around at
	 *	least one tick.
	 */
	if (USERMODE(cs)) {
		/*
		 * Update process virtual time and
		 * user virtual time.
		 */
		u.u_uservirt++;
		u.u_procvirt++;
		type = 0;
	} else {
		if (SYSIDLE(pc)) {
			type = 2;
		} else {
			/* Update process virtual time */
			u.u_procvirt++;
			type = 1;
		}
	}

	/* 	Process the interval-timers list.
	*/

	prp = u.u_procp;

	while (type < 2) {

		hrp = prp->p_italarm[type];

		if (hrp != NULL) {

			/*	There is an active interval timer.
			**	Decrement it and see if it fires.
			*/

			if (--hrp->hrt_time == 0) {

				/*	The timer fired. Post an event. */

				do {
					nhrp = hrp->hrt_next;

					/*	Remove the entry from the list.
					*/
					itimer_dequeue(hrp, type);

					hrp->hrt_fn(hrp, 0, 0);

					if(hrp->hrt_int){
					/* Repeative alarm */
						hrp->hrt_crem += hrp->hrt_rem;
						hrp->hrt_time = hrp->hrt_int;
						if(hrp->hrt_crem >= tick.dl_lop){
							hrp->hrt_time += 1;
							hrp->hrt_crem -= tick.dl_lop;
						};
						itimer_timeout(hrp, type, 0, 0);
					} else {
						hrt_free(hrp, &it_avail);
					}

					hrp = nhrp;
				} while (hrp != NULL && hrp->hrt_time == 0);
			}	
		}
		type++;
	}
	
#ifndef	KPERF
	/*
	** See comment at the start of this routine....
	*/

	++hr_lbolt;
#endif	/* !KPERF */

	if (--unix_tick == 0) {
		unix_tick = ticks_til_clock;
		return(clock(pc,cs,flags,oldipl));	/* Give Unix a tick */
	}

	return(0);	/* no profiling to do now */
}
