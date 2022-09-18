/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/bio.c	1.16.3.14"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

struct buf	bfreelist;	/* Head of the free list of buffers */

/*
 * Convert logical block number to a physical number
 * given block number and block size of the file system.
 * Assumes 512 byte blocks (see param.h).
 */
#define LTOPBLK(blkno, bsize)	(blkno * ((bsize>>SCTRSHFT)))

/*
** The following variables are no longer used in the kernel,
** but are required for backward compatibility.
*/
int basyncnt, basynwait;

struct buf	bhdrlist;	/* free buf header list */
STATIC int	nbuf;		/* number of buffer headers allocated */

STATIC struct buf pgouthdrlist;		/* pageout buffer header list */
STATIC struct buf notpgouthdrlist;	/* sched buffer header list */
STATIC int notpgoutwanted;
extern struct buf pgoutbuf[];		/* global storage for buffer headers */
extern struct buf notpgoutbuf[];
STATIC struct buf *pgoutblast;
STATIC struct buf *notpgoutblast;
extern int npgoutbuf;			/* number of buffers configured */
extern int nnotpgoutbuf;
STATIC int pgoutboutcnt;		/* buffer out counts */
STATIC int notpgoutboutcnt;
STATIC int pgoutbufused;		/* buffer used counts */
STATIC int notpgoutbufused;

extern struct hbuf hbuf[];	/* hash buffer list, defined in kernel.cf */

STATIC int pboverlap();

STATIC struct buf *bufchain;		/* chain of allocated buffers */

/*
 * The following several routines allocate and free
 * buffers with various side effects.  In general the
 * arguments to an allocate routine are a device and
 * a block number, and the value is a pointer to
 * to the buffer header; the buffer is marked "busy"
 * so that no one else can touch it.  If the block was
 * already in core, no I/O need be done; if it is
 * already busy, the process waits until it becomes free.
 * The following routines allocate a buffer:
 *	getblk
 *	bread
 *	breada
 * Eventually the buffer must be released, possibly with the
 * side effect of writing it out, by using one of
 *	bwrite
 *	bdwrite
 *	btwrite
 *	bawrite
 *	brelse
 */

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *
bread(dev, blkno, bsize)
	register dev_t dev;
	daddr_t blkno;
	long bsize;
{
	register struct buf *bp;

	sysinfo.lread++;
	bp = getblk(dev, blkno, bsize);
	if (bp->b_flags & B_DONE)
		return bp;
	bp->b_flags |= B_READ;
	bp->b_bcount = bsize;
	(*bdevsw[getmajor(dev)].d_strategy)(bp);
	u.u_ior++;
	sysinfo.bread++;
	(void) biowait(bp);
	return bp;
}

/*
 * Available interface to read in the block, like bread, but also
 * start I/O on the read-ahead block (which is not allocated to
 * the caller).
 */
struct buf *
breada(dev, blkno, rablkno, bsize)
	register dev_t dev;
	daddr_t blkno, rablkno;
	long bsize;
{
	register struct buf *bp, *rabp;

	bp = NULL;
	if (!incore(dev, blkno, bsize)) {
		sysinfo.lread++;
		bp = getblk(dev, blkno, bsize);
		if ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_READ;
			bp->b_bcount = bsize;
			(*bdevsw[getmajor(dev)].d_strategy)(bp);
			u.u_ior++;
			sysinfo.bread++;
		}
	}
	if (rablkno && bfreelist.b_bcount>1 && !incore(dev, rablkno, bsize)) {
		rabp = getblk(dev, rablkno, bsize);
		if (rabp->b_flags & B_DONE)
			brelse(rabp);
		else {
			rabp->b_flags |= B_READ|B_ASYNC;
			rabp->b_bcount = bsize;
			(*bdevsw[getmajor(dev)].d_strategy)(rabp);
			u.u_ior++;
			sysinfo.bread++;
		}
	}
	if (bp == NULL)
		return bread(dev, blkno, bsize);
	(void) biowait(bp);
	return bp;
}

/*
 * See if there is a buffer associated with the specified block and return
 * it.  This routine is like a variant of getblk which doesn't wait.  If the
 * buffer exists, and it's not busy, then blookup marks the buffer busy and
 * returns a pointer to it; otherwise, blookup returns NULL.
 */
