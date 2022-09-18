/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/osocket/osocket.c	1.8"
#ident	"$Header: $"
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
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/* Enhanced Application Binary Compatibility */
/* SCO Sockets emulation driver.		     */

#include <acc/priv/privilege.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <svc/systm.h>
#include <io/uio.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <mem/immu.h>
#include <proc/user.h>
#include <fs/fstyp.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/filio.h>
#include <mem/kmem.h>
#include <io/ddi.h>
#include <util/cmn_err.h>
#include <net/transport/timod.h>
#include <net/transport/tiuser.h>
#include <net/transport/tihdr.h>
#include <net/transport/sockmod.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/transport/socketvar.h>
#include <util/debug.h>

#include <proc/session.h>
#include <svc/hrtcntl.h>

#include <io/poll.h>
#include <net/tcpip/in.h>
#include <net/tcpip/if.h>
#include <io/mkdev.h>
#include <io/ioctl.h>
#include <net/transport/un.h>

#include <io/osocket/osocket.h>
#include <svc/sco.h>

#include	<util/mod/moddefs.h>

#define	DRVNAME	"osocket - SCO socket emulation driver"

int	osoc_load(), osoc_unload();

MOD_DRV_WRAPPER(osoc, osoc_load, osoc_unload, NULL, DRVNAME);

int
osoc_load(void)
{
	cmn_err(CE_NOTE, "!MOD: in osoc_load()");

	return(0);
}

int
osoc_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in osoc_unload()");

	return(0);
}

/* 
 * This is defined in kernel space.c since it is needed 
 * to hold the protocol mapping even when the module is
 * unloaded.
 */
extern struct odomain  *osoc_family;

/*
 * Define local and external routines.
 */

int 		osocdevflag = 0;
major_t		osockdev = 0;
int             osockinited = 0;

extern int num_osockets;
extern struct osocket *osocket_tab[];
extern char osoc_domainbuf[];

extern int osoc_ncalls;
extern int (*osoc_call[]) ();
extern int osocopen();

extern char qrunflag;
extern long Strcount;
extern long strthresh;


/* 
** The translation of SCO sockets error numbers are done here 
** instead of the common system call exit point because 
** we cannot distinguish between SCO COFF executables and ISC
** COFF executables and both of these executables have conflicting
** error numbers for BSD sockets.  The translations for BSD
** socket errors  will be done here for SCO and in the ISC socket 
** driver for ISC socket errors.  
*/

STATIC short svr4_to_sco[] = {
0,	1, 	2, 	3, 	4, 	5, 	6, 	7, 	8, 	9,
10, 	OEWOULDBLOCK, 	12,	13, 	14, 	
15, 	16, 	17, 	18, 	19,
20,	21, 	22, 	23, 	24, 	25, 	26, 	27, 	28, 	29,
30,	31, 	32, 	33, 	34, 	35, 	36, 	37, 	38, 	39,
40,	41, 	42, 	43, 	44, 	45, 	46, 	47, 	48, 	49,
50,	51, 	52, 	53, 	54, 	55, 	56, 	57, 	58, 	59,
60,	61, 	62, 	63, 	64, 	65, 	66, 	67, 	68, 	69,
70,	71, 	72, 	73, 	74, 	75,	76,	77, 	78, 	79,
80,	81, 	82, 	83, 	84, 	85, 	86, 	87, 	88, 	89,
90,	91, 	92, 	OENOTEMPTY, 	94, 	
OENOTSOCK, 	OEDESTADDRREQ, 	OEMSGSIZE, 	OEPROTOTYPE, 	OENOPROTOOPT,
100,	101, 	102, 	103, 	104, 	105, 	106, 	107, 	108, 	109,
110,	111, 	112, 	113, 	114, 	115, 	116, 	117, 	118, 	119,
OEPROTONOSUPPORT, OESOCKTNOSUPPORT, OEOPNOTSUPP, OEPFNOSUPPORT, OEAFNOSUPPORT,
OEADDRINUSE, OEADDRNOTAVAIL, OENETDOWN, OENETUNREACH, OENETRESET,
OECONNABORTED, OECONNRESET, OENOBUFS, OEISCONN, OENOTCONN,
135, 	136, 	137, 	138, 	139,
140,	141, 	142,	OESHUTDOWN,	OETOOMANYREFS,	
OETIMEDOUT, OECONNREFUSED, OEHOSTDOWN, OEHOSTUNREACH, OEALREADY,
OEINPROGRESS
};

STATIC int nerror_sco = sizeof (svr4_to_sco)/sizeof(svr4_to_sco[0]);

/*
 * Driver initialization routine. 
 */

osocinit(dev)
int	dev;
{
	int	i;
	major_t	maj;

	/* 
	** Save the Major Device number.  It will be use to
	** make a vnode
	*/

	for (maj = 0; maj < cdevcnt; maj++)
		if (cdevsw[maj].d_open == osocopen)
			break;
	if (maj >= cdevcnt)
		return(-1);

	osockdev = maj;

	/*
	 * Reserve first osocket table entry so as to provide a mechanism 
	 * to do get a socket and support admin functions.
	 */
	osocket_tab[0] = OSOCK_RESERVE;
	osockinited = 1;
	return(0);
}


osocopen(devp, flag, type, cr)
dev_t	*devp;
int	flag;
int	type;
struct	cred *cr;
{
	int		rval;
	minor_t		sockno;

	rval = 0;

	if (!osockinited && (osocinit(*devp)  < 0))
		rval = ENODEV;

	sockno = getminor(*devp);

	/* Check minor devices greater than zero */
	if (!rval && sockno) {

		/* If the minor is not Zero then it must be in progress of */
		/* creating a new vnode */

		if ((sockno > num_osockets) || 
		    (osocket_tab[sockno] != OSOCK_INPROGRESS))
			rval = EINVAL;
	}

	rval = svr4_to_scocoff(rval);

	return(rval);
}

osocclose(dev, flag, cr)
dev_t		dev;
int		flag;
struct cred	*cr;
{
	int		rval;
	minor_t		sockno;
	struct osocket	*so;

	sockno = getminor(dev);
	if (sockno == 0)		/* Pseudo Socket */
		return(0);

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		rval = svr4_to_scocoff(rval);
		return(rval);
	}

	/* Break down the parallel Socket/Transport Provider */
	if (so->so_sfp) {
		closef(so->so_sfp);
		if (so->so_sfd) {
			setf(so->so_sfd, NULLFP);
			so->so_sfd = 0;
		}
	}

	rval = osoc_sofree(so);
	if (rval)
		cmn_err(CE_WARN, 
			"osocclose: Error Dropping socket %d \n", sockno);

	osocket_tab[sockno] = OSOCK_AVAIL;
	rval = svr4_to_scocoff(rval);
	return(rval);
}

osocread(dev, uiop, cr)
dev_t		dev;
struct uio	*uiop;
struct cred	*cr;
{
	minor_t		sockno;
	struct osocket 	*so;
	struct msghdr	msg;
	int		rval;
	int		retval;
	struct vnode	*sys_vp;

	sockno = getminor(dev);
	if (sockno == 0 ) {
		rval = svr4_to_scocoff(ENODEV);
		return(rval);
	}

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		rval = svr4_to_scocoff(rval);
		return(rval);
	}

	sys_vp = so->so_svp;
	if (sys_vp) {
		msg.msg_iovlen = uiop->uio_iovcnt;
		msg.msg_iov = uiop->uio_iov;
		msg.msg_namelen = 0;
		msg.msg_name = NULL;
		msg.msg_accrightslen = 0;
		msg.msg_accrights = NULL;
		retval = 0;
		rval = osoc_soreceive(so, &msg, 0, &retval);
		uiop->uio_resid -= retval;
		uiop->uio_offset += retval;
	} else
		rval = EINVAL;

	rval = svr4_to_scocoff(rval);
	return(rval);
}

osocwrite(dev, uiop, cr)
dev_t	dev;
struct uio *uiop;
struct cred *cr;
{
	minor_t		sockno;
	struct osocket 	*so;
	struct msghdr	msg;
	int		rval;
	int		retval;
	struct vnode	*sys_vp;

	sockno = getminor(dev);
	if (sockno ==0 ) {
		rval = svr4_to_scocoff(ENODEV);
		return(rval);
	}

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		rval = svr4_to_scocoff(rval);
		return(rval);
	}

	sys_vp = so->so_svp;
	if (sys_vp) {
		msg.msg_iovlen = uiop->uio_iovcnt;
		msg.msg_iov = uiop->uio_iov;
		msg.msg_namelen = 0;
		msg.msg_name = NULL;
		msg.msg_accrightslen = 0;
		msg.msg_accrights = NULL;
		retval = 0;
		rval = osoc_sosend(so, &msg, 0, &retval);
		uiop->uio_resid -= retval;
		uiop->uio_offset += retval;
	} else
		rval = EINVAL;

	rval = svr4_to_scocoff(rval);
	return(rval);
}

/* ARGSUSED */
osocioctl(dev, cmd, arg, flag, cr, rvalp)
dev_t		dev;
u_int		cmd;
caddr_t		arg;
int             flag;
struct		cred *cr;
int		*rvalp;
{
	int		in_out[(OIOCPARM_MASK+ sizeof(int) - 1)/sizeof(int)];
	int		temp;
	uint		size;
	struct osocket  *so;
	int		rval;
	int		retval;
	minor_t		sockno;
	int		sockfunc;
	int		*args;
	int		pid;
	struct _si_user	*siptr;
	struct file	*sys_fp;
	struct vnode	*sys_vp;

	rval = 0;
	sockno = getminor(dev);
	so = (struct osocket *)OSOCK_AVAIL;
	if (sockno) {
		if ((sockno < 0) || (sockno > num_osockets)) {
			rval = svr4_to_scocoff(ENOTSOCK);
			return(rval);
		} else 
			so = osocket_tab[sockno];
		/*
		 * A socket minor number -- Veriry if setup is done
		 */
		if (((so == OSOCK_AVAIL) || 
		    (so == OSOCK_INPROGRESS) ||
		    (so == OSOCK_RESERVE) ||
		    (so->so_svp == NULL)) &&
			cmd != OSIOCPROTO && cmd != OSIOCXPROTO) {

			rval = svr4_to_scocoff(EINVAL);
			return(rval);

		} else {
			sys_fp = so->so_fp;
			sys_vp = so->so_svp;
			siptr = &so->so_user;
		}
	} else if (cmd != OSIOCSOCKSYS) {
		rval = svr4_to_scocoff(EINVAL);
		return(rval);
	} else {
		sys_fp = NULL;
		sys_vp = NULL;
		siptr = NULL;
	}

	/*
	 * Extract the size of the input/output arguments
	 */

	size = (cmd & ~(OIOC_INOUT | OIOC_VOID)) >> 16;
	if (size > sizeof(in_out)) {
		rval = svr4_to_scocoff(EFAULT);
		return(rval);
	}

	if (cmd & OIOC_IN) {
		if (size) {
			if (copyin(arg, (caddr_t)in_out, size)) {
				rval = svr4_to_scocoff(EFAULT);
				return(rval);
			}
		} else
			*(caddr_t *) in_out = arg;

	} else if ((cmd & OIOC_OUT) && size) {

		/*
		 * Initialize the stack var.
		 */
		bzero((caddr_t)in_out, size);

	} else if (cmd & OIOC_VOID) {
		
		*(caddr_t *) in_out = arg;

	}

	switch (cmd) {
	case OSIOCPROTO:
		/* Add a new protocol to protosw */
		if (!pm_denied(u.u_cred, P_SYSOPS))
			rval = osoc_addproto((struct osocknewproto *)in_out);
		else
			rval = EPERM;
		break;

        case OSIOCXPROTO:
		/* Zap the protosw */
		if (!pm_denied(u.u_cred, P_SYSOPS))
			osoc_relprotos();
		else
			rval = EPERM;
                break;

	case OSIOCSOCKSYS:

		args = ((struct osocksysreq *)in_out)->args;
		sockfunc = *args++;
		if ((sockfunc < 0) || (sockfunc >= osoc_ncalls))
			sockfunc = 0;

		rval = (*osoc_call[sockfunc])(args, rvalp);
		break;

	case OFIONREAD:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		rval = strioctl(sys_vp, FIONREAD, in_out, sys_fp->f_flag, 
				K_TO_K, u.u_cred, rvalp);

		break;

	case OFIONBIO:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		rval = strioctl(sys_vp, FIONBIO, in_out, sys_fp->f_flag, 
				K_TO_K, u.u_cred, rvalp);
		if (!rval) {
			if (in_out[0]) {
				sys_fp->f_flag |= FNDELAY;
			} else {
				sys_fp->f_flag &= ~FNDELAY;
			}
		}
		break;

	case OFIOASYNC:
		/*
 		 * Enable or disable asynchronous I/O
		 * Facilitate SIGIO.
		 */

		/*
		 * Turn on or off async I/O.
		 */
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		retval = 0;
		if (in_out[0]) {
			/*
			 * Turn ON SIGIO if
			 * it is not already on.
			 */
			if ((siptr->flags & S_SIGIO) != 0)
				break;
	
			if (siptr->flags & S_SIGURG)
				retval = S_RDBAND|S_BANDURG;
			retval |= S_RDNORM|S_WRNORM;
	
			rval = strioctl(sys_vp, I_SETSIG, &retval, 
					sys_fp->f_flag, K_TO_K, 
					u.u_cred, rvalp);
			if (rval)
				break;
	
			siptr->flags |= S_SIGIO;
			break;
		}
	
		/*
		 * Turn OFF SIGIO if
		 * not already off.
		 */
		if ((siptr->flags & S_SIGIO) == 0)
			break;
	
		siptr->flags &= ~S_SIGIO;
	
		if (siptr->flags & S_SIGURG)
			retval = S_RDBAND|S_BANDURG;
	
		rval = strioctl(sys_vp, I_SETSIG, &retval, 
				sys_fp->f_flag, K_TO_K, 
				u.u_cred, rvalp);
	
		break;

	case OSIOCGPGRP:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		rval = strioctl(sys_vp, I_GETSIG, in_out, 
				sys_fp->f_flag, K_TO_K, 
				u.u_cred, rvalp);
		if (rval == EINVAL) {
			in_out[0] = 0;
			rval = 0;
		} 
		if (!rval && 
		   (in_out[0] & (S_RDBAND|S_BANDURG|S_RDNORM|S_WRNORM)))
			*(pid_t *)in_out = u.u_procp->p_pid;
		else	
			*(pid_t *)in_out = 0;

		break;

	case OSIOCSPGRP:
		/*
		 * Facilitate receipt of SIGURG.
		 *
		 * We are forgiving in that if a
		 * process group was specified rather
		 * than a process id, we will only
		 * fail it if the process group
		 * specified is not the callers.
		 */
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		pid = *(pid_t *)in_out;
		if (pid < 0) {
			pid = -pid;
			if (pid != u.u_procp->p_pgrp) {
				rval = EINVAL;
				break;
			}
		} else	{
			if (pid != u.u_procp->p_pid) {
				rval = EINVAL;
				break;
			}
		}

		retval = 0;
		if (siptr->flags & S_SIGIO)
			retval = S_RDNORM|S_WRNORM;
		retval |= S_RDBAND|S_BANDURG;
		rval = strioctl(sys_vp, I_SETSIG, &retval, 
				sys_fp->f_flag, K_TO_K, 
				u.u_cred, rvalp);
		break;

	case OSIOCATMARK:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		retval = 0;
		rval = strioctl(sys_vp, I_ATMARK, LASTMARK, 
				sys_fp->f_flag, K_TO_K, 
				u.u_cred, &retval);
		if (!rval) {
			*(int *)in_out = retval;
			*rvalp = 0;
		}
		break;


	case OSIOCGIFFLAGS:
		{
			/* This Request will pass the user datastructure */
			/* for the "struct oifreq"			 */

			struct ifreq	*ifr;
			struct oifreq	*oifr;
			int		len;

			if (sys_vp == NULL) {
				rval = EINVAL;
				break;
			}

			oifr = (struct oifreq *)&in_out[0];
			if (size < sizeof(struct oifreq)) {
				rval = EINVAL;
				break;
			}

			len = sizeof(struct ifreq);

			ifr = kmem_zalloc(len, KM_SLEEP);
			bcopy(oifr->ifr_name, ifr->ifr_name, 
				      sizeof(ifr->ifr_name));
			retval = 0;
			rval = osoc_do_ioctl(so, ifr, len, SIOCGIFFLAGS, 
					     &retval);
			if ((rval == 0) && (retval >= 0)) {
				/* 
				 * Get the flags from the provider and 
				 * copy them to the equivalent position
				 */
				oifr->ifr_flags = ifr->ifr_flags;
			}
			kmem_free((caddr_t)ifr, len);
		}
		break;


	case OSIOCGIFCONF:
		{
			struct ifreq	*ifr;
			struct oifconf	*oifc;
			struct oifreq	*oifr;
			caddr_t		ptr;
			int		len;
			int		olen;

			if (sys_vp == NULL) {
				rval = EINVAL;
				break;
			}

			oifc = (struct oifconf *)&in_out[0];
			oifr = oifc->ifc_req;
			len = oifc->ifc_len;

			if (len <= 0)
				break;

			/* There may be more than one provider */
			ifr = kmem_zalloc(len, KM_SLEEP);
			ptr = (caddr_t)ifr;
			retval = 0;
			rval = osoc_do_ioctl(so, ifr, len, SIOCGIFCONF, 
					     &retval);
			olen = 0;
			if (rval == 0) {
				/* 
				 * Get the contents of each provider and 
				 * copy them to the equivalent position
				 */
				while (retval > 0) {
					rval = copyout(ifr->ifr_name, 
							oifr->ifr_name, 
							sizeof(ifr->ifr_name));
					if (rval != 0) {
						rval = EFAULT;
						break;
					}
					if ((retval - sizeof(ifr->ifr_name)) >=
					    sizeof(struct osockaddr)) {
						rval = copyout(
						    (caddr_t)&ifr->ifr_addr, 
						    (caddr_t)&oifr->oifr_addr,
						    sizeof(struct osockaddr));
						if (rval != 0) {
							rval = EFAULT;
							break;
						}
					} else {
						rval = EINVAL;
						break;
					}
					retval -= sizeof(struct ifreq);
					olen += sizeof(struct oifreq);
					ifr++;
					oifr++;
				}
				oifc->ifc_len = olen;
			}
			kmem_free(ptr, len);
		}
		break;

	default:
		rval = EINVAL;
		break;

	}

	/*
	 * Copyout the data to user.
	 */
	if (rval == 0 && (cmd & OIOC_OUT) && size) {
		if (copyout((caddr_t)in_out, arg, size))
			rval = EFAULT;
	}

	rval = svr4_to_scocoff(rval);
	return(rval);
}

