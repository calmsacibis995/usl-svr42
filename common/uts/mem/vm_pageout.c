/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)uts-comm:mem/vm_pageout.c	1.10.4.12"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/tuneable.h>
#include <mem/vmmeter.h>
#include <mem/vmparam.h>
#include <mem/vmsystm.h>
#include <proc/cred.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/types.h>

#if RESTRICTED_DMA
#include <mem/rdma.h>
#endif

extern void	bp_mapout();
extern void	cleanup();

/*
 * The following parameters control operation of the page replacement
 * algorithm.  The three that are statically initialized to 0 are
 * computed at boot time based on the size of the system.  If they
 * are patched to some non-zero value in a kernel binary they will be
 * left alone; this allows them to be changed per system by using
 * a file patching program such as adb or rss.
 */

STATIC int	slowscan = 0;	/* min pages to scan per second */
STATIC int	fastscan = 0;	/* max pages to scan per second */
STATIC int	handspread = 0;	/* distance between front and back hands */
STATIC int	loopfract = 2;	/* max fraction of pages to scan per second */

extern int	minfree;	/* max free before clock freezes */
extern int	desfreemult;	/* multiple of minfree used to calculate minfree */
extern int	lotsfreemult;	/* multiple of desfree used to calculate lotsfree */
int		lotsfree;	/* minimum free pages before swapping begins */
int		desfree;	/* # of pages to try to keep free via daemon */

int		deficit;	/* estimate of needs of new swapped in procs */
int		nscan;		/* number of scans in last second */
int		desscan;	/* desired pages scanned per second */

struct buf	*bclnlist;	/* async I/Os needing synchronous cleanup */

/* the size of the clock loop */
#define	LOOPPAGES	(epages - pages)

/* ensure non-zero */
#define	nz(x)	((x) != 0 ? (x) : 1)

/*
 * Set up the paging constants for the clock algorithm.
 * Called after the system is initialized and the amount of memory
 * and number of paging devices is known.
 *
 * Threshold constants are defined in vmparam.h.
 */

void
setupclock()
{
	int i;

	/*
	 * Desfree is amount of memory desired free.
	 * If less than this for extended period, do swapping.
	 */

	desfree = desfreemult * minfree;

	/*
	 * Lotsfree is threshold where paging daemon turns on.
	 */

	lotsfree = lotsfreemult * desfree;

	/*
	 * Adjust paging tuneables.
	 * minpagefree is the virtual floor, only pageout and
	 * kma can allocate below here.
	 */

	minfree += minpagefree;
	desfree += minpagefree;
	lotsfree += minpagefree;
	tune.t_gpgslo += minpagefree;

	/*
	 * tune.t_minarmem is the amount of non-locked down memory that
         * that the system must maintain. Clearly, this must be greater
	 * than minpagefree. Prior to now it turned out to be equal
	 * to minfree and tune.gpgslo, so for now we will set it there.
	 */
	if (tune.t_minarmem < desfree)
		tune.t_minarmem = desfree;

	/*
	 * Clock to scan using max of ~~10% of processor time for sampling,
	 *  this estimated to allow maximum of 200 samples per second.
	 * This yields a "fastscan" of roughly (with CLSIZE == 2):
	 *	<=1m	2m	3m	4m	8m
	 * 	5s	10s	15s	20s	40s
	 * The *scan variables are in units of pages scanned per second.
	 */
	if (fastscan == 0)
		fastscan = 200;
	if (fastscan > LOOPPAGES / loopfract)
		fastscan = LOOPPAGES / loopfract;

	/*
	 * Set slow scan time to 1/2 the fast scan time.
	 */
	if (slowscan == 0)
		slowscan = fastscan / 2;
}

/*
 * Pageout scheduling.
 *
 * Schedpaging controls the rate at which the page out daemon runs by
 * setting the global variables nscan and desscan RATETOSCHEDPAGING
 * times a second.  Nscan records the number of pages pageout has examined
 * in its current pass; schedpaging resets this value to zero each time
 * it runs.  Desscan records the number of pages pageout should examine
 * in its next pass; schedpaging sets this value based on the amount of
 * currently available memory.
 */

#define	RATETOSCHEDPAGING	4		/* HZ that is */

/*
 * Schedule rate for paging.
 * Rate is linear interpolation between
 * slowscan with lotsfree and fastscan when out of memory.
 */

