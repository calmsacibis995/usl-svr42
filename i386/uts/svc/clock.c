/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)uts-x86:svc/clock.c	1.19"
#ident	"$Header: $"

#include <mem/anon.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/rm.h>
#include <mem/tuneable.h>
#include <mem/vmsystm.h>
#include <proc/class.h>
#include <proc/tss.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/callo.h>
#include <svc/hrtcntl.h>
#include <svc/resource.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

#ifdef ATT_EISA
#include <svc/pit.h>
#endif

extern int sanity_clk;		/* tunable SANITYCLK */
extern int eisa_bus;		/* set in main for presence of eisa bus */

#define	PRF_ON	01
unsigned	prfstat;	/* state of profiler */
extern int	fsflush();	/* daemon to flush cached data */

/*
 * For the adjtime(2) mechanism.
 * The clock "drift" rate that we will use depends on the total amount
 * of adjustment that remains to be applied.  For adjustments of BIGADJ
 * or less (defined below to be 60 seconds) we skew each tick by SKEW
 * microseconds; (8000/HZ) amounts to 8 milliseconds of drift each second.
 * For larger adjustments the initial drift rate is ten times that.
 * The recommended maximum BIGSKEW is (500000/HZ), which gives a 50% drift
 * rate.  The absolute maximum BIGSKEW allowable is (1000000/HZ), which
 * would "stop the clock" for negative adjustments; anything greater would
 * attempt to run the clock backwards.
 */
STATIC long	timedelta;	/* unapplied time correction, in microsec */
#define BIGADJ	(60*MICROSEC)	/* timedelta is considered "big" if > BIGADJ */
#define SKEW	(8000/HZ)	/* standard clock skew per tick, in microsec */
#define BIGSKEW	(10*SKEW)	/* clock skew for bigger adjustments */

extern int	idleswtch;	/* flag set while idle in pswtch() */

extern int (*io_poll[])();	/* driver entry points to poll every tick */

time_t	time;		/* time in seconds since 1970 */
			/* is here only for compatibility */
timestruc_t hrestime;	/* time in seconds and nsec since since 1970 */

/* Enhanced Application Compatibility Support */
#define LTICKS_MAX 50
int lticks = LTICKS_MAX;/* counts from LTICKS_MAX down to 0 on each clock tick
			 * This variable was used in 3.2 and removed in 4.0.
			 * It is being reintroduced so that some 3.2 apps.
			 * can run unmodified (ie. the Rabbit stuff).  Its full
			 * functionality/semantics have *NOT* been restored.
			 */
/* End Enhanced Application Compatibility Support */

clock_t	lbolt;		/* time in HZ since last boot */

#ifdef  VPIX
extern int v86timer;    /* Current v86 timer value */
extern int vpixenable;	/* Is VP/ix enabled or disabled */
#endif	/* VPIX */

static int one_sec = 1;	/* flag set by BUMPTIME if a second has passed */
STATIC int fsflushcnt;	/* counter for t_fsflushr */

#define BASEPRI (oldipl)        /* true if servicing another interrupt  */

int dotimein;                   /* flag that triggers call to timein()  */
#define timepoke() (dotimein++) /* set dotimein flag                    */

STATIC int calllimit	= -1;	/* index of last valid entry in table */

/*
 * Increment the time by a small number of nanoseconds;
 * set flag if we pass a one-second boundary.
 */
#define BUMPTIME(nsec, flag)				\
	if ((hrestime.tv_nsec += (nsec)) >= NANOSEC) {	\
		hrestime.tv_nsec -= NANOSEC;		\
		hrestime.tv_sec++;			\
		flag = 1;				\
	} else	/* caller supplies trailing semicolon */

/*
 * Kludge for SVID-compliance is preferable to allocating the
 * structure in generic code.  rf_init points this at a
 * counter.
 */
time_t	*rfsi_servep;

#ifdef DEBUG
STATIC int catchmenowcnt;	/* counter for debugging interrupt */
STATIC int catchmestart = 60;	/* counter for debugging interrupt */
STATIC int idlemsg;
extern int idlecntdown;
#endif

#ifdef KPERF
extern proc_t *curproc;
#endif

