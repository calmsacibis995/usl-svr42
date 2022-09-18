/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/gentty/gentty.c	1.17.2.1"
#ident	"$Header: $"

/*
 * Indirect driver for controlling tty.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <io/conf.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <io/strsubr.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <io/ddi.h>
#include <acc/mac/mac.h> 	/* added for security */
#include <io/termios.h>		/* added for security */

int sydevflag = (D_INITPUB | D_NOSPECMACDATA);  /* initialization of sec flags */
						/* public device, no MAC checks */
						/* on data transfer */
extern struct vnode *common_specvp();

/* ARGSUSED */
int
syopen(devp, flag, otyp, cr)
	dev_t *devp;
	int flag;
	int otyp;
	struct cred *cr;
{
	dev_t ttyd;
	register vnode_t *ttyvp;
	register vnode_t *ttycvp;
	int error = 0;

	if ((ttyd = u.u_procp->p_sessp->s_dev) == NODEV)
		return ENXIO;
	if ((ttyvp = u.u_procp->p_sessp->s_vp) == NULL)
		return EIO;
	ttycvp = common_specvp(ttyvp);
	if (mac_installed) {
		int macmode=0;
		/* 
		 * opening /dev/tty requires MAC access
		 * to the controlling tty when security is installed
		 *
		 */
		if (flag &FREAD)
			macmode |= VREAD;
		if (flag &(FWRITE|FTRUNC))
			macmode |=  VWRITE;
		if (error = VOP_ACCESS(ttycvp, macmode, MAC_ACC, cr))
			return error;
	}
	if (cdevsw[getmajor(ttyd)].d_str) {
		VN_HOLD(ttyvp);
		error =  stropen(ttycvp, &ttyd, flag, cr);
		VN_RELE(ttyvp);
		return error;
	}
	return (*cdevsw[getmajor(ttyd)].d_open) (&ttyd, flag, otyp, cr);
}

/* ARGSUSED */
int
syclose(dev, flag, otyp, cr)
	dev_t dev;
	int flag;
	int otyp;
	struct cred *cr;
{
	return 0;
}

/* ARGSUSED */
int
syread(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	register dev_t ttyd;
	register vnode_t *ttyvp;
	register vnode_t *ttycvp; /* for security checks on the controlling tty */

	if ((ttyd = u.u_procp->p_sessp->s_dev) == NODEV)
		return ENXIO;
	if ((ttyvp = u.u_procp->p_sessp->s_vp) == NULL)
		return EIO;
	ttycvp = common_specvp(ttyvp);
	if (mac_installed) {
		int error= 0;
		/*
		 * when MAC is running, need to pass access check on the
		 * controlling tty, to check for device security state
		 * and for security mode (dynamic vs static)
		 */
		if (error = VOP_ACCESS(ttycvp, VREAD, MAC_ACC|MAC_RW, u.u_cred))
			return error;
		}
	if (cdevsw[getmajor(ttyd)].d_str)
		return strread(ttycvp, uiop, cr);
	return (*cdevsw[getmajor(ttyd)].d_read) (ttyd, uiop, cr);
}

/* ARGSUSED */
int
sywrite(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	register dev_t ttyd;
	register vnode_t *ttyvp;
	register vnode_t *ttycvp;

	if ((ttyd = u.u_procp->p_sessp->s_dev) == NODEV)
		return ENXIO;
	if ((ttyvp = u.u_procp->p_sessp->s_vp) == NULL)
		return EIO;
	ttycvp = common_specvp(ttyvp);
	if (mac_installed) {
		int error=0;
		/*
		 * when MAC is running, need to pass access check on the
		 * controlling tty, to check for device security state
		 * and for security mode (dynamic vs static)
		 * checks are based on the invoking's process credentials
		 */
		if (error = VOP_ACCESS(ttycvp, VWRITE, MAC_ACC|MAC_RW, u.u_cred))
			return error;
		}

	if (cdevsw[getmajor(ttyd)].d_str)
		return strwrite(ttycvp, uiop, cr);
	return (*cdevsw[getmajor(ttyd)].d_write) (ttyd, uiop, cr);
}

/* ARGSUSED */
int
syioctl(dev, cmd, arg, mode, cr, rvalp)
	dev_t dev;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	register dev_t ttyd;
	register vnode_t *ttyvp;
	register vnode_t *ttycvp; /* used for security access checks */
	struct cred *newcr = cr;	/* initialzed to u.u_cred by security */

	if ((ttyd = u.u_procp->p_sessp->s_dev) == NODEV)
		return ENXIO;
	if ((ttyvp = u.u_procp->p_sessp->s_vp) == NULL)
		return EIO;
	ttycvp = common_specvp(ttyvp);
	if (mac_installed) {
		int error=0;
		/*
		 * when MAC is running, need to pass access check on the
		 * controlling tty, to check for device security state
		 * and for security mode (dynamic vs static)
		 * checks are based on the invoking's process credentials
		 * ioctls are treated as write operations except for TCGETA 
		 * TCGETS which are treated as read operations.
		 */
		 switch( cmd) {

		 case TCGETS:
		 case TCGETA:
			if (error = VOP_ACCESS(ttycvp, VREAD, MAC_ACC| MAC_IO, u.u_cred))
				return error;
			break;
		default:
			if (error = VOP_ACCESS(ttycvp, VWRITE, MAC_ACC, u.u_cred))
				return error;
			break;
		}
		/* 
		 * for MAC, the credential structure of the invoking
		 * process is checked and passed to the driver
		 */
		 newcr= u.u_cred;
	}
	if (cdevsw[getmajor(ttyd)].d_str)
		return strioctl(ttycvp, cmd, arg, mode, U_TO_K, newcr, rvalp);
	return (*cdevsw[getmajor(ttyd)].d_ioctl)
		  (ttyd, cmd, arg, mode, newcr, rvalp);
}


/* ARGSUSED */
int
sychpoll(dev, events, anyyet, reventsp, phpp)
	dev_t dev;
	short events;
	int anyyet;
	short *reventsp;
	struct pollhead **phpp;
{
	register dev_t ttyd;
	register vnode_t *ttyvp;
	register vnode_t *tvp;

	if ((ttyd = u.u_procp->p_sessp->s_dev) == NODEV)
		return ENXIO;
	if ((ttyvp = u.u_procp->p_sessp->s_vp) == NULL)
		return EIO;
	if (cdevsw[getmajor(ttyd)].d_str) {
		tvp = common_specvp(ttyvp);
		return strpoll(tvp->v_stream, events, anyyet, reventsp, phpp);
	}
	return (*cdevsw[getmajor(ttyd)].d_poll)
		  (dev, events, anyyet, reventsp, phpp);
}
