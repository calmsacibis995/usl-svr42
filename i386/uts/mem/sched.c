/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:mem/sched.c	1.14"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <proc/proc.h>
#include <svc/systm.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/var.h>
#include <mem/as.h>
#include <mem/tuneable.h>
#include <mem/vmsystm.h>
#include <util/inline.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <util/fp.h>
#include <util/mod/mod_k.h>
#include <util/sysmacros.h>
#include <mem/page.h>

/*
 * The scheduler sleeps on runout when there is no one to swap in.
 * It sleeps on runin when it could not find space to swap someone
 * in or after swapping someone in.
 */

char	runout;
char	runin;

static int swapout();

ulong	paging_rate;
extern	int	deficit;
extern int avefree;

/*
 * The following macro, IS_SIGNALLED, is used to determine whether a process
 * has received any signals that it neither blocks nor ignores. It is built
 * around the assumption that k_sigset_t is an unsigned long quantity.
 * If k_sigset_t is not a u_long quantity, then the macro must change
 * accordingly.
 *
 * XXX - must change if k_sigset_t in changed from being ulong
 */

#define IS_SIGNALLED(p) ((p)->p_sig & ~(((p)->p_hold) | ((p)->p_ignore)))
  
/*
 * Memory scheduler daemon.
 */
sched()
{
	register proc_t		*pp; 
	register class_t	*clp;
	register int		maxpri;
	register int		minpri;
	register int		i;
	proc_t			*justloaded = NULL;
	boolean_t		unloadok;
	size_t			savedrss;

	int	min_mem_needed;


	for ( ; ; ) {

		(void) spl0();
		cleanup();			/* process the bclnlist */
		(void) splhi();
		pp = NULL;

		/*
		 * See if there is memory enough to bring in another process.
		 */
		if (freemem > minfree + USIZE) {

			/*
			 * Call the class specific swapin() functions to each
			 * nominate up to one process to bring in.  Select
			 * nominee with highest priority.  (Skip the sys
			 * class, since those processes aren't swapped.)
			 */
			maxpri = -1;
			for (clp = &class[1]; clp < &class[nclass]; clp++) {
				proc_t *rp = NULL;
				int runflag = 0;	/* dummy arg */

				CL_SWAPIN(clp, freemem, &rp, &runflag);
				if (rp != NULL && rp->p_pri > maxpri) {
					pp = rp;
					maxpri = rp->p_pri;
				}
			}

			/*
			 * If there is no nominated process we wait.
			 */
			if (maxpri == -1) {
				runout++;
				justloaded = NULL;
				sleep((caddr_t)&runout, PSWP);
				continue;
			}

			/*
			 * Is freemem > (low threshold + deficit + needs of 
		 	 * this process) ? Then go ahead and bring it in.
			 * Otherwise:
			 * 1. does the process have unblocked and unignored 
			 *	signals pending? bring it in unconditionally;
			 * 2. Is the system paging rate too low? OR
			 * 3. has the process been out very long? bring it 
			 *	in if the current deficit can be met.
			 * The process memory needs are estimated to be
			 * half the process RSS, as a first order approximation
			 * to account for sharing. This is somewhat arbitrary,
			 * but the best we can do without knowing the private
			 * component of process working set.
			 */


			min_mem_needed = USIZE + minfree + deficit;
			if ((min_mem_needed + pp->p_swrss) > freemem) {
				if ((freemem < min_mem_needed) ||
				    ((pp->p_inoutage < MAXINOUTAGE) &&
				     (paging_rate > maxpgio/4))) {

					if (IS_SIGNALLED(pp) == B_FALSE) {
						if (avefree < desfree)
							goto unload;
						goto delay;
					}
				} 
			} 

			ub_lock(pp);
			(void) spl0();
			i = swapinub(pp);
			(void) splhi();
			ub_rele(pp);

			if (i == 0)
				goto unload;	/* couldn't swap in u-block */
			/*
			 * Bump the deficit by half the process RSS size
			 * saved when it was swapped out. The halving
			 * is in order to reflect approximately the
			 * effect of sharing text pages.
			 */
			if ((deficit += pp->p_swrss) > (4 * lotsfree)) {
				deficit = (4*lotsfree);
			}
			vminfo.v_swpin++;
			vminfo.v_pswpin += USIZE;
			pp->p_flag |= SLOAD;
			pp->p_inoutage = 0; 	/* time in core to 0 */
			if (pp->p_stat == SRUN && (pp->p_flag & SPROCIO) == 0)
				dq_sruninc(pp->p_pri);

			justloaded = pp;

			if (freemem > tune.t_gpgslo + USIZE + deficit) {
				continue;
			} else {
				/*
				 * We just successfully brought in a process,
				 * now delay.  (To fall through here could 
				 * cause an infinite loop of swapins and
				 * swapouts involving 2 or more processes.)
				 */
				 goto delay;
			}
		} 

unload:
		/*
		 * Try to unload unused loadable modules. If enough memory
		 * is freed by this routine, we don't need to swap out any
		 * process.
		 */
		if (unload_modules(B_TRUE, lotsfree))
			continue;
			

		/*
		 * Call the class specific swapout() functions to each nominate
		 * one process to swap out.  Select the nominee with lowest
		 * priority.  (Skip the sysclass as those processes aren't 
		 * swapped.)
		 */
	 
		minpri = INT_MAX;
		for (clp = &class[1]; clp < &class[nclass]; clp++) {
			proc_t *rp = NULL;
			boolean_t ulok = B_TRUE;

			CL_SWAPOUT(clp, freemem, justloaded, &rp, &ulok);
			if (rp != NULL && rp->p_pri < minpri) {
				pp = rp;
				minpri = rp->p_pri;
				unloadok = ulok;
			}
		}

		/*
		 * If we have a valid nominee try to swap it out.  If
		 * CL_SWAPOUT set unloadok to B_FALSE we will swap pages but
		 * won't swap out the u-block or make process "unrunnable".
		 * If we can't swap process out, or no class nominated a
		 * process, wait a bit and try again.
		 */
		if (minpri != INT_MAX && (pp->p_flag &
		  (SLOAD|SSYS|SLOCK|SUSWAP|SPROCIO|SSWLOCKS)) == SLOAD) {
			if (unloadok == B_TRUE) {
				if (pp->p_stat == SRUN)
					dq_srundec(pp->p_pri);
				pp->p_flag &= ~SLOAD;
			}

			/*
			 * For an NFS process, p_as can be null and 
			 * the process still in user mode.
			 *
			 * Update swrss to be a_rss/2 for lack of more
			 * accurate information. Since it is all relative
			 * for choosing processes to swap in, it should not
			 * matter that much.
			 */

			if (pp->p_as)
				pp->p_swrss = pp->p_as->a_rss / 2;
			else 
				pp->p_swrss = 0;

			ub_lock(pp);
			(void) spl0();
			i = swapout(pp, unloadok);
			ub_rele(pp);
			if (i != 0) {

				/*
				 * Process successfully swapped out, now we
				 * may be able to bring another in.
				 */
				goto delay;
			} else if (unloadok == B_TRUE) {
				(void) splhi();
				pp->p_flag |= SLOAD;
				/*
				 * We just "shredded" this process's pages,
				 * though we did not swap it out. Reflect
				 * that by bumping the deficit, since we
				 * effectively created a short term memory 
				 * demand as this process will fault in much
				 * of its working set.
				 */
				if ((deficit += pp->p_swrss) > (4 * lotsfree)) {
					deficit = (4 * lotsfree);
				}
				if (pp->p_stat == SRUN &&
				  (pp->p_flag & SPROCIO) == 0)
					dq_sruninc(pp->p_pri);
			}
		}

delay:
		/*
		 * Delay for 1 second and look again later.
		 */
		runin++;
		justloaded = NULL;
		sleep((caddr_t)&runin, PSWP);
	}
}

/*
 * Swap out process p.
 */
static int
swapout(p, ubswapok)
	register proc_t	*p;
	boolean_t	ubswapok;
{
	register int rtn;

	as_swapout(p->p_as);
	vminfo.v_swpout++;
	vminfo.v_pswpout += p->p_swrss;
	p->p_inoutage = 0;		/* time on swap device to 0 */

	if (ubswapok == B_FALSE)
		return 1;
 
	if (p->p_flag & SPROCIO)
		return 0;
	/*
	 * If the process we are swapping owns the floating
	 * point unit, save its state.
	 */
	if (fp_proc == p)
		fpsave();
 
	rtn = swapoutub(p);

	if (rtn)
		vminfo.v_pswpout += USIZE;

	return rtn;
}