struct buf *
blookup(dev, blkno, bsize)
	dev_t dev;
	daddr_t blkno;
	long bsize;
{
	struct buf *bp, *dp;
	int s;

	if (getmajor(dev) >= bdevcnt)
		cmn_err(CE_PANIC,
			"blookup: getmajor return value exceeds bdevcnt");
	blkno = LTOPBLK(blkno, bsize);
	s = spl0();
	if ((dp = bhash(dev, blkno)) == NULL)
		cmn_err(CE_PANIC, "blookup: bhash returns NULL");
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if ((bp->b_blkno == blkno) && (bp->b_edev == dev) &&
				!(bp->b_flags & B_STALE)) {
			spl6();
		/* XXX - do we really need to check B_DONE here? */
			if ((bp->b_flags & B_BUSY) || !(bp->b_flags & B_DONE))
				break;
			bp->b_flags &= ~B_AGE;
			notavail(bp);
			splx(s);
			sysinfo.lread++;
			return (bp);
		}
	}
	splx(s);
	return (NULL);
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
void
bwrite(bp)
	register struct buf *bp;
{
	register flag;

	sysinfo.lwrite++;
	flag = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	u.u_iow++;
	sysinfo.bwrite++;
	if (bp->b_writestrat) {
		(*(bp->b_writestrat))(bp);
	} else {
		(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
	}
	if ((flag & B_ASYNC) == 0) {
		(void) biowait(bp);
		brelse(bp);
	}
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * Also save the time that the block is first marked as delayed
 * so that it will be written in a reasonable time.
 */
void
bdwrite(bp)
	register struct buf *bp;
{
	sysinfo.lwrite++;
	if ((bp->b_flags & B_DELWRI) == 0)
		bp->b_start = lbolt;
	bp->b_flags |= B_DELWRI | B_DONE;
	bp->b_resid = 0;
	brelse(bp);
}

/*
 * Variation on bdwrite, which sets the start time of the request to
 * the start time passed in unless the start time of the buffer is
 * older than the new start time.
 */
void
btwrite(bp, start)
	struct buf *bp;
	clock_t start;
{

	sysinfo.lwrite++;
	if (((bp->b_flags & B_DELWRI) == 0) || (bp->b_start > start))
		bp->b_start = start;
	bp->b_flags |= B_DELWRI | B_DONE;
	bp->b_resid = 0;
	brelse(bp);
}


/*
 * Available interface to release the buffer, start I/O on it,
 * but not wait for completion.
 */
void
bawrite(bp)
	register struct buf *bp;
{

	if (bfreelist.b_bcount > 4)
		bp->b_flags |= B_ASYNC;
	bwrite(bp);
}

/*
 * Release the buffer, with no I/O implied.
 */
void
brelse(bp)
	register struct buf *bp;
{
	register struct buf **backp;
	register s;

	if (bp->b_flags & B_WANTED)
		wakeprocs((caddr_t)bp, PRMPT);
	if (bfreelist.b_flags & B_WANTED) {
		bfreelist.b_flags &= ~B_WANTED;
		wakeprocs((caddr_t)&bfreelist, PRMPT);
	}
	if (bp->b_flags & B_ERROR) {
		bp->b_flags |= B_STALE|B_AGE;
		bp->b_flags &= ~(B_ERROR|B_DELWRI);
		bp->b_error = 0;
		bp->b_oerror = 0;
	}
	s = spl6();
	if (bp->b_flags & B_AGE) {
		backp = &bfreelist.av_forw;
		(*backp)->av_back = bp;
		bp->av_forw = *backp;
		*backp = bp;
		bp->av_back = &bfreelist;
	} else {
		backp = &bfreelist.av_back;
		(*backp)->av_forw = bp;
		bp->av_back = *backp;
		*backp = bp;
		bp->av_forw = &bfreelist;
	}
	bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC);
	bfreelist.b_bcount++;
	bp->b_reltime = lbolt;
	splx(s);
}

/*
 * See if the block is associated with some buffer
 * (e.g., to avoid getting hung up on a wait in breada).
 */
int
incore(dev, blkno, bsize)
	register dev_t dev;
	register daddr_t blkno;
	register long bsize;
{
	register struct buf *bp;
	register struct buf *dp;

	blkno = LTOPBLK(blkno, bsize);
	dp = bhash(dev, blkno);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
		if (bp->b_blkno == blkno && bp->b_edev == dev
		  && (bp->b_flags & B_STALE) == 0)
			return 1;
	return 0;
}

/* 
 * getfreeblk() is called from getblk() or ngeteblk()
 * It runs down the free buffer list to free up
 * buffers when total number of buffer or total memory used
 * by buffers exceeds thresholds
 * If there is a buffer matches the request size, reuse
 * that buffer.
 * Otherwise, free buffers and re-allocate a new buffer
 */
struct buf *
getfreeblk(bsize)
	long bsize;
{
	register struct buf *bp, *savebp = NULL;
	register int s;
	
	s = spl6();
loop:
	bp = bfreelist.av_forw;
	if (bp != &bfreelist
	  && savebp == NULL
	  && (bfreelist.b_bufsize < bsize
            || (bp->b_flags & B_AGE))) {
		ASSERT(bp != NULL);
		notavail(bp);			
		bp->av_forw = bp->av_back = NULL;

		/*
		 * This buffer hasn't been written to disk yet.
		 * Do it now and free it later.
		 */
		if (bp->b_flags & B_DELWRI) {
			bp->b_flags |= B_ASYNC | B_AGE;
			bwrite(bp);
			(void)spl6();
		}
		else {
			bremhash(bp);
			bp->b_forw = bp->b_back = bp;
			if (savebp == NULL && bp->b_bufsize == bsize)
				savebp = bp;
			else {
				/*
				 * If size doesn't match, free it.
				 */
				kmem_free(bp->b_un.b_addr, bp->b_bufsize);
				bfreelist.b_bufsize += bp->b_bufsize;
				struct_zero((caddr_t)bp, sizeof(struct buf));
				bp->b_flags |= B_KERNBUF;
                                bp->b_forw = bp->b_back = bp;
				bp->av_forw = bhdrlist.av_forw;
				bhdrlist.av_forw = bp;
			}
		}
		goto loop;
	}
	if (savebp != NULL) {
		(void)splx(s);
		return savebp;
	}
	/*
	 * If not enough memory for this buffer, sleep.  When we
	 * return from sleep(), we must return to the caller to
	 * check the hash queue again.
	 */	
	if (bfreelist.b_bufsize < bsize) {
		ASSERT(bfreelist.av_forw == &bfreelist);
		bfreelist.b_flags |= B_WANTED;
		sleep((caddr_t)&bfreelist, PRIBIO+1);
	        (void)splx(s);
		return NULL;
	}
	/*
	 * Allocate a new buffer.  Get a buffer header first.
	 * If no free buffer header, allocate a chunk of
	 * buffer headers.
	 */	
	bfreelist.b_bufsize -= bsize;
	(void)splx(s);
	if (bhdrlist.av_forw == NULL) {
		struct buf *dp,*tdp;
		int i;

		dp = (struct buf *)kmem_zalloc(sizeof(struct buf) * (v.v_buf + 1),
				KM_SLEEP);
		ASSERT(dp != NULL);

		if (bhdrlist.av_forw != NULL) {
			kmem_free(dp, sizeof(struct buf) * (v.v_buf + 1));
		} else {
			tdp = dp;
			dp++;
			for (i = 0; i < v.v_buf ; i++,dp++) {
				dp->b_dev = (o_dev_t)NODEV;
				dp->b_edev = (dev_t)NODEV;
				dp->b_un.b_addr = NULL;
				dp->b_flags = B_KERNBUF;
				dp->b_bcount = 0;
                                dp->b_forw = dp->b_back = dp;
				dp->av_forw = dp + 1;
			}
			(--dp)->av_forw = bhdrlist.av_forw;
			bhdrlist.av_forw = tdp + 1;
			nbuf += v.v_buf;
			tdp->av_forw = bufchain;
			bufchain = tdp;
		}
	}
	bp = bhdrlist.av_forw;
	bhdrlist.av_forw = bp->av_forw;
	bp->av_forw = bp->av_back = NULL;
	bp->b_un.b_addr = (caddr_t)kmem_zalloc(bsize, KM_SLEEP);
	bp->b_bufsize = bsize;
	return bp;
}		

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 */
struct buf *
getblk(dev, blkno, bsize)
	register dev_t dev;
	register daddr_t blkno;
	long bsize;
{
	register struct buf *bp;
	register struct buf *dp, *nbp = NULL; 
	register int s;

	if (getmajor(dev) >= bdevcnt)
		cmn_err(CE_PANIC,"blkdev");

	blkno = LTOPBLK(blkno, bsize);
	s = spl0();
loop:
	if ((dp = bhash(dev, blkno)) == NULL)
		cmn_err(CE_PANIC,"devtab");
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_edev != dev
		  || bp->b_flags & B_STALE)
			continue;
		spl6();
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			syswait.iowait++;
			sleep((caddr_t)bp, PRIBIO+1);
			syswait.iowait--;
			spl0();
			goto loop;
		}
		bp->b_flags &= ~B_AGE;
		notavail(bp);
		splx(s);
		if (nbp) {
			nbp->b_forw = nbp->b_back = nbp;
                        nbp->b_flags = B_KERNBUF | B_BUSY;
                        nbp->b_dev = (o_dev_t)NODEV;
                        nbp->b_edev = (dev_t)NODEV;
                        nbp->b_bcount = bsize;
			brelse(nbp);
		}
		return bp;
	}

	splx(s);
	if (nbp == NULL) {
		nbp = getfreeblk(bsize);
		spl0();
		goto loop;
	}
     	bp = nbp;
	bp->b_flags = B_KERNBUF | B_BUSY;
	bp->b_forw = dp->b_forw;
	bp->b_back = dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;
	bp->b_edev = dev;
	bp->b_dev = (o_dev_t)cmpdev(dev);
	bp->b_blkno = blkno;
	bp->b_bcount = bsize;
	bp->b_iodone = NULL;
	bp->b_writestrat = NULL;
	return bp;
}

