/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/nullzero/nullzero.c	1.7.2.3"
#ident	"$Header: $"


#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>	/* define before ddi.h */
#include <svc/systm.h>
#include <io/conf.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/disp.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <proc/mman.h>
#include <mem/vmsystm.h>
#include <mem/as.h>
#include <mem/seg_vn.h>
#include <io/poll.h>

#ifdef __STDC__
STATIC int nzrw(dev_t, struct uio *, struct cred *, enum uio_rw);
#else
STATIC int nzrw();
#endif

#define	M_NULL		0	/* /dev/null - EOF & Rathole */
#define	M_ZERO		1	/* /dev/zero - source of private memory */

int nzdevflag =(D_INITPUB | D_NOSPECMACDATA);   /* initialized security flags */
						/* public device, no Mac checks */
						/* for data transfer */


/* ARGSUSED */
int
nzopen(devp, flag, type, cr)
        dev_t *devp;
        int flag;
	int type;
        struct cred *cr;
{
        return 0;
}

/* ARGSUSED */
int
nzclose(dev, flag, cr)
        dev_t dev;
        int flag;
        struct cred *cr;
{
        return 0;
}

/* ARGSUSED */
int
nzioctl(dev, cmd, arg, flag, cr, rvalp)
	dev_t dev;
	int cmd;
	int arg;
	int flag;
	struct cred *cr;
	int *rvalp;
{
	return ENODEV;
}

/* ARGSUSED */
int
nzread(dev, uiop, cr)
        dev_t dev;
        struct uio *uiop;
        struct cred *cr;
{
	return (nzrw(dev, uiop, cr, UIO_READ));
}

/* ARGSUSED */
int
nzwrite(dev, uiop, cr)
	dev_t dev;
        struct uio *uiop;
        struct cred *cr;
{
	return (nzrw(dev, uiop, cr, UIO_WRITE));
}

/*
 * When reading the M_ZERO device, we simply copyout the zeroes
 * array in NZEROES sized chunks to the user's address.
 *
 * XXX - this is not very elegant and should be redone.
 */
#define NZEROES		0x100
static char zeroes[NZEROES];

/* ARGSUSED */
STATIC int
nzrw(dev, uiop, cr, rw)
	dev_t dev;
	struct uio *uiop;
        struct cred *cr;
	enum uio_rw rw;
{
	register off_t n;
	u_int	m;
	int error = 0;

	if((m = getminor(dev)) != M_NULL && m != M_ZERO)	{
		return ENXIO;
	}

	if(rw == UIO_WRITE)	{	/* write to M_NULL or M_ZERO */
		uiop->uio_offset += uiop->uio_resid;
		uiop->uio_resid = 0;
		return 0;
	}
	else if(m == M_NULL)	{	/* read from M_NULL */
		return 0;
	}

	/*
	 * Read form M_ZERO.
	 */
        while (uiop->uio_resid != 0) {
		/* It may take long here, so we put a preemption point */
		PREEMPT();
		n = MIN(uiop->uio_resid, sizeof (zeroes));
		if (uiomove(zeroes, n, rw, uiop))	{
			error = ENXIO;
			break;
		}
	}
	return (error);
}



/*
 * This function is called when a memory device is mmap'ed.
 * Set up the mapping to the correct device driver.
 */
int
nzsegmap(dev, off, as, addrp, len, prot, maxprot, flags, cred)
	dev_t dev;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cred;
{
	struct segvn_crargs vn_a;
		
	/* cannot map /dev/null */

	if (getminor(dev) == M_NULL)
		return ENXIO;

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * No need to worry about vac alignment since this
		 * is a "clone" object that doesn't yet exist.
		 */
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address -
		 * Blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	/*
	 * Use seg_vn segment driver for /dev/zero mapping.
	 * Passing in a NULL amp gives us the "cloning" effect.
	 */
	vn_a.vp = NULL;
	vn_a.offset = 0;
	vn_a.type = (flags & MAP_TYPE);
	vn_a.prot = (u_char)prot;
	vn_a.maxprot = (u_char)maxprot;
	vn_a.cred = cred;
	vn_a.amp = NULL;

	return (as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a));
}


/* ARGSUSED */
nzchpoll(dev, events, anyyet, reventsp, phpp)
dev_t dev;
short events;
int anyyet;
short *reventsp;
struct pollhead **phpp;
{
	*reventsp = 0;
	if (events & POLLIN)
		*reventsp |= POLLIN;
	if (events & POLLRDNORM)
		*reventsp |= POLLRDNORM;
	if (events & POLLOUT)
		*reventsp |= POLLOUT;
	*phpp = (struct pollhead *) NULL;
	return (0);
}
