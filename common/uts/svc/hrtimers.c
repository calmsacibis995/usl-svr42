/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:svc/hrtimers.c	1.12.3.5"
#ident	"$Header: $"

#include <acc/mac/covert.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/hrtcntl.h>
#include <svc/hrtsys.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/types.h>

/*
 * This file contains the code that manages the hardware clocks and 
 * timers.  We must provide UNIX with a HZ resolution clock and give 
 * the user an interface to the timers through system calls.
 */

static int	hrt_alarm();
static timer_t	*hrt_alloc();
static int	hrt_bsd_cancel();
static int	hrt_checkclock();
static int	hrt_checkres();
static long	hrt_convert();
static void	hrt_enqueue();
static void	hrt_gettofd();
static int	hrt_match_tid();
static int	hrt_past_time();
static int	hrt_setup();
static void	itimer_enqueue();

timer_t		hrt_active;	/* List of active high-resolution	*/
				/* timer structures.			*/
timer_t		hrt_avail;	/* List of available high-resolution	*/
				/* timer structures.			*/
STATIC timer_t	hrt_null;	/* Null structure for initializing.	*/

timer_t		it_avail;	/* List of available interval		*/
				/* timer structures.			*/

STATIC dl_t scalel = { 0, SCALE };

/* control structures for covert channel limiter */
STATIC ccevent_t cc_re_timers = { CC_RE_TIMERS, CCBITS_RE_TIMERS };
STATIC ccevent_t cc_spec_timers = { CC_SPEC_TIMERS, CCBITS_SPEC_TIMERS };

/*
 * from hrt_ms.c
 */

extern dl_t	tick;
extern ulong	hr_lbolt;

/*
 * tunables from hrt.cf
 */

extern timer_t	hrtimes[];
extern int	hrtimes_size;
extern timer_t	itimes[];
extern int	itimes_size;

/*
 * Argument vectors for the various flavors of hrtsys().
 */

#define	HRTCNTL		0
#define	HRTALARM	1

struct 	hrtsysa {
	int	opcode;
};

struct	hrtcntla {
	int		opcode;
	int		cmd;
	int		clk;
	interval_t	*intp;
	hrtime_t	*hrtp;
};

struct	hrtalarma {
	int	opcode;
	hrtcmd_t	*cmdp;
	int		cmds;
};


/* 
 * Hrtcntl (time control) system call.
 */


STATIC int
hrtcntl(uap, rvp)
	register struct hrtcntla *uap;
	rval_t	*rvp;
{
	register int	error = 0;
	register ulong	start_time;
	register ulong	stop_time;
	ulong		new_res;
	int		clock;
	interval_t	it_buf;	
	hrtime_t	temptofd;

	switch(uap->cmd) {

		case	HRT_GETRES:	/* Get the resolution of a clock */

			if ((error = hrt_checkclock(uap->clk)))
				break;
			rvp->r_val1 = timer_resolution;
			break;

		case	HRT_TOFD:	/* Get the time of day 		 */

			if (uap->clk != CLK_STD) {
				error = EINVAL;
				break;
			}

			if (copyin((caddr_t)uap->hrtp,
			    (caddr_t)&temptofd, sizeof(hrtime_t))) {
				error = EFAULT;
				break;
			}

			if ((error = hrt_checkres(temptofd.hrt_res))) {
				break;
			}

			hrt_gettofd(&temptofd);

			if (copyout((caddr_t)&temptofd,
			    (caddr_t)uap->hrtp, sizeof(hrtime_t))) {
				error = EFAULT;
			}

			break;

		case	HRT_STARTIT:	/* Start timing an activity      */	
			
			clock = uap->clk;

			if (clock == CLK_STD)
				it_buf.i_word1 = hr_lbolt;
			else if (clock == CLK_PROCVIRT)
				it_buf.i_word1 = u.u_procvirt;
			else if (clock == CLK_USERVIRT)
				it_buf.i_word1 = u.u_uservirt;
			else {
				error = EINVAL;
				break;
			}
			it_buf.i_clock = clock;
			if (copyout((caddr_t)&it_buf,
			    (caddr_t)uap->intp, sizeof(interval_t)))
				error = EFAULT;
			break;
		case	HRT_GETIT:	/* Get value of interval timer	  */

			/*
			 * 	Record stop time in case we page fault
			 *	and get delayed.
			 */

			if (copyin((caddr_t)uap->intp,
			    (caddr_t)&it_buf, sizeof(interval_t))) { 
				error = EFAULT;
				break;
			}
			clock = it_buf.i_clock;
			if (clock == CLK_STD)
				stop_time = hr_lbolt;
			else if (clock == CLK_PROCVIRT)
				stop_time = u.u_procvirt;
			else if (clock == CLK_USERVIRT)
				stop_time = u.u_uservirt;
			else {
				error = EINVAL;
				break;
			}

			if (copyin((caddr_t)uap->hrtp,
			    (caddr_t)&temptofd, sizeof(hrtime_t))) {
				error = EFAULT;
				break;
			}

			if ((error = hrt_checkres(temptofd.hrt_res))) {
				break;
			}

			start_time = it_buf.i_word1;

			new_res = temptofd.hrt_res;
			temptofd.hrt_secs = 0;
			temptofd.hrt_rem  = stop_time - start_time;
			temptofd.hrt_res  = timer_resolution;
			error = hrt_newres(&temptofd, new_res,
						HRT_TRUNC);

			if (error)
				break;

			if (copyout((caddr_t)&temptofd,
			    (caddr_t)uap->hrtp, sizeof(hrtime_t))) {
				error = EFAULT;
			}

			break;
		default:
			error = EINVAL;
			break;
	}
	return error;
}			

