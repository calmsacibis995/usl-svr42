/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/fsflush.c	1.7.3.5"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/swap.h>
#include <mem/tuneable.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/types.h>
#include <util/var.h>

STATIC void	fsflush_bsync();
STATIC void	fsflush_pagesync();

STATIC clock_t	earliest = 0;

STATIC int	doiflush = 1;	/* non-zero to turn inode flushing on */
STATIC int	dopageflush = 1;/* non-zero to turn page flushing on */
STATIC int	dofsflush = 1;	/* non-zero to turn fsflush on */

/*
 * Fsflush Control
 */
int 
fsflush_control(cmd,arg)
int cmd, arg;
{
	int rval;

	switch(cmd){
	case 0:
		rval = dofsflush;
		dofsflush = arg ? 1 : 0;
		break;

	default:
		rval = -1;
		break;
	}

	return(rval);
}

/*
 * As part of file system hardening, this daemon is woken
 * periodically (rate is controlled by the t_fsflushr tunable)
 * to flush cached data which includes the
 * buffer cache, the page cache, and the inode cache.
 */
void
fsflush()
{
	int autoup, toscan;
	unsigned int i, icount, count = 0;
	u_int	flush_fwmask;
	struct page *ffpp;	/* starting page for fsflush to scan */
	extern void kmem_prod();

	ASSERT(v.v_autoup > 0);
	ASSERT(tune.t_fsflushr > 0);

	autoup = v.v_autoup * HZ;
	ffpp = pages;
	toscan = ((epages-pages) * (tune.t_fsflushr))/v.v_autoup;
	icount = v.v_autoup / tune.t_fsflushr;

	build_mask(flush_fwmask);


	while (1) {
		fsflush_bsync(autoup);

		if (dopageflush)
			fsflush_pagesync(flush_fwmask, toscan, &ffpp);
		
		/*
		 * The rate of flushing the inode cache is determined
		 * by the v_autoup/t_fsflushr ratio.
		 */
		if ((doiflush) && (count++ == icount)) {
			count = 0;
			for (i = 1; i < nfstype; i++) {
				(void)(*vfssw[i].vsw_vfsops->vfs_sync)(NULL,
					SYNC_ATTR, u.u_cred);
			}
		}

		/*
		 * See whether something is waiting for memory.
		 * This is not really a fsflush() type thing to do; but we
		 * don't want to create a separate daemon to check for this.
		 */
		kmem_prod();

		(void)sleep((caddr_t)fsflush, PRIBIO);
	}

}


/*
 * Flush page cache entries at a rate which is proportional to the
 * number of pages not yet flushed and the t_fsflushr/v_autoup ratio.  
 */
STATIC void
fsflush_pagesync(pgmask, numtoscan, ffppp)
	register u_int pgmask;
	int numtoscan;
	struct page **ffppp;
{

	register page_t *pp = *ffppp;
	register int i = 0;
	register int s;
	register struct vnode *vp;

	s = spl6();

	for(i=0; i < numtoscan; i++) {
		/* 
		 * We don't flush a page if one of the flags is set:
		 * p_free, p_keepcnt (because it could block in pvn_getdirty),
		 * p_lock (because it could deadlock in pvn_getdirty) and
		 * p_intrans.
		 */
		if (((vp = pp->p_vnode) == NULL) ||
		    (pp->p_firstw & pgmask) ||
		    (vp->v_flag & VISSWAP)  || (vp->v_type == VCHR)) {
			if ((++pp) >= epages)
				pp = pages;
			continue;
		}

		hat_getpmod(pp);

		if (pp->p_mod) {
			hat_pagesync(pp);
			splx(s);
			VOP_PUTPAGE(vp, pp->p_offset, PAGESIZE, 
				B_ASYNC, sys_cred);
			s = spl6();
		}

		if (++pp >= epages) 
			pp = pages;

	} /* for */

	splx(s);
	*ffppp = pp;
	PREEMPT();
	return;
}


/*
 * Flush buffer cache entries which have not been written out within
 * the supplied interval.  This interval is controlled by the v_autoup
 * tunable.
 */
STATIC void
fsflush_bsync(intval)
{
	register struct buf  *bp;
	register clock_t bstart, timewall;
	int howmany, s;
	register struct buf *bnext;

	if ( (timewall = lbolt - intval) >= earliest) {
		/*	move all eligible buffers
		 * 	to the front of bfreelist,
		 * 	reset earliest.
		 */ 
		s = spl6();
		howmany = 0;
		earliest = lbolt;
		for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bnext) {
			bnext = bp->av_forw;
			if (bp->b_flags & B_DELWRI) {
				if ((bstart = bp->b_start) < timewall) {
					/* move to the head of freelist */	
					bp->av_back->av_forw =  bnext;
					bnext->av_back = bp->av_back;
					bp->av_back = &bfreelist;
					bp->av_forw = bfreelist.av_forw;
					bfreelist.av_forw->av_back = bp;
					bfreelist.av_forw = bp;
					howmany++;
				}
				else 
					if (bstart < earliest) 
						earliest =  bstart;
			}
		}	
		if (howmany == 0) {
			(void)splx(s);
			return;
		}

loop:	
		for (bp=bfreelist.av_forw; bp != &bfreelist; 
			bp= bp->av_forw) {
			if ((bp->b_flags & B_DELWRI) && 
				(bp->b_start < timewall)) {
				bp->b_flags |= B_ASYNC;
				notavail(bp);
				bwrite(bp);
				(void)splx(s);
				if (--howmany == 0)
					return;
				PREEMPT();
				(void)spl6();
				goto loop;
			}
		}
		/* should not get here */
		(void)splx(s);
	}
} 