/*
 * get an empty block,
 * not assigned to any particular device.
 */
struct buf *
ngeteblk(bsize)
	long bsize;
{
	register struct buf *bp;
	register struct buf *dp;

	while ((bp = getfreeblk(bsize)) == NULL)
		;

	dp = &bfreelist;
	bp->b_flags = B_KERNBUF | B_BUSY | B_AGE;
	bp->b_forw = dp->b_forw;
	bp->b_back = dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;
	bp->b_dev = (o_dev_t)NODEV;
	bp->b_edev = (dev_t)NODEV;
	bp->b_bcount = bsize;
	bp->b_iodone = NULL;
	bp->b_writestrat = NULL;
	return bp;
}

/* 
 * Interface of geteblk() is kept intact to maintain driver compatibility.
 * Use ngeteblk() to allocate block size other than 1 KB.
 */
struct buf *
geteblk()
{
	return ngeteblk((long)1024);
}

/*
 * Zero the core associated with a buffer.
 */
void
clrbuf(bp)
	struct buf *bp;
{
	bzero((caddr_t)bp->b_un.b_words, bp->b_bcount);
	bp->b_resid = 0;
}

/*
 * Make sure all write-behind blocks on dev (or NODEV for all)
 * are flushed out.
 */
void
bflush(dev)
	register dev_t dev;
{
	register struct buf *bp;
	register int s;

loop:
	s = spl6();
	for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
		if ((bp->b_flags & B_DELWRI)
		  && (dev == NODEV || dev == bp->b_edev)) {
			bp->b_flags |= B_ASYNC;
			notavail(bp);
			bwrite(bp);
			(void) splx(s);
			goto loop;
		}
	}
	(void) splx(s);
}

