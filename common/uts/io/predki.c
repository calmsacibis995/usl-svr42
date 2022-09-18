/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/predki.c	1.8.6.7"
#ident	"$Header: $"

/*
 * This file
 *	-contains common block and character device interface functions which
 *	 translate new style device driver interface into old style driver
 *	 interface (pre SVR4.0) for old style device drivers before calling the 
 *	 old style device driver.
 *	-sets up the translation functions for the old style drivers.
 */

#include <fs/buf.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/ddi.h>
#include <io/uio.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

extern void gen_strategy();
extern int gen_read();
extern int gen_write();
extern int gen_ioctl();
extern int gen_bopen();
extern int gen_bclose();
extern int gen_copen();
extern int gen_cclose();

/*
 * "Shadow" device switch tables.
 * The space for these is allocated by cunix.
 */
extern struct bdevsw shadowbsw[];	/* block shadow switch table */
extern struct cdevsw shadowcsw[];	/* character shadow switch table */

/*
 * fix_bswtbl() and fix_cswtbl() have been broken out from
 * fix_swtbls(), so they can be used by the dynamically
 * loadable modules feature.
 */

void
fix_bswtbl(int major)
{
	if (*bdevsw[major].d_flag & D_OLD) {
		shadowbsw[major].d_open = bdevsw[major].d_open;
		bdevsw[major].d_open = gen_bopen;
		shadowbsw[major].d_close = bdevsw[major].d_close;
		bdevsw[major].d_close = gen_bclose;
	}
	if (!(*bdevsw[major].d_flag & D_NOBRKUP)) {
		shadowbsw[major].d_strategy = bdevsw[major].d_strategy;
		bdevsw[major].d_strategy = (int(*)())gen_strategy;
	}
}

void
fix_cswtbl(int major)
{
	if (*cdevsw[major].d_flag & D_OLD) {
		shadowcsw[major].d_open = cdevsw[major].d_open;
		cdevsw[major].d_open = gen_copen;
		shadowcsw[major].d_close = cdevsw[major].d_close;
		cdevsw[major].d_close = gen_cclose;
		shadowcsw[major].d_read = cdevsw[major].d_read;
		cdevsw[major].d_read = gen_read;
		shadowcsw[major].d_write = cdevsw[major].d_write;
		cdevsw[major].d_write = gen_write;
		shadowcsw[major].d_ioctl = cdevsw[major].d_ioctl;
		cdevsw[major].d_ioctl = gen_ioctl;
	}
}

void
fix_swtbls()
{
	register int i;

	for (i = 0; i < bdevcnt; i++) {
		fix_bswtbl(i);
	}
	for (i = 0; i < cdevcnt; i++) {
		fix_cswtbl(i);
	}
}

/*
 * Setup old style user and group identification from the process's
 * credentials, since this is where old style drivers will access this
 * information if they need it.
 */
void
gen_setup_idinfo(cr)
	register struct cred *cr;
{
	u.u_uid = cr->cr_uid;
	u.u_ruid = cr->cr_ruid;
	u.u_gid = cr->cr_gid;
	u.u_rgid = cr->cr_rgid;
}

/*
 * The following listed functions translate the new style driver interface
 * into the old style driver interface by setting up certain areas of the
 * u-block (area saved for compatibility) old style drivers expect to be
 * setup and calling the old style driver's interface functions, saved in the
 * shadow switch tables, with the old style arguments and formats:
 *	-gen_copen(), gen_bopen(), gen_cclolse(), gen_bclose(), gen_read(),
 *	 gen_write(), gen_ioctl(), gen_strategy().
 */
/* ARGSUSED */
STATIC int 
gen_copen(devp, flag, type, cr)
	dev_t *devp;
	int flag;
	int type;
	struct cred *cr;
{
	gen_setup_idinfo(cr);
	(void)(*shadowcsw[getmajor(*devp)].d_open)(cmpdev(*devp), flag, type);
	return u.u_error;
}

/* ARGSUSED */
STATIC int 
gen_bopen(devp, flag, type, cr)
	dev_t *devp;
	int flag;
	int type;
	struct cred *cr;
{
	gen_setup_idinfo(cr);
	(void)(*shadowbsw[getmajor(*devp)].d_open)(cmpdev(*devp), flag, type);
	return u.u_error;
}

/* ARGSUSED */
STATIC int 
gen_cclose(dev, flag, type, cr)
	dev_t dev;
	int flag;
	int type;
	struct cred *cr;
{
	gen_setup_idinfo(cr);
	(void)(*shadowcsw[getmajor(dev)].d_close)(cmpdev(dev), flag, type);
	return u.u_error;
}

/* ARGSUSED */
STATIC int 
gen_bclose(dev, flag, type, cr)
	dev_t dev;
	int flag;
	int type;
	struct cred *cr;
{
	gen_setup_idinfo(cr);
	(void)(*shadowbsw[getmajor(dev)].d_close)(cmpdev(dev), flag, type);
	return u.u_error;
}