/*
 * Hrtalarm (start one or more alarms) system call.
 */

STATIC int
hrtalarm(uap, rvp)
	register struct hrtalarma *uap;
	rval_t	*rvp;
{
	register hrtcmd_t	*cp;
	register ulong		base_lbolt;
	register int		oldpri;
	hrtcmd_t		*hrcmdp;
	hrtime_t		*htpnd_p;
	int			error = 0;
	int			cnt;
	int			bsd_cnt = -1;
	int			cmd;
	uint			alarm_cnt;
	int			alarm_type;
	hrtcmd_t		timecmd;
	hrtime_t		delay_ht;

	/*	Get a consistent point in time from which to base
	**	all of the alarms in the list.  Make sure we don't
	**	get an interrupt during the sampling.
	*/

	oldpri		= splhi();
	base_lbolt	= hr_lbolt;
	splx(oldpri);

	/*
	 * Return EINVAL for negative and zero counts.
	 */

	if (uap->cmds <= 0)
		return(EINVAL);

	cp = &timecmd;
	hrcmdp = uap->cmdp;
	alarm_cnt = 0;

	/*	Loop through and process each command.
	*/

	for (cnt = 0; cnt < uap->cmds; cnt++, hrcmdp++) {

		if(copyin((caddr_t)hrcmdp,
		    (caddr_t)cp, sizeof(hrtcmd_t))) {
			error = EFAULT;
			return error;
		}

		/*	See if events are configured.
		*/

		cmd = cp->hrtc_cmd;
		
		if ( (cmd == HRT_ALARM || cmd == HRT_RALARM ||
			cmd == HRT_TODALARM || cmd == HRT_INT_RPT ||
			     cmd == HRT_TOD_RPT || cmd == HRT_PENDING) )
			return(ENOPKG);

		/*
 	         * If we try to post a Berkley Timer remove
		 * previous timers.
		 */

		if (cmd == HRT_BSD || cmd == HRT_RBSD ||
			cmd == HRT_BSD_REP) {
			(void)hrt_bsd_cancel(cp->hrtc_clk);
			/*
			 * If there is more than one BSD command
			 * per hrtalarm() call, we have a covert
			 * channel event.
			 */
			if (bsd_cnt++ == 0)
				cc_limiter(&cc_spec_timers, u.u_cred);
		}

		/*	See what kind of command we have.
		*/
				 
		switch(cp->hrtc_cmd){
			case HRT_BSD:
			case HRT_RBSD:
				if (error = hrt_checkclock(cp->hrtc_clk))
					break;
				
				error = hrt_alarm(cp->hrtc_cmd, cp->hrtc_clk,
						&cp->hrtc_int, NULL,
					  	base_lbolt);
				break;

			case HRT_BSD_REP:
				if (error = hrt_checkclock(cp->hrtc_clk))
					break;

				if (cp->hrtc_cmd == HRT_INT_RPT)
					cmd = HRT_ALARM;
				else
					cmd = HRT_BSD;

				error = hrt_alarm(cmd, cp->hrtc_clk, 
						&cp->hrtc_tod, NULL,
					  	base_lbolt);

				if (error)
					break;

				if (cp->hrtc_cmd == HRT_INT_RPT)
					cmd = HRT_RALARM;
				else
					cmd = HRT_RBSD;

				error = hrt_alarm(cmd, cp->hrtc_clk,
						&cp->hrtc_int, &cp->hrtc_tod,
					  	base_lbolt);
				break;

			case HRT_BSD_PEND:
				if (error = hrt_checkclock(cp->hrtc_clk))
					break;

				alarm_type = 0;

				error = hrt_match_tid(cp->hrtc_cmd,
						cp->hrtc_clk, &alarm_type,
				           	   &delay_ht);
				if (error)
					break;

				if (alarm_type == HRT_TODALARM)
					htpnd_p = &hrcmdp->hrtc_tod;
				else
					htpnd_p = &hrcmdp->hrtc_int;

				if (copyout((caddr_t)&delay_ht,
				    (caddr_t)htpnd_p,  sizeof(hrtime_t))) {
					error = EFAULT;
				}
				
				break;

			case HRT_BSD_CANCEL:
				if (error = hrt_checkclock(cp->hrtc_clk))
					break;

				error = hrt_bsd_cancel(cp->hrtc_clk);

				break;

			default :
				error = EINVAL;
				break;
		}
		if (error) {
			cp->hrtc_flags |= HRTF_ERROR;
			cp->hrtc_error = error;
		} else {
			cp->hrtc_flags |= HRTF_DONE;
			cp->hrtc_error = 0;
			alarm_cnt++;
		}
		if (copyout((caddr_t)&cp->hrtc_flags,
		    (caddr_t)&hrcmdp->hrtc_flags,
		    sizeof(cp->hrtc_flags) + sizeof(cp->hrtc_error))) {
			error = EFAULT;
			return error;
		}
	}
	rvp->r_val1 = alarm_cnt;
	return(0);
}


/*	Do the HRT_TODALARM function.
*/