void
schedpaging()
{
	register int vavail;
	int type;

	nscan = 0;
	vavail = freemem - deficit;
	if (vavail < 0)
		vavail = 0;
	if (vavail > lotsfree)
		vavail = lotsfree;
	desscan = (slowscan * vavail + fastscan * (lotsfree - vavail)) /
			nz(lotsfree) / RATETOSCHEDPAGING;
	if (bclnlist != NULL)
		wakeprocs((caddr_t)proc_pageout, PRMPT);
	else {
		for (type = 0; type < NPAGETYPE; type++) {
			if (mem_freemem[type] < lotsfree) {
				wakeprocs((caddr_t)proc_pageout, PRMPT);
				break;
			}
		}
	}
	timeout(schedpaging, (caddr_t)0, HZ / RATETOSCHEDPAGING);
}

STATIC int pushes;
STATIC int pageout_asleep = 0;
STATIC int last_pageout_type = 0;

int dopageout = 1;	/* must be non-zero to turn page stealing on */

/*
 * The pageout daemon.
 */

pageout()
{
	page_t *last_fronthand[NPAGETYPE], *last_backhand[NPAGETYPE];
	register page_t *fronthand, *backhand;
	register u_int pgmask;
	register int scanflag=0;
	register int count;
	int type;

	u_int	pgout_fwmask;

	build_mask(pgout_fwmask);

	pgmask = pgout_fwmask;

	/*
	 * Set the two clock hands to be separated by a reasonable amount,
	 * but no more than 360 degrees apart.
	 */
	if (handspread == 0)
		handspread = HANDSPREAD;
	for (type = 0; type < NPAGETYPE; type++) {
		int	spread = handspread;

		/*
		 * Handspread is distance (in bytes) between front and back
		 * pageout daemon hands.  It must be < the amount of pageable
		 * memory.
		 */
		if (spread >= (mem_epages[type] - mem_pages[type]) * PAGESIZE)
			spread = (mem_epages[type] - mem_pages[type] - 1) *
				  PAGESIZE;

		/*
		 * Make sure that back hand follows front hand by at least
		 * 1/RATETOSCHEDPAGING seconds.  Without this test, it is
		 * possible for the back hand to look at a page during the same
		 * wakeup of the pageout daemon in which the front hand cleared
		 * its ref bit.
		 */
		if (spread < fastscan * PAGESIZE)
			spread = fastscan * PAGESIZE;
		last_backhand[type] = mem_pages[type];
		last_fronthand[type] = mem_pages[type] + spread / PAGESIZE;
		if (last_fronthand[type] >= mem_epages[type])
			last_fronthand[type] = mem_epages[type] - 1;
	}

loop:
	/*
	 * Before sleeping, look to see if there are any swap I/O headers
	 * in the "cleaned" list that correspond to dirty
	 * pages that have been pushed asynchronously. If so,
	 * empty the list by calling cleanup().
	 *
	 * N.B.: We guarantee never to block while the cleaned list is nonempty.
	 */
	(void) spl6();
	if (bclnlist != NULL) {
		(void) spl0();
		cleanup();
		goto loop;
	}
	pageout_asleep = 1;
	(void) sleep((caddr_t)proc_pageout, PSWP+1);
	pageout_asleep = 0;
	(void) spl0();
	if (!dopageout) goto loop;
	pushes = 0;

	/*
	 * More optimizations, if possible, should be done here as it
	 * is an extremely frequent code path.
	 */
	type = last_pageout_type;
#if RESTRICTED_DMA
	if (rdma_enabled && ++type == NPAGETYPE)
		type = 0;
#endif
	for (;;) {
		count = 0;
		backhand = last_backhand[type];
		fronthand = last_fronthand[type];
		while (mem_freemem[type] < lotsfree) {
			vminfo.v_scan++;
			cnt.v_scan++;
			if ((fronthand->p_firstw & pgmask) == 0 &&
			     fronthand->p_counts == 0) {
				++scanflag;
				/*
				 * Sync the H/W reference bits (if any) into
				 * the page bit.  If referenced, make the page
				 * unreferenced but reclaimable; we'll check it
				 * again when we get to it with the back hand.
				 */
				hat_getprefmod(fronthand);
				if (fronthand->p_ref)
					fronthand->p_ref = 0;
				else {
					/*
					 * If the page is currently dirty, we
					 * have to clean it before it can be
					 * freed.
					 *
					 * An unkept modified page with no vnode
					 * shouldn't really happen; but if it
					 * does, we free the page.
					 */
					if (fronthand->p_mod &&
					    fronthand->p_vnode) {
						/*
						 * Limit pushes to avoid
						 * saturating pageout devices
						 */
						if (pushes <=
						    maxpgio/RATETOSCHEDPAGING){
							/*
							 * If VOP_PUTPAGE fails,
							 * it probably needs
							 * more memory, so we
							 * call cleanup().
							 */
							if (VOP_PUTPAGE(
							   fronthand->p_vnode,
							   fronthand->p_offset,
							   PAGESIZE,
							   B_ASYNC|B_FREE,
							   sys_cred) == 0)
								++pushes;
							else
								cleanup();
						}
					} else {
						/*
						 * Reclaim the page: lock it,
						 * unload all the translations,
						 * and put the page back on
						 * the freelist.
						 */
						page_lock(fronthand);
						hat_pageunload(fronthand);
						page_free(fronthand, 0);
						cnt.v_dfree++;
						vminfo.v_dfree++;
					}
				}
			}
			if (++fronthand >= mem_epages[type]) {
				count++;
				fronthand = mem_pages[type];
			}
			/*
			 * Treat the backhand page like we did the fronthand,
			 * but only handle pages which are (still) unreferenced.
			 */
			if ((backhand->p_firstw & pgmask) == 0 &&
			     backhand->p_counts == 0) {
				++scanflag;
				hat_getprefmod(backhand);
				if (backhand->p_ref == 0) {
					if (backhand->p_mod &&
					    backhand->p_vnode) {
						if (pushes <=
						    maxpgio/RATETOSCHEDPAGING){
							if (VOP_PUTPAGE(
							   backhand->p_vnode,
							   backhand->p_offset,
							   PAGESIZE,
							   B_ASYNC|B_FREE,
							   sys_cred) == 0)
								++pushes;
							else
								cleanup();
						}
					} else {
						page_lock(backhand);
						hat_pageunload(backhand);
						page_free(backhand, 0);
						cnt.v_dfree++;
						vminfo.v_dfree++;
					}
				}
			}
			if (++backhand >= mem_epages[type])
				backhand = mem_pages[type];
			if (scanflag) {
				++nscan;
				scanflag = 0;
			}
			if (nscan >= desscan || count > 2) {
				/*
				 * Either we processed the desired number of
				 * pages, or we hit the extremely unlikely case
				 * of going around the loop twice.
				 * Stop until the next wakeup.
				 */
				last_fronthand[type] = fronthand;
				last_backhand[type] = backhand;
				last_pageout_type = type;
				goto loop;
			}
		}
		last_fronthand[type] = fronthand;
		last_backhand[type] = backhand;
		if (type == last_pageout_type)
			break;
#if RESTRICTED_DMA
		if (rdma_enabled && ++type == NPAGETYPE)
			type = 0;
#endif
	}

	last_pageout_type = type;
	goto loop;
}