int
osocchpoll(dev, events, anyyet, reventsp, phpp)
dev_t	dev;
short	events;
int	anyyet;
short	*reventsp;
struct	pollhead **phpp;
{
	minor_t		sockno;
	struct osocket 	*so;
	struct msghdr	msg;
	int		rval;
	int		retval;
	struct vnode	*sys_vp;

	sockno = getminor(dev);
	if (sockno ==0 ) {
		rval = svr4_to_scocoff(ENODEV);
		return(rval);
	}

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		rval = svr4_to_scocoff(rval);
		return(rval);
	}
	sys_vp = so->so_svp;
	if (sys_vp)
		rval = strpoll(sys_vp->v_stream, events, anyyet, 
			       reventsp, phpp);
	else
		rval = EINVAL;

	rval = svr4_to_scocoff(rval);
	return(rval);
}

int
osocmmap()
{
	int	rval = 0;
	return(rval);
}

int
osocsegmap()
{
	int	rval = 0;
	return(rval);
}


osoc_addproto(nproto)
struct osocknewproto *nproto;
{
	struct odomain	*domp;
	struct oprotosw	*prp;
	int		s;

	/* Check if the family exits */
	for (domp = osoc_family; domp; domp = domp->dom_next)
		if (domp->dom_family == nproto->family)
			break;

	/* Allocate space for the family */
	if (domp == NULL) {
		domp = (struct odomain *) kmem_zalloc(sizeof(struct odomain), 
						   KM_SLEEP);
		s = splstr();
		domp->dom_family = nproto->family;
		domp->dom_protosw = NULL;
		domp->dom_next = osoc_family;
		osoc_family= domp;
		splx(s);
	}

	/* Check if the type/protocol exists */
	for (prp = domp->dom_protosw; prp; prp = prp->pr_next) {
		if (prp->pr_type == nproto->type
		    && prp->pr_protocol == nproto->proto) {
			/*
			 * Do not have to free the memory allocated 
			 * for the family because this protocol/type
			 * would not have existed.
			 */
			return (EPROTOTYPE);
		}
	}

	/* Allocate space for the type/protocol */
	prp = (struct oprotosw *) kmem_zalloc(sizeof(struct oprotosw), 
					      KM_SLEEP);

	/* Add the type/protocol to the family */
	s = splstr();
	prp->pr_type = nproto->type;
	prp->pr_domain = domp;
	prp->pr_protocol = nproto->proto;
	prp->pr_flags = nproto->flags;
	prp->pr_device = nproto->dev;
	prp->pr_next = domp->dom_protosw;
	domp->dom_protosw = prp;
	splx(s);
	return (0);
}

osoc_relprotos()
{
	struct odomain	*domp;
	struct oprotosw	*prp;
	struct odomain	*dompnext;
	struct oprotosw	*prpnext;
	int		s;

	/* Free allocated space for all families and protocols */
	s = splstr();
	for (domp = osoc_family; domp; domp = dompnext) {
		for (prp = domp->dom_protosw; prp; prp = prpnext) {
			prpnext = prp->pr_next;
			kmem_free(prp, sizeof(struct oprotosw));
		}
		dompnext = domp->dom_next;
		kmem_free(domp, sizeof(struct odomain));
	}
	osoc_family = NULL;
	splx(s);
	return(0);
}

struct oprotosw *
osoc_gettype(family, type)
int	family, type;
{
	struct odomain	*domp;
	struct oprotosw	*prp;

	/* Get the family */
	for (domp = osoc_family; domp; domp = domp->dom_next)
		if (domp->dom_family == family)
			break;
	if (!domp)
		return (NULL);

	/* Found the family -- Search for the type */
	for (prp = domp->dom_protosw; prp; prp = prp->pr_next)
		if (prp->pr_type && prp->pr_type == type)
			return (prp);
	return (NULL);
}

/* Match the type and protocol  -- Special handling for Raw Sockets */
struct oprotosw *
osoc_getproto(family, type, proto)
int	family, proto, type;
{
	struct odomain	*domp;
	struct oprotosw	*prp;
	struct oprotosw	*maybe;

	maybe  = NULL;

	if (family == 0)
		return (NULL);

	for (domp = osoc_family; domp; domp = domp->dom_next)
		if (domp->dom_family == family)
			break;

	if (!domp)
		return (NULL);

	/* _s_match() code */
	for (prp = domp->dom_protosw; prp; prp = prp->pr_next) {
		if (proto) {
			if ((prp->pr_type == type) && 
			    (prp->pr_protocol == proto))
				return (prp);
			if ((type == OSOCK_RAW) &&
			    (prp->pr_type == OSOCK_RAW) &&
			    (prp->pr_protocol == 0) &&
			    (maybe == (struct oprotosw *) NULL)) {
				maybe = prp;
			}
		} else if (prp->pr_type == type) 
			return (prp);
	}
	return (maybe);
}


/*
 * The socket functions translated from the user level library 
 * libsocket/socket
 */

extern int nosys();

int	osoc_accept(), osoc_bind(), osoc_connect(); 
int	osoc_getpeername(), osoc_getsockname();
int	osoc_getsockopt(), osoc_listen(), osoc_recv(); 
int	osoc_recvfrom(), osoc_send(), osoc_sendto();
int	osoc_setsockopt(), osoc_shutdown(), osoc_socket();
int	osoc_getipdomain(), osoc_setipdomain();
int	osoc_adjtime();
int	osoc_nosys();

int	(*osoc_call[]) () = {
	                nosys,			/* NOT USED		 */
	                osoc_accept,		/* OSO_ACCEPT		 */
	                osoc_bind,		/* OSO_BIND		 */
	                osoc_connect,		/* OSO_CONNECT		 */
	                osoc_getpeername,	/* OSO_GETPEERNAME	 */
	                osoc_getsockname,	/* OSO_GETSOCKNAME	 */
	                osoc_getsockopt,	/* OSO_GETSOCKOPT	 */
	                osoc_listen,		/* OSO_LISTEN		 */
	                osoc_recv,		/* OSO_RECV		 */
	                osoc_recvfrom,		/* OSO_RECVFROM		 */
	                osoc_send,		/* OSO_SEND		 */
	                osoc_sendto,		/* OSO_SENDTO		 */
	                osoc_setsockopt,	/* OSO_SETSOCKOPT	 */
	                osoc_shutdown,		/* OSO_SHUTDOWN		 */
	                osoc_socket,		/* OSO_SOCKET		 */
	                osoc_nosys,		/* OSO_SELECT		 */
			osoc_getipdomain,	/* OSO_GETIPDOMAIN	 */
			osoc_setipdomain,	/* OSO_SETIPDOMAIN	 */
			osoc_adjtime,		/* OSO_ADJTIME		 */
			osoc_nosys,		/* OSO_SETREUID		 */
			osoc_nosys,		/* OSO_SETREGID		 */
			osoc_nosys,		/* OSO_GETTIME		 */
};

int osoc_ncalls = sizeof(osoc_call) / sizeof(osoc_call[0]);

osoc_nosys()
{
	int	rval;

	rval = svr4_to_scocoff(EINVAL);
	return(rval);
}

struct socketa {
	int             family;
	int             type;
	int             proto;
};

int
osoc_socket(uap, rvalp)
struct socketa	*uap;
int		*rvalp;
{
	int		error;
	struct osocket	*so;




	error = osoc_sockopen(uap->family, uap->type, uap->proto, rvalp);
	if (!error) {
		error = osoc_getsocket_with_fd(&so, *rvalp);
		if (!error && so->so_sfd) {
			setf(so->so_sfd, NULLFP);
			so->so_sfd = 0;
		} else {
			/*
			** It must be setting up the family/type/proto 
			** structures using the Reserved Pseudo socket
			*/
			error = 0;
		}
	}
	return(error);
}

osoc_sockopen(family, type, proto, rvalp)
int	family;
int	type;
int	proto;
int	*rvalp;
{
	minor_t	sockno;
	int	fd;
	int	rdev;
	int	rval;
	int	flags;
	struct	vnode *dev_vp;
	struct osocket  *so;
	struct file *fp;

	flags = FREAD|FWRITE;
	/*
	 * Look for a free socket.
	 * First socket slot is reserved for pseudo system call interface.
	 */
	for (sockno = 1; sockno < num_osockets; sockno++)
		if (osocket_tab[sockno] == OSOCK_AVAIL)
			break;

	if (sockno >= num_osockets) {
		return(ENXIO);
	}
	osocket_tab[sockno] = OSOCK_INPROGRESS;

	rdev = makedevice(osockdev, sockno);
	dev_vp = (struct vnode *)makespecvp(rdev, VCHR);
	if((rval = VOP_OPEN(&dev_vp, flags, u.u_cred)) != 0) {
		VN_RELE(dev_vp);
		osocket_tab[sockno] = OSOCK_AVAIL;
		return(rval);
	}

	if ((rval = falloc(dev_vp, flags, &fp, &fd)) != 0) {
		VOP_CLOSE(dev_vp, flags, 1, 0, u.u_cred);
		osocket_tab[sockno] = OSOCK_AVAIL;
		return (rval);
	}

	so = OSOCK_AVAIL;
	if (setjmp(&u.u_qsav)) {	/* catch half-opens (if any) */
		if (u.u_error == 0)
			u.u_error = EINTR;
		rval = u.u_error;
		closef(fp);
		setf(fd, NULLFP);
		osocket_tab[sockno] = OSOCK_AVAIL;
		return(rval);
	}


	/*
	** XXX If type and proto are non-zero then the BIND process fails
	** For now turn-off the protocol since we have only one protocol
	** for each type of socket.
	*/
	if (type && proto)
		proto = 0;

	rval = osoc_create(&so, family, type, proto, fd);

	if (rval) {
		closef(fp);
		setf(fd, NULLFP);
		osocket_tab[sockno] = OSOCK_AVAIL;
		return(rval);
	}

	so->so_fd = fd;
	so->so_fp = fp;
	so->so_uvp = dev_vp;
	*rvalp = fd;
	osocket_tab[sockno] = so;
	return(0);
}


/* ARGSUSED */
osoc_create(sopp, family, type, proto, fd)
struct osocket **sopp;
register int    family;
register int    type;
int             proto;
int		fd;
{
	struct oprotosw *prp = (struct oprotosw *) NULL;
	struct osocket *so;
	int	rval;

	/* Search for a match in family/type or family/type/proto */

	if (family != 0 || proto != 0 || type != 0) {
		if (proto)
			prp = osoc_getproto(family, type, proto);
		else
			prp = osoc_gettype(family, type);
		if (prp == NULL)
			return (EPROTONOSUPPORT);
		if (prp->pr_type != type)
			return (EPROTOTYPE);
	}

	so = (struct osocket *) kmem_zalloc(sizeof(struct osocket), KM_SLEEP);
	so->so_type = type;
	if ((type == OSOCK_RAW) && pm_denied(u.u_cred, P_SYSOPS)) {
		kmem_free(so, sizeof(struct osocket));
		return (EACCES);
	}

	*sopp = so;

	if (prp == NULL) /* protoless */
		return (0);

	so->so_proto = *prp;
	if (rval = osoc_smodopen(so, proto))
		return (rval);

	so->so_user.family = family;
	so->so_user.fd = fd;

	return(0);
}