static int
hrt_todalarm(utdp, delayp)
register hrtime_t	*utdp;
register hrtime_t	*delayp;
{
	register int		oldpri;
	register ulong		base_lbolt;
	register ulong		rem;
	hrtime_t		remhrt;
	long			interval;
	int			error = 0;

	/*	Get a reliable time base for computing the interval.
	**	Don't let a clock tick happen during this step.
	*/

	oldpri = splhi();
	interval = utdp->hrt_secs - hrestime.tv_sec;
	base_lbolt	  = hr_lbolt;
	rem		  = base_lbolt % timer_resolution;
	splx(oldpri);

	/*
 	 * 	Check for errors
	 */
	if (interval < 0) {
		return(hrt_past_time(HRT_TODALARM, utdp->hrt_res));
	}

	utdp->hrt_secs = interval;
	remhrt.hrt_secs = 0;
	remhrt.hrt_rem  = rem;
	remhrt.hrt_res  = timer_resolution;

	if ((error = hrt_newres(&remhrt, utdp->hrt_res, HRT_RND)))
		return(error);

	if ((interval = hrt_convert(utdp)) == -1)
		return(ERANGE);

	interval -= remhrt.hrt_rem;

	if (interval <= 0) {
		return(hrt_past_time(HRT_TODALARM, utdp->hrt_res));
	}

	utdp->hrt_secs = interval / utdp->hrt_res;
	utdp->hrt_rem  = interval % utdp->hrt_res;

	/*	Initialize the initial delay */

	if (delayp != NULL) {
		delayp->hrt_secs = utdp->hrt_secs;
		delayp->hrt_rem  = utdp->hrt_rem;
		delayp->hrt_res  = utdp->hrt_res;
	}

	/*	Now set the timer.
	*/

	return(hrt_alarm(HRT_TODALARM, CLK_STD, utdp,
			delayp, base_lbolt));
}

/*	Do the HRT_ALARM, HRT_RALARM, or HRT_TODALARM function.
*/


static int
hrt_alarm(cmd, clock, hrtp, delayp, base_lbolt)
register	int	cmd;
register	int	clock;
register	hrtime_t	*hrtp;
register	hrtime_t	*delayp;
ulong			base_lbolt;
{
	register timer_t	*hrp;
	register int		rounding;
	timer_t			*free_list;
	ulong			numerator;
	ulong			kinterval;
	ulong			fudge;
	long			interval;
	long			user_delay;
	ulong			res;
	int			error = 0;
	dl_t			user_int;
	dl_t			user_res;
	dl_t			sys_int;
	dl_t			real_int;

	/*	Check that the requested resolution is
	**	legal.
	*/

	if (error = hrt_checkres(hrtp->hrt_res)) {
		return error;
	}

	if (hrtp->hrt_rem < 0)
		return ERANGE;

	if (hrtp->hrt_rem >= hrtp->hrt_res) {
		hrtp->hrt_secs += hrtp->hrt_rem / hrtp->hrt_res;
		hrtp->hrt_rem = hrtp->hrt_rem % hrtp->hrt_res;
	}

	interval = hrtp->hrt_rem;
	res = hrtp->hrt_res;

	/*	Allocate a hrtime structure.  Fail with
	**	EAGAIN if none available.
	*/

	if (clock == CLK_STD)
		free_list = &hrt_avail;
	else
		free_list = &it_avail;

	hrp = hrt_alloc(free_list);
	if(hrp == NULL){
		error = EAGAIN;
		return error;
	}

	/*	Initialize the hrtime structure.  If this
	**	fails, an error code has already been set.
	*/

	if(error = hrt_setup(hrp, cmd, clock, hrtp->hrt_res)) {
		hrt_free(hrp, free_list);
		return error;
	}


	/*	Determine which way to round depending on whether
	**	we are doing a single alarm or a repeating alarm.
	*/

	if(cmd == HRT_RALARM)
		rounding = HRT_TRUNC;
	else
		rounding = HRT_RND;
	
	/*	Convert the interval from the base the user specified
	**	to our internal base using the rounding just determined.
	**	Check for erroneous specification of interval or
	**	resolution.  This alarm may be part of request for multiple
	**	alarms in which case some time may have elapsed since the start
	**	of the call.  Try to correct for this by subtracting off
	**	of the specified interval the time which has expired
	**	since the start of the call.
	*/

	if ((error = hrt_newres(hrtp, (ulong)timer_resolution, rounding))) {
		hrt_free(hrp, free_list);
		return(error);
	}
	fudge = hr_lbolt - base_lbolt;
	hrp->hrt_time = hrt_convert(hrtp);
	if(hrp->hrt_time == -1) {
		hrt_free(hrp, free_list);
		return(ERANGE);
	}

	if ( (cmd == HRT_ALARM || cmd == HRT_BSD) && (hrp->hrt_time == 0
				 || hrp->hrt_time <= fudge) ) {

		/* For small intervals fire the alarm immediately */

		error = hrp->hrt_fn(hrp, 0, 0);
		hrt_free(hrp, free_list);
		return(error);
	}

	if ( (cmd == HRT_RALARM) && (hrp->hrt_time == 0
				 || hrp->hrt_time <= fudge) ) {

		/* For small intervals return an error */

		hrt_free(hrp, free_list);
		return(ERANGE);
	}


	/*	If we are doing a repeating alarm, we must get the
	**	remainder from the base conversion calculation.
	**	We use this to alter the interval from alarm to
	**	alarm if necessary in order to not accumulate
	**	drift.  Of course, if the remainder is zero, then
	**	all alarms will be exact.  This will happen if the
	**	interval specified is a multiple of our internal
	**	resolution.
	*/

	if(cmd == HRT_RALARM || cmd == HRT_RBSD){

		/*	Calculate the remainder from the above
		**	conversion of the interval to our resolution.
		**	So that it will fit nicely in a 32 bit long,
		**	we really calculate and store the fractional
		**	remainder as if it were multiplied by SCALE
		**	(1 million).  This effectively gives us 6
		**	digits of accuracy to the right of the decimal
		**	point.
		**
		**	We first do the calculation in normal long
		**	arithmetic.  If this works O.K., we use the
		**	result.  However, if we get overflow, then
		**	we use the double precision integer package
		**	created especially for this purpose.
		*/

		hrp->hrt_int = hrp->hrt_time;

		if (delayp) {
			if ((error = hrt_newres(delayp, (ulong)timer_resolution,
						rounding))) {
				return(error);
			}
			user_delay = hrt_convert(delayp);
			if (user_delay == -1) {
				return(ERANGE);
			}
			hrp->hrt_time += user_delay;
		}
		if (interval != 0 && hrtp->hrt_rem != 0) {
			numerator = interval * SCALE;
			kinterval = hrtp->hrt_rem * (SCALE / timer_resolution);

			if(numerator / interval == SCALE  &&
		   		kinterval / hrtp->hrt_rem ==
						 (SCALE / timer_resolution)) {
				hrp->hrt_rem = numerator / res - kinterval;
			} else {
			
				/*	Must use double-precision routines.
				*/

				user_int.dl_hop = 0;
				user_int.dl_lop = interval;
				user_res.dl_hop = 0;
				user_res.dl_lop = res;
				sys_int.dl_hop  = 0;
				sys_int.dl_lop  = hrtp->hrt_rem;

				real_int = ldivide(lmul(user_int, scalel), user_res);
				hrp->hrt_rem =
			     	lsub(real_int, lmul(sys_int, tick)).dl_lop + 1;
			}
		}	
	}

	/*	Now queue the timer request into the
	**	list where it belongs.
	*/

	if (clock == CLK_STD)
		hrt_timeout(hrp, base_lbolt);
	else {
		if (clock == CLK_USERVIRT)
			itimer_timeout(hrp, 0, base_lbolt, clock);
		else
			itimer_timeout(hrp, 1, base_lbolt, clock);
	}

	return 0;
}

