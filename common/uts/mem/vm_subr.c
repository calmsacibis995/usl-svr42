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

#ident	"@(#)uts-comm:mem/vm_subr.c	1.1.3.8"
#ident	"$Header: $"

#include <fs/buf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/tuneable.h>
#include <mem/vmsystm.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/resource.h>
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

#if RESTRICTED_DMA

#include <io/conf.h>
#include <mem/rdma.h>

#endif

extern struct buf	pbuf[];		/* physio buffers, from kernel.cf */
STATIC struct pfree	pfreelist;	/* head of physio buffer headers */
STATIC int		pfreecnt;	/* count of free physio buffers */

/*
 * Initialize the physio buffer list.
 */

void
pinit()
{
	register struct buf *bp;

	pfreecnt = v.v_pbuf;
	pfreelist.av_forw = bp = pbuf;

	for (; bp < &pbuf[v.v_pbuf-1]; bp++) {
		bp->b_flags = B_KERNBUF | B_BUSY;
		bp->av_forw = bp+1;
	}

	bp->av_forw = NULL;
}

/*
 * Perform raw I/O.
 * This is a replacement for physio() with I/O parameters passed via
 * a uio structure instead of the obsolete u.u_base, u.u_count, etc.
 *
 * The arguments are
 *	the strategy routine for the device;
 *	a buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose;
 *	the device number;
 *	a read/write flag;
 *	the I/O parameters.
 *
 * Essentially all the work is computing physical addresses and
 * validating them.
 * If the user has the proper access privileges, the process is
 * marked "delayed unlock" and the pages involved in the I/O are
 * faulted and locked. After the completion of the I/O, the pages
 * are unlocked.
 */

int
uiophysio(strat, bp, dev, rw, uio)
	int (*strat)();
	struct buf *bp;
	dev_t dev;
	int rw;
	struct uio *uio;
{
	register struct iovec *iov;
	register int c;
	faultcode_t fault_err;
	struct proc *procp;
	struct as *asp;
	char *a;
	int hpf, s, error = 0;
	int kernbuf;
#if RESTRICTED_DMA
	register unsigned int maj;
#endif

	ASSERT(syswait.physio >= 0);
	syswait.physio++;
	if (rw)
		sysinfo.phread++;
	else
		sysinfo.phwrite++;

	hpf = (bp == NULL);
	if (hpf) {
		/*
		 * Get a buffer header off the free list.
		 */
		s = spl6();
		while (pfreecnt == 0)
			sleep((caddr_t)&pfreelist, PRIBIO);
		ASSERT(pfreecnt);
		ASSERT(pfreelist.av_forw);
		pfreecnt--;
		bp = pfreelist.av_forw;
		pfreelist.av_forw = bp->av_forw;
		splx(s);
	}

	ASSERT(bp->b_flags & B_BUSY);

	kernbuf = (bp->b_flags & B_KERNBUF);

	if (uio->uio_segflg == UIO_USERSPACE) {
		procp = u.u_procp;
		asp = procp->p_as;
	} else {
		procp = NULL;
		asp = &kas;
	}