struct binda {
	int             s;
	caddr_t         name;
	int             namelen;
};

int
osoc_bind(uap, rvalp)
struct binda	*uap;
int		*rvalp;
{
	
	struct osocket *so;
	struct _si_user *siptr;
	int	sockno;
	int	rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;
	rval = osoc_getargs(&so->so_addr, uap->name, uap->namelen);
	if (rval)
		return(rval);

	if (siptr->udata.so_state & SS_ISBOUND) {
		return (EINVAL);
	}

	/*
	 * Only AF_INET domains
	 */
	if (so->so_addr.sa_family !=  OAF_INET)
		return (EINVAL);

	rval = osoc_dobind(so, &so->so_addr, uap->namelen, NULL, NULL);
	return(rval);
}


osoc_dobind(so, name, namelen, raddr, raddrlen)
struct osocket		*so;
struct osockaddr	*name;
int			namelen;
char			*raddr;
int			*raddrlen;
{
	struct _si_user		*siptr;
	char			*buf;
	struct T_bind_req	*bind_req;
	struct T_bind_ack	*bind_ack;
	int			size;
	int			fflag;
	int			rval;

	siptr = &so->so_user;
	if (siptr->family != OAF_INET)
		return (EINVAL);

	namelen = MIN(namelen, siptr->udata.addrsize);
	buf = siptr->ctlbuf;
	bind_req = (struct T_bind_req *)buf;
	size = sizeof (*bind_req);

	if (buf != (char *)NULL && (siptr->ctlsize >= size)) {
		bind_req->PRIM_type = T_BIND_REQ;
		bind_req->ADDR_length = name == NULL ? 0 : namelen;
		bind_req->ADDR_offset = 0;
		bind_req->CONIND_number = 0;
	} else {
		return(EFAULT);
	}

	if ((int)bind_req->ADDR_length > 0) {
		osoc_aligned_copy(buf, bind_req->ADDR_length, size,
				(caddr_t)name,
				&bind_req->ADDR_offset);
		size = bind_req->ADDR_offset + bind_req->ADDR_length;
	}
	if (siptr->ctlsize < (size + bind_req->ADDR_length)) {
		return(EFAULT);
	}

	rval = osoc_do_ioctl(so, buf, size, TI_BIND, NULL);
	if (rval)
		return (rval);

	bind_ack = (struct T_bind_ack *)buf;
	buf += bind_ack->ADDR_offset;

	/*
	 * Check that the address returned by the
	 * transport provider meets the criteria.
	 */
	rval = 0;
	if (name != (struct osockaddr *)NULL) {
		struct sockaddr_in	*rname;
		struct sockaddr_in	*aname;

		/*
		 * Some programs like inetd(8) don't set the
		 * family field.
		 */

		rname = (struct sockaddr_in *)buf;
		aname = (struct sockaddr_in *)name;

		if (aname->sin_port != 0 &&
			aname->sin_port != rname->sin_port)
			rval = EADDRINUSE;

		if (aname->sin_addr.s_addr != INADDR_ANY &&
		    aname->sin_addr.s_addr != rname->sin_addr.s_addr)
			rval = EADDRNOTAVAIL;
	}

	if (rval) {
		osoc_dounbind(so);
		return (rval);
	}

	/*
	 * Copy back the bound address if requested.
	 */
	if (raddr != NULL) {
		rval = osoc_cpaddr(raddr, *raddrlen,
				buf, bind_ack->ADDR_length, &size );
		if (!rval && (raddrlen != NULL))
			copyout((caddr_t)&size, (caddr_t)raddrlen, 
				sizeof(size));
		else if (raddrlen != NULL) {
			rval = 0;
			copyout((caddr_t)&rval, (caddr_t)raddrlen, 
				sizeof(rval));
		}
	}

	siptr->udata.so_state |= SS_ISBOUND;

	return (0);
}

int
osoc_dounbind(so)
struct osocket *so;
{
	struct _si_user	*siptr;
	int	rval;

	siptr = &so->so_user;

	((struct T_unbind_req *)siptr->ctlbuf)->PRIM_type = T_UNBIND_REQ;

	rval = osoc_do_ioctl(so, siptr->ctlbuf,
				sizeof (struct T_unbind_req),
					TI_UNBIND, NULL);
	if (rval)
		return (rval);

	siptr->udata.so_state &= ~SS_ISBOUND;
	return (0);
}


/* We make the socket module do the unbind,
 * if necessary, to make the timing window
 * of error as small as possible.
 */
struct listena {
	int	s;
	int    qlen;
};

int
osoc_listen(uap, rvalp)
struct listena	*uap;
int		*rvalp;
{
	struct osocket 		*so;
	char			*buf;
	struct T_bind_req	*bind_req;
	int			size;
	struct _si_user		*siptr;
	int			rval;


	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;

	if (siptr->udata.servtype == T_CLTS)
		return (EOPNOTSUPP);

	if (siptr->family != OAF_INET)
		return (EINVAL);

	buf = siptr->ctlbuf;
	bind_req = (struct T_bind_req *)buf;
	size = sizeof (struct T_bind_req);

	if (buf != (char *)NULL && (siptr->ctlsize >= size)) {
		bind_req->PRIM_type = T_BIND_REQ;
		bind_req->ADDR_offset = sizeof (*bind_req);
		bind_req->CONIND_number = uap->qlen;
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		int	family;

		family = siptr->family;

		bcopy((caddr_t)&family, buf + bind_req->ADDR_offset,
				sizeof (short));
		bind_req->ADDR_length = 0;
	} else	bind_req->ADDR_length = siptr->udata.addrsize;

	rval = osoc_do_ioctl(so, siptr->ctlbuf, sizeof (*bind_req) +
				bind_req->ADDR_length, SI_LISTEN, NULL);

	if (rval)
		return (rval);

	siptr->udata.so_options |= OSO_ACCEPTCONN;
	return (0);
}

struct accepta {
	int             s;
	caddr_t         addr;
	int            *addrlen;
};

int
osoc_accept(uap, rvalp)
struct accepta	*uap;
int		*rvalp;
{
	int             namelen;
	struct _si_user	*siptr;
	struct osocket *so;
	int		rval;

	namelen = 0;
	if (uap->addr && uap->addrlen) {
		if (copyin((caddr_t) uap->addrlen,
			   (caddr_t) &namelen, sizeof(namelen)))
			return(EFAULT);
	
		if (useracc((caddr_t) uap->addr, (u_int) namelen, B_WRITE) == 0)
			return (EFAULT);
	}

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;
	if (siptr->udata.servtype == T_CLTS)
		return (EOPNOTSUPP);

	/*
	 * Make sure a listen() has been done
	 * actually if the accept() has not been done, then the
	 * effect will be that the user blocks forever.
	 */
	if ((siptr->udata.so_options & OSO_ACCEPTCONN) == 0)
		return (EINVAL);

	rval = osoc_doaccept(so,  uap->addr, namelen, uap->addrlen, rvalp);
	return (rval);
} 


int
osoc_doaccept(so, addr, len, addrlen, rvalp)
struct osocket	*so;
struct osockaddr *addr;
int		len;
int		*addrlen;
int		*rvalp;
{
	struct _si_user		*siptr;
	struct _si_user		*nsiptr;
	struct T_conn_res	*cres;
	int			s;
	int			s2;
	union T_primitives	*pptr;
	struct strfdinsert	strfdinsert;
	int			flg;
	struct strbuf		ctlbuf;

	int			sys_fd;
	int			nsys_fd;
	struct file 		*sys_fp;
	struct file 		*nsys_fp;
	int			retval;
	int			rval;
	int			size;
	rval_t			rv;
	int			domain;
	int			type;
	int			proto;
	int			nfd;
	struct osocket		*nso;

	flg = 0;
	siptr = &so->so_user;
	s = siptr->fd;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;

	/*
	 * Get/wait for the T_CONN_IND.
	 */
	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	/*
	 * Get message from Sockmod 
	 * We don't expect any data, so no data
	 * buffer is needed.
	 */
	rval = osoc_getmsg(sys_fp, &ctlbuf, NULL, &flg, &rv);
	if (rval) {
		if (rval == EAGAIN)
			rval = EWOULDBLOCK;
		return (rval);
	}
	/*
	 * did I get entire message?
	 */
	if (rv.r_val1)
		return (EIO);

	/*
	 * is ctl part large enough to determine type
	 */
	if (ctlbuf.len < sizeof (long))
		return (EPROTO);

	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_CONN_IND:
			if (ctlbuf.len < (sizeof (struct T_conn_ind)+
				pptr->conn_ind.SRC_length)) {
				return (EPROTO);
			}
			if (addr && addrlen) {
				rval = osoc_cpaddr(addr, len,
					ctlbuf.buf + pptr->conn_ind.SRC_offset,
					pptr->conn_ind.SRC_length, &size);

				if (!rval)
					copyout((caddr_t)&size, 
						(caddr_t)addrlen, sizeof(size));
			}
			break;

		default:
			return(EPROTO);
	}

	/*
	 * Open a new instance to do the accept on
	 */
	domain = so->so_user.family;
	type = so->so_proto.pr_type;

	/* -- XXX -- I know the protocol but I cannot use it 	  */
	/* because smodopen calls setsockopt() for this protocol  */
	/* which later causes TI_BIND below to fail with a 	  */
	/* TLI error TBADADDR in tcp_state()			  */
	/*	proto = so->so_proto.pr_protocol;		  */ 

	proto = 0;

	rval = osoc_sockopen(domain, type, proto, &rv);
	if (rval)
		return(rval);

	nfd = rv.r_val1;
	nso = NULL;
	rval = osoc_getsocket_with_fd(&nso, nfd);
	if (rval) 
		return(rval);

	if (nso->so_proto.pr_device != so->so_proto.pr_device) {
		osoc_doclose(nso);
		return(EINVAL);
	}

	nsiptr = &nso->so_user;
	s2 = nsiptr->fd;
	nsys_fd = nso->so_sfd;
	nsys_fp = nso->so_sfp;

	/*
	 * must be bound for TLI.
	 */
	rval = osoc_dobind(nso, NULL, 0, NULL, NULL);
	if (rval) {
		osoc_doclose(nso);
		return (rval);
	}

	cres = (struct T_conn_res *)siptr->ctlbuf;
	cres->PRIM_type = T_CONN_RES;
	cres->OPT_length = 0;
	cres->OPT_offset = 0;
	cres->SEQ_number = pptr->conn_ind.SEQ_number;

	strfdinsert.ctlbuf.maxlen = siptr->ctlsize;
	strfdinsert.ctlbuf.len = sizeof (*cres);
	strfdinsert.ctlbuf.buf = (caddr_t)cres;

	strfdinsert.databuf.maxlen = 0;
	strfdinsert.databuf.len = -1;
	strfdinsert.databuf.buf = NULL;

	strfdinsert.fildes = nsys_fd;
	strfdinsert.offset = sizeof (long);
	strfdinsert.flags = 0;

	rval = strioctl(so->so_svp, I_FDINSERT, &strfdinsert, 
			sys_fp->f_flag, K_TO_K, u.u_cred, &retval);

	/*Blow away the parallel file-des to sockmod/Transport Provider */
	setf(nsys_fd, NULLFP);
	nso->so_sfd = 0;

	if (rval) {
		osoc_doclose(nso);
		return (rval);
	}

	if (!osoc_is_ok(so, T_CONN_RES, &rval)) {
		osoc_doclose(nso);
		return (rval);
	}

	/*
	 * New socket must have attributes of the
	 * accepting socket.
	 */
	nsiptr->udata.so_state |= OSS_ISCONNECTED;
	nsiptr->udata.so_options = siptr->udata.so_options & ~OSO_ACCEPTCONN;

#ifdef DO_LATER_XXX
	/* No translation into SVR4 code */
	/*
	 * Make the ownership of the new socket the
	 * same as the original.
	 */
	retval = 0;
	if (ioctl(s, SIOCGPGRP, &retval) == 0) {
		if (retval != 0) {
			(void)ioctl(nsiptr->fd, SIOCSPGRP, &retval);
		}
	} else	{
		(void)syslog(LOG_ERR,
			"accept: SIOCGPGRP failed errno %d\n", errno);
		errno = 0;
	}
#endif

	/*
	 * The accepted socket inherits the non-blocking and SIGIO
	 * attributes of the accepting socket.
	 */
	rval = osoc_do_fcntl(sys_fp, F_GETFL, 0, &rv);
	if (rval) {
		cmn_err(CE_WARN,
			"osoc_doaccept: fcntl: F_GETFL failed %d\n", rval);
		rval = 0;
	} else	{
		flg = rv.r_val1;
		flg &= (FREAD|FWRITE|FASYNC|FNDELAY);
		osoc_do_fcntl(nsys_fp, F_SETFL, flg, &rv);
	}

	*rvalp = s2;
	return (rval);
}

struct connecta {
	int             s;
	caddr_t         name;
	int             namelen;
};

int
osoc_connect(uap, rvalp)
struct connecta	*uap;
int		*rvalp;
{
	struct osocket 		*so;
	int			rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	rval = osoc_getargs(&so->so_addr, uap->name, uap->namelen);
	if (rval)
		return(rval);

	rval = osoc_doconnect1(so, uap->namelen, 1);

	return(rval);

}


int
osoc_doconnect1(so, namelen, nameflag)
struct osocket 	*so;
int		namelen;
int		nameflag;
{
	struct _si_user			*siptr;
	struct osockaddr		*name;
	struct t_call			sndcall;
	struct t_call			*call;
	int				sys_fd;
	struct file			*sys_fp;
	rval_t				rv;
	int				rval;
	int				retval;

	struct sockaddr_in 		*saddr_in;

	siptr = &so->so_user;
	name = &so->so_addr;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;
	call = &sndcall;

	bzero((caddr_t)call, sizeof (*call));

	if (name->sa_family != AF_INET)
		return(EINVAL);

	if (namelen < sizeof (struct sockaddr_in))
		return (EINVAL);
	saddr_in = (struct sockaddr_in *)name;
	bzero((caddr_t)&saddr_in->sin_zero, 8);

	call->addr.buf = (caddr_t)name;
	call->addr.len = MIN(namelen, siptr->udata.addrsize);

	return(osoc_doconnect2(so, call));

}

osoc_doconnect2(so, call)
struct osocket	*so;
struct t_call	*call;
{
	struct _si_user			*siptr;
	int				fctlflg;
	int				sys_fd;
	struct file			*sys_fp;
	rval_t				rv;
	int				rval;
	int				retval;