/*	Set a high-resolution timer.
*/

void
hrt_timeout(hrp, base_lbolt)
register timer_t	*hrp;
register ulong		base_lbolt;
{
	register timer_t	*nhrp;
	register int		oldpri;
	register ulong		fudge;

	/*	Find where the new entry belongs in the time ordered
	**	list.  Fix up the time as we go.
	*/

	oldpri = splhi();
	if (base_lbolt) {
		fudge = hr_lbolt - base_lbolt;
		if (fudge >= hrp->hrt_time)
			hrp->hrt_time = 1;
		else
			hrp->hrt_time -= fudge;
	}
	nhrp = hrt_active.hrt_next;
	while(nhrp != &hrt_active  &&  nhrp->hrt_time <= hrp->hrt_time){
		hrp->hrt_time -= nhrp->hrt_time;
		nhrp = nhrp->hrt_next;
	}

	nhrp->hrt_time -= hrp->hrt_time;
	hrt_enqueue(hrp, nhrp);
	splx(oldpri);
}

/*	Set an interval timer. 
*/

void
itimer_timeout(hrp, type, base_lbolt, clock)
register timer_t	*hrp;
register		type;
register ulong		base_lbolt;
int			clock;
{
	register timer_t	*nhrp;
	register proc_t		*pp;
	register ulong		fudge;
	register int		oldpri;

	pp = u.u_procp;

	/*	Find where the new entry belongs in the time ordered
	**	list.  Fix up the time as we go.
	*/

	oldpri = splhi();
	if (base_lbolt && clock == CLK_PROCVIRT) {
		fudge = hr_lbolt - base_lbolt;
		if (fudge >= hrp->hrt_time)
			hrp->hrt_time = 1;
		else
			hrp->hrt_time -= fudge;
	}

	nhrp = pp->p_italarm[type];

	if (nhrp == NULL) {
		pp->p_italarm[type] = hrp;
		hrp->hrt_prev = hrp;
		hrp->hrt_next = NULL;
		splx(oldpri);
		return;
	}	

	while(nhrp->hrt_time <= hrp->hrt_time){
		hrp->hrt_time -= nhrp->hrt_time;
		if(nhrp->hrt_next == NULL) {
			nhrp->hrt_next = hrp;
			hrp->hrt_prev = nhrp;
			hrp->hrt_next = NULL;
			splx(oldpri);
			return;
		}
		nhrp = nhrp->hrt_next;
	}

	nhrp->hrt_time -= hrp->hrt_time;
	itimer_enqueue(hrp, nhrp, type);
	splx(oldpri);
}

/*	Do the hrtcancel function when all timers for the calling
**	process have been requested.
*/