/*
 * counter of the number of processess running during the last
 * second. Used for scheduling classes, currently only the fixed
 * class that care.
 */

int number_current_processes;

/*
 * Handling routine for normal clock interrupts.
 *
 * clock() is called straight from
 * the real time clock interrupt.
 *
 * Functions:
 *	reprime clock
 *	implement callouts
 *	maintain user/system times
 *	maintain date
 *	profile
 *	alarm clock signals
 *	jab the scheduler
 */

int
clock(pc, cs, flags, oldipl)
caddr_t pc;
int cs;
int flags;
int oldipl;
{
	extern	void	clkreld();
	register struct proc *pp;
	register int	retval, i;
	register rlim_t rlim_cur;
	static rqlen, sqlen;
	extern int (*io_poll[])();
#ifdef ATT_EISA
	int sanity_ctl	 = SANITY_CTL;		/* EISA santity ctl word */
	int sanity_ctr0	 = SANITY_CTR0;		/* EISA santity timer */
	int sanity_mode = PIT_C0|PIT_ENDSIGMODE|PIT_READMODE ;
	unsigned int sanitynum = SANITY_NUM;	/* interval for sanitytimer */
	int s;
	char byte;
#endif

	retval = 0;

	/*
	 * If panic, clock should be stopped.
	 */
	ASSERT(panicstr == NULL);

	/*
	 * XENIX Compatibility Change:
	 *  Call the device driver entries for poll on clock ticks,
	 *  if there are any.  This table (io_poll) is created by
	 *  "cunix" for drivers that contain a "poll" routine.
	 */
	for (i=0; io_poll[i]; i++)
		(*io_poll[i])(oldipl);

	/*
	 * Service timeout() requests if any are due at this time.
	 * This code is simpler than the original array-based callout
	 * table since we are using absolute times in the table.
	 * No need to decrement relative times; merely see if the first
	 * (earliest) entry is due to be called.
	 */

	if ((calllimit >= 0) && (callout[0].c_time <= lbolt))
		timepoke();

	if (prfstat & PRF_ON)
		prfintr((u_int)pc, USERMODE(cs));
	pp = u.u_procp;
	if ((flags & PS_VM) || USERMODE(cs)) {
		sysinfo.cpu[CPU_USER]++;
		pp->p_utime++;
		if (u.u_prof.pr_scale & ~1)
			retval = 1;
	} else {
		if (SYSIDLE(pc)) {
			if (syswait.iowait+syswait.swap+syswait.physio) {
				sysinfo.cpu[CPU_WAIT]++;
				if (syswait.iowait)
					sysinfo.wait[W_IO]++;
				if (syswait.swap)
					sysinfo.wait[W_SWAP]++;
				if (syswait.physio)
					sysinfo.wait[W_PIO]++;
			} else {
				sysinfo.cpu[CPU_IDLE]++;
			}
		} else {
			sysinfo.cpu[CPU_KERNEL]++;
			pp->p_stime++;
			if (rfsi_servep && RF_SERVER())
				(*rfsi_servep)++;
		}
	}

	if (pp->p_stat == SONPROC) {
		/*
		 * Update memory usage for the currently running process.
		 */
		u.u_mem = rm_asrss(pp->p_as);

		/*
		 * Call the class specific function to do its 
	 	 * once-per-tick processing for the current process.
	 	 */
		CL_TICK(pp, pp->p_clproc);
	}

	if (idleswtch == 0 && pp->p_cpu < 80)
		pp->p_cpu++;

	lbolt++;	/* time in ticks */

#ifdef ATT_EISA		/* EISA specific hooks */

	/* If EISA machine and sanity tunable on, reprime sanity timer*/
	if (eisa_bus && sanity_clk) {
		s = splhi();
		outb(sanity_ctl, sanity_mode);
		byte = sanitynum;
		outb(sanity_ctr0, byte);
		byte = sanitynum>>8;
		outb(sanity_ctr0, byte);
		splx(s);
	}
#endif

#ifdef  VPIX
	if (vpixenable && (--v86timer <= 0))   /* Count ticks for v86 process */
		v86timerint();          /* Timer interrupts for V86 tasks */
#endif

	/*
	 * "double" long arithmetic for minfo.mi_freemem.
	 */
	if (!BASEPRI) {
		unsigned long ofrmem;

		ofrmem = minfo.mi_freemem[0];
		minfo.mi_freemem[0] += freemem;
		if (minfo.mi_freemem[0] < ofrmem)
			minfo.mi_freemem[1]++;
	}
	
	/*
	 * Increment the time-of-day.
         */
	if (timedelta == 0)
		BUMPTIME(TICK, one_sec);
	else {
		/*
		 * Drift the clock.
		 * A negative adjustment means we want more ticks per second,
		 * because we want the clock to advance more slowly;
		 * a positive adjustment means the opposite.  The difference
		 * is reflected in the sign of delta.
		 */
		register long delta;

		if (timedelta < 0)
			delta = (timedelta < -BIGADJ)? -BIGSKEW : -SKEW;
		else
			delta = (timedelta > BIGADJ)? BIGSKEW : SKEW;

		timedelta -= delta;

		/*
		 * Compute the hrestime increment for this tick,
		 * converting from microseconds to nanoseconds.
		 */
		delta = ((MICROSEC/HZ) + delta) * (NANOSEC/MICROSEC);
		BUMPTIME(delta, one_sec);

		/*
		 * If finished applying correction,
		 * update the hardware time-of-day clock.
		 */
		if (timedelta == 0)
			wtodc();
	}

/* Enhanced Application Compatibility Support */
	if (--lticks <= 0)
		lticks = LTICKS_MAX;
/* End Enhanced Application Compatibility Support */

	if (one_sec) {
		extern void rf_clock();

		time++;
		one_sec = 0;

		/*
		 * Enforce CPU rlimit.
		 */
		rlim_cur = u.u_rlimit[RLIMIT_CPU].rlim_cur;
		if ((rlim_cur != RLIM_INFINITY) &&
		    (((pp->p_utime/HZ) + (pp->p_stime/HZ)) > rlim_cur))
			psignal(pp, SIGXCPU);

		if (BASEPRI)
			return retval;

#ifdef DEBUG
            	if (idlemsg && --idlecntdown == 0)
                        cmn_err(CE_WARN, "System is idle\n");
#endif

		minfo.freeswap = anoninfo.ani_free;

		rqlen = 0;
		sqlen = 0;
		number_current_processes = 0;

		for (pp = practive; pp != NULL; pp = pp->p_next) {
			if ((pp->p_flag & SSYS) == 0)
				number_current_processes++;
			pp->p_cpu >>= 1;
			if (pp->p_inoutage <= MAXINOUTAGE)
				pp->p_inoutage++;
			if (pp->p_stat == SRUN || pp->p_stat == SONPROC) {
				if (pp->p_flag & SLOAD)
					rqlen++;
				else
					sqlen++;
                                pp->p_slptime = 0;
			} else if (pp->p_stat == SSLEEP || pp->p_stat == SSTOP) {
				if (pp->p_slptime <= MAXSLEEPTIME)
					pp->p_slptime++;
			}
			else
				pp->p_slptime = 0;
		}

		if (rqlen) {
			sysinfo.runque += rqlen;
			sysinfo.runocc++;
		}
		if (sqlen) {
			sysinfo.swpque += sqlen;
			sysinfo.swpocc++;
		}
#ifdef DEBUG
		/*
                 * call this routine at regular intervals
                 * to allow debugging.
                 */
                if (--catchmenowcnt <= 0) {
                        catchmenowcnt = catchmestart;
                        catchmenow();
		}
#endif

		/*
		 * Wake up fsflush to write out DELWRI
		 * buffers, dirty pages and other cached
		 * administrative data, e.g. inodes.
		 */
		if (--fsflushcnt <= 0) {
			fsflushcnt = tune.t_fsflushr;
			wakeprocs((caddr_t)fsflush, PRMPT);
		}
		/*
		 * XXX
		 * All VFSs should have a VFS_CLOCK operation called from
		 * here.
		 */
		rf_clock(pc, USERMODE(cs));
		vmmeter();
		if (runin != 0) {
			runin = 0;
			setrun(proc_sched);
		}
                if (((freemem <= tune.t_gpgslo) || sqlen) && runout != 0) {
                        runout = 0;
                        setrun(proc_sched);
		}
	}
	return retval;
}

