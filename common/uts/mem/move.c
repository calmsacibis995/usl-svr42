/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:mem/move.c	1.5.2.3"
#ident	"$Header: $"

#include <io/uio.h>
#include <mem/faultcatch.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Move "n" bytes at byte address "cp"; "rw" indicates the direction
 * of the move, and the I/O parameters are provided in "uio", which is
 * updated to reflect the data that was moved.  Returns 0 on success or
 * a non-zero errno on failure.
 * This interface includes protection against kernel page fault errors.
 * A fault error in any pageable kernel address will cause a non-zero
 * errno to be returned.
 */
int
uiomove(cp, n, rw, uio)
	register caddr_t cp;
	register long n;
	enum uio_rw rw;
	register struct uio *uio;
{
	register struct iovec *iov;
	uint cnt;
	int error = 0;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = MIN(iov->iov_len, n);
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		switch (uio->uio_segflg) {

		case UIO_USERSPACE:
		case UIO_USERISPACE:
			error = (rw == UIO_READ
				? ucopyout(cp, iov->iov_base, cnt,
							CATCH_KERNEL_FAULTS)
				: ucopyin(iov->iov_base, cp, cnt,
							CATCH_KERNEL_FAULTS));
			break;

		case UIO_SYSSPACE:
			CATCH_FAULTS(CATCH_KERNEL_FAULTS) {
				if (rw == UIO_READ)
					bcopy((caddr_t)cp, iov->iov_base, cnt);
				else
					bcopy(iov->iov_base, (caddr_t)cp, cnt);
			}
			error = END_CATCH();
			break;

		default:
			error = EFAULT;		/* invalid segflg value */
		}
		if (error)
			return error;
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return 0;
}



/* function: ureadc()
 * purpose:  transfer a character value into the address space
 *           delineated by a uio and update fields within the
 *           uio for next character. Return 0 for success, EFAULT
 *           for error.
 */
int
ureadc(val, uiop)
	int val;
	register struct uio *uiop;
{
	register struct iovec *iovp;
	unsigned char c;

	/*
	 * first determine if uio is valid.  uiop should be 
	 * non-NULL and the resid count > 0.
	 */
	if (!(uiop && uiop->uio_resid > 0)) 
		return EFAULT;

	/*
	 * scan through iovecs until one is found that is non-empty.
	 * Return EFAULT if none found.
	 */
	while (uiop->uio_iovcnt > 0) {
		iovp = uiop->uio_iov;
		if (iovp->iov_len <= 0) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else
			break;
	}

	if (uiop->uio_iovcnt <= 0)
		return EFAULT;

	/*
	 * Transfer character to uio space.
	 */

	c = (unsigned char) (val & 0xFF);

	switch (uiop->uio_segflg) {

	case UIO_USERISPACE:
	case UIO_USERSPACE:
		if (ucopyout((caddr_t)&c, iovp->iov_base,
					sizeof(unsigned char), 0))
			return EFAULT;
		break;

	case UIO_SYSSPACE: /* can do direct copy since kernel-kernel */
		*iovp->iov_base = c;
		break;

	default:
		return EFAULT;		/* invalid segflg value */
	}

	/*
	 * bump up/down iovec and uio members to reflect transfer.
	 */
	iovp->iov_base++;
	iovp->iov_len--;
	uiop->uio_resid--;
	uiop->uio_offset++;
	return 0; /* success */
}


/* function: uwritec()
 * purpose:  return a character value from the address space
 *           delineated by a uio and update fields within the
 *           uio for next character. Return the character for success,
 *           -1 for error.
 */
int
uwritec(uiop)
	register struct uio *uiop;
{
	register struct iovec *iovp;
	unsigned char c;

	/* verify we were passed a valid uio structure.
	 * (1) non-NULL uiop, (2) positive resid count
	 * (3) there is an iovec with positive length 
	 */

	if (!(uiop && uiop->uio_resid > 0)) 
		return -1;

	while (uiop->uio_iovcnt > 0) {
		iovp = uiop->uio_iov;
		if (iovp->iov_len <= 0) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else
			break;
	}

	if (uiop->uio_iovcnt <= 0)
		return -1;

	/*
	 * Get the character from the uio address space.
	 */
	switch (uiop->uio_segflg) {

	case UIO_USERISPACE:
	case UIO_USERSPACE:
		if (ucopyin(iovp->iov_base, (caddr_t)&c,
					sizeof(unsigned char), 0))
			return -1;
		break;

	case UIO_SYSSPACE:
		c = *iovp->iov_base;
		break;

	default:
		return -1; /* invalid segflg */
	}

	/*
	 * Adjust fields of iovec and uio appropriately.
	 */
	iovp->iov_base++;
	iovp->iov_len--;
	uiop->uio_resid--;
	uiop->uio_offset++;
	return (int)c & 0xFF; /* success */
}