int
hrt_cancel_proc()
{
	register timer_t	*hrp;
	register timer_t	*nhrp;
	register proc_t		*pp;
	register int		cnt;
	register int		oldpri;
	long			time;
	int			type;

	pp	= u.u_procp;
	cnt	= 0;
	type	= 0;
	oldpri	= splhi();

	while (type < 2) {

		time	= 0;
		hrp = nhrp = pp->p_italarm[type];

		for(; hrp != NULL; hrp = nhrp) {
			nhrp = hrp->hrt_next;
			time += hrp->hrt_time;
			cnt++;
			if (nhrp != NULL)
				nhrp->hrt_time += hrp->hrt_time;
			hrp->hrt_time    = time;
			itimer_dequeue(hrp, type);
			hrt_free(hrp, &it_avail);
		}
		pp->p_italarm[type] = NULL;
		type++;
	}

	/*	Loop through all of the active timer requests looking
	**	for those which were specified by the calling process.
	*/

	time = 0;
	hrp	= hrt_active.hrt_next;
	for( ; hrp != &hrt_active ; hrp = nhrp){
		nhrp = hrp->hrt_next;
		time += hrp->hrt_time;
		if(hrp->hrt_proc == pp){

			/*	Before we take the entry off of
			**	the list, add its time to the
			**	following entry so that it's time
			**	will stay correct.  Then fix up
			**	the time field of the entry we
			**	are deleteing to be the time from
			**	now when it would have fired.  
			**	This is so that the event generated
			**	will have the correct time in it.
			*/

			cnt++;
			nhrp->hrt_time += hrp->hrt_time;
			hrp->hrt_time   = time;
			hrt_dequeue(hrp);
			hrt_free(hrp, &hrt_avail);
		}
	}
	splx(oldpri);
	return(cnt);
}

/*
 * Cancel BSD timers 
 */ 

static int
hrt_bsd_cancel(clock)
int	clock;
{
	register timer_t	*hrp;
	register timer_t	*nhrp;
	register proc_t		*pp = u.u_procp; 
	int			oldpri;
	int			time;
	int			type;

	oldpri = splhi();

	if (clock == CLK_STD) {

		hrp = nhrp = hrt_active.hrt_next;
		time = 0;

		for( ; hrp != &hrt_active ; hrp = nhrp){
			nhrp = hrp->hrt_next;
			time += hrp->hrt_time;
			if ( (hrp->hrt_cmd == HRT_BSD || 
				hrp->hrt_cmd == HRT_RBSD) &&
				  hrp->hrt_proc == pp ) {
				nhrp->hrt_time += hrp->hrt_time;
				hrp->hrt_time   = time;
				hrt_dequeue(hrp);
				hrt_free(hrp, &hrt_avail);
			}
		}
		splx(oldpri);
		return(0);
	}
	else if (clock == CLK_USERVIRT)
		type = 0;
	else
		type = 1;

	hrp = pp->p_italarm[type];
	time = 0;

	for(; hrp !=NULL; hrp = nhrp) {
		nhrp = hrp->hrt_next;
		time += hrp->hrt_time;
		if ( (hrp->hrt_cmd == HRT_BSD || 
			hrp->hrt_cmd == HRT_RBSD) &&
			  hrp->hrt_proc == pp ) {
			if (nhrp != NULL)
				nhrp->hrt_time += hrp->hrt_time;
			hrp->hrt_time   = time;
			itimer_dequeue(hrp, type);
			hrt_free(hrp, &it_avail);
		}
	}
	splx(oldpri);
	return(0);
}

/*
** Do the HRT_PENDING command when the event identifier is specified.
**
 */

static int
hrt_match_tid(cmd, clock, alarm_typep, htp)
int	cmd;
int	clock;
int	*alarm_typep;
register hrtime_t   *htp;
{
	register timer_t	*hrp;
	register timer_t	*nhrp;
	long			time_to_fire;
	int			oldpri;
	int			type;
	ulong			user_res;
	proc_t			*pp;

	pp = u.u_procp;
	if (clock == CLK_USERVIRT)
		type = 0;
	else if (clock == CLK_PROCVIRT)
		type = 1;
	else
		type = 2;
	htp->hrt_secs = 0;
	htp->hrt_rem  = 0;
	time_to_fire = 0;


	oldpri = splhi();

	if (clock == CLK_STD) {
		hrp = nhrp = hrt_active.hrt_next;

		for ( ; hrp != &hrt_active ; hrp = nhrp) {
			nhrp = hrp->hrt_next;
			time_to_fire += hrp->hrt_time;
			if ( (cmd == HRT_BSD_PEND &&
			       (hrp->hrt_cmd == HRT_BSD ||
 				  hrp->hrt_cmd == HRT_RBSD) &&
					 hrp->hrt_proc == pp) ) {
				*alarm_typep = hrp->hrt_cmd;
				user_res = hrp->hrt_res;
				htp->hrt_res  = timer_resolution;
				if (hrp->hrt_cmd != HRT_TODALARM) {
					htp->hrt_secs = 0;
					htp->hrt_rem  = time_to_fire;
				} /* if then */
				else { 
					htp->hrt_secs = hrestime.tv_sec;
					htp->hrt_rem  = hr_lbolt % 
							  timer_resolution +
					 		  time_to_fire;
				} /* else */
				splx(oldpri);
				(void)hrt_newres(htp, user_res, HRT_RND);
				return(0);
			} /* end if */
		} /* end for */
	} /* end if (clock = CLK_STD) */
	/*
	**	Search the process lists for pending alarms.
	**
	 */
	if (type != 2) {

		hrp = pp->p_italarm[type];

		for(; hrp != NULL; hrp = nhrp) {
			nhrp = hrp->hrt_next;
			time_to_fire += hrp->hrt_time;	
			if ( (cmd == HRT_BSD_PEND &&
			       (hrp->hrt_cmd == HRT_BSD ||
 				  hrp->hrt_cmd == HRT_RBSD) &&
					 hrp->hrt_proc == pp) ) {
				*alarm_typep = hrp->hrt_cmd;
				user_res = hrp->hrt_res;
				htp->hrt_res  = timer_resolution;
				htp->hrt_secs = 0;
				htp->hrt_rem  = time_to_fire;

				splx(oldpri);
				(void)hrt_newres(htp, user_res, HRT_RND);
				return(0);
			} /* end if */
		} /* end for */

	} /* end while */

	splx(oldpri);
	return(EDOM);
}