/*
 * Available interface to ensure that a specified block is up-to-date on disk.
 */
void
blkflush(dev, blkno, bsize)
	dev_t dev;
	daddr_t blkno;
	int bsize;
{
	register struct buf *bp, *dp;
	int s;

	blkno = LTOPBLK(blkno, bsize);
	dp = bhash(dev, blkno);
loop:
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_edev != dev
		  || (bp->b_flags & B_STALE))
			continue;
		s = spl6();
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			syswait.iowait++;
			(void) sleep((caddr_t) bp, PRIBIO+1);
			syswait.iowait--;
			(void) splx(s);
			goto loop;
		}
		if (bp->b_flags & B_DELWRI) {
			notavail(bp);
			(void) splx(s);
			bwrite(bp);
			goto loop;
		} 
		(void) splx(s);
	}
}

/*
 * Invalidate blocks for a dev after last close.
 */
void
binval(dev)
	register dev_t dev;
{
	register struct buf *dp;
	register struct buf *bp;
	register i;

	for (i = 0; i < v.v_hbuf; i++) {
		dp = (struct buf *)&hbuf[i];
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
			if (bp->b_edev == dev)
				bp->b_flags |= B_STALE|B_AGE;
	}
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device hash buffer lists to empty.
 */
void
binit()
{
	register struct buf *bp;
	register unsigned i;
		
	/*
	 * Change buffer memory usage high-water-mark from kbytes
	 * to bytes.
	 */
	bfreelist.b_bufsize = v.v_bufhwm * 1024;

	bp = &bfreelist;
	bp->b_forw = bp->b_back = bp->av_forw = bp->av_back = bp;
	bhdrlist.av_forw = NULL;

	for (i = 0; i < v.v_hbuf; i++)
		hbuf[i].b_forw = hbuf[i].b_back = (struct buf *)&hbuf[i];

	pgouthdrlist.av_forw = bp = pgoutbuf;
	for (; bp < &pgoutbuf[npgoutbuf-1]; bp++)
		bp->av_forw = bp+1;
	bp->av_forw = NULL;
	pgoutblast = bp;
	notpgouthdrlist.av_forw = bp = notpgoutbuf;
	for (; bp < &notpgoutbuf[nnotpgoutbuf-1]; bp++)
		bp->av_forw = bp+1;
	bp->av_forw = NULL;
	notpgoutblast = bp;
}

/*
 * Wait for I/O completion on the buffer; return error code.
 * If bp was for synchronous I/O, bp is invalid and associated
 * resources are freed on return.
 */
int
biowait(bp)
	register struct buf *bp;
{
	int error = 0, s;

	syswait.iowait++;
	s = spl6();
	while ((bp->b_flags & B_DONE) == 0) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO);
	}
	(void) splx(s);
	syswait.iowait--;
	error = geterror(bp);

	if ((bp->b_flags & B_ASYNC) == 0) {
		if (bp->b_flags & B_PAGEIO)
			pvn_done(bp);
		else if (bp->b_flags & B_REMAPPED)
			bp_mapout(bp);
	}
	return error;
}

