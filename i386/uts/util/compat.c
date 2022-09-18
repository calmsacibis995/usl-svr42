/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/compat.c	1.4"
#ident	"$Header: $"

/*
 * Routines in this file exist only for backward compatibility and should
 * be unused in the porting base.  Each of them will go away eventually.
 *
 * When you add a routine to this file, it would be helpful if you
 * included comments as to what the routine does, when it was introduced,
 * why it is being retained, and when it can be eliminated.
 */

#include <acc/audit/audit.h>
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <io/uio.h>
#include <mem/seg_kmem.h>
#include <mem/vmparam.h>
#include <mem/faultcatch.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/resource.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Move "n" bytes at byte location "cp" to or from (flag) a user
 * or kernel (u.segflg) area.  The I/O parameters are in u.u_base,
 * u.u_count, and u.u_offset, and all are updated to reflect the
 * number of bytes moved.
 *
 * This routine has been replaced by uiomove() and is retained
 * only for backward compatibility with old device drivers.
 */

void
iomove(cp, n, flag)
	register caddr_t cp;
	register n;
	int flag;
{
	register t;

	if (n == 0)
		return;
	if (u.u_segflg != 1)  {
		if (flag == B_WRITE)
			t = ucopyin(u.u_base, (caddr_t)cp, n, 0);
		else
			t = ucopyout((caddr_t)cp, u.u_base, n, 0);
		if (t) {
			u.u_error = EFAULT;
			return;
		}
	} else
		if (flag == B_WRITE)
			bcopy(u.u_base, (caddr_t)cp, n);
		else
			bcopy((caddr_t)cp, u.u_base, n);
	u.u_base += n;
	u.u_offset += n;
	u.u_count -= n;
}

/*
 * Pass back "c" to the user at location u_base;
 * update u_base, u_count, and u_offset.  Return -1
 * on the last character of the user's read.
 * u_base is in the user data space.
 *
 * Retained for compatibility with old device drivers.
 */

passc(c)
	register c;
{
	if (subyte(u.u_base, c) < 0) {
		u.u_error = EFAULT;
		return -1;
	}
	u.u_count--;
	u.u_offset++;
	u.u_base++;
	return (u.u_count == 0) ? -1: 0;
}

/*
 * Pick up and return the next character from the user's
 * write call at location u_base;
 * update u_base, u_count, and u_offset.  Return -1
 * when u_count is exhausted.
 * u_base is in the user data space.
 *
 * Retained for compatibility with old device drivers.
 */

cpass()
{
	register c;

	if (u.u_count == 0)
		return -1;
	if ((c = fubyte(u.u_base)) < 0) {
		u.u_error = EFAULT;
		return -1;
	}
	u.u_count--;
	u.u_offset++;
	u.u_base++;
	return c;
}

/*
 * Raw I/O. The arguments are
 *	the strategy routine for the device;
 *	a buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose;
 *	the device number;
 *	a read/write flag.
 *
 * We simply set up the I/O parameters using the (obsolete)
 * u.u_base, u.u_count, etc., and let uiophysio() do the work.
 *
 * This routine has been replaced by uiophysio() and is retained
 * only for backward compatibility with old device drivers.
 */

void
physio(strat, bp, dev, rw)
	void (*strat)();
	register struct buf *bp;
	dev_t dev;
	int rw;
{
	struct iovec iovec;
	struct uio uio;
	int error;

	iovec.iov_base = u.u_base;
	iovec.iov_len = u.u_count;
	uio.uio_iov = &iovec;
	uio.uio_iovcnt = 1;
	uio.uio_offset = u.u_offset;
	uio.uio_segflg = (u.u_segflg == 0) ? UIO_USERSPACE : UIO_SYSSPACE;
	uio.uio_fmode = (rw == B_READ) ? FREAD : FWRITE;	/* XXX */
	uio.uio_limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	uio.uio_resid = u.u_count;				/* XXX */

		/* old driver interface so expand the dev */
	if ((error = uiophysio(strat, bp, expdev(dev), rw, &uio)) != 0)
		u.u_error = (char) error;

	u.u_count = uio.uio_resid;
	u.u_offset = uio.uio_offset;
}

/*
 * physck() checks the limits of the access specified by "rw"
 * for a read or write system call to a raw block device.
 * nblocks is the size of the device in sectors.
 *
 * This routine is obsolete and is retained only for backward
 * compatibility with old device drivers.
 */

int
physck(nblocks, rw)
	daddr_t nblocks;
	int rw;
{
	register unsigned over;
	register off_t upper, limit;
	struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap;

	limit = nblocks << SCTRSHFT;
	if (u.u_offset >= limit) {
		if (u.u_offset > limit || rw == B_WRITE)
			u.u_error = ENXIO;
		return 0;
	}
	upper = u.u_offset + u.u_count;
	if (upper > limit) {
		over = upper - limit;
		u.u_count -= over;
		uap = (struct a *)u.u_ap;
		uap->count -= over;
	}
	return 1;
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 *
 * iowait() was renamed biowait() in SVR4.0, although the last
 * calls to it were not removed from the porting base until SVR4.1.
 * This stub is retained for compatibility with third-party code.
 */

int
iowait(bp)
	struct buf *bp;
{
	return biowait(bp);
}

/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 *
 * iodone() was renamed biodone() in SVR4.0, although the last
 * calls to it were not removed from the porting base until SVR4.1.
 * This stub is retained for compatibility with third-party code.
 */

void
iodone(bp)
	struct buf *bp;
{
	biodone(bp);
}

/*
 * This routine will be removed in p8, use pn_get instead.
 */
int
userstrlen(str)
	caddr_t	str;
{
	int	len;

	if (!VALID_USR_RANGE(str, 1))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT) {
		len = strlen(str);
	}
	if (END_CATCH() != 0)
		return -1;
	return len;
}

/*
 * Test if the supplied credentials identify a privileged process.
 *
 * NOTE: This routine is very similar to the pm_denied() routine found in the
 *	 privilege mechanism except that the requested privilege is not passed
 *	 as an argument to the routine and the value returned by this routine
 *	 to indicate if this is a privileged process is opposite that of pm_denied.
*/ 

extern	int	adt_priv();	/* defined in acc/audit/auditrec.c */

int
suser(cr)
	register struct cred *cr;
{
	register int	ret = 0,
			priv = P_ALLPRIVS;
	/*
	 * The call to the pm_privon() macro will check for ANY bit
	 * set to ``1'' in the working privilege vector.  If this is
	 * the case, the process is then considered privileged.
	 *
	 * NOTE:	This is not totally correct as far as the new
	 *		privilege mechanism is concerned, but it should
	 *		suffice for those routines that still call suser()
	 *		to determine if the process is privileged.
	 *
	 * WARNING:	This is NOT considered to be a secure implementation
	 *		of the privilege mechanism.
	*/

	ASSERT(cr != NULL);

	if (pm_privon(cr, priv)) 
		ret = 1;
	/*
	 * Since the logic of the pm_denied() routine is opposite
	 * that of the suser() routine, the inverse of the ``ret''
	 * value must be sent to the adt_priv() routine to report
	 * the correct status as to SUCCESS or FAILURE of the
	 * privilege request.
	*/
	ADT_PRIV(priv, !ret, cr);	/* audit the "use of privilege" */

	if (ret) 
		acctevt(ASU);
	return ret;
}