/*
 * For gen_read() and gen_write() also have to setup the old style areas in
 * the u-block in order to do I/O.
 * u_count is set to uio_resid, since uio_resid is initially set to the length
 * of the I/O request (i.e. the sum of all the iov_len for every iovec
 * comprising the I/O request).
 * After the read or write I/O request has completed, uio_resid (which actually
 * is defined to indicate the number of bytes not read or written) is set
 * to u_count since old style drivers set u_count to the number of bytes
 * not read or written from the I/O request.
 */

/* ARGSUSED */
STATIC int
gen_read(dev, uiop, cr)
	register dev_t dev;
	register struct uio *uiop;
	struct cred *cr;
{
	gen_setup_idinfo(cr);
	u.u_offset = uiop->uio_offset;
	u.u_base = uiop->uio_iov->iov_base;
	u.u_count = uiop->uio_resid;
	u.u_segflg = uiop->uio_segflg;
	u.u_fmode = uiop->uio_fmode;

	(void)(*shadowcsw[getmajor(dev)].d_read)(cmpdev(dev));

	uiop->uio_resid = u.u_count;
	uiop->uio_offset = u.u_offset;
	return u.u_error;
}

/* ARGSUSED */
STATIC int
gen_write(dev, uiop, cr)
	register dev_t dev;
	register struct uio *uiop;
	struct cred *cr;
{
	gen_setup_idinfo(cr);
	u.u_offset = uiop->uio_offset;
	u.u_base = uiop->uio_iov->iov_base;
	u.u_count = uiop->uio_resid;
	u.u_segflg = uiop->uio_segflg;
	u.u_fmode = uiop->uio_fmode;

	(void)(*shadowcsw[getmajor(dev)].d_write)(cmpdev(dev));

	uiop->uio_resid = u.u_count;
	uiop->uio_offset = u.u_offset;
	return u.u_error;
}

/* ARGSUSED */
STATIC int
gen_ioctl(dev, cmd, arg, mode, cr, rvalp)
	register dev_t dev;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	gen_setup_idinfo(cr);
	(void)(*shadowcsw[getmajor(dev)].d_ioctl)(cmpdev(dev), cmd, arg, mode);
	*rvalp = u.u_rval1;
	return u.u_error;
}

/*
 * This is the "buffer management" I/O completion function for "split up"
 * (by gen_strategy()) I/O requests.
 * When gen_strategy() splits up an I/O request, whose data is referenced by
 * the page cache (from now on referred to as Paged I/O), into sub-requests
 * (of size in bytes PAGESIZE), it sets the iodone function (b_iodone) in 
 * the sub requests buffer header to gen_iodone().  When the block device
 * driver completes an I/O request it calls iodone() to indicate to the
 * buffer management that the I/O request has completed.  If the b_iodone
 * field in the buffer header is not NULL, iodone() calls the function specified
 * in b_iodone (which is gen_iodone()).
 */
STATIC void
gen_iodone(bp)
	register struct buf *bp;
{
	register struct buf *parentbp;	/* buffer header of originating I/O */

	if (bp->b_chain == NULL) {
		biodone(bp);
		return;
	}
	/* 
	 * This is cleared so that when biodone() is called from here, to
	 * finish the I/O completion for the buffer header, this function will
	 * not be called again.
	 */
	bp->b_iodone = NULL;

	parentbp = bp->b_chain;
	/*
	 * Pass along error notifications and causes from child I/O request
	 * (i.e. I/O sub request) to the parent I/O request (i.e. the original
	 * I/O request when gen_strategy() was called).
	 */
	if (bp->b_flags & B_ERROR) {
		parentbp->b_flags |= B_ERROR;
		if (bp->b_error)
			parentbp->b_error = bp->b_error;
		else if (bp->b_oerror)
			parentbp->b_error = bp->b_oerror;
	}
	/*
	 * Buffer header cleanup of asynchronous I/O is done here; synchronous
	 * is done in gen_strategy().
	 * The start of the asynchronous write was noted in pageio_setup().
	 * Free buffer head (via pageio_done()) used for the split up I/O.
	 */
	if (bp->b_flags & B_ASYNC) {
		pageio_done(bp);
		parentbp->b_reqcnt--;
		/*
		 * When all split up I/O requests have completed, notify buffer
		 * management that the parent (i.e. the entire originating)
		 * I/O request has completed.
		 */
		if (parentbp->b_reqcnt == 0) {
			biodone(parentbp);
			return;
		}
	} else {	/* for synchronous I/O */
		/*
		 * Called with b_iodone set to NULL.  Mark I/O complete on the
		 * buffer header.
		 */
		biodone(bp);
		return;
	}
}

/*
 * This number should be larger than the number of pages requested in any
 * "paged I/O" request.
 */
#define MAXIOREQ 32  