/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 */
void
biodone(bp)
	register struct buf *bp;
{
	if (bp->b_iodone && (bp->b_flags & B_KERNBUF)) {
		(*(bp->b_iodone))(bp);
		return;
	}
	ASSERT((bp->b_flags & B_DONE) == 0);
	bp->b_flags |= B_DONE;
	if (bp->b_flags & B_ASYNC) {
		if (bp->b_flags & (B_PAGEIO|B_REMAPPED))
			swdone(bp);
		else
			brelse(bp);		/* release bp to 1k freelist */
	} else {
		bp->b_flags &= ~B_WANTED;
		wakeprocs((caddr_t)bp, PRMPT);
	}
}

#undef bioreset
/*
 * void bioreset(buf_t *bp)
 *	Reset a buffer after completed I/O so it can be used again.
 *	(DDI/DKI interface.)
 */
void
bioreset(bp)
	register struct buf *bp;
{
	ASSERT(bp->b_flags & B_BUSY);

	bp->b_flags &= ~(B_DONE|B_ERROR);
	if (bp->b_flags & B_KERNBUF)
		bp->b_error = 0;
	bp->b_oerror = 0;
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized code.
 */

/* macro defined in buf.h to avoid function call overhead */
#undef geterror

int
geterror(bp)
	register struct buf *bp;
{
	int error = 0;

	if (bp->b_flags & B_ERROR) {
		if (bp->b_flags & B_KERNBUF)
			error = bp->b_error;
		if (!error)
			error = bp->b_oerror;
		if (!error)
			error = EIO;
	}
	return error;
}

/*
 * Support for pageio buffers.
 *
 * This stuff should be generalized to provide a generalized bp
 * header facility that can be used for things other than pageio.
 */

/*
 * Pageio_out is a list of all the buffers currently checked out
 * for pageio use.
 */
STATIC struct bufhd pageio_out = {
	B_HEAD,
	(struct buf *)&pageio_out,
	(struct buf *)&pageio_out,
};

#define	NOMEMWAIT() (u.u_procp == proc_pageout || u.u_procp == proc_sched)
#define PAGEOUT() (u.u_procp == proc_pageout)

/*
 * Allocate and initialize a buf struct for use with pageio.
 */
struct buf *
pageio_setup(pp, len, vp, flags)
	struct page *pp;
	u_int len;
	struct vnode *vp;
	int flags;
{
	register struct buf *bp;
	register int s;

loop:
        bp = (struct buf *) kmem_zalloc(sizeof (*bp),
          NOMEMWAIT() ? KM_NOSLEEP : KM_SLEEP);

        if (bp == NULL) {
		/* use hidden buffer headers rather than sleep on memory.
		* Even regular I/O can cause deadlock if inodes
		* doing I/O are the ones that must be locked
		* to free up memory.
		*/
		if (!PAGEOUT()) {
			if ((bp = notpgouthdrlist.av_forw) != NULL) {
                                s = splhi();
                                notpgouthdrlist.av_forw = bp->av_forw;
                                bp->av_forw = bp->av_back = NULL;
                                splx(s);
                                notpgoutbufused++;
                                goto gotone;
			}
			notpgoutboutcnt++;
			notpgoutwanted = 1;
			(void) sleep((caddr_t)&notpgoutwanted, PSWP+1);
			goto loop;
		} else {
			if ((bp = pgouthdrlist.av_forw) != NULL) {
                                s = splhi();
                                pgouthdrlist.av_forw = bp->av_forw;
                                bp->av_forw = bp->av_back = NULL;
                                splx(s);
                                pgoutbufused++;
                                goto gotone;
			}
			pgoutboutcnt++;
		}
                /*
                 * We are pageout and cannot risk sleeping for more
                 * memory so we return an error condition instead.
                 */
                return NULL;
        }         

gotone:
	s = spl6();
	binshash(bp, (struct buf *)&pageio_out);
	(void) splx(s);
	bp->b_un.b_addr = 0;
	bp->b_error = 0;
	bp->b_oerror = 0;
	bp->b_resid = 0;
	bp->b_bcount = len;
	bp->b_bufsize = len;
	bp->b_pages = pp;
	bp->b_flags = B_KERNBUF | B_PAGEIO | B_NOCACHE | B_BUSY | flags;

	/*
	 * If vp is not NULL, this is the parent bp. Increment the
	 * vcount of the vnode.
	 */
	if (vp) {
		VN_HOLD(vp);
		bp->b_vp = vp;
	} else {
		bp->b_vp = NULL;
	}

	/*
	 * Caller sets dev & blkno and can adjust
	 * b_addr for page offset and can use bp_mapin
	 * to make pages kernel addressable.
	 */
	return bp;
}

/*
 * Removes mapping for a given buffer, gets it off the pageio-request
 * chain, and frees the buf structure.
 */
void
pageio_done(bp)
	register struct buf *bp;
{
	register int s;

	if (bp->b_flags & B_REMAPPED)
		bp_mapout(bp);
	s = spl6();
	bremhash(bp);
	(void) splx(s);
	if (bp->b_vp)
		VN_RELE(bp->b_vp);
	if (bp >= pgoutbuf && bp <= pgoutblast) {
		s = splhi();
		bp->av_forw = pgouthdrlist.av_forw;
		pgouthdrlist.av_forw = bp;
		splx(s);
	} else if (bp >= notpgoutbuf && bp <= notpgoutblast) {
		s = splhi();
		bp->av_forw = notpgouthdrlist.av_forw;
		notpgouthdrlist.av_forw = bp;
		splx(s);
		if (notpgoutwanted) {
			notpgoutwanted = 0;
			wakeprocs((caddr_t)&notpgoutwanted, PRMPT);
		}
	} else
		kmem_free((caddr_t)bp, sizeof (*bp));
}
	
/*
 * Break up the request that came from bread/bwrite into chunks of
 * contiguous memory so we can get around the DMAC limitations
 */

/*
 * Determine number of bytes to page boundary.
 */
#define	pgbnd(a)	(NBPP - ((NBPP - 1) & (int)(a)))

void
buf_breakup(strat, obp)
	int (*strat)();
	register struct buf *obp;
{
	register int cc, iocount, s;
	register struct buf *bp;	
	
/*	ASSERT((obp->b_flags & B_PAGEIO)== NULL); */
	bp = (struct buf *)kmem_zalloc(sizeof (*bp), KM_SLEEP);
	bcopy((caddr_t)obp, (caddr_t)bp, sizeof(*bp));
	iocount = obp->b_bcount;
	bp->b_iodone = NULL;
	bp->b_writestrat = NULL;
	bp->b_flags &= ~B_ASYNC;

	/*
	 * The buffer is on a sector boundary but not necessarily
	 * on a page boundary.
	 */
	if ((bp->b_bcount = cc = 
	  min(iocount, pgbnd(bp->b_un.b_addr))) < NBPP) {
		/*
		 * Do the fragment of the buffer that's in the
		 * first page.
		 */
		bp->b_flags &= ~B_DONE;
		(*strat)(bp);
		s = spl6();
		while ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_WANTED;
			sleep((caddr_t)bp, PRIBIO);
		}
		(void) splx(s);
		if (bp->b_flags & B_ERROR) {
			goto out;
		}
		bp->b_blkno += btod(cc);
		bp->b_un.b_addr += cc;
		iocount -= cc;
	}

	/*
	 * Now do the DMA a page at a time.
	 */
	while (iocount > 0) {
		bp->b_bcount = cc = min(iocount, NBPP);
		bp->b_flags &= ~B_DONE;
		(*strat)(bp);
		s = spl6();
		while ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_WANTED;
			sleep((caddr_t)bp, PRIBIO);
		}
		(void) splx(s);
		if (bp->b_flags & B_ERROR)
			goto out;
		bp->b_blkno += btod(cc);
		bp->b_un.b_addr += cc;
		iocount -= cc;
	}
	kmem_free((caddr_t)bp, sizeof(*bp));
	s = spl6();
	biodone(obp);
	splx(s);
	return;