	struct sockaddr_in 		*saddr_in;

	siptr = &so->so_user;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;

	rval = osoc_do_fcntl(sys_fp, F_GETFL, 0, &rv);
	if (rval)
		return(rval);

	fctlflg = rv.r_val1;

	if (fctlflg & O_NDELAY && siptr->udata.servtype != T_CLTS) {
		/*
		 * Secretly tell sockmod not to pass
		 * up the T_CONN_CON, because we
		 * are not going to wait for it.
		 * (But dont tell anyone - especially
		 * the transport provider).
		 */
		call->opt.len = (ulong)-1;	/* secret sign */
	}

	/*
	 * Must be bound for TPI.
	 */
	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		rval = osoc_dobind(so, NULL, 0, NULL, NULL);
		if (rval)
			return (rval);
	}

	rval = osoc_snd_conn_req(so, call);
	if (rval)
		return (rval);

	/*
	 * If no delay, return with error if not CLTS.
	 */
	if (fctlflg & O_NDELAY && siptr->udata.servtype != T_CLTS) {
		siptr->udata.so_state |= SS_ISCONNECTING;
		return (EINPROGRESS);
	}

	/*
	 * If CLTS, don't get the connection confirm.
	 */
	if (siptr->udata.servtype == T_CLTS) {
		if (call->addr.len == 0)
			/*
			 * Connect to Null address, breaks
			 * the connection.
			 */
			siptr->udata.so_state &= ~OSS_ISCONNECTED;
		else	siptr->udata.so_state |= OSS_ISCONNECTED;
		return (0);
	}

	rval = osoc_rcv_conn_con(so);
	if (rval)
		return (rval);

	siptr->udata.so_state |= OSS_ISCONNECTED;
	return (0);
}

int
osoc_snd_conn_req(so, call)
struct osocket	*so;
struct t_call	*call;
{
	struct _si_user		*siptr;
	int			sys_fd;
	struct file		*sys_fp;
	struct T_conn_req	*creq;
	char			*buf;
	int			size;
	struct strbuf		ctlbuf;
	int			rval;
	rval_t			rv;

	siptr = &so->so_user;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;

	buf = siptr->ctlbuf;
	creq = (struct T_conn_req *)buf;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = call->addr.len;
	creq->DEST_offset = 0;
	creq->OPT_length = call->opt.len;
	creq->OPT_offset = 0;
	size = sizeof (struct T_conn_req);

	if ((int)call->addr.len > 0 && buf != (char *)NULL) {
		osoc_aligned_copy(buf, call->addr.len, size,
			call->addr.buf, &creq->DEST_offset);
		size = creq->DEST_offset + creq->DEST_length;
	}
	if ((int)call->opt.len > 0 && buf != (char *)NULL) {
		osoc_aligned_copy(buf, call->opt.len, size,
			call->opt.buf, &creq->OPT_offset);
		size = creq->OPT_offset + creq->OPT_length;
	}

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	rval = osoc_putmsg(sys_fp, &ctlbuf, 
			(call->udata.len? &call->udata: NULL), 0, &rv);
	if (rval)
		return (rval);

	if (!osoc_is_ok(so, T_CONN_REQ, &rval))
		return (rval);

	return (0);
}

/*
 * Rcv_conn_con - get connection confirmation off
 * of read queue
 */
int
osoc_rcv_conn_con(so)
struct osocket *so;
{
	struct _si_user		*siptr;
	int			sys_fd;
	struct file		*sys_fp;
	struct strbuf		ctlbuf;
	struct strbuf		databuf;
	union T_primitives	*pptr;
	int			retval;
	int			rval;
	rval_t			rv;
	int			flg;
	char			dbuf[128];

	siptr = &so->so_user;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;

	flg = 0;
	if (siptr->udata.servtype == T_CLTS)
		return (EOPNOTSUPP);

again:
	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	databuf.maxlen = sizeof (dbuf);
	databuf.len = 0;
	databuf.buf = dbuf;

	/*
	 * No data expected, but we play safe.
	 */
	rv.r_val1 = 0;
	rval = osoc_getmsg(sys_fp, &ctlbuf, &databuf, &flg, &rv);
	if (rval) {
		if (rval == ENXIO)
			rval = ECONNREFUSED;
		return (rval);
	}

	/*
	 * did we get entire message
	 */
	if (rv.r_val1)
		return (EIO);

	/*
	 * is cntl part large enough to determine message type?
	 */
	if (ctlbuf.len < sizeof (long))
		return (EPROTO);

	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_CONN_CON:
			return (0);

		case T_DISCON_IND:
			if (ctlbuf.len < sizeof (struct T_discon_ind))
				rval = ECONNREFUSED;
			else	rval = pptr->discon_ind.DISCON_reason;
			return (rval);

		default:
			break;
	}

	return (EPROTO);
}

struct recva {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
};

int
osoc_recv(uap, rvalp)
struct recva	*uap;
int		*rvalp;
{
	struct osocket 		*so;
	struct _si_user		*siptr;
	struct msghdr		msg;
	struct iovec		msg_iov[1];
	int			rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = 0;
	msg.msg_name = NULL;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	*rvalp = 0;
	rval = osoc_soreceive(so, &msg, uap->flags, rvalp);
	return(rval);
}

struct recvfa {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
	caddr_t         from;
	int            *fromlen;
};

int
osoc_recvfrom(uap, rvalp)
struct recvfa	*uap;
int		*rvalp;
{
	struct socket *so;
	int		flen;
	struct _si_user	*siptr;
	int		retlen;
	struct iovec	msg_iov[1];
	struct msghdr	msg;
	int		rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	flen = 0;
	if (uap->from && uap->fromlen && 
	    copyin((caddr_t) uap->fromlen, (caddr_t) &flen,
		   sizeof(flen)))
		return(EFAULT);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = flen;
	msg.msg_name = uap->from;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	*rvalp = 0;
	rval = osoc_soreceive(so, &msg, uap->flags, rvalp);

	if (!rval && uap->fromlen) {
		flen = msg.msg_namelen;
		rval = copyout((caddr_t)&flen, (caddr_t)uap->fromlen,
	    		       sizeof(int));
		if (rval)
			rval = EFAULT;
	}

	return(rval);
}

struct senda {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
};

int
osoc_send(uap, rvalp)
struct senda	*uap;
int		*rvalp;
{
	struct osocket 		*so;
	struct _si_user		*siptr;
	struct msghdr		msg;
	struct iovec		msg_iov[1];
	int			rval;

	if ((uap->len <= 0) || (uap->buf == NULL))
		return(0);

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = 0;
	msg.msg_name = NULL;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	return (osoc_sosend(so, &msg, uap->flags, rvalp));
}

struct sendfa {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
	caddr_t         to;
	int             tolen;
};

int
osoc_sendto(uap, rvalp)
struct sendfa	*uap;
int		*rvalp;
{

	struct socket *so;
	struct msghdr	msg;
	struct iovec	msg_iov[1];
	int		rval;
	int		tlen;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = uap->tolen;
	msg.msg_name = uap->to;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	return (osoc_sosend(so, &msg, uap->flags, rvalp));

}

/*
 * Get name of peer for connected socket. 
 */
struct getpeera {
	int             fdes;
	caddr_t         name;
	int            *namelen;
};

int
osoc_getpeername(uap, rvalp)
struct getpeera	*uap;
int		*rvalp;
{
	struct socket		*so;
	int			len;
	struct osockaddr	addr;
	int			rval;
	int			sname;

	rval = osoc_getsocket_with_fd(&so, uap->fdes);
	if (rval)
		return(rval);

	if (uap->name == NULL || uap->namelen == NULL)
		return (EINVAL);

	if (copyin((caddr_t) uap->namelen, (caddr_t) &len, sizeof(len)))
		return(EFAULT);

	if (len > sizeof(struct osockaddr))
		len = sizeof(struct osockaddr);

	rval = osoc_dogetpeername(so, uap->name, &sname, len);

	/* Copyout the address size */
	if (!rval) {
		rval = copyout((caddr_t)&sname, (caddr_t)uap->namelen, 
				sizeof(sname));
		if (rval)
			rval = EFAULT;
	}

	return(rval);
}

osoc_dogetpeername(so, name, snamelen, len)
struct osocket		*so;
struct sockaddr		*name;
int			*snamelen;
int			len;
{
	struct _si_user		*siptr;
	struct netbuf		netbuf;
	int			sys_fd;
	struct file		*sys_fp;
	struct vnode		*sys_vp;
	int			rval;
	int			retval;
	rval_t			rv;

	siptr = &so->so_user;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;
	sys_vp = so->so_svp;

	netbuf.len = 0;
	netbuf.maxlen = siptr->ctlsize;
	netbuf.buf = siptr->ctlbuf;


	rval = strioctl(so->so_svp, TI_GETPEERNAME, &netbuf,
			sys_fp->f_flag, K_TO_K, u.u_cred, &retval);
	if (rval) {
		switch (rval) {
			case ENXIO:
			case EPIPE:
				rval = ENOTCONN;
				break;

			case ENOTTY:
			case ENODEV:
			case EINVAL:
				rval = ENOTSOCK;
				break;
		}
		return (rval);
	}

	rval = osoc_cpaddr(name, len, netbuf.buf, netbuf.len, snamelen);

	return (rval);
}

/*
 * Get socket name. 
 */
struct getsocka {
	int			s;
	struct osockaddr	*name;
	int			*namelen;
};

int
osoc_getsockname(uap, rvalp)
struct getsocka	*uap;
int		*rvalp;
{
	struct osocket	*so;
	struct _si_user	*siptr;
	int             len;
	int		rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;
	if (uap->name == NULL || uap->namelen == NULL)
		return (EINVAL);

	if (copyin((caddr_t) uap->namelen, (caddr_t) (&len), sizeof(len))) {
		return(EFAULT);
	}

	if (len > sizeof(struct osockaddr))
		len = sizeof(struct osockaddr);

	return (osoc_dogetsockname(so, uap->name, uap->namelen, len));
}

osoc_dogetsockname(so, name, namelen, len)
struct osocket		*so;
struct osockaddr	*name;
int			*namelen;
int			 len;
{
	int	rval;
	int	retval;
	struct _si_user	*siptr;
	struct netbuf	netbuf;
	int		size;
	int		sys_fd;
	struct file	*sys_fp;
	struct vnode	*sys_vp;

	siptr = &so->so_user;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;
	sys_vp = so->so_svp;

	netbuf.len = 0;
	netbuf.maxlen = siptr->ctlsize;
	netbuf.buf = siptr->ctlbuf;

	/*
	 * Get it from sockmod.
	 */
	rval = strioctl(sys_vp, TI_GETMYNAME, (caddr_t)&netbuf, 
			sys_fp->f_flag, K_TO_K, u.u_cred, &retval);
	if (rval) {
		switch (rval) {
			case ENXIO:
			case EPIPE:
				rval = 0;
				break;

			case ENOTTY:
			case ENODEV:
			case EINVAL:
				rval = ENOTSOCK;
				break;
		}
		if(rval)
			return(rval);
	}

	rval = osoc_cpaddr(name, len, netbuf.buf, netbuf.len, &size);

	if (!rval)
		copyout((caddr_t)&size, (caddr_t)namelen, sizeof(size));

	return (0);
}

struct getsockopta {
	int	s;
	int	level;
	int	optname;
	caddr_t	optval;
	int	*optlen;
};

int
osoc_getsockopt(uap, rvalp)
struct getsockopta	*uap;
int			*rvalp;
{
	struct osocket  *so;

	int			sys_optlen;
	int			sys_optval;
	char			*buf;
	struct T_optmgmt_req	*opt_req;
	struct T_optmgmt_ack	*opt_ack;
	struct _si_user		*siptr;
	int			size;
	struct opthdr		*opt;
	int			retlen;
	int			rval;


	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;
	sys_optlen = 0;
	if (uap->optval) {
		if (uap->optlen) {
			rval = copyin((caddr_t) uap->optlen, 
				      (caddr_t)&sys_optlen, sizeof(sys_optlen));
		} else
			rval++;

		if (rval)
			return(EFAULT);
	}

	if (uap->level == OSOL_SOCKET && uap->optname == OSO_TYPE) {
		if (sys_optlen < sizeof (int))
			return (EINVAL);

		if (siptr->udata.servtype == T_CLTS)
			sys_optval = SOCK_DGRAM;
		else	sys_optval = SOCK_STREAM;

		sys_optlen = sizeof (int);

		if (uap->optval)
			rval = copyout(uap->optval, (caddr_t)sys_optval, 
				       sizeof(int));
		if (!rval)
			rval = copyout((caddr_t)uap->optlen, 
				       (caddr_t)sys_optlen, sizeof(int));
		if (rval)
			return(EFAULT);

		return (0);
	}

	buf = siptr->ctlbuf;
	opt_req = (struct T_optmgmt_req *)buf;
	opt_req->PRIM_type = T_OPTMGMT_REQ;
	opt_req->OPT_length = sizeof (*opt) + sys_optlen;
	opt_req->OPT_offset = sizeof (*opt_req);
	opt_req->MGMT_flags = T_CHECK;
	size = sizeof (*opt_req) + opt_req->OPT_length;

	if (size > siptr->ctlsize)
		return(EFAULT);

	opt = (struct opthdr *)(buf + opt_req->OPT_offset);
	opt->level = uap->level;
	opt->name = uap->optname;
	opt->len = sys_optlen;

	rval = osoc_do_ioctl(so, buf, size, TI_OPTMGMT, &retlen);
	if (rval)
		return(rval);

	if (retlen < (sizeof (*opt_ack) + sizeof (*opt)))
		return(EPROTO);

	opt_ack = (struct T_optmgmt_ack *)buf;
	opt = (struct opthdr *)(buf + opt_ack->OPT_offset);

	sys_optlen = opt->len;
	rval = copyout((caddr_t)opt + sizeof (*opt), uap->optval, opt->len);
	if (!rval && uap->optlen)
		rval = copyout((caddr_t)&sys_optlen, (caddr_t)uap->optlen, 
				sizeof(sys_optlen));
	if (rval)
		return (EFAULT);

	return (0);
}

struct setsockopta {
	int             s;
	int             level;
	int             optname;
	caddr_t         optval;
	int             optlen;
};

osoc_setsockopt(uap, rvalp)
struct setsockopta	*uap;
int			*rvalp;
{
	struct osocket  *so;
	int		rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	rval = osoc_dosetsockopt(so, uap->level, uap->optname, uap->optval, 
			  uap->optlen, U_TO_K);
	return(rval);
}