void
gen_strategy(bp) 
	struct buf *bp;
{
	register struct page *pp;
	register int i, flags;
	struct buf *bufp[MAXIOREQ];
	int bytescnt, s, req, err;
	int blkincr;	/* num disk sectors (physical disk blocks) per virtual page */
	
	gen_setup_idinfo(u.u_cred);
	/*
	 * Do not need to split up an I/O request if number of bytes requested
	 * to transfer is a page or less.
	 */
	if (bp->b_bcount <= PAGESIZE) {
		/*
		 * Old style drivers need b_addr to be a kernel virtual address.
		 * If buffer header indicates that it set up to do "paged I/O",
		 * need to convert b_addr to a kernel virtual address, since
		 * b_addr is just a byte offset into the first page of the
		 * list of pages specified by b_pages when buffer header is
		 * setup for "paged I/O".
		 */
		if (bp->b_flags & B_PAGEIO) {
			bp_mapin(bp);
		}
		(*shadowbsw[getmajor(bp->b_edev)].d_strategy)(bp);
		return;
	}
	/*
	 * I/O request is greater than PAGESIZE;  calculate the physical disk
	 * block increment, blkincr, and calculate the number of subrequests
	 * to split the I/O.
	 */
	blkincr = PAGESIZE / NBPSCTR;
	req = bp->b_bcount / PAGESIZE;
	if (bp->b_bcount % PAGESIZE)
		req++;
	bp->b_reqcnt = req;
	
	pp = bp->b_pages;
	/*
	 * I/O request is not from/to "paged I/O".  buf_breakup() will split up
	 * the I/O request PAGESIZE chunks and call gen_strategy() again.
	 * gen_strategy() will then call the device driver's real strategy
	 * function.
	 */
	if (pp == NULL) {
		(void)buf_breakup(gen_strategy, bp);
		return;
	}
	/*
	 * I/O request is from "paged I/O".  Split up I/O request into PAGESIZE
	 * chunks and convert appropriate buffer header fields to old driver
	 * style before calling the device driver's strategy function.
	 * -pageio_setup(): allocates and initializes a buffer header for
	 *  "paged I/O".
	 * -b_blkno: it is assumed that entire I/O request is from/to physically
	 *  contiguous disk blocks.
	 * -b_iodone: gen_iodone() is called before the actual iodone() function
	 *  completes.
	 * -b_addr: the page number is converted into the data buffer's
	 *  (associated with the buffer header) kernel virtual address for old
	 *  style device drivers.
	 */
	i = 0;
	do {
		/*
		 * Assumption: pages in the list were sorted.
		 */
		if (i == (req - 1)) {	/* last I/O */
		    if (bp->b_bcount % PAGESIZE)
		    	bytescnt = bp->b_bcount % PAGESIZE;
		    else 
		    	bytescnt = PAGESIZE;
		} else
		    bytescnt = PAGESIZE;
		flags = (bp->b_flags & B_ASYNC) ? B_ASYNC : 0;
		flags |= (bp->b_flags & B_READ) ? B_READ : B_WRITE;
		bufp[i] = pageio_setup(pp, bytescnt, 0, flags);
		bufp[i]->b_edev = bp->b_vp->v_rdev;
		bufp[i]->b_dev = cmpdev(bufp[i]->b_edev);
		bufp[i]->b_blkno = bp->b_blkno + (blkincr * i);
		bufp[i]->b_chain = bp;
		bufp[i]->b_iodone = gen_iodone;
		bufp[i]->b_flags &= ~B_PAGEIO;
		bufp[i]->b_un.b_addr = (caddr_t)pfntokv(page_pptonum(pp));
		(*shadowbsw[getmajor(bufp[i]->b_edev)].d_strategy)(bufp[i]);
		i++;
		pp = pp->p_next;
	} while ((pp != bp->b_pages) && (i < MAXIOREQ));
	ASSERT(i != MAXIOREQ);

	/*
	 * Wait for all non-asynchronous I/O sub-requests to complete
	 * (asynchronous I/O sub requests are handled in gen_iodone());
	 * Cleanup allocated buffer headers used by the I/O sub-requests.
	 * After all I/O sub-requests have completed call biodone() to
	 * mark I/O complete for entire (originating) I/O request.
	 */
	if (!(bp->b_flags & B_ASYNC)) {
		err = 0;
		for (i = 0; i < req; i++) {
	    		if (err)
				(void) biowait(bufp[i]);
	    		else
				err = biowait(bufp[i]);
	    		pageio_done(bufp[i]);
	    		bp->b_reqcnt--;
		}
		/* arbitrarily picking up one of the errors */
		if (err) {
			bp->b_flags |= B_ERROR;
			bp->b_error = err;
		}
		ASSERT(bp->b_reqcnt == 0);
	    	ASSERT((bp->b_flags & B_DONE) == 0);
	    	s = spl6();
		biodone(bp);
	    	splx(s);
	}	
}