/*
 * Timeout(), untimeout(), timein(), heap_up(), heap_down():
 *
 * These routines manage the callout table as a heap.  The interfaces
 * and table structure are identical to the standard array-based version;
 * the routines impose the heap structure internally to improve 
 * the overhead of using timein(), timeout(), and untimeout() when the
 * table has more than 2 or 3 entries.
 */

STATIC int timeid = 0;	/* unique sequence number for entry id */

/*
 * Timeout() is called to arrange that fun(arg) be called in tim/HZ seconds.
 * An entry is added to the callout heap structure.  The time in each structure
 * entry is the absolute time at which the function should be called (compare
 * with the relative timing scheme used in the standard array-based version).
 * Returns a unique id that may be used in a later call to untimeout() to
 * cancel the timeout.  We guarantee that this id will be non-zero.
 *
 * The panic is there because there is nothing intelligent to be done if
 * an entry won't fit.
 */
int
timeout(fun, arg, tim)
	void (*fun)();
	caddr_t arg;
	long tim;
{
	register struct	callo	*p1;	/* pointer to entry we are adding */
	register int	j;		/* index to entry we are adding */
	int	t;			/* absolute time fun should be called */
	int	id;			/* id of the entry added */
	int	s;			/* temp variable for spl() */

	t = lbolt + tim;		/* absolute time in the future */

	s = spl7();

	if ((j = calllimit + 1) == v.v_call)
		cmn_err(CE_PANIC,"Timeout table overflow");

	/*
	 * We add the new entry into the next empty slot in the
	 * array representation of the heap.  heap_up() will
	 * restore the heap by moving the new entry up until
	 * it lies in a valid position.
	 */

	calllimit = j;
	j = heap_up(t, j);

	/*
	 * As a convenience to callers, we never return a zero id.
	 * This permits code such as { if (id) untimeout(id); }
	 * and addresses concern about races in such code as well.
	 * This is all arguably paranoid.
	 */

	while ((id = ++timeid) == 0)
		;

	/*
	 * j is the index of the new entry in the correct
	 * (legal heap) position.  Fill in the particulars
	 * of the request.
	 */

	p1		= &callout[j];
	p1->c_time	= t;
	p1->c_func	= fun;
	p1->c_arg	= arg;
	p1->c_id	= id;

	splx(s);
	return id;
}