int
osoc_dosetsockopt(so, level, optname, optval, optlen, copyflag)
struct osocket *so;
int		level;
int		optname;
char		*optval;
int		optlen;
int		copyflag;
{
	struct _si_user		*siptr;
	char			*buf;
	struct T_optmgmt_req	*opt_req;
	register int		size;
	struct opthdr		*opt;
	int			rval;
	int			totsize;

	rval = 0;
	siptr = &so->so_user;
	buf = siptr->ctlbuf;
	totsize = sizeof (*opt_req) + sizeof (*opt) + optlen;
	
	if (buf && (siptr->ctlsize >= totsize)) {
		opt_req = (struct T_optmgmt_req *)buf;
		opt_req->PRIM_type = T_OPTMGMT_REQ;
		opt_req->OPT_length = sizeof (*opt) + optlen;
		opt_req->OPT_offset = sizeof (*opt_req);
		opt_req->MGMT_flags = T_NEGOTIATE;
	
		opt = (struct opthdr *)(buf + sizeof (*opt_req));
		opt->level = level;
		opt->name = optname;
		opt->len = optlen;
		if (copyflag == U_TO_K)
			rval = copyin(optval, (caddr_t)opt + sizeof (*opt), 
				      optlen);
		else
			(void)bcopy(optval, (caddr_t)opt + sizeof (*opt), 
				    optlen);
		if (rval)
			return(rval);
	
		size = opt_req->OPT_offset + opt_req->OPT_length;
	
		rval = osoc_do_ioctl(so, buf, size, TI_OPTMGMT, 0);
	} else {
		rval = EFAULT;
	}
	return (rval);
}

struct shutdowna {
	int             s;
	int             how;
};

int
osoc_shutdown(uap, rvalp)
struct shutdowna	*uap;
int			*rvalp;
{
	struct osocket  	*so;
	struct   _si_user	*siptr;
	int			rval;
	int			sys_how;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);
	
	siptr = &so->so_user;

	sys_how = uap->how;
	if (sys_how < 0 || sys_how > 2)
		return (EINVAL);

	if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
		rval = osoc_getudata(so, 0);
		if (rval)
			return (rval);
		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0)
			return (ENOTCONN);
	}

	sys_how = uap->how;
	rval = osoc_do_ioctl(so, &sys_how, sizeof (sys_how), SI_SHUTDOWN, 0);
	if (rval) {
		if (rval != EPIPE)
			return (rval);
		else	
			rval= 0;
	}

	/*
	 * If we got EPIPE back from the ioctl, then we can
	 * no longer talk to sockmod. The best we can do now
	 * is set our local state and hope the user doesn't
	 * use read/write.
	 */
	if (sys_how == 0 || sys_how == 2)
		siptr->udata.so_state |= SS_CANTRCVMORE;
	if (sys_how == 1 || sys_how == 2)
		siptr->udata.so_state |= SS_CANTSENDMORE;

	return (0);
}

struct adjtimea {
	struct timeval *delta;
	struct timeval *olddelta;
};

int
osoc_adjtime(uap, rvalp)
struct adjtimea	*uap;
int		*rvalp;
{
	int	rval;

	rval = osoc_doadjtime(uap->delta, uap->olddelta);
	return(rval);
}

struct setipdoma {
	caddr_t	namep;
	int	size;
};

int
osoc_setipdomain(uap, rvalp)
struct setipdoma	*uap;
int			*rvalp;
{
	if (pm_denied(u.u_cred, P_SYSOPS))
		return(EPERM);

	/* Reserve one byte for the NULL termination */
	if (uap->size < 1 || uap->size > (OMAXHOSTNAMELEN - 1))
		return (EINVAL);

	if (copyin(uap->namep, osoc_domainbuf, uap->size))
		return(EFAULT);
	osoc_domainbuf[uap->size] = 0;

	return(0);
}

struct getipdoma {
	caddr_t	namep;
	int	size;
};

int
osoc_getipdomain(uap, rvalp)
struct getipdoma	*uap;
int			*rvalp;
{
	int 		domainlen;

	domainlen = strlen(osoc_domainbuf) + 1;

	if (domainlen > uap->size)
		return (EFAULT);
	
	if (copyout(osoc_domainbuf, uap->namep, domainlen))
		return(EFAULT);
	return(0);
}


/* Functions from libsocket/socket/_utility.c */

osoc_smodopen(so, proto)
struct osocket *so;
{
	int	fd;
	int	rval;
	int	retval;
	int	madefp;
	struct  file *fp;
	struct	vnode *sys_vp;
	int	flags;
	int	arg;

	retval = 0;
	madefp = 0;
	flags = FREAD|FWRITE;
	sys_vp = (struct vnode *)makespecvp(so->so_proto.pr_device, VCHR);

	if((rval = VOP_OPEN(&sys_vp, flags, u.u_cred)) != 0) {
		VN_RELE(sys_vp);
		return(rval);
	}

	if ((rval = falloc(sys_vp, flags, &fp, &fd)) != 0) {
		VOP_CLOSE(sys_vp, flags, 1, 0, u.u_cred);
		return rval;
	}

	/* Must allocate a file pointer and a file descriptor	*/
	/* So that we can do an accept.				*/
	madefp = 1;			/* For half-opens */


	/* XXX --- MOVE the closing of device and stream file/vnode 
	** 	   to the caller
	**	   for now it is dealt with in an incomplete manner.
	*/

	if (setjmp(&u.u_qsav)) {	/* catch half-opens (if any) */
		if (u.u_error == 0)
			u.u_error = EINTR;
		rval = u.u_error;
		if (madefp) {
			closef(fp);
			setf(fd, NULLFP);
		}
		return(rval);
	}

	/* Push the Socket Module */