/*
 * If the ECBF_LATEEER flag is set return EINVAL.
 * otherwise, post an event
 */


static int
hrt_past_time(cmd, res)
register int	cmd;
register ulong	res;
{
	register timer_t	*hrp;
	register vnode_t	*vp;
	timer_t			hrtime;
	int			error;

	hrp = &hrtime;


	error = hrt_setup(hrp, cmd, CLK_STD, res);
	if (error)
		return error;

	error = hrp->hrt_fn(hrp, 0, 0);

	vp = hrp->hrt_vp;
	if (vp != NULL) {
		VN_RELE(vp);
	}

	return(error);
}

/*
 * This routine will be used to implement BSD timers.
 */

/* ARGSUSED */
static int
hrt_sndsignal(hrp, arg1, arg2)
register timer_t	*hrp;
int			arg1, arg2;
{
	if (hrp->hrt_clk == CLK_STD)
		psignal(hrp->hrt_proc, SIGALRM);
	else if (hrp->hrt_clk == CLK_USERVIRT)
		psignal(hrp->hrt_proc, SIGVTALRM);
	else if (hrp->hrt_clk == CLK_PROCVIRT)
		psignal(hrp->hrt_proc, SIGPROF);
	return(0);
}


/*	Initialize an hrtime structure for a user request.
*/


static int
hrt_setup(hrp, cmd, clk, res)
register timer_t	*hrp;
register int		cmd;
register int		clk;
register ulong		res;
{
	register vnode_t	*vp;
	int			error;
	vnode_t			*rvp;

	vp = NULL;

	hrp->hrt_proc	= u.u_procp;
	hrp->hrt_clk	= clk;
	hrp->hrt_res	= res;
	hrp->hrt_cmd	= (ushort)cmd;
	hrp->hrt_vp	= vp;
	if (cmd == HRT_BSD || cmd == HRT_RBSD) {
		hrp->hrt_fn = hrt_sndsignal;
		return 0;
	}
	return 0;
}

/*
**	Return 0 if "res" is a legal resolution. Otherwise,
**	return an error code, ERANGE.
 */


static int
hrt_checkres(res)
ulong	res;
{
	if (res <= 0 || res > NANOSEC)
		return ERANGE;
	else
		return 0;
}

/*
**	Return 0 if "clock" is a valid clock. Otherwise,
**	return an error code, EINVAL.
 */

static int
hrt_checkclock(clock)
register	clock;
{
	if (clock != CLK_STD && clock != CLK_USERVIRT &&
			clock != CLK_PROCVIRT)
		return EINVAL;
	else
		return 0;
}

/*
**	Convert the high-resolution time to HZ and return it
**	in htp->hrt_rem. Return EDOM if the value of
**	htp->hrt_sec plus htp->hrt_rem convert to more HZ
**	than will fit in the htp->hrt_rem. Otherwise, return
**	zero.
**	Round the conversion according to "rnd" which is either
**	HRT_RND or HRT_TRUNC.
 */

int
hrt_tohz(htp, rnd)
hrtime_t	*htp;
int		rnd;
{
	long	result;
	int	error;

	if ((error = hrt_newres(htp, HZ, rnd)))
		return(error);

	if ((result = hrt_convert(htp)) == -1)
		return(EDOM);

	htp->hrt_secs = 0;
	htp->hrt_rem  = result;
	return(0);
}

/*	Set the current time of day in a specified resolution into
**	a hrtime_t structure.
*/

static void
hrt_gettofd(td)
register hrtime_t	*td;
{
	register int	oldpri;
	register ulong	new_res;

	oldpri = splhi();	/* Make sure we get a	*/
				/* consistent time	*/
				/* sample.		*/
	/* save time here  */
	/* since we might page fault */
	/* or loose time in the      */
	/* conversion routines	     */

	td->hrt_secs = hrestime.tv_sec;
	td->hrt_rem  = hrestime.tv_nsec / TICK;
	splx(oldpri);

	new_res = td->hrt_res;
	td->hrt_res = timer_resolution;

	if(new_res != td->hrt_res){
		hrt_newres(td, new_res, HRT_TRUNC); 
	}
}

/*	Convert "htp" from resolution "htp->hrt_res" resolution
**	to new resolution. Round using "rnd" which is either
**	HRT_RND or HRT_TRUNC. Change "htp->hrt_res" to be
**	"new_res".
**
**	Calculate: (interval * new_res) / htp->hrt_res  rounding off as
**		specified by rnd.
**
**	Note:	All args are assumed to be positive.  If
**	the last divide results in something bigger than
**	a long, then ERANGE is returned instead,
**	othewise, zero is returned.
*/