/*
 * uioupdate -
 * Update "uio" to reflect that "n" bytes of data were
 * (or were not) moved.  The state of the iovecs are not
 * updated and are not consistent with uio_resid.  Positive
 * "n" means n bytes were copied.  Negative "n" means "uncopy"
 * n bytes.
 */
void
uioupdate(uiop, n)
	register struct uio *uiop;
	register long n;
{
	if ((n > 0) && (n > uiop->uio_resid))
		n = uiop->uio_resid;
	uiop->uio_resid -= n;
	uiop->uio_offset += n;
}

/*
 * Drop the next n chars out of *uiop.
 */
void
uioskip(uiop, n)
	register uio_t	*uiop;
	register size_t	n;
{
	if (n > uiop->uio_resid)
		return;
	while (n != 0) {
		register iovec_t	*iovp = uiop->uio_iov;
		register size_t		niovb = MIN(iovp->iov_len, n);

		if (niovb == 0) {
			uiop->uio_iov++;
			uiop->uio_iovcnt--;
			continue;
		}	
		iovp->iov_base += niovb;
		uiop->uio_offset += niovb;
		iovp->iov_len -= niovb;
		uiop->uio_resid -= niovb;
		n -= niovb;
	}
}

/*
 * Move MIN(ruio->uio_resid, wuio->uio_resid) bytes from addresses described
 * by ruio to those described by wuio.  Both uio structures are updated to
 * reflect the move. Returns 0 on success or a non-zero errno on failure.
 */
int
uiomvuio(ruio, wuio)
	register uio_t *ruio;
	register uio_t *wuio;
{
	register iovec_t *riov;
	register iovec_t *wiov;
	register long n;
	uint cnt;
	int kerncp;
	int err;

	n = MIN(ruio->uio_resid, wuio->uio_resid);
	kerncp = ruio->uio_segflg == UIO_SYSSPACE &&
	  wuio->uio_segflg == UIO_SYSSPACE;

	riov = ruio->uio_iov;
	wiov = wuio->uio_iov;
	while (n) {
		while (!wiov->iov_len) {
			wiov = ++wuio->uio_iov;
			wuio->uio_iovcnt--;
		}
		while (!riov->iov_len) {
			riov = ++ruio->uio_iov;
			ruio->uio_iovcnt--;
		}
		cnt = MIN(wiov->iov_len, MIN(riov->iov_len, n));

		if (kerncp)
			bcopy(riov->iov_base, wiov->iov_base, cnt);
		else {
		    if (ruio->uio_segflg == UIO_SYSSPACE)
			err = ucopyout(riov->iov_base, wiov->iov_base, cnt, 0);
		    else
			err = ucopyin(riov->iov_base, wiov->iov_base, cnt, 0);
		    if (err)
			return err;
		}

		riov->iov_base += cnt;
		riov->iov_len -= cnt;
		ruio->uio_resid -= cnt;
		ruio->uio_offset += cnt;
		wiov->iov_base += cnt;
		wiov->iov_len -= cnt;
		wuio->uio_resid -= cnt;
		wuio->uio_offset += cnt;
		n -= cnt;
	}
	return 0;
}

/*
 * Copy in a string from user space, with size check.
 * The number of characters copied is returned via the
 * result parameter np, provided it is non-null.
 * Returns 0 on success, a non-zero errno on failure.
 */
int
copyinstr(from, to, max, np)
	char *from;	/* Source address (user space) */
	char *to;	/* Destination address */
	size_t max;	/* Maximum number of characters to move */
	size_t *np;	/* Number of characters moved is returned here */
{
	int n;

	if (np)
		*np = 0;
	switch (n = upath(from, to, max)) {
	case -2:
		return ENAMETOOLONG;
	case -1:
		return EFAULT;
	default:
		if (np)
			*np = n + 1;	/* Include null byte */
		return 0;
	}
	/* NOTREACHED */
}

/*
 * Copy a string, with size check.
 * The number of characters copied is returned via the
 * result parameter np.  Unlike copyinstr(), we require
 * np to be non-NULL.
 * Returns 0 on success, a non-zero errno on failure.
 */
int
copystr(from, to, max, np)
	char *from;	/* Source address (system space) */
	char *to;	/* Destination address */
	size_t max;	/* Maximum number of characters to move */
	size_t *np;	/* Number of characters moved is returned here */
{
	int n;

	ASSERT(np != NULL);
	*np = 0;
	switch (n = spath(from, to, max)) {
	case -2:
		return ENAMETOOLONG;
	case -1:
		return EFAULT;
	default:
		*np = n + 1;	/* Include null byte */
		return 0;
	}
	/* NOTREACHED */
}

/*
 * Zero a kernel buffer with protection against kernel page fault errors.
 * A fault error in any pageable kernel address will cause a non-zero
 * errno to be returned.
 */
int
kzero(dst, cnt)
	caddr_t	dst;
	uint	cnt;
{
	CATCH_FAULTS(CATCH_KERNEL_FAULTS)
		bzero(dst, cnt);
	return END_CATCH();
}