	while(uio->uio_iovcnt > 0) {
		iov = uio->uio_iov;
		if ((uio->uio_segflg == UIO_USERSPACE) &&
		    (useracc(iov->iov_base, (uint)iov->iov_len,
		    rw == B_READ ? B_WRITE : B_READ) == 0)) {
			error = EFAULT;
			break;
		}

		bp->b_oerror = 0;		/* old error field */
		bp->b_error = 0;
		bp->b_proc = procp;

		while (iov->iov_len > 0) {
			if (uio->uio_resid == 0)
				break;
			bp->b_flags = kernbuf | B_BUSY | B_PHYS | rw;
			bp->b_edev = dev;
			bp->b_dev = cmpdev(dev);
			bp->b_blkno = btodt(uio->uio_offset);
			/*
			 * Don't count on b_addr remaining untouched by the
			 * code below (it may be reset because someone does
			 * a bp_mapin on the buffer) -- reset from the iov
			 * each time through, updating the iov's base address
			 * instead.
			 */
			a = bp->b_un.b_addr = iov->iov_base;
			c = bp->b_bcount = MIN(iov->iov_len, uio->uio_resid);
			fault_err = as_fault(asp, a, (uint)c, F_SOFTLOCK,
				rw == B_READ? S_WRITE : S_READ);
			if (fault_err != 0) {
				/*
				 * Even though the range of addresses were
				 * valid and had the correct permissions,
				 * we couldn't lock down all the pages for
				 * the access we needed. (e.g. we needed to
				 * allocate filesystem blocks for
				 * rw == B_READ but the file system was full).
				 */
				if (FC_CODE(fault_err) == FC_OBJERR)
					error = FC_ERRNO(fault_err);
				else
					error = EFAULT;
				bp->b_flags |= B_ERROR;
				bp->b_error = error;
				bp->b_flags &= ~B_PHYS;
				break;
			}
			if (buscheck(bp) < 0) {
				/*
				 * The io was not requested across legal pages.
				 */
				bp->b_flags |= B_ERROR;
				bp->b_error = error = EFAULT;
			} else {
#if RESTRICTED_DMA
				ASSERT(getmajor(dev) >= cdevcnt ||
					cdevsw[getmajor(dev)].d_flag);
				if (rdma_enabled &&
				    ((maj = getmajor(dev)) >= cdevcnt ||
				     (*cdevsw[maj].d_flag & D_DMA))) {
					error = rdma_physio(strat, bp, rw);
				} else
#endif /* RESTRICTED_DMA */
				{
					PHYSIO_STRAT(strat, bp);
					error = biowait(bp);
					PHYSIO_DONE(bp, a, c);
				}
			}
			if (as_fault(asp, a, (uint)c, F_SOFTUNLOCK,
			  rw == B_READ ? S_WRITE : S_READ) != 0)
				cmn_err(CE_PANIC, "physio unlock");

			c -= bp->b_resid;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_resid -= c;
			uio->uio_offset += c;
			/* bp->b_resid - temp kludge for tape drives */
			if (bp->b_resid || error)
				break;
		}
		bp->b_flags &= ~B_PHYS;
		/* bp->b_resid - temp kludge for tape drives */
		if (bp->b_resid || error)
			break;
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}

	if (hpf) {
		s = spl6();
		bp->av_forw = pfreelist.av_forw;
		pfreelist.av_forw = bp;
		pfreecnt++;
		wakeprocs((caddr_t)&pfreelist, PRMPT);
		splx(s);
	}

	ASSERT(syswait.physio);
	syswait.physio--;
	return(error);
}

/*
 * Determine whether user access of the specified type (B_READ or B_WRITE)
 * is permitted to the range of addresses [addr, addr+count).
 * Returns true if access is permitted.
 */

int
useracc(addr, count, access)
	register caddr_t addr;
	register uint count;
	register int access;
{
	uint prot;

	prot = PROT_USER | ((access == B_READ) ? PROT_READ : PROT_WRITE);
	return(as_checkprot(u.u_procp->p_as, (addr_t)addr, count, prot) == 0);
}

/*
 * Determine whether kernel access of the specified type (B_READ or B_WRITE)
 * is permitted to the range of addresses [addr, addr+count).
 * Returns true if access is permitted.
 */

int
kernacc(addr, count, access)
	register caddr_t addr;
	register uint count;
	register int access;
{
	uint prot;

	prot = ((access == B_READ) ? PROT_READ : PROT_WRITE);
	return(as_checkprot(&kas, (addr_t)addr, count, prot) == 0);
}

/*
 * Return true if free memory is "low".
 * This is an interface provided for callers outside the
 * memory management subsystem, so that they do not have to
 * understand exactly what we mean by "low" and how we check.
 */

int
memlow()
{
	return (freemem <= tune.t_gpgslo);
}