int
hrt_newres(htp, new_res, rnd)
register hrtime_t	*htp;
register ulong		new_res;
register int		rnd;
{
	register long  interval;
	dl_t		dint;
	dl_t		dto_res;
	dl_t		drem;
	dl_t		dfrom_res;
	dl_t		prod;
	dl_t		quot;
	register long	numerator;
	register long	result;
	ulong		modulus;
	ulong		twomodulus;
	long		temp;
	int error;

	if (error = hrt_checkres(new_res))
		return(error);

	if (htp->hrt_rem >= htp->hrt_res) {
		htp->hrt_secs += htp->hrt_rem / htp->hrt_res;
		htp->hrt_rem = htp->hrt_rem % htp->hrt_res;
	}

	interval = htp->hrt_rem;
	if (interval == 0) {
		htp->hrt_res = new_res;
		return(0);
	}

	/*	Try to do the calculations in single precision first
	**	(for speed).  If they overflow, use double precision.
	**	What we want to compute is:
	**
	**		(interval * new_res) / hrt->hrt_res
	*/

	numerator = interval * new_res;

	if (numerator / new_res  ==  interval) {
			
		/*	The above multiply didn't give overflow since
		**	the division got back the original number.  Go
		**	ahead and compute the result.
		*/
	
		result = numerator / htp->hrt_res;
	
		/*	For HRT_RND, compute the value of:
		**
		**		(interval * new_res) % htp->hrt_res
		**
		**	If it is greater than half of the htp->hrt_res,
		**	then rounding increases the result by 1.
		**
		**	For HRT_RNDUP, we increase the result by 1 if:
		**
		**		result * htp->hrt_res != numerator
		**
		**	because this tells us we truncated when calculating
		**	result above.
		**
		**	We also check for overflow when incrementing result
		**	although this is extremely rare.
		*/
	
		if (rnd == HRT_RND) {
			modulus = numerator - result * htp->hrt_res;
			if ((twomodulus = 2 * modulus) / 2 == modulus) {

				/*
				 * No overflow (if we overflow in calculation
				 * of twomodulus we fall through and use
				 * double precision).
				 */
				if (twomodulus >= htp->hrt_res) {
					temp = result + 1;
					if (temp - 1 == result)
						result++;
					else
						return(ERANGE);
				}
				htp->hrt_res = new_res;
				htp->hrt_rem = result;
				return(0);
			}
		} else if (rnd == HRT_RNDUP) {
			if (result * htp->hrt_res != numerator) {
				temp = result + 1;
				if (temp - 1 == result)
					result++;
				else
					return(ERANGE);
			}
			htp->hrt_res = new_res;
			htp->hrt_rem = result;
			return(0);
		} else {	/* rnd == HRT_TRUNC */
			htp->hrt_res = new_res;
			htp->hrt_rem = result;
			return(0);
		}
	}
	
	/*	We would get overflow doing the calculation is
	**	single precision so do it the slow but careful way.
	**
	**	Compute the interval times the resolution we are
	**	going to.
	*/

	dint.dl_hop	= 0;
	dint.dl_lop	= interval;
	dto_res.dl_hop	= 0;
	dto_res.dl_lop	= new_res;
	prod		= lmul(dint, dto_res);

	/*	For HRT_RND the result will be equal to:
	**
	**		((interval * new_res) + htp->hrt_res / 2) / htp->hrt_res
	**
	**	and for HRT_RNDUP we use:
	**
	**		((interval * new_res) + htp->hrt_res - 1) / htp->hrt_res
	**
	** 	This is a different but equivalent way of rounding.
	*/

	if (rnd == HRT_RND) {
		drem.dl_hop = 0;
		drem.dl_lop = htp->hrt_res / 2;
		prod	    = ladd(prod, drem);
	} else if (rnd == HRT_RNDUP) {
		drem.dl_hop = 0;
		drem.dl_lop = htp->hrt_res - 1;
		prod	    = ladd(prod, drem);
	}

	dfrom_res.dl_hop = 0;
	dfrom_res.dl_lop = htp->hrt_res;
	quot		 = ldivide(prod, dfrom_res);

	/*	If the quotient won't fit in a long, then we have
	**	overflow.  Otherwise, return the result.
	*/

	if (quot.dl_hop != 0) {
		return(ERANGE);
	} else {
		htp->hrt_res = new_res;
		htp->hrt_rem = quot.dl_lop;
		return(0);
	}
}


/*
**	Convert "htp" to htp->hrt_res. Return the result.
**/

static long
hrt_convert(htp)
register hrtime_t	*htp;
{
	register long	sum;
	register long	product;

	product = htp->hrt_secs * htp->hrt_res;

	if (product / htp->hrt_res == htp->hrt_secs) {
		sum = product + htp->hrt_rem;
		if (sum - htp->hrt_rem == product) {
			return(sum);
		}
	}
	return(-1);
}



/*	Initialize the hrtimes array.  Called from startup.
*/

void
hrtinit()
{
	register int	cnt;

	for(cnt = 0 ; cnt < hrtimes_size ; cnt++){
		hrtimes[cnt].hrt_next = &hrtimes[cnt + 1];
		hrtimes[cnt].hrt_prev = &hrtimes[cnt - 1];
	}

	hrtimes[0].hrt_prev = &hrt_avail;
	hrtimes[cnt - 1].hrt_next = &hrt_avail;
	hrt_avail.hrt_next = &hrtimes[0];
	hrt_avail.hrt_prev = &hrtimes[cnt - 1];

	hrt_active.hrt_next = &hrt_active;
	hrt_active.hrt_prev = &hrt_active;
}