/*
 * Add a buffer to the "cleaned" list.  Called from biodone().
 */

void
swdone(bp)
	register struct buf *bp;
{
	int s;

	s = spl6();
	bp->av_forw = bclnlist;
	bclnlist = bp;
	(void) splx(s);

	/*
	 * Rather than wake up the pageout daemon, look for
	 * a sleeper on one of the pages associated with the
	 * buffer and wake it up, with the expectation that
	 * it will clean the list.
	 */
	{
		register struct page *pp = bp->b_pages;

		if (pp) {
			do {
				if (pp->p_want) {
					wakeprocs((caddr_t)pp, PRMPT);
					pp->p_want = 0;
					break;
				}
				pp = pp->p_next;
			} while (pp != bp->b_pages);
		}
	}
}

/*
 * Process the list of "cleaned" pages
 * or asynchronous I/O requests that need
 * synchronous cleanup.
 */

void
cleanup()
{
	register struct buf *bp;
	register int s;

	for (;;) {
		s = spl6();
		if ((bp = bclnlist) == NULL) {
			(void) splx(s);
			break;
		}
		bclnlist = bp->av_forw;
		(void) splx(s);

		if ((bp->b_flags & B_ASYNC) == 0)
			cmn_err(CE_PANIC, "cleanup: not async");
		if (bp->b_flags & B_PAGEIO)
			pvn_done(bp);
		else {
			/* bp was B_ASYNC and B_REMAPPED (and not B_PAGEIO) */
			if ((bp->b_flags & B_REMAPPED) == 0)
				cmn_err(CE_PANIC, "cleanup: not remapped");
			bp_mapout(bp);
			brelse(bp);
		}
	}
}
