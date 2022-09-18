/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/clone/clone.c	1.3.3.7"
#ident	"$Header: $"
/*
 * Clone Driver.
 */

#include <fs/vnode.h>
#include <io/conf.h>
#include <io/mkdev.h>
#include <io/stream.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>

#define		CLN_ERR(e)	{ error = (e); goto cln_done; }

extern	clock_t	lbolt;
extern	struct	modctl	*mod_shadowcsw[];

/*
 * Defining the variable clndevflag indicates the new style driver
 * open/close interface.
 */
int clndevflag = 0;

int clnopen();
static struct module_info clnm_info = { 0, "CLONE", 0, 0, 0, 0 };
static struct qinit clnrinit = { NULL, NULL, clnopen, NULL, NULL, &clnm_info, NULL };
static struct qinit clnwinit = { NULL, NULL, NULL, NULL, NULL, &clnm_info, NULL };
struct streamtab clninfo = { &clnrinit, &clnwinit };

/*
 * Clone open.  Maj is the major device number of the streams
 * device to open.  Look up the device in the cdevsw[].  Attach
 * its qinit structures to the read and write queues and call its
 * open with the sflag set to CLONEOPEN.  Swap in a new vnode with
 * the real device number constructed from either
 *	a) for old-style drivers:
 *		maj and the minor returned by the device open, or
 *	b) for new-style drivers:
 *		the whole dev passed back as a reference parameter
 *		from the device open.
 */
int
clnopen(qp, devp, flag, sflag, crp)
	register queue_t *qp;
	dev_t *devp;
	int flag;
	int sflag;
	cred_t *crp;
{
	register struct streamtab *stp;
	struct	modctl	*modctlp;
	const	char	*mod_name;
	dev_t newdev;
	int error;
	major_t maj;
	minor_t emaj;

	if (sflag)
		return (ENXIO);

	/*
	 * Get the device to open.
	 */
	emaj = getminor(*devp); /* minor is major for a cloned drivers */
	maj = etoimajor(emaj);	/* get internal major of cloned driver */

	if(maj >= cdevcnt || !(stp = cdevsw[maj].d_str))	{
		return(ENXIO);
	}

	if(modctlp = mod_shadowcsw[maj])	{
		MOD_HOLD(modctlp);
	}

	/*
	 * Substitute the real qinit values for the current ones.
	 */
	setq(qp, stp->st_rdinit, stp->st_wrinit);

	/*
	 * Call the device open with the stream flag CLONEOPEN.  The device
	 * will either fail this or return a minor device number (for old-
	 * style drivers) or the whole device number (for new-style drivers).
	 */
	newdev = makedevice(emaj, 0);	/* create new style device number  */

	error = 0;

	if (*cdevsw[maj].d_flag & D_OLD) {
		int oldev;

		/*
		 * The QOLD flag is an indication to various Stream Head
		 * functions that the old style (Pre SVR4.0) open/close
		 * interface is being used.
		 */
		qp->q_flag |= QOLD;
		WR(qp)->q_flag |= QOLD;

		/*
		 * Old style drivers get the old device format
		 * so make sure it fits by compressing new device format
		 * into old device format.
		 */
		if ((oldev = (o_dev_t)cmpdev(newdev)) == NODEV)
			CLN_ERR(ENXIO);
		/*
		 * newdev will be the minor device number since this is
		 * for pre-SVR4 drivers.
		 */
		if ((newdev = (*qp->q_qinfo->qi_qopen)(qp, oldev, flag, CLONEOPEN)) == OPENFAIL)
			CLN_ERR(u.u_error == 0 ? ENXIO : u.u_error);	/* XXX */


		/* "Return" device number to caller in new style format since
		 * the original device driver opened, the clone device driver,
		 * is a new style driver.  The calling function expects the
		 * type of device returned to be the type it opened.
		 */
		*devp = makedevice(emaj, (newdev & OMAXMIN));
	} else {
		qp->q_flag &= ~QOLD;
		WR(qp)->q_flag &= ~QOLD;
		if (error = (*qp->q_qinfo->qi_qopen)(qp, &newdev, flag, CLONEOPEN, crp))
			CLN_ERR(error);
		if ((getmajor(newdev) > cdevcnt) || !(stp = cdevsw[getmajor(newdev)].d_str)) {
			(*qp->q_qinfo->qi_qclose)(qp, flag, crp);
			CLN_ERR(ENXIO);
		}
		/*
		 * This is done again in case the major device number was
		 * changed when the open of the drivers Clone minor returned.
		 */
		setq(qp, stp->st_rdinit, stp->st_wrinit);
		*devp = newdev;
	}

cln_done:

	if(error && modctlp)	{
		MOD_RELE(modctlp);
	}
	return(error);
}	