/*
 * untimeout(id) is called to remove an entry in the callout
 * table that was originally placed there by a call to timeout().
 * id is the unique identifier returned by the timeout() call.
 */
void
untimeout(id)
	int id;
{
	register struct	callo	*p1;	/* pointer to entry with proper id */
	register struct	callo	*pend;	/* pointer to last valid table entry */
	register int	f;		/* index to entry with proper id */
	int	s;			/* temp variable for spl() */
	int	t;			/* temp variable for time for reheap */
	int	j;			/* index for last element in reheap */ 

	s = splhi();
	
	/*
	 * Linear search through table looking for an entry
	 * with an id that matches the id requested for removal.
	 */

	f = -1;
	pend = &callout[calllimit];
	for (p1 = &callout[0]; p1 <= pend; p1++) {
		++f;
		if (p1->c_id == id)
			goto found;
	}
	goto badid;

	/*
	 * We have the entry at f; delete it, move the last entry
	 * of the table into this location, and reheap.
	 */

found:
	if (f == calllimit--)	/* last entry in table; no reheap necessary */
		goto done;

	if (calllimit >= 0) {
		t = pend->c_time;
		j = (f-1) >> 1;	/* j is the parent of f */

		if (f > 0 && callout[j].c_time > t)
			j = heap_up(t, f);
		else
			j = heap_down(t, f);

		callout[j] = *pend;
	}
badid:
done:
	splx(s);
}

/*
 * timein() is called via a PIR9 which was set by timepoke()
 * in clock()
 */