/*	Initialize the itimes array.  Called from startup.
*/

void
itinit()
{
	register int	cnt;

	for(cnt = 0 ; cnt < itimes_size ; cnt++){
		itimes[cnt].hrt_next = &itimes[cnt + 1];
		itimes[cnt].hrt_prev = &itimes[cnt - 1];
	}

	itimes[0].hrt_prev = &it_avail;
	itimes[cnt - 1].hrt_next = &it_avail;
	it_avail.hrt_next = &itimes[0];
	it_avail.hrt_prev = &itimes[cnt - 1];

}

/*	Allocate a timer_t structure.
*/

static timer_t	*
hrt_alloc(free_list)
register timer_t	*free_list;
{
	register timer_t	*hrp;
	register int		oldpri;

	oldpri = splhi();
	hrp = free_list->hrt_next;

	if(hrp == free_list){
		splx(oldpri);
		cc_limiter(&cc_re_timers, u.u_cred);
		return(NULL);
	}

	hrp->hrt_next->hrt_prev = free_list;
	free_list->hrt_next = hrp->hrt_next;
	splx(oldpri);
	*hrp = hrt_null;
	return(hrp);
}

/*	Free an timer_t structure.
*/

void
hrt_free(hrp, free_list)
register timer_t	*hrp;
register timer_t	*free_list;
{
	register vnode_t	*vp;
	register int		oldpri;

	vp = hrp->hrt_vp;
	if(vp != NULL){
		VN_RELE(vp);
	}
	oldpri = splhi();
	hrp->hrt_next = free_list->hrt_next;
	hrp->hrt_prev = free_list;
	free_list->hrt_next->hrt_prev = hrp;
	free_list->hrt_next = hrp;
	splx(oldpri);
}

/*	Enqueue a new hrtime structure (hrp) in front of an
**	existing one (ohrp) on the active list.
*/

static void
hrt_enqueue(hrp, nhrp)
register timer_t	*hrp;
register timer_t	*nhrp;
{
	register int	oldpri;

	oldpri = splhi();
	hrp->hrt_next		 = nhrp;
	hrp->hrt_prev		 = nhrp->hrt_prev;
	nhrp->hrt_prev->hrt_next = hrp;
	nhrp->hrt_prev		 = hrp;
	splx(oldpri);
}

/*	Enqueue a new hrtime structure (hrp) in front of an
**	existing one (ohrp) on the appropriate list.
*/

static void
itimer_enqueue(hrp, nhrp, type)
register timer_t	*hrp;
register timer_t	*nhrp;
register		type;
{
	register int	oldpri;
	register proc_t *pp = u.u_procp;	

	oldpri = splhi();

	if (nhrp == nhrp->hrt_prev) {
		pp->p_italarm[type] = hrp;
		hrp->hrt_next = nhrp;
		hrp->hrt_prev = hrp;
		nhrp->hrt_prev = hrp;
		splx(oldpri);
		return;
	}

	hrp->hrt_next		 = nhrp;
	hrp->hrt_prev		 = nhrp->hrt_prev;
	nhrp->hrt_prev->hrt_next = hrp;
	nhrp->hrt_prev		 = hrp;
	splx(oldpri);
}
/*	Unlink an timer_t structure from the doubly linked list
**	it is on.
*/

void
hrt_dequeue(hrp)
register timer_t	*hrp;
{
	register int	oldpri;

	oldpri = splhi();
	hrp->hrt_prev->hrt_next = hrp->hrt_next;
	hrp->hrt_next->hrt_prev = hrp->hrt_prev;
	splx(oldpri);
}

/*
**	Unlink an timer_t structure from the doubly linked list
**	it is on. The list starts from the proc table.
*/

void
itimer_dequeue(hrp, type)
register timer_t	*hrp;
register int		type;
{
	register int	oldpri;
	register proc_t	*pp;

	pp = u.u_procp;

	oldpri = splhi();

	/* hrp the first in the list */

	if (hrp == hrp->hrt_prev) {
		pp->p_italarm[type] = hrp->hrt_next;
		if (hrp->hrt_next != NULL) {
			hrp->hrt_next->hrt_prev = hrp->hrt_next;
		}
	splx(oldpri);
	return;
	}	

	/* hrp the last entry in the list */

	if (hrp->hrt_next == NULL) {
		hrp->hrt_prev->hrt_next = NULL;
		splx(oldpri);
		return;
	}

	hrp->hrt_prev->hrt_next = hrp->hrt_next;
	hrp->hrt_next->hrt_prev = hrp->hrt_prev;

	splx(oldpri);

}

/*
 * System entry point for hrtcntl and hrtalarm system calls.
 */

int
hrtsys(uap, rvp)
	register struct	hrtsysa *uap;
	rval_t *rvp;
{
	register int	error;

	switch (uap->opcode) {
	case	HRTCNTL:
		error = hrtcntl((struct hrtcntla *)uap, rvp);
		break;
	case	HRTALARM:
		error = hrtalarm((struct hrtalarma *)uap, rvp);
		break;
	default:
		error = EINVAL;
		break;
	}

	return error;
}