	rval = strioctl(sys_vp, I_PUSH, "sockmod", 
			fp->f_flag, K_TO_K, u.u_cred, &retval);
	if (rval) {
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	if( retval ) {
		if ((retval & 0xff) == TSYSERR)
			rval = (retval >> 8) & 0xff;
		else    
			rval = osoc_tlitosyserr(retval & 0xff);
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}


	/* Set the stream head close time to 0 */
	/* Do not care about the return value from strioctl */

	arg = 0;
	strioctl(sys_vp, I_SETCLTIME, &arg, fp->f_flag, K_TO_K, 
		u.u_cred, &retval);

	/* Turn on SIGPIPE stream head write option */

	rval = strioctl(sys_vp, I_SWROPT, SNDPIPE, fp->f_flag, K_TO_K, 
			u.u_cred, &retval);

	if (rval) {
		cmn_err(CE_WARN, 
		    "osoc_open: Cannot set SNDPIPE : %d", rval);
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	if( retval ) {
		if ((retval & 0xff) == TSYSERR)
			rval = (retval >> 8) & 0xff;
		else    
			rval = osoc_tlitosyserr(retval & 0xff);
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	so->so_sfd = fd;
	so->so_sfp = fp;
	so->so_svp = sys_vp;

	rval = osoc_getudata(so, 1);
	if (rval) {
		/* Freeup the sockets here */
		so->so_sfd = 0;
		so->so_sfp = NULL;
		so->so_svp = NULL;
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	if (proto) {
		/* Need to send down the protocol number */
		rval = osoc_dosetsockopt(so, SOL_SOCKET, SO_PROTOTYPE, &proto, 
					sizeof (proto), K_TO_K);
		if (rval) {
			/* Freeup the sockets here */
			so->so_sfd = 0;
			so->so_sfp = NULL;
			so->so_svp = NULL;
			closef(fp);
			setf(fd, NULLFP);
			return(rval);
		}
	}

	return(0);
}

int
osoc_getudata(so, init)
struct osocket *so;
int	init;
{
	struct si_udata			udata;
	struct _si_user			*nsiptr;
	int				retlen;
	int				pid;
	int				retval;
	int				rval;
	int				arg;

	rval = osoc_do_ioctl(so, (caddr_t)&udata, sizeof (struct si_udata),
			SI_GETUDATA, &retlen);
	if (rval)
		return(rval);

	if (retlen != sizeof (struct si_udata)) {
		rval = EPROTO;
		return (rval);
	}

	nsiptr = &so->so_user;
	if (init) {
		 
		osoc_alloc_bufs(nsiptr, &udata);

		/* Init nsiptr->fd in osoc_create() */

		nsiptr->udata = udata;		/* structure copy */
		nsiptr->family = -1;
		nsiptr->flags = 0;

		/*
		 * Get SIGIO and SIGURG disposition
		 * and cache them.
		 */
		arg = 0;
		rval = strioctl(so->so_svp, I_GETSIG, &arg, 0, K_TO_K, 
				u.u_cred, &retval);

		/* Check for any registered events */
		/* If there are no registered events then rval == EINVAL */
		if (rval && (rval != EINVAL))
			return (rval);

		rval = 0;
		if (retval & (S_RDNORM|S_WRNORM))
			nsiptr->flags |= S_SIGIO;

		if (retval & (S_RDBAND|S_BANDURG))
			nsiptr->flags |= S_SIGURG;

		return (0);
	} else {
		nsiptr->udata = udata;		/* Structure Copy */
	}

	return (0);
}

/*
 * timod ioctl
 */
int
osoc_do_ioctl(so, buf, size, cmd, retlen)
struct osocket *so;
char	*buf;
int	size;
int	cmd;
int		*retlen;
{
	int	retval;
	int	rval;
	struct strioctl		strioc;
	struct file	*sys_fp;
	struct vnode	*sys_vp;

	sys_fp = so->so_sfp;
	sys_vp = so->so_svp;

	strioc.ic_cmd = cmd;
	strioc.ic_timout = -1;
	strioc.ic_len = size;
	strioc.ic_dp = buf;

	rval = strioctl(sys_vp, I_STR, &strioc, 
			sys_fp->f_flag, K_TO_K, u.u_cred, &retval);
	if (rval) {
		/*
		 * Map the rval as appropriate.
		 */
		switch (rval) {
			case ENOTTY:
			case ENODEV:
			case EINVAL:
				rval = ENOTSOCK;
				break;

			case EBADF:
				break;

			case ENXIO:
				rval = EPIPE;

			default:
				break;
		}
		return (rval);
	}

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			rval = (retval >>  8) & 0xff;
		else
			rval = osoc_tlitosyserr(retval & 0xff);
		return (rval);
	}
	if (retlen)
		*retlen = strioc.ic_len;
	return (0);
}


/*
 * Allocate buffers
 */
int
osoc_alloc_bufs(siptr, udata)
struct _si_user	*siptr;
struct si_udata	*udata;
{
	unsigned	size2;

	size2 = sizeof (union T_primitives) + udata->addrsize + sizeof (long) +
			udata->optsize + sizeof (long);

	siptr->ctlbuf = kmem_zalloc(size2, KM_SLEEP);
	siptr->ctlsize = size2;
	return (0);
}

/*
 * Wait for T_OK_ACK
 */
osoc_is_ok(so, type, rvalp)
struct osocket *so;
long	type;
int	*rvalp;
{

	struct _si_user			*siptr;
	struct strbuf			ctlbuf;
	union T_primitives		*pptr;
	int				flags;
	int				retval;
	int				rval;
	int				fmode;
	rval_t				rv;
	struct file			*sys_fp;

	
	*rvalp = 0;
	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	fmode = osoc_do_fcntl(sys_fp, F_GETFL, 0);
	if (fmode & O_NDELAY) {
		osoc_do_fcntl(sys_fp, F_SETFL, fmode & ~O_NDELAY);
	}

	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;
	ctlbuf.maxlen = siptr->ctlsize;
	flags = RS_HIPRI;

	rv.r_val1 = 0;
	while ((rval = osoc_getmsg(sys_fp, &ctlbuf, NULL, &flags, &rv)) 
		!= 0) {
		if (rval == EINTR)
			continue;
		*rvalp = rval;
		return (0);
	}

	/*
	 * Did I get entire message
	 */
	if (rv.r_val1) {
		*rvalp = EIO;
		return (0);
	}

	/*
	 * Is ctl part large enough to determine type?
	 */
	if (ctlbuf.len < sizeof (long)) {
		*rvalp = EPROTO;
		return (0);
	}

	if (fmode & O_NDELAY)
		(void)osoc_do_fcntl(sys_fp, F_SETFL, fmode, &rv);

	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_OK_ACK:
			if ((ctlbuf.len < sizeof (struct T_ok_ack)) ||
			    (pptr->ok_ack.CORRECT_prim != type)) {
				*rvalp = EPROTO;
				return (0);
			}
			return (1);

		case T_ERROR_ACK:
			if ((ctlbuf.len < sizeof (struct T_error_ack)) ||
			    (pptr->error_ack.ERROR_prim != type)) {
				*rvalp = EPROTO;
				return (0);
			}
			if (pptr->error_ack.TLI_error == TSYSERR)
				*rvalp = pptr->error_ack.UNIX_error;
			else	*rvalp = osoc_tlitosyserr(pptr->error_ack.TLI_error);
			return (0);

		default:
			*rvalp = EPROTO;
			return (0);
	}
}

/*
 * Translate a TLI error into a system error as best we can.
 */
ushort osoc_tlierrs[] = {
		0,		/* no error	*/
		EADDRNOTAVAIL,  /* TBADADDR	*/
		ENOPROTOOPT,	/* TBADOPT	*/
		EACCES,		/* TACCES	*/
		EBADF,		/* TBADF	*/
		EADDRNOTAVAIL,	/* TNOADDR	*/
		EPROTO,		/* TOUTSTATE	*/
		EPROTO,		/* TBADSEQ	*/
		0,		/* TSYSERR - will never get	*/
		EPROTO,		/* TLOOK - should never be sent by transport */
		EMSGSIZE,	/* TBADDATA	*/
		EMSGSIZE,	/* TBUFOVFLW	*/
		EPROTO,		/* TFLOW	*/
		EWOULDBLOCK,	/* TNODATA	*/
		EPROTO,		/* TNODIS	*/
		EPROTO,		/* TNOUDERR	*/
		EINVAL,		/* TBADFLAG	*/
		EPROTO,		/* TNOREL	*/
		EOPNOTSUPP,	/* TNOTSUPPORT	*/
		EPROTO,		/* TSTATECHNG	*/
};

int
osoc_tlitosyserr(terr)
register int	terr;
{
	if (terr > (sizeof (osoc_tlierrs) / sizeof (ushort)))
		return (EPROTO);
	else	return (int)osoc_tlierrs[terr];
}

osoc_do_fcntl(fp, cmd, arg, rvp)
struct file	*fp;
int		cmd;
int		arg;
rval_t		*rvp;
{
	int	rval;

	rval = 0;
	switch(cmd) {
	case F_GETFL:
		rvp->r_val1 = fp->f_flag + FOPEN;
		break;

	case F_SETFL:
		if ((arg & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY))
			arg &= ~FNDELAY;
 		/*
		 * FRAIOSIG is a new flag added for the raw
		 * disk async io feature. This only applies
 		 * to character special files. But in case
		 */

		/* This is a socket - fd -- Thus a special file */
		/* SPECFS at this time does not have a setfl()	*/

		arg &= FMASK;
		fp->f_flag &= (FREAD|FWRITE);
		fp->f_flag |= (arg-FOPEN) & ~(FREAD|FWRITE);
		break;
	default:
		rval = EINVAL;
	}
	return(rval);
}

#ifdef _REMOVE
/*
 * This function was duplicated allow invoking poll from within the kernel
 * Code originally from fs/vncalls.c
 *
 * Poll file descriptors for interesting events.
 */
extern int pollwait;

int
osoc_smodpoll(fdp, nfds, timo, rvp)
struct pollfd *fdp;
unsigned long nfds;
long	timo;
rval_t *rvp;
{
	register int i, s;
	register fdcnt = 0;
	struct pollfd *pollp = NULL;
	clock_t t;
	int lastd;
	int rem;
	int id;
	int psize;
	int dsize;
	file_t *fp;
	struct pollhead *php;
	struct pollhead *savehp = NULL;
	struct polldat *darray;
	struct polldat *curdat;
	int error = 0;
	proc_t *p = u.u_procp;
	extern clock_t lbolt;

	if (nfds < 0 || nfds > u.u_rlimit[RLIMIT_NOFILE].rlim_cur)
		return (EINVAL);
	t = lbolt;

	/*
	 * Do not need to allocate space or copy the pollfd array since
	 * this array is in the kernel.
	 */
	if (nfds != 0) {
		psize = nfds * sizeof(struct pollfd);
		pollp = fdp;
		dsize = nfds * sizeof(struct polldat);
		if ((darray = (struct polldat *) kmem_zalloc(dsize, KM_NOSLEEP)) == NULL) {
			return EAGAIN;
		}

		/*
		 * Chain the polldat array together.
		 */
		lastd = nfds - 1;
		if (lastd > 0) {
			darray[lastd].pd_chain = darray;
			for (i = 0; i < lastd; i++) {
				darray[i].pd_chain = &darray[i+1];
			}
		} else {
			darray[0].pd_chain = darray;
		}
		curdat = darray;
	}

	/*
	 * Retry scan of fds until an event is found or until the
	 * timeout is reached.
	 */
retry:		

	/*
	 * Polling the fds is a relatively long process.  Set up the
	 * SINPOLL flag so that we can see if something happened
	 * to an fd after we checked it but before we go to sleep.
	 */
	p->p_pollflag = SINPOLL;
	if (savehp) {			/* clean up from last iteration */
		polldel(savehp, --curdat);
		savehp = NULL;
	}
	curdat = darray;
	php = NULL;
	for (i = 0; i < nfds; i++) {
		s = splhi();
		if (pollp[i].fd < 0) 
			pollp[i].revents = 0;
		else if (pollp[i].fd >= u.u_nofiles || getf(pollp[i].fd, &fp))
			pollp[i].revents = POLLNVAL;
		else {
			php = NULL;
			error = VOP_POLL(fp->f_vnode, pollp[i].events, fdcnt,
			    &pollp[i].revents, &php);
			if (error) {
				splx(s);
				goto pollout;
			}
		}
		if (pollp[i].revents)
			fdcnt++;
		else if (fdcnt == 0 && php) {
			polladd(php, pollp[i].events, pollrun,
			  (long)p, curdat++);
			savehp = php;
		}
		splx(s);
	}
	if (fdcnt) 
		goto pollout;

	/*
	 * If you get here, the poll of fds was unsuccessful.
	 * First make sure your timeout hasn't been reached.
	 * If not then sleep and wait until some fd becomes
	 * readable, writeable, or gets an exception.
	 */
	rem = timo < 0 ? 1 : timo - ((lbolt - t)*1000)/HZ;
	if (rem <= 0)
		goto pollout;

	s = splhi();

	/*
	 * If anything has happened on an fd since it was checked, it will
	 * have turned off SINPOLL.  Check this and rescan if so.
	 */
	if (!(p->p_pollflag & SINPOLL)) {
		splx(s);
		goto retry;
	}
	p->p_pollflag &= ~SINPOLL;

	if (timo > 0) {
		/*
		 * Turn rem into milliseconds and round up.
		 */
		rem = ((rem/1000) * HZ) + ((((rem%1000) * HZ) + 999) / 1000);
		p->p_pollflag |= SPOLLTIME;
		id = timeout((void(*)())polltime, (caddr_t)p, rem);
	}

	/*
	 * The sleep will usually be awakened either by this poll's timeout 
	 * (which will have cleared SPOLLTIME), or by the pollwakeup function 
	 * called from either the VFS, the driver, or the stream head.
	 */
	if (sleep((caddr_t)&pollwait, (PZERO+1)|PCATCH)) {
		if (timo > 0)
			untimeout(id);
		splx(s);
		error = EINTR;
		goto pollout;
	}
	splx(s);

	/*
	 * If SPOLLTIME is still set, you were awakened because an event
	 * occurred (data arrived, can write now, or exceptional condition).
	 * If so go back up and poll fds again. Otherwise, you've timed
	 * out so you will fall through and return.
	 */
	if (timo > 0) {
		if (p->p_pollflag & SPOLLTIME) {
			untimeout(id);
			goto retry;
		}
	} else
		goto retry;

pollout:

	/*
	 * Poll cleanup code.
	 */
	p->p_pollflag = 0;
	if (savehp)
		polldel(savehp, --curdat);
	if (error == 0) {
		/*
		 * Copy out the events and return the fdcnt to the user.
		 */
		rvp->r_val1 = fdcnt;

		/* Do not have to copyout since the pollp was a local copy */

	}
	if (nfds != 0)
		kmem_free((caddr_t)darray, dsize);
	return error;
}
#endif

/*
 * Get access rights and associated data.
 *
 * Only UNIX domain supported.
 */
int
osoc_recvaccrights(so, msg, fmode, rvalp)
struct osocket *so;
struct msghdr		*msg;
int			fmode;
int			*rvalp;

{
	/* Only UNIX domain supported and osocket does not support AF_UNIX. */
	*rvalp = -1;
	return(EINVAL);
}

/*
 * Peeks at a message. If no messages are
 * present it will block in a poll().
 * Note ioctl(I_PEEK) does not block.
 *
 * Returns:
 *	0	On success
 *	-1	On error. In particular, EBADMSG is returned if access
 *		are present.
 */
int
osoc_msgpeek(so, ctlbuf, rcvbuf, fmode)
struct osocket	*so;
struct strbuf	*ctlbuf;
struct strbuf	*rcvbuf;
int		fmode;
{
	struct	_si_user	*siptr;
	int			retval;
	struct strpeek		strpeek;
	int			rval;
	rval_t			rv;
	int			sys_fd;
	struct file		*sys_fp;
	struct vnode		*sys_vp;
	struct pollfd		fds[1];

	siptr = &so->so_user;
	sys_fd = so->so_fd;
	sys_fp = so->so_sfp;
	sys_vp = so->so_svp;
	
	strpeek.ctlbuf.buf = ctlbuf->buf;
	strpeek.ctlbuf.maxlen = ctlbuf->maxlen;
	strpeek.ctlbuf.len = 0;
	strpeek.databuf.buf = rcvbuf->buf;
	strpeek.databuf.maxlen = rcvbuf->maxlen;
	strpeek.databuf.len = 0;
	strpeek.flags = 0;

	for (;;) {
		rval = strioctl(sys_vp, I_PEEK, &strpeek, sys_fp->f_flag,
				K_TO_K, u.u_cred, &retval);
		if (rval)
			return(rval);

		if (retval == 1) {
			ctlbuf->len = strpeek.ctlbuf.len;
			rcvbuf->len = strpeek.databuf.len;
			return (0);
		} else	if ((fmode & O_NDELAY) == 0) {
			retval = 0;
			rval = osoc_strwaitq(sys_vp, GETWAIT, (off_t)0, 
					     sys_fp->f_flag, &retval);
			if (rval || retval)
				return(rval);
#ifdef _REMOVE
			/* strwaitq() replaced this code */
			/*
			 * Sit in a osoc_smodpoll()
			 */

			fds[0].fd = sys_fd;
			fds[0].events = POLLIN;
			fds[0].revents = 0;
			for (;;) {
				rval = osoc_smodpoll(fds, 1L, -1, &rv);
				if (rval)
					return (rval);
				if (fds[0].revents != 0)
					break;
			}
#endif
		} else	{
			return(EAGAIN);
		}
	}
	/* NOTREACHED*/
}

/*
 * Receive a message according to flags.
 *
 * On Returns:
 *	count 	in *rvalp
 *	 0 	return val on success
 *	-1 	return val on error
 */
int
osoc_recvmsg(so, msg, flags, rvalp)
struct osocket 	*so;
struct msghdr	*msg;
int		flags;
int		*rvalp;
{  
	int		fmode;
	int		s;
	int		len;
	int		pos;
	int		i;
	char		*addr;
	struct strbuf	ctlbuf;
	struct strbuf	rcvbuf;
	int 		addrlen;
	int		flg;

	struct _si_user	*siptr;
	int		rval;
	int		retval;
	int		sys_fd;
	struct file	*sys_fp;
	rval_t		rv;
	caddr_t		kbuf;
	int		klen;
	int		count;

	siptr = &so->so_user;
	s = siptr->fd;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;

	for (i = 0, len = 0; i < msg->msg_iovlen; i++)
		len += msg->msg_iov[i].iov_len;

	if (len == 0 && msg->msg_accrightslen == 0)
		return (0);

	/*
	 * Allocate a kernel Memory for the receive buffer
	 */
	klen = len;
	kbuf = kmem_alloc(klen, KM_SLEEP);
	rcvbuf.buf = kbuf;

	rval = osoc_do_fcntl(sys_fp, F_GETFL, 0, &rv);
	if (rval) {
		kmem_free(kbuf, klen);
		return(rval);
	}
	
	fmode = rv.r_val1;

tryagain:
	rcvbuf.maxlen = len;
	rcvbuf.len = 0;

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	if (flags & MSG_OOB) {
		/*
		 * Handles the case when MSG_PEEK is set
		 * or not.
		 */
		rval = osoc_do_ioctl(so, rcvbuf.buf, rcvbuf.maxlen, flags,
								&rcvbuf.len);
		if (rval)
			goto rcvout;
	} else if (flags & MSG_PEEK) {
		rval = osoc_msgpeek(so, &ctlbuf, &rcvbuf, fmode);
		if (rval) {
			if (rval == EBADMSG) {
				rval = 0;
				rval = osoc_recvaccrights(so, msg, fmode, 
							  &retval);
				rcvbuf.len = retval;
			}
			goto rcvout;
		}
	} else	{
		flg = 0;
		/*
		 * Have to prevent spurious SIGPOLL signals
		 * which can be caused by the mechanism used
		 * to cause a SIGURG.
		 */
		rval = osoc_getmsg(sys_fp, &ctlbuf, &rcvbuf, &flg, &rv);
		if (rval) {
			if (rval == EBADMSG) {
				rval = 0;
				rval = osoc_recvaccrights(so, msg, fmode,
							  &retval);
				rcvbuf.len = retval;
			}
			goto rcvout;
		}
	}

	if (rcvbuf.len == -1)
		rcvbuf.len = 0;

	if (ctlbuf.len == sizeof (struct T_exdata_ind) &&
				*(long *)ctlbuf.buf == T_EXDATA_IND &&
				rcvbuf.len == 0) {
		/*
		 * Must be the message indicating the position
		 * of urgent data in the data stream - the user
		 * should not see this.
		 */
		if (flags & MSG_PEEK) {
			/*
			 * Better make sure it goes.
			 */
			flg = 0;
			(void)osoc_getmsg(sys_fp, &ctlbuf, &rcvbuf, &flg, &rv);
		}
		goto tryagain;
	}

	/*
	 * Copy it all back as per the users
	 * request.
	 */
	for (i=pos=0, len=rcvbuf.len; i < msg->msg_iovlen; i++) {
		count = MIN(msg->msg_iov[i].iov_len, len);
		if (copyout(&rcvbuf.buf[pos], msg->msg_iov[i].iov_base, 
					count)) {
			rval = EFAULT;
			goto rcvout;
		}
		pos += count;
		len -= count;
		if (len == 0)
			break;
		else if (len < 0 ) {
			cmn_err(CE_WARN, "osoc_recvmsg negative len %d\n",
				len);
		}
	}

	/*
	 * Copy in source address if requested.
	 */
rcvout:
	if (rval == 0 && msg->msg_name && msg->msg_namelen) {
		if (siptr->udata.servtype == T_CLTS) {
			if (ctlbuf.len != 0) {
				register struct T_unitdata_ind *udata_ind;

				udata_ind = (struct T_unitdata_ind *)ctlbuf.buf;
				osoc_cpaddr(
					msg->msg_name,
					msg->msg_namelen,
					udata_ind->SRC_offset + ctlbuf.buf,
					udata_ind->SRC_length, &count);

				msg->msg_namelen = count;
			}
		} else	{
			if (rval) {
				rval = osoc_dogetpeername(so, msg->msg_name, 
						&addrlen, msg->msg_namelen);
				if (rval) {
					rval = 0;
					msg->msg_namelen = 0;
				}
				msg->msg_namelen = addrlen;
			}
		}
	}

	kmem_free(kbuf, klen);
	if (!rval)
		*rvalp = rcvbuf.len;
	return (rval);
}

/*
 * Common receive code.
 */
int
osoc_soreceive(so, msg, flags, rvalp)
struct osocket	*so;
struct msghdr	*msg;
int		flags;
int		*rvalp;
{
	struct _si_user		*siptr;
	int			retval;
	int			rval;

	rval = 0;
	siptr = &so->so_user;

	if (siptr->udata.so_state & OSS_CANTRCVMORE)
		return (0);

	if (siptr->udata.servtype == T_COTS ||
			siptr->udata.servtype == T_COTS_ORD) {
		if ((siptr->udata.so_state & OSS_ISCONNECTED) == 0) {
			rval = osoc_getudata(so, 0);
			if (rval)
				return (rval);

			if ((siptr->udata.so_state & OSS_ISCONNECTED) == 0)
				return (ENOTCONN);
		}
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		/*
		 * Need to bind it for TLI.
		 */
		rval = osoc_dobind(so, NULL, 0, NULL, NULL);
		if (rval)
			return (rval);
	}

	rval = osoc_recvmsg(so, msg, flags, rvalp);

	return (rval);
}


/*
 * This Code is here to preserve state between osoc_get_msg_slice and
 * osoc_sosend.  It was previously doing this in the user libs by 
 * using static variables -- VERY VERY BAD.
 */

struct hold {
	char	*pos;
	int	left;
	int	i;
};

/*
 * Common send code.
 */
int
osoc_sosend(so, msg, flags, rvalp)
struct osocket	*so;
struct msghdr	*msg;
int		flags;
int		*rvalp;
{
	register int		s;
	register int		i;
	register int		len;
	register int		retval;
	struct strbuf		ctlbuf;
	struct strbuf		databuf;

	struct _si_user		*siptr;
	int			rval;
	rval_t			rv;
	char			*kbuf;
	int			klen;
	int			sys_fd;
	struct file		*sys_fp;
	struct	vnode		*sys_vp;

	kbuf = NULL;
	klen = 0;
	rval = 0;
	siptr = &so->so_user;
	sys_fd = so->so_sfd;
	sys_fp = so->so_sfp;
	sys_vp = so->so_svp;

	if (siptr->udata.so_state & SS_CANTSENDMORE) {
		return (EPIPE);
	}

	s = siptr->fd;
	if ((siptr->udata.servtype == T_CLTS && msg->msg_namelen <= 0) ||
					siptr->udata.servtype != T_CLTS) {
		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
			rval = osoc_getudata(so, 0);
			if (rval)
				return (rval);
			if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
				if (siptr->udata.servtype == T_CLTS)
					rval = EDESTADDRREQ;
				else	rval = ENOTCONN;
				return (rval);
			}
		}
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		/*
		 * Need to bind it for TLI.
		 */
		rval = osoc_dobind(so, NULL, 0, NULL, NULL);
		if (rval)
			return (rval);
	}