int
timein()
{
	register struct	callo	*p0;	/* pointer to first entry in table */
	register struct	callo	*plast;	/* pointer to last entry in table */
	struct	callo	svcall;		/* pointer to current entry */
	int	t;			/* time of current entry */
	int	j;			/* index of current entry */
	int	s;			/* temp variable for spl() */

	s = splhi();

	p0 = &callout[0];
	while ((p0->c_time <= lbolt) && (calllimit >= 0)) {

		svcall = *p0;

		/*
		 * timein() deletes the first entry in the table.
		 * Move the last entry up, and reheap.
		 */

		plast = &callout[calllimit--];
		if (calllimit >= 0) {
			t = plast->c_time;
			j = heap_down(t,0);
			callout[j] = *plast;
		}

#if defined(KPERF)
	if (kpftraceflg)
		kperf_write(KPT_TIMEIN, svcall.c_func, curproc);
#endif

		(svcall.c_func)(svcall.c_arg);
	}
	splx(s);
	return 0;
}

/*
 * heap_up and heap_down are internal support functions that
 * maintain the heap structure of the callout table.
 * heap_up will take an illegal entry and percolate it up until
 * it lies in a legal position; heap_down will percolate an
 * illegal entry down until it falls in a legal position.
 */

STATIC int
heap_up(t, j)
	int t, j;
{
	register int	k;		/* index of parent of entry j */
	register struct	callo *p1;	/* pointer to parent of entry j */

	while (j-- > 0) {

		k = j >> 1;
		p1 = &callout[k];

		if (p1->c_time > t) {
			callout[++j] = *p1;
			j = k;
		} else
			break;
	}

	return 1+j;
}

STATIC int
heap_down(t, j)
	int t, j;
{
	register int	k;		/* index of child to exchange */
	register struct callo	*pk;	/* pointer to child to exchange */

	for (;;) {

	  	if ((k = 1 + (j << 1)) > calllimit)	/* left child ? */
			break;

		pk = &callout[k];

		if ((k < calllimit)			/* right child? */
		    && ((pk + 1)->c_time < pk->c_time)) {
			pk++;
			k++;
		}

		if (pk->c_time >= t)
			break;

		callout[j] = *pk;
		j = k;
	}
	return j;
}

#define	PDELAY	(PZERO-1)

/* delay number of ticks, called by various kernel routines */
void
delay(ticks)
	long ticks;
{
	int s;

	if (ticks <= 0)
		return;
	s = splhi();
	(void)timeout((void(*)())wakeup, (caddr_t)u.u_procp+1, ticks);
	sleep((caddr_t)u.u_procp+1, PDELAY);
	splx(s);
}

/*
 * clockintr() is required for configuring the clock interrupt priority.
 * It should never be called.
 */
clockintr()
{
	cmn_err(CE_PANIC,
	      "clockintr() entered; probably bad clock intpri configured\n");
}

/*
 * SunOS function to generate monotonically increasing time values.
 */
void
uniqtime(tv)
	register struct timeval *tv;
{
 	static struct timeval last;

	if (last.tv_sec != (long)hrestime.tv_sec) {
		last.tv_sec = (long)hrestime.tv_sec;
		last.tv_usec = (long)0;
	} else {
		last.tv_usec++;
	}
	*tv = last;
}

/*
 * Adjust time by specified delta in microseconds;
 * may be positive, negative or zero.  Returns the
 * remaining unapplied delta from the previous call.
 * Used by the adjtime system call.
 */
long
clockadj(delta)
	register long delta;
{
	long	previous;
	int	s;

	/*
	 * Sanity checks.  BIGSKEW must be a multiple of SKEW
	 * so that we're sure timedelta will count down exactly
	 * to zero in clock().  Skew rates cannot exceed 100%,
	 * to avoid trying to run time backwards.
	 */
	ASSERT((BIGSKEW % SKEW) == 0);
	ASSERT(BIGSKEW <= (MICROSEC/HZ) && SKEW <= BIGSKEW);

	/*
	 * Force the adjustment to be a multiple of SKEW
	 * in order to simplify testing for completion in clock().
	 */
	if (delta % SKEW)
		delta = delta / SKEW * SKEW;

	s = splhi();

	/*
	 * Setting timedelta to zero would "cancel" any
	 * uncompleted adjustment and prevent the hardware
	 * time-of-day clock from being set.  To avoid that,
	 * we just cheat.
	 */
	if (delta == 0 && timedelta != 0)
		delta = SKEW;

	previous = timedelta;
	timedelta = delta;

	splx(s);
	return previous;
}