out:
	if (bp->b_error)
		obp->b_error = bp->b_error;
	else if (bp->b_oerror)
		obp->b_error = bp->b_oerror;
	obp->b_flags |= B_ERROR;
	kmem_free((caddr_t)bp, sizeof(*bp));
	s = spl6();
	biodone(obp);
	splx(s);
}


/*
 * Read in (if necessary) the physical block and return a buffer pointer.
 * This routine takes the physical block number, and the length to read
 * as input.  The old bread() routine reads only a logical block
 * for the logical blocksize.  This new routine allows file systems
 * that have fragmented blocks to read partial logical blocks containing
 * control data without mapping the device special file via fbread.
 *
 * Though this routine and bread() are almost identical, it is kept as
 * a separate routine for performance, and the fact that the interface
 * of bread() may not change for backward compatibility.
 */
struct buf *
pbread(dev, blkno, bsize)
	register dev_t dev;
	daddr_t blkno;
	long bsize;
{
	register struct buf *bp;

	sysinfo.lread++;
	bp = pgetblk(dev, blkno, bsize);
	if (bp->b_flags & B_DONE)
		return bp;
	bp->b_flags |= B_READ;
	bp->b_bcount = bsize;
	(*bdevsw[getmajor(dev)].d_strategy)(bp);
	u.u_ior++;
	sysinfo.bread++;
	(void) biowait(bp);
	return bp;
}