	for (i= 0, len = 0; i < msg->msg_iovlen; i++)
		len += msg->msg_iov[i].iov_len;

	if (flags & MSG_DONTROUTE) {
		int	val;

		val = 1;
		rval = osoc_dosetsockopt(so, SOL_SOCKET, SO_DONTROUTE, &val,
				sizeof (val), K_TO_K);
		if (rval)
			return (rval);
	}

	/*
	 * Access rights only in UNIX domain.
	 */
	if (msg->msg_accrightslen) {
		rval = EOPNOTSUPP;
		goto sndout;
	}

	if (flags & MSG_OOB) {
		/*
		 * If the socket is SOCK_DGRAM or
		 * AF_UNIX which we know is not to support
		 * MSG_OOB or the TP does not support the
		 * notion of expedited data then we fail.
		 *
		 * Otherwise we hope that the TP knows
		 * what to do.
		 */
		if (siptr->family == OAF_UNIX ||
				siptr->udata.servtype == T_CLTS ||
				siptr->udata.etsdusize == 0) {
			rval = EOPNOTSUPP;
			goto sndout;
		}
	}

	if (siptr->udata.servtype == T_CLTS) {
		register struct T_unitdata_req	*udata_req;
		register char			*dbuf;
		register char			*tmpbuf;
		register int			pos;
		register int			tmpcnt;
		struct ux_dev			ux_dev;

		if (len < 0 || len > siptr->udata.tidusize) {
			rval = EMSGSIZE;
			goto sndout;
		}

		if ((siptr->udata.so_state & OSS_ISCONNECTED) == 0) {
			switch (siptr->family) {
			case AF_INET:
				if (msg->msg_namelen !=
						sizeof (struct sockaddr_in))
					rval = EINVAL;
				break;

			default:
				if (msg->msg_namelen > siptr->udata.addrsize)
					rval = EINVAL;
				break;
			}
			if (rval)
				goto sndout;
		}

		if (msg->msg_namelen > 0 && siptr->family == AF_UNIX) {
			rval = EOPNOTSUPP;
			goto sndout;
		}

		klen = len;
		kbuf = dbuf = kmem_alloc(klen, KM_SLEEP);
		/*
		 * Have to make one buffer
		 */
		for (i= 0, pos = 0; i < msg->msg_iovlen; i++) {
			rval = copyin(msg->msg_iov[i].iov_base,
					&dbuf[pos],
					msg->msg_iov[i].iov_len);
			if (rval) {
				rval = EFAULT;
				goto sndout;
			}
			pos += msg->msg_iov[i].iov_len;
		}

		if (msg->msg_accrightslen) {
			rval = EOPNOTSUPP;
			kmem_free(kbuf, klen);
			kbuf = NULL;
			goto sndout;
		}

		tmpbuf = siptr->ctlbuf;
		udata_req = (struct T_unitdata_req *)tmpbuf;
		udata_req->PRIM_type = T_UNITDATA_REQ;
		udata_req->DEST_length = MIN(msg->msg_namelen,
				siptr->udata.addrsize);
		udata_req->DEST_offset = 0;
		tmpcnt = sizeof (*udata_req);

		if ((int)udata_req->DEST_length > 0 && tmpbuf != (char *)NULL) {
			/* Copy the msg_name from User Space */
			osoc_ualigned_copy(tmpbuf, udata_req->DEST_length, 
				tmpcnt, msg->msg_name, &udata_req->DEST_offset);
			tmpcnt += udata_req->DEST_length;
		}

		ctlbuf.len = tmpcnt;
		ctlbuf.buf = tmpbuf;

		databuf.len = len == 0 ? -1 : len;
		databuf.buf = dbuf;

#if 0
		cmn_err(CE_WARN, "osoc_sosend: TCLTS sending %d bytes\n",
			databuf.len);
#endif

		rval = osoc_putmsg(sys_fp, &ctlbuf, &databuf, 0, &rv);
		if (rval) {
			if (rval == EAGAIN)
				rval = ENOMEM;
		}
		kmem_free(kbuf, klen);
		kbuf = NULL;

		if (rval == 0) {
			retval = databuf.len == -1 ? 0 : databuf.len;
		}
		goto sndout;
	} else	{
		register struct T_data_req	*data_req;
		register int			tmp;
		register int			tmpcnt;
		register int			firsttime;
		register int			error;
		char				*tmpbuf;
		struct hold			hold;

		if (len == 0) {
			retval = 0;
			goto sndout;
		}

		if (msg->msg_accrightslen) {
			rval = EOPNOTSUPP;
			goto sndout;
		}

		data_req = (struct T_data_req *)siptr->ctlbuf;

		ctlbuf.len = sizeof (*data_req);
		ctlbuf.buf = siptr->ctlbuf;

		/* Allocate space for the whole message */
		klen = len;
		kbuf = kmem_alloc(klen, KM_SLEEP);

		tmp = len;
		firsttime = 0;
		while (tmpcnt = osoc_get_msg_slice(msg, &tmpbuf,
				siptr->udata.tidusize, firsttime, &hold)) {
			if (flags & MSG_OOB) {
				data_req->PRIM_type = T_EXDATA_REQ;
				if ((tmp - tmpcnt) != 0)
					data_req->MORE_flag = 1;
				else	data_req->MORE_flag = 0;
			} else	{
				data_req->PRIM_type = T_DATA_REQ;
			}

			/*
			 * Urgent data.
			 */
			if (tmpcnt > klen) {
				/* Just in case that the above went beyond */
				/* required message */
				kmem_free(kbuf, klen);
				kbuf = NULL;
				rval = EFAULT;
				goto sndout;
			}
			
			rval = copyin(tmpbuf, kbuf, tmpcnt);
			if (!rval) {
				databuf.len = tmpcnt;
				databuf.buf = kbuf;
#if 0
				cmn_err(CE_WARN, 
					"soc_sosend:TCOTS sending %d bytes\n", 
					tmpcnt);
#endif
				rval = osoc_putmsg(sys_fp, &ctlbuf, &databuf, 
						  0, &rv);
			} else {
				rval = EFAULT;
			}

			if (rval) {
				if (len == tmp) {
					if (rval == EAGAIN)
						rval = ENOMEM;
					kmem_free(kbuf, klen);
					kbuf = NULL;
					goto sndout;
				} else	{
					rval = 0;
					retval = len - tmp;
					kmem_free(kbuf, klen);
					kbuf = NULL;
					goto sndout;
				}
			}
			firsttime = 1;
			tmp -= tmpcnt;
		}
		retval = len - tmp;
		kmem_free(kbuf, klen);
		kbuf = NULL;
	}
sndout:
	if (flags & MSG_DONTROUTE) {
		int	val;

		val = 0;
		rval = osoc_setsockopt(so, SOL_SOCKET, SO_DONTROUTE, &val,
			sizeof (val), K_TO_K);
	}
	if (rval) {
		if (rval == ENXIO || rval == EIO)
			rval = EPIPE;
		return (rval);
	}
	*rvalp = retval;
	return	(rval);
}


/*
 * On return, ptr points at the next slice of
 * data of askedfor size. Returns the actual
 * amount.
 */
int
osoc_get_msg_slice(msg, ptr, askedfor, firsttime, hold)
struct msghdr		*msg;
char			**ptr;
int			askedfor;
int			firsttime;
register struct	hold	*hold;
{
	register int		count;

	if (!firsttime) {
		if (msg->msg_iovlen <= 0) {
			*ptr = NULL;
			return (0);
		}
		hold->i = 0;
		hold->left = msg->msg_iov[hold->i].iov_len;
		hold->pos = msg->msg_iov[hold->i].iov_base;
	}
again:
	if (hold->left) {
		if (hold->left > askedfor) {
			*ptr = hold->pos;
			hold->pos += askedfor;
			hold->left -= askedfor;
			return (askedfor);
		} else	{
			*ptr = hold->pos;
			count = hold->left;
			hold->left = 0;
			hold->i++;
			return (count);
		}
	}

	if (hold->i == msg->msg_iovlen)
		return (0);

	hold->pos = msg->msg_iov[hold->i].iov_base;
	hold->left = msg->msg_iov[hold->i].iov_len;

	goto again;
}

/*
 * Close a socket on last file table reference removal.  Initiate disconnect
 * if connected. Free socket when disconnect complete. 
 */
osoc_doclose(so)
struct osocket *so;
{
	int             rval;
	
	rval = 0;
	if (so && so->so_fp) {
		/* The device close function will call osoc_sofree() */
		rval = closef(so->so_fp);
		if(so->so_fd) {
			setf(so->so_fd, NULLFP);
			so->so_fd = 0;
		}
	}
	return (rval);
}

osoc_sofree(so)
struct osocket *so;
{
	if ((so == OSOCK_AVAIL) || 
	    (so == OSOCK_RESERVE) || 
	    (so == OSOCK_INPROGRESS)) {
		return(EINVAL);
	} else  {
		if (so->so_user.ctlbuf)
			kmem_free(so->so_user.ctlbuf, so->so_user.ctlsize);
		kmem_free(so, sizeof(struct osocket));
	}
	return (0);
}


/*
 * Copy data to output buffer and align it as in input buffer
 * This is to ensure that if the user wants to align a network
 * addr on a non-word boundry then it will happen.
 */

osoc_aligned_copy(buf, len, init_offset, datap, rtn_offset)
char	*buf;
int	len;
int	init_offset;
char	*datap;
int	*rtn_offset;
{
	if (valid_usr_range(buf, len))
		return(EFAULT);
	if (valid_usr_range(datap, len))
		return(EFAULT);
	if (valid_usr_range(rtn_offset, 4))
		return(EFAULT);
		
	*rtn_offset = ROUNDUP(init_offset) + ((unsigned int)datap&0x03);
	bcopy(datap, (buf + *rtn_offset), len);
	return(0);
}

osoc_ualigned_copy(buf, len, init_offset, datap, rtn_offset)
char	*buf;
int	len;
int	init_offset;
char	*datap;
int	*rtn_offset;
{
	int	rval;

	if (valid_usr_range(buf, len))
		return(EFAULT);
	if (!valid_usr_range(datap, len))
		return(EFAULT);
	if (valid_usr_range(rtn_offset, 4))
		return(EFAULT);
		
	*rtn_offset = ROUNDUP(init_offset) + ((unsigned int)datap&0x03);
	rval = copyin(datap, (buf + *rtn_offset), len);

	return (rval ? EFAULT : 0);
}

int
osoc_cpaddr(to, tolen, from, fromlen, rsizep)
char		*to;
int		tolen;
char		*from;
int		fromlen;
int		*rsizep;
{
	int rval;

	if (!valid_usr_range(to, tolen))
		return(EFAULT);
	if (valid_usr_range(from, tolen))
		return(EFAULT);
	if (valid_usr_range(rsizep, 4))
		return(EFAULT);

	bzero(to, tolen);
	if (tolen > sizeof (struct sockaddr_in))
		tolen = sizeof (struct sockaddr_in);

	rval = copyout(from, to, MIN(fromlen, tolen));
	if (rval == 0)
		*rsizep = tolen;
	return(rval);
}

osoc_getsocket_with_fd(sopp, fd)
struct osocket **sopp;
int	fd;
{
	struct file *fp;
	struct socket  *so;
	int             min;
	dev_t           rdev;
	int		rval;

	rval = getf(fd, &fp);
	if (rval)
		return (EINVAL);

	rdev = fp->f_vnode->v_rdev;
	return (osoc_getsocket_with_dev(sopp, rdev));
}

osoc_getsocket_with_dev(sopp, rdev)
struct osocket **sopp;
dev_t	rdev;
{
	struct osocket  *so;
	int             min;

	min = getminor(rdev);
	if (!osockinited ||
	    (getmajor(rdev) != osockdev) ||
	    (min < 0) ||
	    (min >= num_osockets)) {
		return (ENOTSOCK);
	}

	so = osocket_tab[min];
	if ((so == OSOCK_AVAIL) || 
	    (so == OSOCK_RESERVE) || 
	    (so == OSOCK_INPROGRESS)) {
		return(EINVAL);
	}

	/*Initialized Socket ? */
	if (so->so_svp == NULL)
		return (EINVAL);

	*sopp = so;
	return (0);
}


osoc_getargs(snamep, unamep, namelen)
caddr_t snamep;
caddr_t unamep;
int	namelen;
{

	if ((unamep == NULL) || (namelen == 0))
		return (EINVAL);

	if (!valid_usr_range(unamep, namelen))
		return(EFAULT);
	if (valid_usr_range(snamep, namelen))
		return(EFAULT);

	if (copyin(unamep, snamep, namelen)) {
		return(EFAULT);
	} 
	return (0);
}


/*
 * The osoc_getmsg(), osoc_putmsg() and osoc_msgio() are duplicates of the 
 * Base getmsg/putmsg/msgio with handling to use kernel addresses instead
 * of user addresses.
 */
int
osoc_getmsg(fp, ctl, data, flags, rvp)
struct file	*fp;
struct strbuf 	*ctl;
struct strbuf 	*data;
int 		*flags;
rval_t		*rvp;
{
	int error;
	int localflags;
	int realflags = 0;
	unsigned char pri = 0;

	/*
	 * Convert between old flags (localflags) and new flags (realflags).
	 */
	localflags = *flags;
	switch (localflags) {
	case 0:
		realflags = MSG_ANY;
		break;

	case RS_HIPRI:
		realflags = MSG_HIPRI;
		break;

	default:
		return (EINVAL);
	}

	if ((error = osoc_msgio(fp, ctl, data, rvp, FREAD, 
				&pri, &realflags)) == 0) {
		/*
		 * massage realflags based on localflags.
		 */
		if (realflags == MSG_HIPRI)
			localflags = RS_HIPRI;
		else
			localflags = 0;
		*flags = localflags;
	}
	return (error);
}