/*
 * Assign a buffer for the given physical block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 */
struct buf *
pgetblk(dev, blkno, bsize)
	register dev_t dev;
	register daddr_t blkno;
	long bsize;
{
	register struct buf *bp;
	register struct buf *dp, *nbp = NULL; 
	register int s;

	if (getmajor(dev) >= bdevcnt)
		cmn_err(CE_PANIC,"blkdev");

	s = spl0();
loop:
	if ((dp = bhash(dev, blkno)) == NULL)
		cmn_err(CE_PANIC,"devtab");
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_edev != dev
		  || bp->b_flags & B_STALE)
			continue;
		spl6();
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			syswait.iowait++;
			sleep((caddr_t)bp, PRIBIO+1);
			syswait.iowait--;
			spl0();
			goto loop;
		}
		bp->b_flags &= ~B_AGE;
		notavail(bp);
		splx(s);
		if ((bp->b_bcount != bsize) && (pboverlap(bp, bsize) == 0)) {
			spl0();
			goto loop;
		}
		if (nbp) {
			nbp->b_forw = nbp->b_back = nbp;
			brelse(nbp);
		}
		return bp;
	}

	splx(s);
	if (nbp == NULL) {
		nbp = getfreeblk(bsize);
		spl0();
		goto loop;
	}
     	bp = nbp;
	bp->b_flags = B_KERNBUF | B_BUSY;
	bp->b_forw = dp->b_forw;
	bp->b_back = dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;
	bp->b_edev = dev;
	bp->b_dev = (o_dev_t)cmpdev(dev);
	bp->b_blkno = blkno;
	if (pboverlap(bp, bsize) == 0) {
		nbp = NULL;
		spl0();
		goto loop;
	}
	bp->b_bcount = bsize;
	bp->b_iodone = NULL;
	bp->b_writestrat = NULL;
	return bp;
}