int
osoc_putmsg(fp, ctl, data, flags, rvp)
struct file	*fp;
struct strbuf 	*ctl;
struct strbuf 	*data;
int 		flags;
rval_t		*rvp;
{
	unsigned char pri = 0;

	switch (flags) {
	case RS_HIPRI:
		flags = MSG_HIPRI;
		break;

	case 0:
		flags = MSG_BAND;
		break;

	default:
		return (EINVAL);
	}
	return (osoc_msgio(fp, ctl, data, rvp, FWRITE, &pri, &flags));
}

/*
 * Common code for osoc_getmsg and osoc_putmsg calls: check permissions,
 * copy in args, do preliminary setup, and switch to
 * appropriate stream routine.
 */
osoc_msgio(fp, ctl, data, rvp, mode, prip, flagsp)
struct file	*fp;
struct strbuf 	*ctl;
struct strbuf 	*data;
rval_t 		*rvp;
int 		mode;
unsigned char 	*prip;
int		*flagsp;
{
	vnode_t *vp;
	struct strbuf msgctl, msgdata;
	int error;
	union	sco_args *sco_ap;
	char	*ctl_addr;
	char	*data_addr;
	int	ctl_size;
	int	data_size;
	int	size;

	if ((fp->f_flag & mode) == 0)
		return (EBADF);
	vp = fp->f_vnode;
	if ((vp->v_type != VFIFO && vp->v_type != VCHR) || vp->v_stream == NULL)
		return (ENOSTR);

	/* Setup Control */
	if (ctl)
		msgctl = *ctl;		/* Structure Copy */
	else {
		msgctl.len = -1;
		msgctl.maxlen = -1;
	}

	/* Setup Data */
	if (data)
		msgdata = *data;		/* Structure Copy */
	else {
		msgdata.len = -1;
		msgdata.maxlen = -1;
	}

	if (mode == FREAD) {
		error = osoc_strgetmsg(vp, &msgctl, &msgdata, prip,
				  flagsp, fp->f_flag, rvp);
		if (error)
			return(error);

		if (ctl)
			*ctl = msgctl;		/* Structure Copy */
		if (data)
			*data = msgdata;	/* Structure Copy */
	} else  {
		/*
		 * FWRITE case 
		 */
		error = osoc_strputmsg(vp, &msgctl, &msgdata, *prip, 
				  *flagsp, fp->f_flag);
	}

	return(error);
}


/*
 * This is copy from the BASE of strgetmsg and strputmsg.  They
 * are here to support the use of Kernel addresses instead of 
 * User addresses since its getmsg/putmsg are from kernel space.
 *
 * Get the next message from the read queue.  If the message is 
 * priority, STRPRI will have been set by strrput().  This flag
 * should be reset only when the entire message at the front of the
 * queue as been consumed.
 */
int
osoc_strgetmsg(vp, mctl, mdata, prip, flagsp, fmode, rvp)
	register struct vnode *vp;
	register struct strbuf *mctl;
	register struct strbuf *mdata;
	unsigned char *prip;
	int *flagsp;
	int fmode;
	rval_t *rvp;
{
	register s;
	register struct stdata *stp;
	register mblk_t *bp, *nbp;
	mblk_t *savemp = NULL;
	mblk_t *savemptail = NULL;
	int n, bcnt;
	int done = 0;
	int flg = 0;
	int more = 0;
	int error = 0;
	char *kbuf;
	int mark;
	unsigned char pri;
	queue_t *q;

	ASSERT(vp->v_stream);
	stp = vp->v_stream;
	q = RD(stp->sd_wrq);
	rvp->r_val1 = 0;

	if (error = straccess(stp, JCREAD))
		return (error);
	if (stp->sd_flag & (STRDERR|STPLEX))
		return ((stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror);

	switch (*flagsp) {
	case MSG_HIPRI:
		if (*prip != 0)
			return (EINVAL);
		break;

	case MSG_ANY:
	case MSG_BAND:
		break;

	default:
		return (EINVAL);
	}

	s = splstr();
	mark = 0;
	while (((*flagsp & MSG_HIPRI) && !(stp->sd_flag & STRPRI)) ||
	    ((*flagsp & MSG_BAND) && (!q->q_first ||
	    ((q->q_first->b_band < *prip) && !(stp->sd_flag & STRPRI)))) ||
	    !(bp = getq(q))) {
		/*
		 * If STRHUP, return 0 length control and data.
		 */
		if (stp->sd_flag & STRHUP) {
			mctl->len = mdata->len = 0;
			*flagsp = flg;
			splx(s);
			return (error);
		} 
		if ((error = strwaitq(stp, GETWAIT, (off_t)0, fmode, &done)) || done) {
			splx(s);
			return (error);
		}
	}
	if (bp == stp->sd_mark) {
		mark = 1;
		stp->sd_mark = NULL;
	}
	splx(s);
	
	if (bp->b_datap->db_type == M_PASSFP) {
		s = splstr();
		if (mark && !stp->sd_mark)
			stp->sd_mark = bp;
		putbq(q, bp);
		splx(s);
		return (EBADMSG);
	}

	pri = bp->b_band;
	if (qready())
		runqueues();

	/*
	 * Set HIPRI flag if message is priority.
	 */
	if (stp->sd_flag & STRPRI)
		flg = MSG_HIPRI;
	else
		flg = MSG_BAND;

	/*
	 * First process PROTO or PCPROTO blocks, if any.
	 */
	if (mctl->maxlen >= 0 && bp && bp->b_datap->db_type != M_DATA) {
		bcnt = mctl->maxlen;
		kbuf = mctl->buf;
		while (bp && bp->b_datap->db_type != M_DATA && bcnt >= 0) {
			if ((n = MIN(bcnt, bp->b_wptr - bp->b_rptr))) {
				CATCH_FAULTS(CATCH_KERNEL_FAULTS) {
					bcopy((caddr_t)bp->b_rptr, kbuf, n);
				}
				error = END_CATCH();
				if (error) {
					error = EFAULT;
					s = splstr();
					stp->sd_flag &= ~STRPRI;
					splx(s);
					more = 0;
					freemsg(bp);
					goto getmout;
				}
			}
			kbuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mctl->len = mctl->maxlen - bcnt;
	} else
		mctl->len = -1;
	
		
	if (bp && bp->b_datap->db_type != M_DATA) {	
		/*
		 * More PROTO blocks in msg.
		 */
		more |= MORECTL;
		savemp = bp;
		while (bp && bp->b_datap->db_type != M_DATA) {
			savemptail = bp;
			bp = bp->b_cont;
		}
		savemptail->b_cont = NULL;
	}

	/*
	 * Now process DATA blocks, if any.
	 */
	if (mdata->maxlen >= 0 && bp) {
		bcnt = mdata->maxlen;
		kbuf = mdata->buf;
		while (bp && bcnt >= 0) {
			if ((n = MIN(bcnt, bp->b_wptr - bp->b_rptr))) {
				CATCH_FAULTS(CATCH_KERNEL_FAULTS) {
					bcopy((caddr_t)bp->b_rptr, kbuf, n);
				}
				error = END_CATCH();
				if (error) {
					error = EFAULT;
					s = splstr();
					stp->sd_flag &= ~STRPRI;
					splx(s);
					more = 0;
					freemsg(bp);
					goto getmout;
				}
			}
			kbuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mdata->len = mdata->maxlen - bcnt;
	} else
		mdata->len = -1;

	if (bp) {			/* more data blocks in msg */
		more |= MOREDATA;
		if (savemp)
			savemptail->b_cont = bp;
		else
			savemp = bp;
	} 

	s = splstr();
	if (savemp) {
		savemp->b_band = pri;
		if (mark && !stp->sd_mark) {
			savemp->b_flag |= MSGMARK;
			stp->sd_mark = savemp;
		}
		putbq(q, savemp);
	} else {
		stp->sd_flag &= ~STRPRI;
	}
	splx(s);

	*flagsp = flg;
	*prip = pri;

	/*
	 * Getmsg cleanup processing - if the state of the queue has changed
	 * some signals may need to be sent and/or poll awakened.
	 */
getmout:
	while ((bp = q->q_first) && (bp->b_datap->db_type == M_SIG)) {
		bp = getq(q);
		strsignal(stp, *bp->b_rptr, (long)bp->b_band);
		freemsg(bp);
		if (qready())
			runqueues();
	}



	/*
	 * If we have just received a high priority message and a
	 * regular message is now at the front of the queue, send
	 * signals in S_INPUT processes and wake up processes polling
	 * on POLLIN.
	 */
	if ((bp = q->q_first) && !(stp->sd_flag & STRPRI)) {
 	    if (flg & MSG_HIPRI) {
		s = splstr();
		if (stp->sd_sigflags & S_INPUT) 
			strsendsig(stp->sd_siglist, S_INPUT, (long)bp->b_band);
		if (stp->sd_pollist.ph_events & POLLIN)
			pollwakeup(&stp->sd_pollist, POLLIN);

		if (bp->b_band == 0) {
		    if (stp->sd_sigflags & S_RDNORM)
			    strsendsig(stp->sd_siglist, S_RDNORM, 0L);
		    if (stp->sd_pollist.ph_events & POLLRDNORM) 
			    pollwakeup(&stp->sd_pollist, POLLRDNORM);
		} else {
		    if (stp->sd_sigflags & S_RDBAND)
			    strsendsig(stp->sd_siglist, S_RDBAND,
				(long)bp->b_band);
		    if (stp->sd_pollist.ph_events & POLLRDBAND) 
			    pollwakeup(&stp->sd_pollist, POLLRDBAND);
		}
		splx(s);
	    } else {
		if (pri != bp->b_band) {
		    s = splstr();
		    if (bp->b_band == 0) {
			if (stp->sd_sigflags & S_RDNORM)
				strsendsig(stp->sd_siglist, S_RDNORM, 0L);
			if (stp->sd_pollist.ph_events & POLLRDNORM) 
				pollwakeup(&stp->sd_pollist, POLLRDNORM);
		    } else {
			if (stp->sd_sigflags & S_RDBAND)
				strsendsig(stp->sd_siglist, S_RDBAND,
				    (long)bp->b_band);
			if (stp->sd_pollist.ph_events & POLLRDBAND) 
				pollwakeup(&stp->sd_pollist, POLLRDBAND);
		    }
		    splx(s);
		}
	    }
	}
	rvp->r_val1 = more;
	return (error);
}

/*
 * Put a message downstream.
 */
int
osoc_strputmsg(vp, mctl, mdata, pri, flag, fmode)
	register struct vnode *vp;
	register struct strbuf *mctl;
	register struct strbuf *mdata;
	unsigned char pri;
	register flag;
	int fmode;
{
	register struct stdata *stp;
	mblk_t *mp;
	register s;
	register long msgsize;
	long rmin, rmax;
	int error, done;
	struct uio uio;
	struct iovec iov;
	int strmakemsg();

	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	if (error = straccess(stp, JCWRITE))
		return (error);
	if (stp->sd_flag & STPLEX)
		return (EINVAL);
	if (stp->sd_flag & (STRHUP|STWRERR)) {
		if (stp->sd_flag & STRSIGPIPE)
			psignal(u.u_procp, SIGPIPE);
		return(stp->sd_werror);
	}

	/*
	 * firewall - don't use too much memory
	 */

	if (strthresh && (Strcount > strthresh) && pm_denied(u.u_cred, P_SYSOPS))
		return (ENOSR);

	/*
	 * Check for legal flag value.
	 */
	switch (flag) {
	case MSG_HIPRI:
		if ((mctl->len < 0) || (pri != 0))
			return (EINVAL);
		break;
	case MSG_BAND:
		break;

	default:
		return (EINVAL);
	}

	/*
	 * Make sure ctl and data sizes together fall within the
	 * limits of the max and min receive packet sizes and do
	 * not exceed system limit.
	 */
	rmin = stp->sd_wrq->q_next->q_minpsz;
	rmax = stp->sd_wrq->q_next->q_maxpsz;
	ASSERT((rmax >= 0) || (rmax == INFPSZ));
	if (rmax == 0)
		return (ERANGE);
	if (strmsgsz != 0) {
		if (rmax == INFPSZ)
			rmax = strmsgsz;
		else
			rmax = MIN(strmsgsz, rmax);
	}
	if ((msgsize = mdata->len) < 0) {
		msgsize = 0;
		rmin = 0;	/* no range check for NULL data part */
	}
	if ((msgsize < rmin) ||
	    ((msgsize > rmax) && (rmax != INFPSZ)) ||
	    (mctl->len > strctlsz))
		return (ERANGE);

	s = splstr();
	while (!(flag & MSG_HIPRI) && !bcanput(stp->sd_wrq->q_next, pri)) {
		if ((error = strwaitq(stp, WRITEWAIT, (off_t) 0, fmode, &done)) || done) {
			splx(s);
			return (error);
		}
	}
	splx(s);

	iov.iov_base = mdata->buf;
	iov.iov_len = mdata->len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = iov.iov_len;
	if ((error = strmakemsg(mctl, mdata->len, &uio, stp, (long)flag, &mp)) || !mp)
		return (error);
	mp->b_band = pri;

	/*
	 * Put message downstream.
	 */
	(*stp->sd_wrq->q_next->q_qinfo->qi_putp)(stp->sd_wrq->q_next, mp);
	if (qready())
		runqueues();
	return (0);
}

osoc_strwaitq(vp, flag, count, fmode, done)
struct vnode	*vp;
int flag;
off_t count;
int fmode;
int *done;
{
	int rval;
	
	rval = strwaitq(vp->v_stream, flag, count, fmode, done);
	return(rval);
}

int
osoc_doadjtime(delta, olddelta)
struct timeval *delta;
struct timeval *olddelta;
{
	register long	previous;	/* uncompleted previous adjustment */
	struct timeval	tv;

	if (pm_denied(u.u_cred, P_SYSOPS))
		return EPERM;

	if (copyin((caddr_t)delta, (caddr_t)&tv, sizeof tv))
		return EFAULT;

	previous = clockadj(tv.tv_sec * MICROSEC + tv.tv_usec);

	if (olddelta) {
		tv.tv_sec = previous / MICROSEC;
		tv.tv_usec = previous % MICROSEC;

		if (copyout((caddr_t)&tv, (caddr_t)olddelta, sizeof tv))
			return EFAULT;
	}

	return 0;
}

svr4_to_scocoff(errno)
int errno;
{
	if ((errno > 0) && (errno < nerror_sco))
		return(svr4_to_sco[errno]);
	return(errno);
}

/* Enhanced Application Binary Compatibility */