/*
 *	Check for overlap of this buffer with any
 *	other allocated buffer.
 *	Similar to old ufs "brealloc" code.
 */

STATIC int
pboverlap(bp, size)
	register struct buf *bp;
	register int size;
{
	register daddr_t start, last;
	register struct buf *ep;
	register int s;
	struct buf *dp;
	
	/*
	 * First need to make sure that all
	 * overlapping previous i/o is
	 * dispatched.
	 */
	
	if (size == bp->b_bcount)
		return 1;
	if (size < bp->b_bcount) {
		if (bp->b_flags & B_DELWRI) {
			bwrite(bp);
			return 0;
		}
		return 1;
	}
	bp->b_flags &= ~B_DONE;
	if (bp->b_dev == (dev_t) 0)
		return 1;

	/*
	 * 	Search cache for any buffers that overlap
	 *	the one that we are looking at.  Overlapping
	 *	buffers must be marked invalid, after being
	 * 	written out if they are dirty (indicated by
	 * 	B_DELWRI).  A disk block must be mapped by
	 *	at most one buffer at any point in time.  
	 * 	Care must be taken to avoid deadlocking when
	 * 	two buffer headers are trying to get at the
	 * 	same set of disk blocks.
	 */

	start = bp->b_blkno;
	last = start + btodb(size) - 1;
	dp = bhash(bp->b_dev, start);
	s = spl0();
loop:
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if ((ep == bp) || (ep->b_dev != bp->b_dev) ||
		    ((ep->b_flags & B_STALE) != 0)) 
			continue;
		/* look for overlap */
		if ((ep->b_bcount == 0) || (ep->b_blkno > last) ||
			((ep->b_blkno + btodb(ep->b_bcount)) <= start))
			continue;

		spl6();
		if ((ep->b_flags & B_BUSY) != 0) {
			ep->b_flags |= B_WANTED;
			syswait.iowait++;
			sleep((caddr_t)ep, PRIBIO + 1);
			syswait.iowait--;
			spl0();
			goto loop;
		}
		notavail(ep);
		splx(s);
		if ((ep->b_flags & B_DELWRI) != 0) {
			bwrite(ep);
			spl0();
			goto loop;
		}
		ep->b_flags |= B_STALE;
		brelse(ep);
		spl0();
	}	/* end for */
	splx(s);
	return 1;
}


/* 
 * Debugging print of buffer headers.  Can be invoked from kernel debugger.
 */
#ifdef DEBUG

STATIC void
printbuf()
{
	struct buf *bp;
	register i;

	printf("bufno		av_f		av_b		forw	back	flag	count\n");
	bp = &bfreelist;
	printf("%x\n", bp);
	printf("freelist	%x	%x	%x	%x	%d\n",
	  bp->av_forw, bp->av_back,bp->b_forw, bp->b_back,
	  bp->b_bcount);
	bp = &bhdrlist;
	printf("%x\n", bp);
	printf("bhdrlist	%x\n", bp->av_forw);

}

#endif

/*
 * Wait for all asynchronous scheduled i/o to complete
 * for a particular device (or any device if NODEV).
 * Normally called when unmounting a file system.
 *
 * Replaces original bdwait() which used basyncnt and basyndone()
 * to keep track of asynchronous i/o.
 */
void
bdwait(dev)
	register dev_t dev;
{
	register struct buf *bc;
	register struct buf *bp;
	register int s;
	extern void cleanup();

bloop:
	bc = bufchain;
	while (bc) {
		for (bp = bc + v.v_buf; bp != bc; bp--) {
			if ((bp->b_flags & (B_DONE|B_ASYNC)) == B_ASYNC &&
				(dev == NODEV || dev == bp->b_edev)) {
				goto bloop;
			}
		}
		bc = bc->av_forw;
	}
        
/*
 * It's somewhat expensive to run the following code within spls,
 * but given the current implementation, there is not
 * much choice.  This routine is not a high runner anyway.
*/
ploop:
	s = spl6();
	for (bp = pageio_out.b_forw; bp != (struct buf *)&pageio_out; 
		bp = bp->b_forw) {
		if ((bp->b_flags & (B_DONE|B_ASYNC)) == B_ASYNC &&
			(dev == NODEV || dev == bp->b_edev)) {
			(void) splx(s);
			cleanup();
			goto ploop;
		}
	}
	(void) splx(s);
}
