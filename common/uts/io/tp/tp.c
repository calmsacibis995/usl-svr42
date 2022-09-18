/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/tp/tp.c	1.5.3.5"
#ident	"$Header: $"
#include <util/debug.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/cred.h>
#include <io/ddi.h>
#include <mem/kmem.h>
#include <io/ttydev.h>
#include <io/termios.h>
#include <io/termio.h>
#include <io/strtty.h>
#include <fs/file.h>
#include <util/cmn_err.h>
#include <io/tp/tp.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>

/**********/
/* external function declarations */
	extern int	sleep();
	extern int	splx();
	extern int	bcopy();
	extern int 	bzero();
	extern void	wakeup();
/**********/

/**********/
/* Local function declarations */
STATIC int		tpopen(),tpclose(),tpuwput(),tpuwsrv(),
			tplwsrv(),tplrput(),tplrsrv(),tpursrv();
STATIC int		tp_growlist(),
			tp_setupcons(),tp_setupctrl(),tp_setupdata(),
			tp_conndata(),tp_conncons(),tp_disccons(),
			tp_verifysak(),tp_sendioctl(),
			tp_ioctlok(),tp_stdioctl(),tp_consset();
STATIC	void		tp_hookchan(),tp_setsak(),
			tp_discdata(),tp_discinput(),tp_senddiscioctl(),
			tp_dataioctl(),tp_ctrlioctl(),tp_lctrlioctl(),
			tp_admioctl(),tp_consioctl(),
			tp_freechan(),tp_freedev(),tp_newmsgs(),
			tp_sendmsg(),tp_putsak(),tp_fillinfo(),
			tp_setcons(),tp_resetcons(),
			tp_saktypeswitch();
STATIC	struct tpchan 	*tp_allocchan(),*tp_findchan();
STATIC	struct tpdev	*tp_allocdev(),*tp_finddev();
STATIC	mblk_t		*tp_allocioctlmsg();
/**********/


/**********/
/* Initialization of required STREAMS data structures */

/* External variables used by streams */
STATIC struct module_info tp_minfo = {	/* module/driver information */
	0xbeef,			/* module/driver id number */
	"tp",			/* module/driver name */
	0,			/* min packet size acceptable */
	INFPSZ,			/* max packet size acceptable */
	512,			/* hi-water mark for message flow control */
	128			/* lo-water mark for message flow control */
};

STATIC struct qinit tp_urinit = {	/* Queue info for upper read side */
	NULL,			/* ...put() */
	tpursrv,		/* ...srv() */
	tpopen,			/* ...open() */
	tpclose,		/* ...close() */
	NULL,			/* ...admin() */
	&tp_minfo,		/* module_info */
	NULL			/* stat */
};

STATIC struct qinit tp_uwinit = {	/* Queue info for upper write side */
	tpuwput,		/* put procedure*/
	tpuwsrv,		/* service procedure*/
	NULL,			/* open procedure*/
	NULL,			/* close procedure*/
	NULL,			/* admin procedure*/
	&tp_minfo,		/* module info*/
	NULL			/* stat*/
};

STATIC struct qinit tp_lrinit = {	/* Queue info for lower read side */
	tplrput,		/* put procedure*/
	tplrsrv,		/* service procedure*/
	NULL,			/* open procedure*/
	NULL,			/* close procedure*/
	NULL,			/* admin procedure*/
	&tp_minfo,		/* module info*/
	NULL			/* stat*/
};

STATIC struct qinit tp_lwinit = {	/* Queue info for lower write side */
	NULL,			/* put procedure*/
	tplwsrv,		/* service procedure*/
	NULL,			/* open procedure*/
	NULL,			/* close procedure*/
	NULL,			/* admin procedure*/
	&tp_minfo,		/* module info*/
	NULL			/* stat*/
};

/* External variables used by streams */
struct streamtab tpinfo = {	/* saved in cdewsw.  external access point to
				** driver
				*/
	&tp_urinit,
	&tp_uwinit,
	&tp_lrinit,
	&tp_lwinit
};
/**********/


/**********/
/* tuneable values from tp.cf */ 
extern major_t		tp_imaj;		/* TP's internal major number*/
extern int		tp_listallocsz;		/* List expansion chunk size*/
extern clock_t		tp_saktypeDATA_switchdelay; /* Delay time to change sak type to DATA*/
extern struct termios	tp_charmasktermios;	/* Char SAK termios mask*/
extern struct termios	tp_dropmasktermios;	/* Linedrop SAK termios mask*/
extern struct termios	tp_breakmasktermios;	/* Break SAK termios mask*/
extern struct termios	tp_charvalidtermios;	/* Char SAK valid termios*/
extern struct termios	tp_dropvalidtermios;	/* Linedrop SAK valid termios*/
extern struct termios	tp_breakvalidtermios;	/* Break SAK valid termios*/
extern major_t		tp_consoledevmaj;	/* Default Major Device number
						** of console
						*/
extern minor_t		tp_consoledevmin;	/* Default Minor Device number
						** of console
						*/
/*local external statics, used only in tp.c*/

STATIC dev_t tp_consoledev;	/* Default Device number of console assigned
				** from tp_consoledevmaj and tp_consoledevmin in
				** tpstart().
				*/
STATIC major_t tpemaj;		/* external TP major # calculated by tpstart()*/

STATIC int tpchan_curcnt = 0;	/* Current length of TP channel list*/ 

STATIC int tpdev_curcnt = 0;	/* Current length of TP device list*/ 

STATIC struct tpchan **tpchanhead;	/* TP channel list*/

STATIC struct tpdev **tpdevhead;	/* TP device list*/

STATIC struct tpdev *tpconsdev = (struct tpdev *)0;/*the current console MUX*/
/**********/



int tpdevflag = 0;		/* This is expected by cunix for new DDI.*/




#define	HI16		(0xFFFF0000)
#define	TPINFOSZ	(sizeof(struct tp_info))
#define	DATACONNECTFLAGS (TPINF_DISCONNDATA_ONHANGUP | TPINF_FAILUNLINK_IFDATA)
#define	MSIZ(a)		((a)->b_cont->b_datap->db_lim - (a)->b_cont->b_datap->db_base)
#define	ASCII_7BITMASK	(0x7f)
#define	FINDDEVLINKED (1)
#define FILLINFO_ALL (0)
#define FILLINFO_RESTRICTED (1)
#define	MILLITOMICRO(time) ((time) * 1000)


/* allocate tpchan and tpdev structure pointers */
int
tpstart()
{
	register ulong	sz,cnt;

	/*
	** Did not put in error handling if kmem_zalloc() was not able to
	** allocate memory.  If it could not, it will be handled by tpopen(),
	** If the system has not already panicked.
	*/
	if (!tp_listallocsz){
		cmn_err(CE_WARN,
		"Tuneable value 'tp_listallocsz' cannot be 0, forcing to 1\n");
		tp_listallocsz = 1;
	}

	/*
	** The initial allocation of channel and device entries must not be
	** less than TP_NONCLONE because the special channels must be
	** allocated.  Once the initial allocation is made, the lists can 
	** grow by any non-zero increment. 
	*/
	cnt = (tp_listallocsz < TP_NONCLONE) ? TP_NONCLONE : tp_listallocsz;
	sz = cnt * sizeof(struct tpchan *);
	tpchanhead = (struct tpchan **)kmem_zalloc(sz, KM_NOSLEEP);
	if (!tpchanhead){
		return (0);
	}

	tpdevhead = (struct tpdev **)kmem_zalloc(sz, KM_NOSLEEP);
	if (!tpdevhead){
		kmem_free((caddr_t)tpchanhead, sz);
		return (0);
	}

	tpchan_curcnt = cnt;
	tpdev_curcnt = cnt;


	/* calculate external major device number for tp */

	tpemaj = itoemajor(tp_imaj, -1);

	/* calculate default device number for system console */
	tp_consoledev = makedevice(tp_consoledevmaj, tp_consoledevmin);

	return (0);
}

/*
** Get Trusted Path's major device number.
*/
int
tp_getmajor(majornump)
major_t	*majornump;
{
	*majornump = itoemajor(tp_imaj, -1);
	return (1);	/* Return "TRUE" value, since stub function
			** returns "FALSE" value.
			*/
}



STATIC int
tpopen(q, devp, oflag, sflag, credp)
queue_t	*q;
dev_t	*devp;
int	oflag;		/* open flags */
int	sflag;		/* STREAM open flags */
cred_t	*credp;
{

	register ulong		flags=0,type;
	register int		(*setup)();
	register minor_t	min;
	register struct tpchan	*tpcp;


	/*
	** TP devices cannot be opened as STREAMS modules (sflag == MODOPEN)
	** or via the CLONE device (sflag == CLONEOPEN).  TP device must be
	** opened as a normal driver (sflag == 0).
	*/
	if (sflag)
		return (ENXIO);

	/*
	** The way this switch works, all clone masters set up necessary
	** information and break out of the switch to finish allocating and
	** initializing the clone.  All non-clone devices must do any necessary
	** work and return directly from the switch.
	*/
	min = geteminor(*devp);
	switch(min){
	case TP_CONSDEV:
		/*
		** If "delay open" is set, wait until tpconsdev gets set.
		** When tpconsdev gets set, the real/physical device is linked
		** under the TP device labeled the 'console' device.
		*/
		if (!(oflag & (FNDELAY|FNONBLOCK))){
			while (!tpconsdev){
				if (sleep((caddr_t)&tpconsdev, TTIPRI|PCATCH)){
					return (EINTR);
				}
			}
		}
		tpcp = tp_findchan(min);
		if (!tpcp){
			tpcp = tp_allocchan(TPC_CONS);
			if (!tpcp){
				return (EAGAIN);
			}
		}
		return (tp_setupcons(tpcp, q));
	case TP_CTRLCLONE:
		type  = TPC_CTRL;
		setup = tp_setupctrl;
		break;
	case TP_DATACLONE:
		type = TPC_DATA;
		setup = tp_setupdata;
		break;
	case TP_ADMDEV:
		tpcp = tp_findchan(min);
		if (!tpcp){
			tpcp = tp_allocchan(TPC_ADM);
			if (!tpcp){
				return (EAGAIN);
			}
			tp_hookchan(tpcp,q);
		}
		return (0);
	case TP_RESERVDEV1:
	case TP_RESERVDEV2:
	case TP_RESERVDEV3:
	case TP_RESERVDEV4:
	case TP_RESERVDEV5:
	case TP_RESERVDEV6:
		return (ENXIO);
	default:
		/*
		** The request is for a specific clone.  The clone must have
		** a non-NULL channel entry and be connected.  All channels
		** are disconnected on close, so, if the device is connected
		** it is open.
		** NOTE: Since a control channel can only be opened by one
		** process at a time. There is, therefore, no legal, direct way
		** to open a control clone.
		*/
		tpcp = tp_findchan(min);
		if (!tpcp)
			return (ENXIO);	/*Channel does not exist*/

		if (tpcp->tpc_type != TPC_DATA)
			return (EBUSY);	/*Channel is CONTROL (see NOTE above)*/

		if (!tpcp->tpc_devp)
			return (EIO);	/*IO Error, channel not connected*/

		if (tpcp->tpc_flags & TPC_BLOCKED){
			return (EIO);
		}
		return (0);
	}

	/*
	** If we got here the channel was either a CONTROL or DATA clone master
	** device and the type specific information has been initialized. 
	** Proceed to allocate the clone and fill it out.
	*/
	tpcp = tp_allocchan(type);
	if (!tpcp){
		return (EAGAIN);
	}

	/*
	** Send back the major minor pair created by tp_allocchan.
	*/
	*devp = tpcp->tpc_dev;
	return ((*setup)(tpcp,q));
}

/*
** Set the cons channel TP device pointer to 'console' TP device.
** Send the M_SETOPTS message up stream to tell the stream head
** that this stream is a tty.
*/
STATIC	int
tp_setupcons(tpcp,q)
struct	tpchan	*tpcp;
queue_t		*q;
{
	mblk_t			*bp;
	struct stroptions	*sop;

	tp_hookchan(tpcp,q);
	tpcp->tpc_devp = tpconsdev;

	bp = allocb(sizeof(struct stroptions),BPRI_HI);
	if (!bp){
		tp_freechan(tpcp);
		return (EAGAIN);
	}
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof(struct stroptions);
	sop = (struct stroptions *)bp->b_rptr;
	sop->so_flags = SO_ISTTY;
	putnext(q, bp);
	return (0);
}

/*
** Allocate a M_HANGUP message to be sent up the data channel when a SAK
** is detected.  Also send the M_SETOPTS message up stream to tell the
** stream head that this stream is a tty.
*/
STATIC	int
tp_setupdata(tpcp,q)
struct	tpchan	*tpcp;
queue_t		*q;
{
	mblk_t			*bp;
	struct stroptions	*sop;

	/*
	** Allocate necessary messages.
	*/
	tp_newmsgs(tpcp);
	if (!tpcp->tpc_hupmsg){
		tp_freechan(tpcp);
		return (EAGAIN);
	}

	tp_hookchan(tpcp,q);
	/*
	** Allocate and send the M_SETOPTS message.
	*/
	bp = allocb(sizeof(struct stroptions),BPRI_HI);
	if (!bp){
		tp_freechan(tpcp);
		return (EAGAIN);
	}
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof(struct stroptions);
	sop = (struct stroptions *)bp->b_rptr;
	sop->so_flags = SO_ISTTY;
	putnext(q, bp);
	return (0);
}

/*
** Allocate the M_PCPROTO buffer to be sent up the control channel to
** the stream head when a SAK is detected.
*/
STATIC	int
tp_setupctrl(tpcp,q)
struct	tpchan	*tpcp;
queue_t		*q;
{
	tp_newmsgs(tpcp);
	if ((!tpcp->tpc_sakmsg) || (!tpcp->tpc_hupmsg)){
		tp_freechan(tpcp);
		return (EAGAIN);
	}
	tp_hookchan(tpcp,q);
	return (0);
}

/*
** Set up channel queue pointers.
*/
STATIC	void
tp_hookchan(tpcp,q)
struct	tpchan	*tpcp;
queue_t		*q;
{
	q->q_ptr = (caddr_t)tpcp;
	WR(q)->q_ptr = (caddr_t)tpcp;
	tpcp->tpc_rq = q;
	tpcp->tpc_devp = (struct tpdev *)0;
}

STATIC int
tpclose(q, flag, credp)
queue_t	*q;
int	flag;
cred_t	*credp;
{
	register struct tpchan	*tpcp;
	register struct	tpdev	*devp;
	register struct tpchan *datap;
	register	int	s;


	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		** This should never happen!
		** Nothing further can be done so just return.
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tpclose()\n");
		return (0);
	}

	/*
	** If the channel is not connected or it is the adm channel,
	** just free the channel and get out. 
	*/
	if (!tpcp->tpc_devp || tpcp->tpc_type == TPC_ADM){
		tpcp->tpc_devp = (struct tpdev *)0;
		tp_freechan(tpcp);
		q->q_ptr = (struct tpchan *)0;
		WR(q)->q_ptr = (struct tpchan *)0;
		return (0);
	}

	/*
	** If we got to here we have a DATA or CTRL channel and it is
	** connected, or we have a the CONS channel that is connected for
	** output and it may be connected for input.  The associated TP device
	** may or may not have a real/physical device linked underneath.
	*/
	devp = tpcp->tpc_devp;
	switch (tpcp->tpc_type){
	case TPC_CTRL:
		/*
		** Clear out the fields in the TP device that will indicate
		** to the interrupt level that the ctrl channel is closed.
		** This includes the tdp_ctrlchp, and tpd_ioctlchan if it
		** matches the channel being closed.
		*/
		s = splstr();
		devp->tpd_ctrlchp = (struct tpchan *)0;
		if (devp->tpd_ioctlchan == tpcp){
			devp->tpd_ioctlchan = (struct tpchan *)0;
			devp->tpd_ioctlid = 0;
		}
		if (tpcp->tpc_ioctlmp){
			freemsg(tpcp->tpc_ioctlmp);
			tpcp->tpc_ioctlmp = (mblk_t *)0;
		}
		splx(s);

		devp->tpd_ctrlrq = (queue_t *)0;
		tpcp->tpc_devp = (struct tpdev *)0;
		q->q_ptr = (caddr_t)0;
		WR(q)->q_ptr = (caddr_t)0;
		tp_freechan(tpcp);

		/*
		** If the SAKSET flag is set and the SAK type is none (if SAK
		** type is none, the only time SAKSET would be set is the
		** special case of SAK type none indicated by saktypeDATA),
		** then a 'fake' SAK has been received (see tplrput() for
		** details).  Since the control channel is going away there is
		** no need to remember the fake SAK, so clear the SAKSET flag.
		*/
		if (devp->tpd_sak.sak_type == saktypeDATA){
			devp->tpd_flags &= ~TPD_SAKSET;
		}
		/*
		** If the device is not persistently linked, the real/physical
		** device was already unlinked (called internally by Stream
		** Head functions) before the close of this channel was called.
		** We will not get any more messages from tplrput() so it is
		** safe to dismantle the TP device.  Since the TP device is
		** being dismantled, the data and cons channel must be
		** disconnected if they are connected to the TP device.
		*/
		if (!(devp->tpd_flags & TPD_PERSIST)){
			/*
			** If device is the 'console' deivce reset
			** 'console' device
			*/
			if (devp == tpconsdev){
				tp_resetcons((struct tpdev *)NULL);
			}
			datap = devp->tpd_datachp;
			if (datap){
				tp_discdata(devp);
			}
			/*
			** Before freeing TP device call untimeout() if there
			** is a pending function to be called that was setup
			** by timeout().  This is necessary since the pending
			** function's argument is a pointer to the TP device
			** structure.
			** NOTE: Other candidate areas to call untimeout()
			** are when the data channel is connected to a TP
			** device, tp_conndata(), and when the sak type is
			** changed to a type other than NONE, tp_putsak().
			** It is not done since the pending function,
			** tp_saktypeswitch() makes checks that prevents
			** the sak type from being changed to DATA when it
			** is not appropriate, and having the splhi() called
			** for every data channel connect and change of sak is
			** not worth it.
			**
			** Prevent the timeout function from being called
			** (via clock interrput) while checking tpd_timeoutid.
			*/
			s = splhi();
			if (devp->tpd_timeoutid){
				untimeout(devp->tpd_timeoutid);
				devp->tpd_timeoutid = 0;
			}
			splx(s);
			tp_freedev(devp);
		}else{
			/*
			** Device is persistently linked, if a SAK
			** is pending, disconnect and error out the
			** data channel.  This is to handle the case
			** were a SAK was detected before tpclose()
			** marked the ctrl channel as closed.
			** tplrput() would have sent up SAK
			** notification up the ctrl channel, but since
			** we are closing the the ctrl channel, the
			** notification would go unprocessed and the
			** data channel would remain connected.  This
			** would be a problem if the intent was to
			** disconnect the data channel.  So to be safe
			** the data channel is disconnected.  This is
			** the same action as is in tplrput() when it
			** detects a SAK and there is no ctrl channel.
			*/
			if (datap && (devp->tpd_flags & TPD_SAKSET)){
				tp_discdata(devp);
				tp_discinput(devp);
			}
		}
		break;

	case TPC_DATA:

		tp_discdata(devp);
		q->q_ptr = (caddr_t)0;
		WR(q)->q_ptr = (caddr_t)0;
		tpcp->tpc_devp = (struct tpdev *)0;
		tp_freechan(tpcp);
		break;

	case TPC_CONS:

		(void)tp_disccons();
		/*
		** The tpd_ioctlchan is only cleared here, ( if the
		** tpd_ioctlchan is equal to the TPC_CONS channel) and not
		** in tp_disccons() (like it is done in tp_discdata()) because
		** tp_disccons() also gets called from other areas (eg when
		** switching the 'console' TP device) that still have the
		** cons channel retaining output access to a TP device.
		*/
		if (devp->tpd_ioctlchan == tpcp){
			devp->tpd_ioctlchan = (struct tpchan *)0;
			devp->tpd_ioctlid = 0;
		}
		if (tpcp->tpc_ioctlmp){
			freemsg(tpcp->tpc_ioctlmp);
			tpcp->tpc_ioctlmp = (mblk_t *)0;
		}
		q->q_ptr = (caddr_t)0;
		WR(q)->q_ptr = (caddr_t)0;
		tpcp->tpc_devp = (struct tpdev *)0;
		tp_freechan(tpcp);
		break;

	default:
		break;

	} /* end switch */
	return (0);
}


/*
** tpuwput: Upper write put procedure.
**
** This routine handles only M_FLUSH and M_IOCTL.  Other message types
** are passed on down the stream, provided the channel on which they were
** sent supports messages other than M_FLUSH and M_IOCTL.
** There is special handling for M_IOCDATA messages.
** NOTE: The TP driver does not support Transparent ioctls but it does send
** all non-TP ioctls downstream including Transparent ioctls.
*/

STATIC int
tpuwput(q, mp)
register queue_t	*q;
register mblk_t		*mp;
{
	register struct tpdev *tpdp = (struct tpdev *)0;
	register struct tpchan *tpcp = (struct tpchan *)0;
	register queue_t *realwq = (queue_t *)0;

	tpcp = (struct tpchan *)q->q_ptr;
	if (tpcp){
		tpdp = tpcp->tpc_devp;
		if ((tpdp) && (tpdp->tpd_realrq)){
			realwq = WR(tpdp->tpd_realrq);
		}
	}

	if (mp->b_datap->db_type == M_FLUSH){
		if (*mp->b_rptr & FLUSHRW){
			if (*mp->b_rptr & FLUSHW){
				if (*mp->b_rptr & FLUSHBAND){
					flushband(q, *(++mp->b_rptr),
					 FLUSHDATA);
					--mp->b_rptr;
				}else{
					flushq(q, FLUSHDATA);
				}
			}
			/*
			** If this is the channel that is connected for input
			** and a real/physical device is linked underneath, the
			** M_FLUSH message will be sent down to the
			** real/physical device driver.
			** Otherwise the M_FLUSH message will be sent back up
			** the channel it originated from if the M_FLUSH message
			** had indicated to flush the read Queue.
			*/ 
			if (tpdp && (tpdp->tpd_inputchp == tpcp) && realwq){
				putnext(realwq, mp);
			}else if (*mp->b_rptr & FLUSHR){
				if (*mp->b_rptr & FLUSHBAND){
					flushband(RD(q), *(++mp->b_rptr),
					 FLUSHDATA);
					--mp->b_rptr;
				}else{
					flushq(RD(q), FLUSHDATA);
				}
				*mp->b_rptr &= ~FLUSHW;
				qreply(q, mp);
			}else{
				freemsg(mp);
			}
		}else{
			freemsg(mp);
		}
		return (0);
	}

	if (!tpcp){
		/*
		** This should never happen! But if it does, the test is
		** put after M_FLUSH processing since putctl1() M_ERROR
		** sends an M_FLUSH back to here.  If it was before M_FLUSH,
		** an inifinite loop of putctl1(), strrput(), qreply(), and
		** tpuwput() will occur.
		** DO NOT MOVE THIS TEST BEFORE M_FLUSH PROCESSING!! 
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tpuwput()\n");
		freemsg(mp);
		putctl1(RD(q)->q_next,M_ERROR,EIO);
		return (EIO);
	}

	switch (mp->b_datap->db_type){

	/*
	** M_IOCDATA messages are handled a little differently then all
	** the other message types (besides M_IOCTL). They are not
	** enqueued if the channel becomes blocked or no physical device
	** is linked underneath; they are turned into an M_IOCNAK and sent
	** back upstream if the M_IOCDATA indicated that the M_COPYIN or
	** M_COPYOUT had succeeded.
	*/
	case M_IOCDATA:

		switch(tpcp->tpc_type){
		case TPC_CTRL:
		case TPC_DATA:
		case TPC_CONS:
			break;
		default:
			/*
			** ADM channel does not pass transparent ioctls
			** downstream, therefore it should not recieve
			** M_IOCDATA messages
			*/
			freemsg(mp);
			return (EINVAL);
		}
		if (!(tpcp->tpc_flags & TPC_BLOCKED) && tpdp && realwq &&
		 realwq->q_next &&
		 (tpdp->tpd_ioctlid == ((struct copyresp *)(mp->b_rptr))->cp_id)){
			putnext(realwq, mp);
		}else{
			struct iocblk	*iocp = (struct iocblk *)(mp->b_rptr); 
			struct copyresp	*cpresp = (struct copyresp *)(mp->b_rptr);

			if (!realwq->q_next){
				/*
				** This should never happen!
				*/
				cmn_err(CE_WARN,
				 "tpuwput(): Have lower write queue without next pointer\n");
			}
			/*
			** If the ioctl id in the M_IOCDATA is the same as the
			** ioctl id saved in the TP device, clear out the saved
			** ioctl id and channel the ioctl originated from in
			** the TP device.  If there are other ioctls waiting
			** to be sent downstream, schedule the lower ctrl
			** channel write Queue.  The TP driver's lower write
			** service function will schedule the write Queue's
			** service function of the channel that has a pending
			** ioctl to be sent downstream.
			*/
			if (tpdp && realwq &&
			 (tpdp->tpd_ioctlid ==  cpresp->cp_id)){
				tpdp->tpd_ioctlid = 0;
				tpdp->tpd_ioctlchan = (struct tpchan *)0;

				if (tpdp->tpd_flags & TPD_WAIT_IOCTL){
					enableok(realwq);
					qenable(realwq);
				}
			}
			/*
			** cp_rval == 0 indicates that the previous M_COPYIN
			** or M_COPYOUT was successful.  If it was not
			** successful, the Stream Head is not expecting a
			** M_IOCACK or M_IOCNAK, so just free the message.
			*/
			if (cpresp->cp_rval){
				freemsg(mp);
			}else{
				freemsg(unlinkb(mp));
				if (cpresp->cp_private){
					freemsg(cpresp->cp_private);
				}
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_count = 0;
				iocp->ioc_error = EIO;
				iocp->ioc_rval = 0;
				qreply(q, mp);
			}
		}
		return (0);

	case M_IOCTL:

		switch(tpcp->tpc_type){
		case TPC_CTRL:
			tp_ctrlioctl(q,mp);
			break;
		case TPC_DATA:
			tp_dataioctl(q,mp);
			break;
		case TPC_ADM:
			tp_admioctl(q,mp);
			break;
		case TPC_CONS:
			tp_consioctl(q,mp);
			break;
		default:
			mp->b_datap->db_type = M_IOCNAK;
			((struct iocblk *)mp->b_rptr)->ioc_error = EINVAL;
			qreply(q, mp);
			return (EINVAL);
		}
		return (0);

	default:
		break;
	}

	/*
	** If we got this far it is not a flush or ioctl() or M_IOCDATA.
	** Since ctrl and adm channels can only handle flush or ioctl message
	** types (ctrl can also handle M_IOCDATA messages), ignore the message
	** if the channel is ctrl or adm.
	*/
	if ((tpcp->tpc_type == TPC_CTRL) || (tpcp->tpc_type == TPC_ADM)){
		freemsg(mp);
		return (0);
	}
	/*
	** If channel is blocked, just free the message.
	** Once the channel is blocked, it has to be closed inorder to become
	** eligible to be re-used again.
	*/
	if (tpcp->tpc_flags & TPC_BLOCKED){
		freemsg(mp);
		return (0);
	}
	/*
	** If it is a data or console channel and is connected to TP device
	** and a physical device is linked underneath, send the message
	** (or queue it) down the stream.
	** If the channel is not connected to a TP device, or there
	** is no physical device linked underneath,  prevent the Queue from
	** being schedule and enqueue the message on the Queue.  When the
	** channel is connected or physical device linked underneath the TP
	** device, the channel Queue's service function will be scheduled.
	*/
	if (tpdp){
		if (realwq){
			if (!realwq->q_next){
				/*
				** This should never happen!
				*/
				cmn_err(CE_WARN,
				 "tpuwput(): Have lower write queue without next pointer\n");
				freemsg(mp);
				return (0);
			}
			if (mp->b_datap->db_type >= QPCTL){
				putnext(realwq, mp);
			}else if (!q->q_first && canput(realwq->q_next)){
				putnext(realwq, mp);
			}else{
				putq(q, mp);
			}		
		}else{
			noenable(q);
			putq(q, mp);
		}
		return (0);
	}
	noenable(q);
	putq(q, mp);
	return (0);
}

/*
** tpuwsrv: Upper write service procedure:
**
** Get messages off queue when flow control permits.
**
** NOTE: The write queue is enabled from the lower write service
**       procedure when the lower service procedure is back-enabled. It
**       is also enabled whenever a physical device is linked underneath
**       the queue.
*/

STATIC int
tpuwsrv(q)
register queue_t *q;
{
	register struct tpchan *tpcp;
	register struct tpdev *tpdp;
	register queue_t *realrq;
	register mblk_t *mp;

	/*
	** If there is no channel, issue an error and get out. This should
	** not happen.
	*/
	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		** Messages will be flushed when Stream Head sends M_FLUSH
		** as a result of the M_ERROR being sent upstream.
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tpuwsrv()\n");
		putctl1(RD(q)->q_next, M_ERROR, EIO);
		return (0);
	}
	/*
	** The "queued" ioctl for the channel gets handled before any other
	** messages.
	*/
	if ((mp = tpcp->tpc_ioctlmp) != (mblk_t *)0){
		tpcp->tpc_ioctlmp = (mblk_t *)0;
		switch(tpcp->tpc_type){
		case TPC_CTRL:
			tp_ctrlioctl(q, mp);
			break;
		case TPC_DATA:
			tp_dataioctl(q, mp);
			break;
		case TPC_CONS:
			tp_consioctl(q, mp);
			break;
		default:
			cmn_err(CE_WARN,
			 "tpuwsrv(): Invalid tp channel %d specified for processing \"queued\" ioctl\nmessage type = %d\n",
			 tpcp->tpc_type, mp->b_datap->db_type);
			if (mp->b_datap->db_type == M_IOCTL){
				cmn_err(CE_WARN,
				 "tpuwsrv(): cmd = %d\n",
				 ((struct iocblk *)(mp->b_rptr))->ioc_cmd);
			}
			freemsg(mp);
			break; /*Ignore all other types*/
		}
	}

	/*
	** If the channel is not connected to a TP device or no real/physical
	** device is linked under the TP device, no service can
	** happen, prevent the queue from being schedule and get out but do not
	** issue an error.  If the channel becomes connected its queue will
	** be enabled.
	*/
	tpdp = tpcp->tpc_devp;
	if (!tpdp){
		noenable(q);
		return (0);
	}
	realrq = tpdp->tpd_realrq;
	if (!realrq){
		noenable(q);
		return (0);
	}
	/*
	** Everything is set up, serve the messages one at a time as long as
	** there are messages to send and flow control allows.
	*/
	while (((mp = getq(q)) != (mblk_t *)0) && (canput(WR(realrq)->q_next))){
		/*
		** The TPC_CTRL and TPC_ADM channels should never have any
		** messages on their write Queue.  The service function for
		** TPC_CTRL should only be called if the channel has an M_IOCTL
		** message to send downstream.  TPC_ADM does not handle
		** downstream ioctls.
		*/
		if ((tpcp->tpc_type == TPC_CTRL) || (tpcp->tpc_type == TPC_ADM)){
			cmn_err(CE_WARN,
			 "tpuwsrv(): Messages should not be on CTRL or ADM channel, message type = %d\n",
			 mp->b_datap->db_type);
			freemsg(mp);
			continue;
		}
		/*
		** TPC_BLOCKED is checked for every loop iteration since it
		** may be set if an interrupt occurs (input arrives) and a
		** SAK or M_HANGUP is detected in the lower read procedure
		** (tplrput()).
		*/
		if (tpcp->tpc_flags & TPC_BLOCKED){
			flushq(q, FLUSHDATA);
			return (0);
		}
		putnext(WR(realrq), mp);
	}
	if (mp){
		putbq(q, mp);
	}
	return (0);
}


/* tplwsrv: lower write service procedure
**
** This routine
** 1. sends internally generated ioctls downsteam.  Internally generated
** ioctls get enqueued on the lower ctrl channel write Queue, when the
** lower Stream has an ioctl message that it has not ACK/NAK yet.  tplrput()
** enables the lower ctrl channel's write Queue when the it receives the
** ACK/NAK.
** 2. enables write Queues of the connected channels (the upper Streams) on
** the TP device.  Messages sent downstream from the connected channels on
** the TP device get enqueued on the lowest write Queue (the upper side of TP
** Multiplexor)  of the connected channels (all tp channels are upper Streams).
** Enabling the write Queues of the connected channels schedules tpuwsrv()
** to run which actually does the work of sending the messages downstream.
**
** This function gets scheduled to run:
** 1. when the TP device lower ctrl channel's write Queue
** (aka the real/physical device's upper most write Queue) is back-enabled by
** the STREAMS scheduler.  This can happen when the next downstream queue falls
** below its low water mark.
** 2. by tplrput after recieving the negative or positive acknowlegement of
** an ioctl.  This is done to schedule Queues of channels that have an ioctl
** to send downstream.
**
** NOTE: tplwsrv has to enable tpuwsrv, because back-enabling can not
** cross multiplexer boundaries 
*/

STATIC int
tplwsrv(q)
register queue_t *q;
{
	register struct tpdev *tpdp;
	register queue_t *uwq;		/* for write queue of upper channels */
	register mblk_t	*mp;
	register struct tpchan *constpcp;
	int		ioctlsvc = 0;	/* indicates whether or not an ioctl
					** has been serviced or is in progress.
					** 0 indicates no.
					*/

	tpdp = (struct tpdev *)q->q_ptr;
	if (!tpdp){
		/*
		** This should not happen!
		*/
		cmn_err(CE_WARN,
			"Queue without TP device pointer in tplwsrv()\n");
		return (0);
	}

	/*
	** Servicing ioctls on this Write Queue (the lower CTRL channel) and
	** scheduling Queues for channels with ioctl to be sent downstream
	** is done first.  Pending ioctls are handled in the following order;
	** internally generated, ctrl channel, channel connected for input,
	** data channel, and cons channel.
	**
	** Only M_IOCTL messages should be on this Queue.
	** NOTE: the q_flag for QNOENB check is done to prevent an infinite
	** loop.  When tp_sendioctl() can not send the message it puts the
	** message back on the Queue and no-enables the Queue.
	** It may not be able to send the ioctl message if the lower Stream
	** still has an an outstanding ioctl.
	*/
	if (!(tpdp->tpd_flags & TPD_BUSY_IOCTL)){
		while ((q->q_first) && !(q->q_flag & QNOENB) && (mp = getq(q))){
			if (mp->b_datap->db_type == M_IOCTL){
				tp_lctrlioctl(q, mp);
				ioctlsvc++;
				break;
			}else{
				cmn_err(CE_WARN,
				 "tplwsrv(): Got a non-ioctl message on lower ctrl channel write Queue; freeing message\n");
				freemsg(mp);
			}
		}
	}else{
		ioctlsvc++;
	}

	/* The Write Queues for the channels are scheduled for service if
	** they are connected to the TP device AND (there is a message on
	** the Queue OR (there is an ioctl message the channel AND there is
	** not another ioctl scheduled or in progress downstream)).
	**
	** NOTE: The ctrl channel may not be connected even though the data
	** channel and TP device are intact.  This can occur if the
	** real/physical device was linked via I_PLINK. 
	*/
	if (tpdp->tpd_ctrlrq && tpdp->tpd_ctrlchp){
		uwq = WR(tpdp->tpd_ctrlrq);
		if (uwq && (uwq->q_first ||
		 (tpdp->tpd_ctrlchp->tpc_ioctlmp && !ioctlsvc))){
			enableok(uwq);
			qenable(uwq);
		}
	}

	if (tpdp->tpd_inputrq && tpdp->tpd_inputchp){
		uwq = WR(tpdp->tpd_inputrq);
		if (uwq && (uwq->q_first ||
		 (tpdp->tpd_inputchp->tpc_ioctlmp && !ioctlsvc))){
			enableok(uwq);
			qenable(uwq);
		}
	}

	/*
	** If the data channel is not the channel connected for input,  its
	** Write Queue may need to be scheduled for service.
	*/
	if (tpdp->tpd_datarq &&  tpdp->tpd_datachp &&
	 (tpdp->tpd_datarq != tpdp->tpd_inputrq)){
		uwq = WR(tpdp->tpd_datarq);
		if (uwq && (uwq->q_first ||
		 (tpdp->tpd_datachp->tpc_ioctlmp && !ioctlsvc))){
			enableok(uwq);
			qenable(uwq);
		}
	}


	/*
	** If this TP device is also the console TP device enable the cons
	** channel write queue, if it is connected (for at least output)
	** to this TP device.
	*/
	if (tpdp == tpconsdev){
		if (((constpcp = tp_findchan(TP_CONSDEV)) != (struct tpchan *)0)
		 && (constpcp->tpc_rq)){
			uwq = WR(constpcp->tpc_rq);
			if (uwq && (uwq->q_first ||
			 (constpcp->tpc_ioctlmp && !ioctlsvc))){
				enableok(uwq);
				qenable(uwq);
			}
		}
	}

	return (0);
}
/* tp_ctrlioctl: process ioctls sent via the ctrl channel
**
** This function handles all tp specific ioctl's.  The rules are as
** follows:
**	-The first ioctl on a ctrl channel must be TP_CONNECT.
**	-TP_DEFSAK must be issued before a physical device can be
**	 linked.
**	-A physical device must be linked before any ioctl will be sent
**	 further downstream.
**	-A TCSET* must be sent down the control channel and an ACK must
**	 be received from the lower stream before a TP_DATACONNECT may
**	 occur.
**	-TP_DATACONNECT clears the sakset flag and unblocks the data
**	 channel.
**
** When the real/physical device is being linked,  an internal TCGETS
** M_IOCTL message is allocated and enqueued on the real device's
** (lower ctrl channel's) write Queue.  When the service proceedure
** is run (tplwsrv),  the TCGETS M_IOCTL message will be sent down to the
** real/physical tty device. The termios value returned from the
** real/physical tty device will be used to initialize the TP device's
** termios, tpd_curterm.
*/

STATIC void
tp_ctrlioctl(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	register struct tpchan	*tpcp;
	register struct tpdev	*tpdp;
	register struct iocblk	*iocp;
	register struct linkblk	*linkp;
	register struct tp_info	*infp;
	struct tpdev		*newtpdp;
	mblk_t			*ioctlmp;
	int			cmd;
	unsigned char		reply = M_IOCACK;
	int			error = 0;
	int			s;

	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		** This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_ctrlioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;
	/*
	** The ctrl channel must be connected to a tp device before any
	** other ioctl can be considered. 
	*/
	tpdp = tpcp->tpc_devp;
	if (!tpdp && (cmd != TP_CONNECT)){
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_error = ENXIO;
		qreply(q,mp);
		return;
	}
	/*
	** Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_CONNECT:
	case TP_DATACONNECT:
	case TP_DATADISCONNECT:
	case TP_DEFSAK:
	case TP_CONSCONNECT:
	case TP_CONSDISCONNECT:
	case TP_CONSSET:
	case TP_GETINF:
		if ( iocp->ioc_count < TPINFOSZ){
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		break;
	}
	switch (cmd){
	case TP_CONNECT:
		/*
		** If the ctrl channel is not already connected, allocate
		** a free tp device.
		*/
		if (tpcp->tpc_devp){
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		}
		tpdp = tp_finddev(infp->tpinf_rdev, 0);
		if (!tpdp){
			tpdp = tp_allocdev(infp->tpinf_rdev);
			if (!tpdp){
				reply = M_IOCNAK;
				error = EAGAIN;
				break;
			}
		}
		if (tpdp->tpd_ctrlchp){
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		}
		/*
		** Connect the ctrl channel to the allocated tpdev and
		** set up the control channel TP device queue.
		*/
		tpdp->tpd_ctrlchp = tpcp;
		tpdp->tpd_ctrlrq = RD(q);
		tpcp->tpc_devp = tpdp;
		if (infp->tpinf_flags & TPINF_CONSOLE){
			tpdp->tpd_userflags |= (infp->tpinf_flags & TPINF_CONSOLE);
		/* Do not mark console output to be swithed to the default
		** console (indicated by tp_consoledev) if a console is
 		** already defined (indicated by tpconsdev).  The switch
		** back to the default console (provided that it is still
		** linked) is made when the real device referenced by
		** tpconsdev is unlinked.
		*/
		}else if ((infp->tpinf_rdev == tp_consoledev) && (!tpconsdev)){
			/*tpdp->tpd_userflags |= (infp->tpinf_flags & TPINF_CONSOLE);*/
			tpdp->tpd_userflags |= TPINF_CONSOLE;
		}
		/*
		 * Copy in real device's stat information from tpinf to the
		 * TP device.
		 */ 
		tpdp->tpd_realdevfsdev = infp->tpinf_rdevfsdev;
		tpdp->tpd_realdevino = infp->tpinf_rdevino;
		tpdp->tpd_realdevmode = infp->tpinf_rdevmode;
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		break;

	case TP_DATACONNECT:
		infp->tpinf_rdev = tpcp->tpc_devp->tpd_realdev;
		if (error = tp_conndata(q, TPC_CTRL, infp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_DATADISCONNECT:
		if (!tpdp->tpd_datachp){
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		/*
		** To prevent a process from disconnecting a channel that
		** it does not own, the TP driver assigns a new
		** "connection id" to a channel every time it allocates a
		** channel.  This connection id must be passed to the
		** disconnect routine in the info block and will be checked
		** against the one on the channel to be disconnected.  If
		** the two don't match, the disconnect will fail.
		**
		** If the dconnid supplied by the user is 0, the check is
		** not applied (this allows an unconditional disconnect
		** since the control channel is a form of privileged access
		** anyway).
		*/
		if (infp->tpinf_dconnid &&
		   (infp->tpinf_dconnid != tpdp->tpd_datachp->tpc_connid)){
			error =EINVAL;
			reply = M_IOCNAK;
			break;
		}
		tp_discdata(tpdp);
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		break;

	case TP_DEFSAK:
		if (error = tp_verifysak(infp)){
			reply = M_IOCNAK;
			break;
		}
		tp_newmsgs(tpcp);
		if (!(tpcp->tpc_sakmsg && tpcp->tpc_hupmsg)){
			error = EAGAIN;
			reply = M_IOCNAK;
			break;
		}
		tp_setsak(tpcp,infp);
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		break;

	case TP_CONSCONNECT:
		if (error = tp_conncons(TPC_CTRL)){
			reply = M_IOCNAK;
			break;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSDISCONNECT:
		/*
		** Disconnect the read/input side of the cons channel from
		** the 'console' TP device
		*/
		if (error = tp_disccons()){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSSET:
		if (error = tp_consset(infp, &newtpdp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, newtpdp, infp, FILLINFO_ALL);
		}

		break;

	case TP_GETINF:
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		break;

	case I_LINK:
	case I_PLINK:
		if (tpdp->tpd_sak.sak_type == saktypeUNDEF){
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}
		if (tpdp->tpd_realrq){
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}

		/*
		** Allocated internal M_IOCTL message to be sent to the
		** real/physicall tty device.
		*/
		if (!(ioctlmp = tp_allocioctlmsg(TCGETS, sizeof(struct termios)))){
			reply = M_IOCNAK;
			error = EAGAIN;
			break;
		}

		linkp = (struct linkblk *)mp->b_cont->b_rptr;
		tpdp->tpd_muxid = linkp->l_index;
		/*
		** Save the kind of link so the control channel close
		** routine can handle closes correctly.
		*/
		if (cmd == I_PLINK){
			tpdp->tpd_flags |= TPD_PERSIST;
		}
		/*
		** Save the tpdp in the bottom mux q_ptr for flow control
		** from the bottom mux to the top mux.
		**
		** NOTE: For Future Enhancement.  If/When the STREAMS linkblk
		** struct is expanded to include the device number of the
		** lower Stream, put an additional check here to verify that
		** the device being linked is the device that indicated
		** in TP_CONNECT.
		*/
		tpdp->tpd_realrq = RD(linkp->l_qbot);
		tpdp->tpd_realrq->q_ptr = tpdp;
		WR(tpdp->tpd_realrq)->q_ptr = tpdp;

		/* If this is a 'console' device being linked (indicated by
		** the TPINF_CONSOLE flag, set it up as the console.
		*/
		if (tpdp->tpd_userflags & TPINF_CONSOLE){
			tp_resetcons(tpdp);
		}

		/*
		** enqueue internal TCGETS M_IOCTL messsage on lower ctrl
		** channel's (aka real/physical tty device's) write Queue
		** and enable the Queue.
		*/
		enableok(WR(tpdp->tpd_realrq));
		putq(WR(tpdp->tpd_realrq), ioctlmp);

		break;

	case I_UNLINK:
	case I_PUNLINK:
		if (!tpdp->tpd_realrq){
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}
		/*
		** TPINF_FAILUNLINK_IFDATA is only valid for I_PUNLINK.
		** (See tp.h for details)
		*/
		if (tpdp->tpd_datachp && (cmd == I_PUNLINK) &&
		   (tpdp->tpd_userflags & TPINF_FAILUNLINK_IFDATA)){
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		}
		/*
		** If this is the console device and the ioctl is I_PUNLINK
		** do not allow the unlink to occur.
		** NOTE:  We must allow the unlink to occur if the ioctl
		** is I_UNLINK, since the ioctl could have been issued from
		** a close.  In that case the mux is dismantled as far as
		** the Stream Head is concerned, regardless of whether or
		** not this function returns indication that the unlink
		** did not occur.
		*/
		if (tpdp == tpconsdev){
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		} 
		linkp = (struct linkblk *)mp->b_cont->b_rptr;
		/*
		** If the mux_id does not match the mux_id of the channel,
		** disallow the unlink request (since this is not the channel
		** to be unlinked).  OK to fail I_UNLINKs here becuase we
		** have to assume if I_UNLINK was sent as a result of the ctrl
		** channel closing, the Stream Head calling functions are
		** sending the correct multiplexor id (l_index).
		*/
		if (linkp->l_index != tpdp->tpd_muxid){
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}
		/*
		** From the TP driver's point of view, mark the TP device
		** unlinked.  When this returns, the Stream Head function
		** will have to complete the unlink by re-assigning all
		** the the real/physical device Top Queue's (which is now
		** the TP devices lower ctrl channel Top Queue) fields to
		** Stream Head related information.  But between now ("now"
		** is after the code is executed between the next set of
		** splstr() and splx()) and when the Stream Head re-assigns
		** the Queue, tplrput() can still receive messages from
		** downstream.  If tplrput() receives any messages, it will
		** act as if if did not receive any.
		*/
		s = splstr();
		tpdp->tpd_realrq->q_ptr = (caddr_t)0;
		WR(tpdp->tpd_realrq)->q_ptr = (caddr_t)0;
		tpdp->tpd_realrq = (queue_t *)0;
		tpdp->tpd_flags &= ~TPD_PERSIST;
		splx(s);
		if (tpdp->tpd_tcsetp){
			freemsg(tpdp->tpd_tcsetp);
			tpdp->tpd_tcsetp = (mblk_t *)0;
		}

		/* If this is the console being unlinked free or redirect
		** console information, and disconnect read/input side of
		** cons channel from the TP device (tpconsdev).
		*/
		if (tpdp == tpconsdev){
			tp_resetcons((struct tpdev *)NULL);
		}
		
		break;
	/*
	** When a TCSET* ioctl comes down the control channel and the
	** TPD_DEFSAK flag is set, the TCSET message is held until the new
	** SAK is set.  This prevents disruption of the current data channel.
	** See tp_setsak() for a description of the TPD_DEFSAK flag and
	** tp_discdata() for a description of what happens when a data
	** channel disconnects.
	*/
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
		if (tpdp->tpd_realrq && (tpdp->tpd_flags & TPD_DEFSAK)){
			if (tpdp->tpd_tcsetp){
				freemsg(tpdp->tpd_tcsetp);
			}
			tpdp->tpd_tcsetp = copymsg(mp);
			if (!tpdp->tpd_tcsetp){
				reply = M_IOCNAK;
				error = EAGAIN;
			}else{
				/*
				** Set the credentials to sys_cred, since
				** this ioctl has now become an internal
				** ioctl.  When execution is completed on the
				** the ioctl (occurs after a data channel is
				** disconnected), it is executed on behalf of
				** the kernel.
				*/
				((struct iocblk *)(tpdp->tpd_tcsetp->b_rptr))->ioc_cr = sys_cred;
			}
			break;
		}
	default:	/*Falls through*/
		if (error = tp_stdioctl(cmd,mp,tpcp)){
			reply = M_IOCNAK;
			break;
		}
		if (error = tp_sendioctl(tpdp,tpcp,q,mp)){
			reply = M_IOCNAK;
			break;
		}
		return;
	} /*end switch*/
	iocp->ioc_error = error;
	mp->b_datap->db_type = reply;
	qreply(q,mp);
}

/* tp_lctrlioctl: process ioctls enqueued on the lower ctrl channel's write
** Queue.
**
** This function handles TCSET* type ioctls and the TCGETS ioctl.
** All other types of ioctls are thrown away.
*/

STATIC void
tp_lctrlioctl(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	register struct tpdev	*tpdp;
	register struct iocblk	*iocp;
	int			cmd;

	tpdp = (struct tpdev *)q->q_ptr;
	if (!tpdp){
		/*
		** This should not happen!
		*/
		cmn_err(CE_WARN,
			"Queue without TP device pointer in tp_lctrlioctl()\n");
		return;
	}

	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	switch (cmd){
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
	case TCGETS:
		if (tp_sendioctl(tpdp, (struct tpchan *)0, q, mp)){
			freemsg(mp);
		}
		break;
	default:
		cmn_err(CE_WARN,
		 "tp_lctrlioctl(): ioctl not a TCSET* type or TCGETS; freeing message\n");
		freemsg(mp);
		break;
	}
}


/*
** tp_admioctl: Process ioctls on the administrative channel.
*/
STATIC	void
tp_admioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	register struct tpchan	*tpcp;
	register struct tpdev	*tpdp;
	register struct iocblk	*iocp;
	register struct tp_info	*infp;
	struct tpdev		*newtpdp;
	int			cmd;
	unsigned char		reply = M_IOCACK;
	int			error = 0;

	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		** This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_admioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}

	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	/*
	** Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_DATACONNECT:
	case TP_CONSCONNECT:
	case TP_CONSDISCONNECT:
	case TP_CONSSET:
	case TP_GETINF:
		if ( iocp->ioc_count < TPINFOSZ){
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_error = EINVAL;
		qreply(q,mp);
		return;
	}

	switch (cmd){
	case TP_DATACONNECT:
		if (error = tp_conndata(q, TPC_ADM, infp)){
			reply = M_IOCNAK;
		}else{
			tpdp = tp_finddev(infp->tpinf_rdev, 0);
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSCONNECT:
		if (error = tp_conncons(TPC_ADM)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpconsdev, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSDISCONNECT:
		/*
		** Disconnect the read/input side of the cons channel from
		** the 'console' TP device
		*/
		if (error = tp_disccons()){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpconsdev, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSSET:
		if (error = tp_consset(infp, &newtpdp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, newtpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_GETINF:
		tpdp = tp_finddev(infp->tpinf_rdev, 0);
		if (!tpdp){
			reply = M_IOCNAK;
			error = ENXIO;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;
	default:
		break;
	}

	mp->b_datap->db_type = reply;
	iocp->ioc_error = error;
	qreply(q, mp);
}

/*
** Connect a data channel to a real device based on information found in
** the tp_info block pointed to by infp.  This routine returns 0 on
** success and an errno value on failure.
*/
STATIC	int
tp_conndata(q, type, infp)
queue_t		*q;
int		type;
struct tp_info	*infp;
{
register struct	tpchan	*tpcp;
register struct	tpchan	*tpctrlp;
register struct	tpdev	*tpdp;
register	int	s;

	/*
	** Make sure the specified external major device number is the
	** same as the TP external major device number.
	*/
	if (getemajor(infp->tpinf_ddev) != tpemaj){
		return (EINVAL);
	}
	tpcp = tp_findchan(geteminor(infp->tpinf_ddev));
	if (!tpcp){
		return (ENXIO);
	}
	/*
	** Make sure the data channel is not already connected and that
	** the data channel is not dirty (was not previously connected).
	*/
	if (tpcp->tpc_devp || (tpcp->tpc_flags & TPC_DIRTY)){
		return (EBUSY);
	}
	tpdp = tp_finddev(infp->tpinf_rdev, 0);
	if (!tpdp){
		return (ENXIO);
	}
	if ((type == TPC_ADM) && (tpdp->tpd_flags & TPD_SAKSET)){
		return (EIO);
	}
	if (tpdp->tpd_flags & TPD_WAITTCSET){
		return (EBUSY);
	}
	/*
	** Check for both.  May be in the middle of disconnecting data channel.
	*/
	if (tpdp->tpd_datachp || tpdp->tpd_datarq){
		return (EBUSY);
	}
	/*
	** Allocate space for a M_IOCTL message, TCSETS ioctl,
	** which is to be sent to the real/physical device when all
	** channels connected for receiving input messages are
	** disconnected.
	*/
	if (!tpdp->tpd_discioctl){
		if (!(tpdp->tpd_discioctl =
		 tp_allocioctlmsg(TCSETS, sizeof(struct termios)))){
			return (EAGAIN);
		}
	}
	/*
	** Make sure all appropriate message buffers are allocated.
	*/
	tpctrlp = tpdp->tpd_ctrlchp;
	if (tpctrlp){
		tp_newmsgs(tpctrlp);
		if (!(tpdp->tpd_ctrlchp->tpc_sakmsg &&
		     tpdp->tpd_ctrlchp->tpc_hupmsg)){
			return (EAGAIN);
		}
	}
	tp_newmsgs(tpcp);
	if (!tpcp->tpc_hupmsg){
		return (EAGAIN);
	}
	if (!tpcp->tpc_rq){
		/*
		** This should never happen, but make sure. This prevents
		** a panic when the SAK is NONE and data goes up the data
		** channel.
		*/
		cmn_err(CE_WARN,"Data channel has no queue.\n");
		return (EIO);
	}
	/*
	** All is well, connect the data channel and clear the TPD_SAKSET
	** flag so SAK can be detected again.  If the SAK type is DATA,
	** also set the the SAK type to NONE, to prevent a SAK from being
	** generated by the next data packet.
	** If there is no other channel receiving input messages (currently
	** only the cons channel could be a possiblity) mark the data channel
	** as the channel to receive input messages. Don't allow interrupts
	** from stream data while connection is being made and the SAK type
	** is being changed.
	*/
	s = splstr();
		tpdp->tpd_datarq = tpcp->tpc_rq;
		tpdp->tpd_datachp = tpcp;
		if (tpdp->tpd_sak.sak_type == saktypeDATA){
			tpdp->tpd_sak.sak_type = saktypeNONE;
		}
		tpdp->tpd_userflags |= infp->tpinf_flags & DATACONNECTFLAGS;
		tpdp->tpd_flags &= ~TPD_SAKSET;
		tpcp->tpc_devp = tpdp;
		if (!tpdp->tpd_inputchp){
			tpdp->tpd_inputrq = tpcp->tpc_rq;
			tpdp->tpd_inputchp = tpcp;
		}
	splx(s);
	/* If there are any messages queued on the write queue, enable the
	** the queue
	*/
	if (WR(tpcp->tpc_rq)->q_first){
		enableok(WR(tpcp->tpc_rq));
		qenable(WR(tpcp->tpc_rq));
	}
	return (0);
}


/* tp_dataioctl: process ioctls sent via the data channel
**
** This routine filters ioctls sent down the data channel. I_LINK/I_PLINK and
** I_UNLINK/I_PUNLINK ioctls are not allowed on the data channel. 
** TCSET* type operations are checked to make sure they do not mask or
** otherwise interfere with the SAK processing.  If a problem is detected,
** an error is sent up the stream.
*/
STATIC void
tp_dataioctl(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	register struct tpchan	*tpcp;
	register struct tpdev	*tpdp;
	register struct iocblk	*iocp;
	struct   tp_info	*infp;
	unsigned char		reply = M_IOCACK;
	int			error = 0;
	int			cmd;


	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		** This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_dataioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}


	/*
	** If the data channel is not connected or it is write blocked, no
	** ioctls are allowed.  Issue an EIO and get out right away. 
	*/
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	if (!(tpdp = tpcp->tpc_devp) || (tpcp->tpc_flags&TPC_BLOCKED)){
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_error = EIO;
		qreply(q,mp);
		return;
	}

	/*
	** Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_GETINF:
		if ( iocp->ioc_count < TPINFOSZ){
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		break;
	}

	switch (cmd){
	case I_LINK:
	case I_PLINK:
	case I_UNLINK:
	case I_PUNLINK:
		reply = M_IOCNAK;
		error = EINVAL;
		break;
	case TP_GETINF:
		/*
		** Must have privilege to get all tpinf information.
		*/
		if (drv_priv(iocp->ioc_cr)){ /* returns 0 on success */
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_RESTRICTED);
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;
	default:
		if (!tpdp->tpd_realrq ){
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		/*
		** NOTE: For Future Enhancement: May eventuall want to enqueue
		** ioctl onto data channel if it is not currently receiving
		** input messages and send the ioctl down once this channel
		** became the channel receiving input messages. (The case
		** that the data channel would not be the channel receiving
		** input will occur very rarely and only with the 'console'
		** TP device.
		*/

		if (error = tp_stdioctl(cmd,mp,tpcp)){
			reply = M_IOCNAK;
			break;
		}
		if (error = tp_sendioctl(tpdp,tpcp,q,mp)){
			reply = M_IOCNAK;
			break;
		}
		/*
		** Everything was successful. We return here instead of
		** replying since the reply will come back up stream from
		** the driver below us.
		*/
		return;
	}
	mp->b_datap->db_type = reply;
	iocp->ioc_error = error;
	qreply(q,mp);
}

/* tp_conncons: connect cons channel to read/input side of the TP device
** labeled as the 'console' device (indicated by tpconsdev).
*/
STATIC int
tp_conncons(type)
int	type;
{
	struct tpchan	*tpcp;
	int		s;

	/* Verify that a 'console' TP device exists. */
	if (!tpconsdev){
		return (ENXIO);
	}

	/* Verify that the cons chan is open. */
	tpcp = tp_findchan(TP_CONSDEV);
	if (!tpcp){
		return (ENXIO);
	}

	/*
	** Verify that the cons chan is not already connected to the
	** read/input side of 'console' TP device.
	*/
	s = splstr();
	if (tpconsdev->tpd_inputchp == tpcp){
		splx(s);
		return (EBUSY);
	}

	if (((type == TPC_ADM) || (type == TPC_CONS)) &&
	 (tpconsdev->tpd_flags & TPD_SAKSET)){
		return (EIO);
	}
	if (tpconsdev->tpd_flags & TPD_WAITTCSET){
		return (EBUSY);
	}
	/*
	** If the SAK type is DATA, set the the SAK type to NONE,
	** to prevent a SAK from being generated by the next data packet.
	** Clear indication that a SAK was entered.
	*/
	if (tpconsdev->tpd_sak.sak_type == saktypeDATA){
		tpconsdev->tpd_sak.sak_type = saktypeNONE;
	}
	tpconsdev->tpd_flags &= ~TPD_SAKSET;

	tpconsdev->tpd_inputchp = tpcp;
	tpconsdev->tpd_inputrq = tpcp->tpc_rq;
	splx(s);

	/*
	** If there are any messages queued on the cons channel write Queue,
	** enable the Queue.
	*/
	if (WR(tpcp->tpc_rq)->q_first){
		enableok(WR(tpcp->tpc_rq));
		qenable(WR(tpcp->tpc_rq));
	}

	return (0);
}

/* tp_consioctl: process ioctls sent via the cons channel.
**	process restricted set of TP ioctls.
**	I_LINK/I_PLINK and I_UNLINK/I_PUNLINK ioctls are not allowed.
**	process all other ioctls if cons channel is connected to the
**	'console' TP device for at least output and 'console' TP device
**	has a real/physical device linked underneath.
**
** Must have privilege to do TP_CONSCONNECT, TP_CONSDISCONNECT, and
** TP_CONSSET ioctls.
**
** TCSET* type operations are checked to make sure they do not mask or
** otherwise interfere with the SAK processing.  If a problem is detected,
** an error is sent up the stream.
*/
STATIC void
tp_consioctl(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	register struct tpchan	*tpcp;
	register struct tpdev	*tpdp;
	register struct iocblk	*iocp;
	struct   tp_info	*infp;
	struct	 tpdev		*newtpdp;
	unsigned char		reply = M_IOCACK;
	int			error = 0;
	int			cmd;


	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		** This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_consioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}
	tpdp = tpcp->tpc_devp;
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	/*
	** Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_CONSCONNECT:
	case TP_CONSDISCONNECT:
	case TP_CONSSET:
		if (error = drv_priv(iocp->ioc_cr)){ /* returns 0 on success */
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = error;
			qreply(q,mp);
			return;
		}
	case TP_GETINF:	/*Falls through*/
		if ( iocp->ioc_count < TPINFOSZ){
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		break;
	}

	switch (cmd){
	case I_LINK:
	case I_PLINK:
	case I_UNLINK:
	case I_PUNLINK:
		reply = M_IOCNAK;
		error = EINVAL;
		break;
	case TP_GETINF:
		/*
		** Must have privilege to get all tpinf information.
		*/
		if (drv_priv(iocp->ioc_cr)){ /* returns 0 on success */
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_RESTRICTED);
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSCONNECT:
		if (error = tp_conncons(TPC_CONS)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSDISCONNECT:
		/*
		** Disconnect the read/input side of the cons channel from
		** the 'console' TP device
		*/
		if (error = tp_disccons()){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		break;

	case TP_CONSSET:
		if (error = tp_consset(infp, &newtpdp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, newtpdp, infp, FILLINFO_ALL);
		}

		break;

	default:

		/*
		** If the cons channel is not connected or the 'console' device
		** does not have a real/physical device linked underneath,
		** all other ioctls are not allowed.
		*/
		if (!tpdp){
			reply = M_IOCNAK;
			error = EIO;
			break;
		}
		if (!tpdp->tpd_realrq ){
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}

		if (error = tp_stdioctl(cmd,mp,tpcp)){
			reply = M_IOCNAK;
			break;
		}
		if (error = tp_sendioctl(tpdp,tpcp,q,mp)){
			reply = M_IOCNAK;
			break;
		}
		/*
		** Everything was successful. We return here instead of
		** replying since the reply will come back up stream from
		** the driver below us.
		*/
		return;
	}
	mp->b_datap->db_type = reply;
	iocp->ioc_error = error;
	qreply(q,mp);
}

/*
** tplrput: lower read put procedure
**
** This routine processes all M_DATA, M_BREAK, M_HANGUP, M_SIG and
** M_PCSIG messages to detect SAK conditions.  If no SAK is detected the
** message is sent up the channel indicated by tpd_inputchp;
** It also makes sure M_IOCNACK and M_IOCACK messages are directed to the
** correct channel and handles M_FLUSH messages.
**
** When an M_IOCACK message is received and the ioctl was a TCSET* the 
** TPD_WAITTCSET flag is cleared.
*/
STATIC int
tplrput(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct tpdev	*tpdp;
	register struct tpchan	*datap;
	register struct tpchan	*inputp;
	register struct tpchan	*ctrlp;
	register struct tpchan	*chanp;
	register char		*cp,*endp;
	register char		sakc;
	register uint		sakfound = 0;



	tpdp = (struct tpdev *)q->q_ptr;

	/*
	** If the message is an M_FLUSH process it regardless of whether
	** or not the device is linked and return.
	** Handling this M_FLUSH may have come from stream head as a result of
	** sending a putctl1() for a M_ERROR message from tp_discdata().
	** tp_discdata() does not clear tpd_inputrq until after the putctl1()
	** completes which is after this M_FLUSH has completed (assuming
	** M_FLUSH message are not enqueued on any Queues).
	** NOTE: checks for tpd_inputrq rather then tpd_inputchp since the
	** channel may be in the middle of disconnecting.  (See tp_discdata()).
	*/
	if (mp->b_datap->db_type == M_FLUSH){
		if (*mp->b_rptr & FLUSHRW){
			if (*mp->b_rptr & FLUSHR){
				if (*mp->b_rptr & FLUSHBAND){
					flushband(q, *(++mp->b_rptr),
					 FLUSHDATA);
					--mp->b_rptr;
				}else{
					flushq(q, FLUSHDATA);
				}
			}
			/*
			** If there is a channel connected for input, send
			** M_FLUSH message up that channel.  Otherwise send
			** the M_FLUSH message back down stream if the M_FLUSH
			** message indicated to flush the write Queue.
			*/
			if (tpdp && tpdp->tpd_inputrq){
				putnext(tpdp->tpd_inputrq, mp);
			}else if (*mp->b_rptr & FLUSHW){
				if (*mp->b_rptr & FLUSHBAND){
					flushband(WR(q), *(++mp->b_rptr),
					 FLUSHDATA);
					--mp->b_rptr;
				}else{
					flushq(WR(q), FLUSHDATA);
				}
				*mp->b_rptr &= ~FLUSHR;
				qreply(q, mp);
			}else{
				freemsg(mp);
			}
		}else{
			freemsg(mp);
		}
		return (0);		
	}

	if (!tpdp){
		/*
		** Most likely in the middle of an I_UNLINK and the 
		** TP driver may have marked us as unlinked but the
		** Stream Head has not re-assigned the Queue's fields
		** (ie. q_qinfo etc.).  Just free message and return.
		*/
		freemsg(mp);
		mp = (mblk_t *)0;
		return (0);
	}
	datap = tpdp->tpd_datachp;
	inputp = tpdp->tpd_inputchp;
	ctrlp = tpdp->tpd_ctrlchp;
	/*
	** If the message was not an M_FLUSH the device must be linked,
	** otherwise the message is ignored.
	*/
	if (!tpdp->tpd_realrq){
		cmn_err(CE_WARN,
		 "tplrput:Message received TP device indicated no read queue.\n");
	}
	switch (mp->b_datap->db_type){
	case M_DATA:
		/*
		** Handle M_DATA messages as fast as possible for
		** saktypeNONE.  There is no need to test for sakfound or
		** TPC_BLOCKED at end of switch since neither will be set
		** while sak type is NONE
		**
		** HISTORICAL NOTE: (Prior to the delay sak type change
		** functionality; When a non-zero delay time value is defined
		** can no longer assume that there is a data channel connected
		** when the SAK type is NONE)
		** If the SAK type is NONE a data or the cons channel must be
		** connected (and the data or cons queue in place), so there
		** is no need to verify that these pointers are okay.
		*/
		if (tpdp->tpd_sak.sak_type == saktypeNONE){
			if (inputp){
				if (!q->q_first && canput(tpdp->tpd_inputrq->q_next)){
					putnext(tpdp->tpd_inputrq, mp);
				}else if (mp->b_datap->db_type >= QPCTL){
					putnext(tpdp->tpd_inputrq, mp);
				}else{
					putq(q, mp);
				}
			}else{
				/*
				** A data or the cons channel is not connected
				** so "drop the message on the floor".
				*/
				freemsg(mp);
				mp = (mblk_t *)0;
			}
			return (0);
		}
		/*
		** If the SAK type is DATA and there is a ctrl channel
		** connected, then issue a 'fake' SAK to the ctrl channel
		** and set the SAKSET flag to prevent adm channel connections.
		** A data channel or the cons channel is not connected
		** to receive input so the code to handle re-directing input
		** messages upstream is skipped
		*/
		if (tpdp->tpd_sak.sak_type == saktypeDATA){
			if (tpdp->tpd_ctrlchp){
				sakfound = 1;
			}
		}else if (tpdp->tpd_sak.sak_type == saktypeCHAR){
			sakc = tpdp->tpd_sak.sak_char;
			cp = (char *)mp->b_rptr;
			endp = (char *)mp->b_wptr;
			while (cp < endp){
				/*
				** Mask out the high order (8th) bit to prevent
				** potential spoofing from a terminal set for 7
				** bits or sending parity
				*/
				if (((*cp) & ASCII_7BITMASK) == sakc){
					sakfound = 1;
					break;
				}
				cp++;
			}
		}
		break;

	/*
	** M_SIG, M_PCSIG cases and test for SIGHUP put here in case a
	** module or driver below converts a M_HANGUP to M_SIG or M_PCSIG
	*/
	case M_SIG:
	case M_PCSIG:
		if (*mp->b_rptr != SIGHUP){
			break;
		}

	/* FALLTHRU */
	case M_HANGUP:	/* If the signal was SIGHUP fall through*/
		if (((tpdp->tpd_sak.sak_type == saktypeLINECOND) &&
		    (tpdp->tpd_sak.sak_linecond == saklinecondLINEDROP)) ||
		    (tpdp->tpd_sak.sak_secondary == saksecYES)){
			sakfound = 1;
			break;
		}
		/* 
		** This is not a SAK, but just a hangup condition.  In this
		** case, send the M_HANGUP up the data channel. If a ctrl
		** channel is connected, send the hangup notification up the
		** ctrl channel as well.
		** NOTE: The hangup message is specifically sent up the data
		** channel and not the channel labeled input (although the
		** data and input are usually the same channel) since this
		** could be the cons channel and we do not want to put the
		** cons channel in the STRHUP state.
		*/
		if (datap){
			putnext(tpdp->tpd_datarq, mp);
			if (tpdp->tpd_flags & TPINF_DISCONNDATA_ONHANGUP){
				tp_discdata(tpdp);
			}
		}else{
			freemsg(mp);
		}
		if (ctrlp){ 
			tp_sendmsg(ctrlp, tpdp->tpd_ctrlrq, TP_M_HANGUP);
		}
		mp = (mblk_t *)0;
		break;

	case M_BREAK:
		if ((tpdp->tpd_sak.sak_type == saktypeLINECOND) &&
		    (tpdp->tpd_sak.sak_linecond == saklinecondBREAK))
			sakfound = 1;
		break;

	case M_IOCACK:
		/*
		** If ioctl command is a TCSET* type copy tpd_nextterm to
		** tpd_curterm.
		**	-NOTE: termios values are actually not reflected
		**	 in tp device until the M_IOCACK is passed back up
		**	 from the physcial device linked underneath.
		** The TPD_WAITTCSET flag is cleared so data channels can
		** subsequently be connected.
		**
		** If ioctl command is a TCGETS and was internally generated
		** (i.e. tpd_ioctlchan == NULL) (refer to tp_ctrlioctl()
		** case I_LINK, I_PLINK) initialize tpd_curterm with the
		** termios values retrieved from the real/physicall tty device
		** linked below.
		*/

		switch (((struct iocblk *)(mp->b_rptr))->ioc_cmd){

		case TCSETS:
		case TCSETSW:
		case TCSETSF:
		case TCSETA:
		case TCSETAW:
		case TCSETAF:

			bcopy((caddr_t)&(tpdp->tpd_nextterm),
			      (caddr_t)&(tpdp->tpd_curterm),
			      sizeof(struct termios));
			if (tpdp->tpd_flags & TPD_WAITTCSET){
				tpdp->tpd_flags &= ~(TPD_WAITTCSET|TPD_SAKSET);
			}
			if (!tpdp->tpd_ioctlchan){
				freemsg(mp);
				mp = (mblk_t *)0;
			}
			break;

		case TCGETS:

			if (!tpdp->tpd_ioctlchan){
				if (mp->b_cont){
					bcopy((struct termios *)mp->b_cont->b_rptr,
					 &tpdp->tpd_curterm,
					 sizeof(struct termios));
				}
				freemsg(mp);
				mp = (mblk_t *)0;
			}
			break;
		default:
			break;
		}
	case M_IOCNAK:	/* fall through */
	case M_COPYIN:
	case M_COPYOUT:
		/*
		** If the channel for the ioctl response is connected and the
		** saved ioctl id on the TP device matches the ioctl id of the
		** M_IOCACK or M_IOCNAK message send the message up appropiate
		** channel.  If some other channel is waiting for the current
		** ioctl to complete, enable the write queue of that channel.
		** This is done be enabling (scheduling) the lower ctrl
		** channel's Write Queue service function, tplwsrv().
		** There can be more than one channel including the lower ctrl
		** channel waiting for the current ioctl to complete. tplwsrv()
		** enables (schedules) the Write Queue service function of
		** any upper Stream channel that has messages enqueued on its
		** write Queues or has an ioctl (saved in the channel structure)
		** to send downstream.  tplwsrv() also processes messages on
		** the lower Stream channel if there are any.
		** NOTE: Internally generated ioctls are enqueued on the
		** lower ctrl channel's write Queue (aka the real write Queue).
		**
		** NOTE: For Future Enhancement: The TP driver's ioctl
		** serialization relies on M_IOCTLs returning M_IOCACK/M_IOCNAK
		** messages.  After processing the M_IOCACK/M_IOCNAK,  the
		** lower ctrl channel Write Queue service function, tplwsrv()
		** would be scheduled if there were any ioctls waiting to be
		** sent downstream.  If tty type device drivers, which get
		** linked underneath, ever flushed M_IOCTL messages (this would
		** not be an expected functionality), the TP driver's ioctl
		** handling functionality would be impared.  The result is that
		** any other ioctl waiting to be sent downstream would not have
		** their respective channel Write Queue service function
		** scheduled, unless the Write Queue is enabled by some other
		** means (eg. backenabled by the lower Stream {the one
		** associated with the real/physical device} if its Write Queue
		** were previously full and subsequently the Queue message
		** count dropped below the lo-water mark.)
		** The impairment could be reduced, if checks for saved
		** (pending) ioctls on the channel were put in the Write Queue's
		** put function, but this would slow down write processing.
		*/

		/*
		** If the ioctlchan is NULL, (the ioctl came from inside the
		** driver), the channel is no longer connected to the TP
		** device, or the ioctl id of the message (M_IOCACK, M_IOCNAK,
		** M_COPYIN, or M_COPYOUT) does not match the saved ioctl id in
		** the TP device, the message is freed.
		** NOTE:  The iocblk casting is used on the message even if
		** the message is a M_COPYIN or M_COPYOUT.  This is all right
		** to do since the definition of the first three fields
		** (including the ioctl id) of a iocblk and copyreq structure
		** correspond to each other.
		*/
		chanp = tpdp->tpd_ioctlchan;
		if (chanp && chanp->tpc_devp &&
		 (((struct iocblk *)(mp->b_rptr))->ioc_id == tpdp->tpd_ioctlid)){
			putnext(tpdp->tpd_ioctlchan->tpc_rq, mp);
		}else if (mp){
			freemsg(mp);
		}
		mp = (mblk_t *)0;
		tpdp->tpd_ioctlchan = (struct tpchan *)0;
		tpdp->tpd_ioctlid = 0;
		tpdp->tpd_flags &= ~(TPD_BUSY_IOCTL);
		if (tpdp->tpd_flags & TPD_WAIT_IOCTL){
			tpdp->tpd_flags &= ~(TPD_WAIT_IOCTL);
			enableok(WR(tpdp->tpd_realrq));
			qenable(WR(tpdp->tpd_realrq));
		}
		break;

	default:
		break;
	}

	/*
	** If the SAK is found, ignore all data in the message containing
	** the SAK.
	**
	** If no other SAK is pending (the SAKSET flag is not set on the TP
	** device) set the SAKSET flag.
	**
	**	If the data channel is connected send a HUP up the data
	**	channel and set the BLOCKED flag in the data channel.
	**
	**	If the ctrl channel is connected, send the SAK
	**	notification up the control channel. 
	**
	**	If the ctrl channel is not connected, disconnect the
	**	data channel (this will send an EIO up the data channel)
	**	and the channel receiving input messages.
	** 
	** If no SAK was found, and there is a data message to send, and the
	** read queue is not blocked either send or queue the message.
	** If the read queue is blocked, ignore the message.
	*/

	if (sakfound){
		if (mp){
			freemsg(mp);
			mp = (mblk_t *)0;
		}
		if (!(tpdp->tpd_flags & TPD_SAKSET)){
			tpdp->tpd_flags |= TPD_SAKSET;
			if (datap && datap->tpc_devp){
				tp_sendmsg(datap, tpdp->tpd_datarq, TP_M_HANGUP);
				datap->tpc_flags |= TPC_BLOCKED;
				tp_discinput(tpdp);
			}
			if (ctrlp){
				tp_sendmsg(ctrlp, tpdp->tpd_ctrlrq, TP_M_SAK);
			}else{
				tp_discinput(tpdp);
				tp_discdata(tpdp);
			}
		}
	}else if (mp){
		if (inputp && inputp->tpc_devp && !(inputp->tpc_flags & TPC_BLOCKED)){
			if (!q->q_first && canput(tpdp->tpd_inputrq->q_next)){
				putnext(tpdp->tpd_inputrq, mp);
			}else if (mp->b_datap->db_type >= QPCTL){
				putnext(tpdp->tpd_inputrq, mp);
			}else{
				putq(q, mp);
			}
		}else{
			freemsg(mp);
			mp = (mblk_t *)0;
		}
	}
	return (0);
}


/*
** Allocate new messages for a ctrl or data channels and send the one that
** corresponds to the specified message type up the q.  If the message is not
** currently allocated and cannot be allocated, don't send anything.  Having
** separate sak and hup messages ensures that there will always be a SAK
** message prepared to go up the stream (at least the first time a SAK is
** detected) even if a HANGUP message comes at a time when no memory is
** available.
*/
STATIC	void
tp_sendmsg(chan, q, msg)
struct	tpchan	*chan;
queue_t		*q;
int		msg;
{
	mblk_t	*mp;

	tp_newmsgs(chan);
	switch (chan->tpc_type){
	case TPC_CTRL:
		switch (msg){
		case TP_M_HANGUP:
			mp = chan->tpc_hupmsg;
			chan->tpc_hupmsg = (mblk_t *)0;
			break;
		case TP_M_SAK:
			mp = chan->tpc_sakmsg;
			chan->tpc_sakmsg = (mblk_t *)0;
			break;
		default:
			mp = (mblk_t *)0;
			break;
		}
		break;
	case TPC_DATA:
		switch (msg){
		case TP_M_HANGUP:
			mp = chan->tpc_hupmsg;
			chan->tpc_hupmsg = (mblk_t *)0;
			break;
		default:
			mp = (mblk_t *)0;
			break;
		}
		break;
	default:
		return;
	}
	if (mp){
		putnext(q, mp);
	}
}

/*
** Allocate new messages for a channel based on the type of the
** channel. Currently, all channels have the potential to have a SAK
** message and a HANGUP message preallocated.  This routine is
** responsible for preallocating those messages.
**
** NOTE: The initial allocation of SAK and HANGUP messages is not done
** by this routine because this is done in the tpopen() process and is
** allowed to sleep waiting for free memory.  This routine is only used
** by parts of the driver that cannot reasonably sleep.
*/
STATIC	void
tp_newmsgs(tpcp)
struct	tpchan	*tpcp;
{
	register mblk_t	*p;

	switch(tpcp->tpc_type){
	case TPC_CTRL:
		if (!tpcp->tpc_sakmsg){
			p=tpcp->tpc_sakmsg = allocb(sizeof(tpproto_t),BPRI_HI);
			if (p){
				p->b_datap->db_type = M_PCPROTO;
				((tpproto_t *)(p->b_rptr))->tpp_type = TP_M_SAK;
				p->b_wptr = p->b_rptr + sizeof(tpproto_t);
			}
		}
		if (!tpcp->tpc_hupmsg){
			p=tpcp->tpc_hupmsg = allocb(sizeof(tpproto_t),BPRI_HI);
			if (p){
				p->b_datap->db_type = M_PROTO;
				((tpproto_t *)(p->b_rptr))->tpp_type =
				 TP_M_HANGUP;
				p->b_wptr = p->b_rptr + sizeof(tpproto_t);
			}				
		}
		break;
	case TPC_DATA:
		tpcp->tpc_sakmsg = (mblk_t *)0;
		if (!tpcp->tpc_hupmsg){
			p = tpcp->tpc_hupmsg = allocb(0,BPRI_HI);
			if (p){
				p->b_datap->db_type = M_HANGUP;
			}				
		}
		break;
	default:
		/* Messages are not needed for either ADM or CONS channels*/
		break;
	}
}

/* tplrsrv: lower read service procedure
**
** When the upper read service routine enables the read queue the lower
** read service routine is called to dequeue and send the pending
** message upstream.
*/

STATIC int
tplrsrv(q)
register queue_t *q;
{
	register struct tpdev *tpdp;
	register struct tpchan *inputp;
	register mblk_t *mp;

	tpdp = (struct tpdev *)q->q_ptr;
	if (!tpdp){
		cmn_err(CE_WARN,
			"Enountered queue with no device in tplrsrv()\n");
		flushq(q, FLUSHALL);
		return (0);
	}
	inputp = tpdp->tpd_datachp;
	/*
	** If the channel receiving input messages is
	** a) not connected, b) not linked, or c) blocked,
	** just flush the queue.
	*/
	if (!inputp || (inputp->tpc_flags & TPC_BLOCKED)){
		flushq(q, FLUSHALL);
	}else{
		/*
		** Channel receiving input is okay, send any pending messages
		** up the ** channel until flow control says to stop.
		*/
		if (!tpdp->tpd_inputrq->q_next){
			cmn_err(CE_WARN,
				"Queue has no next pointer in tplrsrv()\n");
			flushq(q,FLUSHALL);
			return (0);
		}
		while (((mp = getq(q)) != (mblk_t *)0) &&
		 (canput(tpdp->tpd_inputrq->q_next))){
			putnext(tpdp->tpd_inputrq, mp);
		}
		if (mp){
			putbq(q, mp);
		}
	}
	return (0);
}

/* tpursrv: upper read service procedure
**
** When a module upstream from the multiplexer is ready for more data
** from the read queue it will back enable the queue which causes this
** routine to run.  This routine enables the lower read service routine
** which actually does the read.  This routine only operates on a data
** or cons channel.
*/

STATIC int
tpursrv(q)
register queue_t *q;
{
	register struct tpchan *tpcp;
	register struct tpdev *tpdp;

	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp)
		return (0);
	tpdp = tpcp->tpc_devp;
	if (tpdp && ((tpcp->tpc_type == TPC_DATA) ||
	 (tpcp->tpc_type == TPC_CONS)) && tpdp->tpd_realrq){
		qenable(tpdp->tpd_realrq);
	}
	return (0);
}

/*
** Make sure the specified SAK structure contains valid legal values. 
** If the SAK is a character, it must be 0x00 <= c < 0x0E  OR  0x0F < c <= 1F.
** If the SAK is a line condition it must be line drop or break. If all
** is well return 0 otherwise return EINVAL.
**
** NOTE: 0x0E and 0x0F are excluded because their 0x8E and 0x8F are "single
** shift" announcement characters for multi-byte character.  Since TP does
** not test the 8th bit, 0x8E and 0x8F would be interpreted incorrectly. 
*/

STATIC	int
tp_verifysak(infp)
struct tp_info	*infp;
{
	switch(infp->tpinf_sak.sak_type){
	case saktypeNONE:
		return (0);
	case saktypeCHAR:
		{
			ulong	sakchar;
			sakchar = infp->tpinf_sak.sak_char;
			if ((!(sakchar & ~0x1F)) && (sakchar != 0x0E) &&
			 (sakchar != 0x0F))
				break;
			return (EINVAL);
		}
	case saktypeLINECOND:
		if ((infp->tpinf_sak.sak_linecond == saklinecondLINEDROP) ||
		   (infp->tpinf_sak.sak_linecond == saklinecondBREAK))
			break;
		return (EINVAL);
	default:
		return (EINVAL);
	}
	switch(infp->tpinf_sak.sak_secondary){
	case saksecYES:
	case saksecNO:
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
** Handle the decision making associated with setting a SAK on a TP
** device. This routine is called when a TP_DEFSAK ioctl is recognized.
**
** If a TP_DEFSAK ioctl comes down the control channel and a channel is
** connected to receive input (typically the data channel), the new SAK is
** copied into a holding buffer in the TP device structure, the TPD_DEFSAK
** flag is set and any pending TCSET* ioctl is freed.  This way, the TCSET*
** must be sent down after the SAK is defined inorder for the tp device to be
** able to connect another data channel (after the data channel has
** disconnected) or the cons channel.
**
** See tp_discdata() and tp_disccons() for an explanation of what happens
** at a data and cons channel disconnect, and tp_ctrlioctl() for an
** explanation of what happens to TCSET* ioctls.
**
** NOTE: always call tp_verifysak() before calling tp_setsak()
*/
STATIC void
tp_setsak(tpcp, infp)
struct tpchan	*tpcp;
struct tp_info	*infp;
{
	register struct sak	*sakp;
	register struct	tpdev	*tpdp;

	sakp = &(infp->tpinf_sak);
	tpdp = tpcp->tpc_devp;
	/*
	** Since the data channel and the channel connected to receive input
	** will be disconnected before the SAK is actually put on the device,
	** translate SAKs of type NONE to type DATA so the initial SAK will
	** be correct.  This code should not be interrupt sensitive since a
	** SAK can't cause the data channel to disconnect when a ctrl channel
	** is connected.
	*/
	if (sakp->sak_type == saktypeNONE){
		sakp->sak_type = saktypeDATA;
	}

	if (tpcp->tpc_devp->tpd_inputchp){
	/*
	** If there is a data channel connected to the device,  hold the SAK. 
	*/
		tpdp->tpd_flags |= TPD_DEFSAK;
		if (tpdp->tpd_tcsetp){
			freemsg((mblk_t *)(tpdp->tpd_tcsetp));
			tpdp->tpd_tcsetp = (mblk_t *)0;
		}
		bcopy(sakp,&(tpdp->tpd_heldsak),sizeof(struct sak));
	}else{
		tp_putsak(tpdp,sakp);
	}
	/* IF SAK type is DATA, switch it back to NONE in the tp_info (infp)
	** structure, since infp is returned to user.
	*/
	if (sakp->sak_type == saktypeDATA){
		sakp->sak_type = saktypeNONE;
	}
}

/*
** NOTE: The delay time to change the SAK type from NONE to DATA, after the
** data or cons channel is disconnected, can be circumvented as follows:
** If the SAK for the TP device is defined again to saktypeNONE while a
** channel is connected for input it is translated into a saktypeDATA and
** held (made pending) until all channels to the TP device are disconnected
** for input.  When the channels have been disconnected tp_putsak() is called
** to load the new SAK definition.  If there was a delay time defined, the SAK
** would get set to saktypeDATA, before the delay time expired.  The code
** to prevent this is not implemented here since it is unlikely to occur
** and code needs to be surrounded by splhi()s which should be used as little
** as possible.
** If it becomes an issue and needs to be implemented, the following is a
** code fragment to be included: 
**
**	-Before the bcopy
**	 int s;
**	 if (sakp->sak_type == saktypeDATA){
**		-prevent the timeout function from being called (via clock
**		 interrput) while checking tpd_timeoutid.
**		s = splhi();
**		if (tpdp->tpd_timeoutid){
**			-will get changed to saktypeDATA when the timeout
**			 function tp_saktypeswitch() is called
**			sakp->sak_type = saktypeNONE;
**		}
**
**		-Where checking whether or not to set TPD_WAITTCSET need to
**		 include case for saktypeNONE since that is now a valid sak
**		 type in this function
**		if ((tpdp->tpd_sak.sak_type != saktypeDATA) &&
**		 (tpdp->tpd_sak.sak_type != saktypeNONE){
*/
STATIC	void
tp_putsak(tpdp, sakp)
struct	tpdev	*tpdp;
struct	sak	*sakp;
{
	register queue_t	*q;
	register struct termios	*maskp,*validp;

	bcopy(sakp,&(tpdp->tpd_sak),sizeof(struct sak));
	tpdp->tpd_flags &= ~TPD_DEFSAK;
	/*
	** Set termio(s) protection mask.  The choice of the mask to use
	** depends on the type of sak and possibly the value of the sak.
	*/
	switch(sakp->sak_type){
	case saktypeLINECOND:
		switch(sakp->sak_linecond){
		case saklinecondLINEDROP:
			maskp =  &tp_dropmasktermios;
			validp = &tp_dropvalidtermios;
			break;
		case saklinecondBREAK:
			maskp =  &tp_breakmasktermios;
			validp = &tp_breakvalidtermios;
			break;
		default:
			return;
		}

		break;

	case saktypeCHAR:
		maskp = &tp_charmasktermios;
		validp = &tp_charvalidtermios;
		break;

	default:
		bzero(tpdp->tpd_mask, sizeof(struct termios));
		bzero(tpdp->tpd_valid, sizeof(struct termios));
		return;
	}
	bcopy(maskp, &tpdp->tpd_mask, sizeof(struct termios));
	bcopy(validp, &tpdp->tpd_valid, sizeof(struct termios));


	/* 
	** If saktype is saktypeNONE (initially == saktypeDATA before data
	** channel is connected) do not set TPD_WAITTCSET.  This will allow
	** the DATA channel to be connected whether or not a TCSET* type
	** of ioctl has been sent down.  There is no need to verify whether
	** or not a TCSET* type ioctl can comprise SAK detection, since the
	** SAK is defined to be NONE.
	*/
	if (tpdp->tpd_sak.sak_type != saktypeDATA){
		tpdp->tpd_flags |= TPD_WAITTCSET;
	}
	if (tpdp->tpd_tcsetp){
		q = WR(tpdp->tpd_realrq);
		/*
		** This is a queued ioctl that was sent down the ctrl channel
		** (after a TP_DEFSAK) and already "ACKed" (eventhough it was
		** not sent downstream).  tp_sendioctl()'s second arguement
		** (the ioctl chan) is set to NULL so that the ACK or NAK
		** that comes back upstream is not sent up the ctrl channel
		** since this ioctl was already "ACKed".
		*/
		if (tp_sendioctl(tpdp,(struct tpchan *)0,q,tpdp->tpd_tcsetp)){
			freemsg(tpdp->tpd_tcsetp);
		}
		tpdp->tpd_tcsetp = (mblk_t *)0;
	}
}

/*
** Send an ioctl request downstream.  If an ioctl is currently being
** processed downstream save this ioctl on the channel's data structure if
** a channel arguement, tpcp, has been defined.  If tpcp is NULL, (indicates
** an internally generated ioctl),  queue the ioctl at the beginning of
** lower ctrl write Queue and disable that Queue for scheduling.
** The write Queues for the channels that have the ioctl message saved or
** the lower ctrl write Queue will be scheduled to run when the acknowledgement
** for ioctl downstream is received.
** If this is ioctl originated from the same channel as the ioctl that is
** currently being processed downstream, the ioctl currently being processed
** downstream either timed out or was interrupted at the Stream Head.  The
** tpd_ioctlchan and tpd_ioctl fields are cleared out so that when the
** acknowledgement for the current ioctl is returned to the TP device,
** it is just freed.  NOTE: If the acknolwedgement was sent upstream, it
** would have just been freed at the Stream Head.
*/
STATIC int
tp_sendioctl(tpdp,tpcp,q,mp)
struct	tpdev	*tpdp;
struct	tpchan	*tpcp;
queue_t		*q;
mblk_t		*mp;
{
	register queue_t	*realwq;
	int			s;

	if (!tpdp->tpd_realrq || !(realwq = WR(tpdp->tpd_realrq))){
		return (EIO);
	}
	if (!realwq->q_next){
		/*
		** This should never happen!
		*/
		cmn_err(CE_WARN,
			"TP Queue has no next pointer in tp_sendioctl()\n");
		return (EIO);
	}
	s = splstr();
	/*
	** If there currently is an ioctl saved on this channel, free it.
	** This indicates that the ioctl saved on this channel had either
	** interrupted or timed out at the Stream Head.  We free the
	** message here since it would just be freed at the Stream Head
	** if we were to send a negative ackowledgement upstream.
	*/
	if (tpcp && tpcp->tpc_ioctlmp){
		freemsg(tpcp->tpc_ioctlmp);
		tpcp->tpc_ioctlmp = (mblk_t *)0;
	}
	if (tpdp->tpd_flags & TPD_BUSY_IOCTL){
		/*
		** If ioctl originated from same channel as the ioctl currently
		** being processed downstream, clear tpd_ioctlchan and
		** tpd_ioctlid fields before saving ioctl on the channel.
		*/
		if (tpcp && (tpcp == tpdp->tpd_ioctlchan)){
			tpdp->tpd_ioctlchan = (struct tpchan *)0;
			tpdp->tpd_ioctlid = 0; /* 0 is not a valid ioctl id */
		}
		tpdp->tpd_flags |= TPD_WAIT_IOCTL;
		if (tpcp){
			tpcp->tpc_ioctlmp = mp;
		}else{
			noenable(q);
			putbq(q, mp);
		}
		splx(s);
	}else{
		tpdp->tpd_flags |= TPD_BUSY_IOCTL;
		tpdp->tpd_ioctlchan = tpcp;
		tpdp->tpd_ioctlid = ((struct iocblk *)(mp->b_rptr))->ioc_id;
		splx(s);
		putnext(realwq, mp);
	}
	return (0);
}

STATIC	void
tp_fillinfo(tpcp, tpdp, infp, fillflag)
struct	tpchan	*tpcp;
struct	tpdev	*tpdp;
struct	tp_info	*infp;
int		fillflag;
{
	if (fillflag == FILLINFO_ALL){
		if (tpdp){
			infp->tpinf_rdev = tpdp->tpd_realdev;
			infp->tpinf_rdevfsdev = tpdp->tpd_realdevfsdev;
			infp->tpinf_rdevino = tpdp->tpd_realdevino;
			infp->tpinf_rdevmode = tpdp->tpd_realdevmode;
			if (tpdp->tpd_datachp){
				infp->tpinf_ddev = tpdp->tpd_datachp->tpc_dev;
				infp->tpinf_dconnid = tpdp->tpd_datachp->tpc_connid;
			}else{
				infp->tpinf_ddev = NODEV;
				infp->tpinf_dconnid = 0;
			}
			if (tpdp->tpd_ctrlchp){
				infp->tpinf_cdev = tpdp->tpd_ctrlchp->tpc_dev;
				infp->tpinf_cconnid = tpdp->tpd_ctrlchp->tpc_connid;
			}else{
				infp->tpinf_cdev = NODEV;
				infp->tpinf_cconnid = 0;
			}
			infp->tpinf_muxid = tpdp->tpd_muxid;
			bcopy(&(tpdp->tpd_sak), &(infp->tpinf_sak),sizeof(struct sak));
			infp->tpinf_flags = tpdp->tpd_userflags;
			bcopy(&(tpdp->tpd_valid),&(infp->tpinf_valid),
							sizeof(struct termios));
			bcopy(&(tpdp->tpd_mask), &(infp->tpinf_mask),
						sizeof(struct termios));
		}else{
			infp->tpinf_rdev = NODEV;
			infp->tpinf_rdevfsdev = NODEV;
			infp->tpinf_rdevino = 0;
			infp->tpinf_rdevmode = 0;
			if (tpcp->tpc_type == TPC_DATA){
				infp->tpinf_ddev = tpcp->tpc_dev;
				infp->tpinf_dconnid = tpcp->tpc_connid;
				infp->tpinf_cdev = NODEV;
				infp->tpinf_cconnid = 0;
			}			
			if (tpcp->tpc_type == TPC_CTRL){
				infp->tpinf_cdev = tpcp->tpc_dev;
				infp->tpinf_cconnid = tpcp->tpc_connid;
				infp->tpinf_ddev = NODEV;
				infp->tpinf_dconnid = 0;
			}else{			
				infp->tpinf_cdev = NODEV;
				infp->tpinf_cconnid = 0;
				infp->tpinf_ddev = NODEV;
				infp->tpinf_dconnid = 0;
			}
			infp->tpinf_muxid = 0;
			bzero(&(infp->tpinf_sak), sizeof(struct sak));
			infp->tpinf_flags = 0;
			bzero(&(infp->tpinf_valid), sizeof(struct termios));
			bzero(&(infp->tpinf_mask), sizeof(struct termios));
		}
		infp->tpinf_dev = tpcp->tpc_dev;
		infp->tpinf_connid = tpcp->tpc_connid;
	}else{ /* FILLINFO_RESTRICTED */
		if (tpdp){
			infp->tpinf_rdev = tpdp->tpd_realdev;
			infp->tpinf_rdevfsdev = tpdp->tpd_realdevfsdev;
			infp->tpinf_rdevino = tpdp->tpd_realdevino;
			infp->tpinf_rdevmode = tpdp->tpd_realdevmode;
			infp->tpinf_muxid = tpdp->tpd_muxid;
			bcopy(&(tpdp->tpd_sak), &(infp->tpinf_sak),sizeof(struct sak));
		}else{
			infp->tpinf_rdev = NODEV;
			infp->tpinf_rdevfsdev = NODEV;
			infp->tpinf_rdevino = 0;
			infp->tpinf_rdevmode = 0;
			infp->tpinf_muxid = 0;
		}
		infp->tpinf_ddev = NODEV;
		infp->tpinf_dconnid = 0;
		infp->tpinf_cdev = NODEV;
		infp->tpinf_cconnid = 0;
		infp->tpinf_flags = 0;
		bzero(&(infp->tpinf_valid), sizeof(struct termios));
		bzero(&(infp->tpinf_mask), sizeof(struct termios));
		infp->tpinf_dev = NODEV;
		infp->tpinf_connid = 0;
	}
}

/* 
** Verify TCSET* type ioctls down from the data or cons channel can not
** potentially interfere with SAK detection.  If the process has the
** P_DRIVER privilege, do not make the check.
** Privileged processes may need to set the initial termio setting via the 
** data channels or cons channel to sync up termios structures in modules
** and drivers above us.  If saktype is saktypeNONE (saktypeDATA when no
** data channel is connected) do not do check.
** If all is well return 0.
**
** NOTE: credp contains calling user's process credentials when MAC is
** installed and the file's (associated with the ioctl) credentials when MAC is
** NOT installed.
*/
STATIC int
tp_ioctlok(tpcp, credp)
struct tpchan *tpcp;
cred_t	*credp;
{
	register struct termios	*maskp,*nextp,*curp;
	struct tpdev		*tpdp;
	int			i;
	char			sak_cc;

	tpdp = tpcp->tpc_devp;
	if (!tpdp){
		return (EIO);
	}
	nextp = &tpdp->tpd_nextterm;
	if (((tpcp->tpc_type == TPC_DATA) || (tpcp->tpc_type == TPC_CONS)) &&
	 !((tpdp->tpd_sak.sak_type == saktypeDATA) ||
	 (tpdp->tpd_sak.sak_type == saktypeNONE))){
		if (!drv_priv(credp)){ /* returns 0 on success */
			return (0);
		}
		maskp = &(tpdp->tpd_mask);
		curp = &(tpdp->tpd_curterm);
		/*
		** Make sure requested modes are compatible with the SAK.
		*/
		if ((nextp->c_iflag ^ curp->c_iflag) & maskp->c_iflag){
			return (EPERM);
		}
		if ((nextp->c_oflag ^ curp->c_oflag) & maskp->c_oflag){
			return (EPERM);
		}
		if ((nextp->c_cflag ^ curp->c_cflag) & maskp->c_cflag){
			return (EPERM);
		}
		if ((nextp->c_lflag ^ curp->c_lflag) & maskp->c_lflag){
			return (EPERM);
		}
		/*
		** Make sure all special special characters are compatible
		** with SAK.
		*/
		if (tpdp->tpd_sak.sak_type == saktypeCHAR){
			sak_cc = tpdp->tpd_sak.sak_char;
			for (i = 0;i < NCCS;i++){
				if (tpdp->tpd_nextterm.c_cc[i] == sak_cc){
					return (EPERM);
				}
			}
		}
	}
	return (0);
}

/*
** Disconnect the data channel.
**
** When a data channel disconnects from the TP device for any reason,
** the TPD_DEFSAK flag is checked.  If the flag is set, tp_setsak() is called
** and does the following:
**	-the queued (pending) SAK defintion is set and the TPD_WAITTCSET flag
**	 is set to prevent a data channel connection until an M_IOCACK is
**	 received for a TCSET* command.
**	-if a TCSET* ioctl is queued (pending), it is sent to the driver below.
**
** See tplrput() for an explanation of how TPD_WAITTCSET gets cleared.
*/
STATIC	void
tp_discdata(tpdp)
struct tpdev	*tpdp;
{
	struct	tpchan	*tpcp;
	register int	s;

	tpdp->tpd_userflags &= ~DATACONNECTFLAGS;

	s = splstr();
	tpcp = tpdp->tpd_datachp;
	if (!tpcp){
		splx(s);
		return;		/*No data channel to disconnect*/
	}
	/*
	** Clear out the ioctl channel if it is the
	** data channel.
	*/
	if (tpdp->tpd_ioctlchan == tpcp){
		tpdp->tpd_ioctlchan = (struct tpchan *)0;
		tpdp->tpd_ioctlid = 0;
	}
	/*
	** Free any saved ioctl messages on the channel.
	*/
	if (tpcp->tpc_ioctlmp){
		freemsg(tpcp->tpc_ioctlmp);
		tpcp->tpc_ioctlmp = (mblk_t *)0;
	}
	tpcp->tpc_flags |= TPC_DIRTY;
	tpdp->tpd_datachp = (struct tpchan *)0;
	if (tpdp->tpd_inputchp == tpcp){
		tpdp->tpd_inputchp = (struct tpchan *)0;
	}
	/*
	** If the SAK type is NONE when a data channel is disconnected,
	** the SAK type changes to DATA.  This means that the first data
	** packet received from the real device will cause a SAK to be sent
	** up the control channel, notifying the process that has the
	** control channel of activity on the device.
	** NOTE: The SAK type is not changed from NONE to DATA if a time
	** interval to delay the change is defined.  The SAK type will be
	** changed from NONE to DATA when the timer expires.
	** NOTE: If there still is another channel on this TP device
	** (the cons channel may still be connected for input), do not
	** set to saktypeDATA.  We do not want a 'fake' SAK issued while
	** there is a channel connected that is receiving input messages.
	**
	** HISTORICAL NOTE: (Prior to the delay sak type change functionality;
	** When a non-zero delay time value is defined can no longer assume
	** that there is a data channel connected when the SAK type is NONE)
	** The processing for SAK type NONE in the lower read put routine
	** assumes that whenever the SAK is NONE there is a data channel
	** connected and and a read queue in place, so setting the SAK
	** type to DATA should not be interrupted by data coming up the stream.
	*/
	if ((tpdp->tpd_sak.sak_type == saktypeNONE) && !(tpdp->tpd_inputchp)){
		if (tp_saktypeDATA_switchdelay){
			/*
			** If an outstanding timeout is pending from a previous
			** call to tp_discdata() of a previous data channel,
			** clear it (via untimeout()) before calling timeout()
			** for this data channel disconnect.
			** If this is not done and the TP device's memory
			** is freed  (in tpclose()) before the outstanding
			** timeout() function, tp_saktypeswitch() is called,
			** the system will panic.
			** NOTE: tpclose() could not clear the outstanding
			** timeout since its timeoutid would have been
			** overwritten by this call to timeout().
			*/
			s = splhi();
			if (tpdp->tpd_timeoutid){
				untimeout(tpdp->tpd_timeoutid);
			}
			splx(s);
			tpdp->tpd_timeoutid = timeout(tp_saktypeswitch,
			 tpdp, drv_usectohz(MILLITOMICRO(tp_saktypeDATA_switchdelay)));
		} else {
			tpdp->tpd_sak.sak_type = saktypeDATA;
		}
	}
	splx(s);


	/*
	** Once the data channel has been disconnected, any pending
	** requests should be woken up and receive an EIO. putctrl1()
	** M_ERROR will send back an M_FLUSH for read and write queues.
	**
	** putctl1() is called before the the data channel's tpc_devp is
	** cleared, so M_FLUSH will also flush every thing from the lower
	** mux on down (assuming that the M_FLUSH, is not enqueued onto any
	** Queue.
	*/
	putctl1(tpcp->tpc_rq->q_next, M_ERROR, EIO);
	s = splstr();
	tpdp->tpd_datarq = (queue_t *)0;
	if (tpdp->tpd_inputrq == tpcp->tpc_rq){
		tpdp->tpd_inputrq = (queue_t *)0;
	}
	tpcp->tpc_devp = (struct tpdev *)0;
	splx(s);
	/*
	** If no channel is connected to receive input and a new SAK has been
	** defined, make it the active SAK.
	** If no channel is connected to receive input drop real/physical
	** device's DTR
	*/
	if (!(tpdp->tpd_inputchp)){
 		if ((tpdp->tpd_flags & TPD_DEFSAK)){
			tp_putsak(tpdp, &(tpdp->tpd_heldsak));
		}
		tp_senddiscioctl(tpdp);
	}
}

/*
** Disconnect the read side of cons channel from the TP device.
**
** Restore ("re-connect") the data channel (if connected) as the channel to
** direct upstream messages (ie. make data channel the active channel).
**
** NOTE for a Future Enhancement: May eventually want to issue an internal
** TCSET* ioctl to reset termios values when ever a channel (currently only
** the data channel) is restored as the active channel.  Additional
** functionality would also be needed to save the termios information at the
** time the channel was made inactive (ie. would not receive upstream
** messages)... For now since making channels "inactive" and "active" 
** (currently only data channels) is rare occurrence (currently only the cons
** channel would effect the active state other channels), we rely on the
** user level application to restore termios information if neccessary.
*/
STATIC int
tp_disccons()
{
	struct tpchan	*tpcp;
	int	s;

	tpcp = tp_findchan(TP_CONSDEV);
	s = splstr();
	if (!tpcp || !tpconsdev || (tpconsdev->tpd_inputchp != tpcp)){
		splx(s);
		return (ENXIO);
	}
	/*
	** No need to check whether channel becoming the channel to receive
	** input is connected or not.
	*/
	tpconsdev->tpd_inputchp = tpconsdev->tpd_datachp;
	tpconsdev->tpd_inputrq = tpconsdev->tpd_datarq;
	/*
	** If the SAK type is NONE the SAK type changes to DATA if there is
	** not another channel (the data channel) connected to this
	** TP device.  We do not want a 'fake' SAK issued while
	** there is a channel connected that is receiving input messages.
	** NOTE: The SAK type is not changed from NONE to DATA if a time
	** interval to delay the change is defined.  The SAK type will be
	** changed from NONE to DATA when the timer expires.
	*/
	if ((tpconsdev->tpd_sak.sak_type == saktypeNONE) &&
	 !(tpconsdev->tpd_inputchp)){
		if (tp_saktypeDATA_switchdelay){
			tpconsdev->tpd_timeoutid = timeout(tp_saktypeswitch,
			 tpconsdev, drv_usectohz(MILLITOMICRO(tp_saktypeDATA_switchdelay)));
		} else {
			tpconsdev->tpd_sak.sak_type = saktypeDATA;
		}
	}
	splx(s);
	/*
	** If no channel is connected to receive input and a new SAK has been
	** defined, make it the active SAK.
	** If no channel is connected to receive input drop real/physical
	** devices DTR.
	*/
	if (!(tpconsdev->tpd_inputchp)){
 		if ((tpconsdev->tpd_flags & TPD_DEFSAK)){
			tp_putsak(tpconsdev, &(tpconsdev->tpd_heldsak));
		}
		tp_senddiscioctl(tpconsdev);
	}
	return (0);
}

/*
** Disconnect the read/input side of TP device.
**
** Since only disconnected read/input side, only needed to clear data
** structures in the TP device (inputchp and inputrq).
*/
STATIC void
tp_discinput(tpdp)
struct tpdev	*tpdp;
{
	register int	s;

	s = splstr();
	tpdp->tpd_inputchp = (struct tpchan *)0;
	tpdp->tpd_inputrq = (queue_t *)0;
	splx(s);
}


/*
** tp_senddiscioctl: Sets up a TCSETS ioctl with the baud rate set to B0
** and sends it downstream to the real/physical device.  "Setting" the baud
** rate to B0, will cause the port to drop its out going DTR.
*/
STATIC void
tp_senddiscioctl(tpdp)
struct tpdev	*tpdp;
{
	register mblk_t		*mp;
	register struct termios	*tiosp;


	mp = tpdp->tpd_discioctl;
	tpdp->tpd_discioctl = (mblk_t *)0;

	if (!mp){
		/*
		** This is OK. tpd_discioctl may not be set up if only the
		** cons channel was connected for input for the TP device.
		** tpd_discioctl is only set up when connecting the data
		** channel
		*/
		return;
	}

	/*
	** If no real/physical device is linked underneath or HUPCL flag in the
	** current termios settings (ie. tpd_curterm.c_cflag) is not set,
	** do not send ioctl.
	*/
	if ((!tpdp->tpd_realrq) || !(tpdp->tpd_curterm.c_cflag & HUPCL)){
		freemsg(mp);
		return;
	}

	tiosp = (struct termios *)mp->b_cont->b_rptr;
	bcopy(&tpdp->tpd_curterm, tiosp, sizeof(struct termios));
	tiosp->c_cflag &= ~CBAUD;
	tiosp->c_cflag |= B0; /* statement there for clarity */

	/*
	** Schedule the ioctl, with the STREAMS scheduler, to be sent down
	** to the real/physical device.  This ioctl is always scheduled
	** since this function may be called from the interrupt level and
	** we want to minimize the number of stack frames on the interrupt
	** stack as much as possible.
	*/
	putq(WR(tpdp->tpd_realrq), mp);
	enableok(WR(tpdp->tpd_realrq));
	qenable(WR(tpdp->tpd_realrq));

	return;
}


/*
** Filter sensitive standard ioctl calls.
*/
STATIC int
tp_stdioctl(cmd,mp,tpcp)
int		cmd;
mblk_t		*mp;
struct	tpchan	*tpcp;
{
	register struct termios	*nexttiosp;
	register struct termios	*tiosp;
	register struct termio	*tiop;
	register struct tpdev	*tpdp;
	register struct iocblk	*iocp;

	tpdp = tpcp->tpc_devp;
	iocp = (struct iocblk *)mp->b_rptr;
	nexttiosp = &(tpdp->tpd_nextterm);

	switch(cmd){
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
		/* 
		** Fail if no termios buffer attached
		*/
		if (!mp->b_cont){
			return (EINVAL);
		}
		tiosp = (struct termios *)mp->b_cont->b_rptr;
		bcopy((caddr_t)tiosp,(caddr_t)nexttiosp,
		 sizeof(struct termios));
		return (tp_ioctlok(tpcp, iocp->ioc_cr));

	/* old style termio */
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
		/*
		** Fail if no termio buffer attached
		*/
		if (!mp->b_cont){
			return (EINVAL);
		}
		tiop = (struct termio *)mp->b_cont->b_rptr;
		/*
		** Copy current termios to next termios to save the non-termio
		** portion of the termios definition.
		** Replace termio portion of tpd_nextterm (which has been
		** copied in from tpd_curterm) with the termio values sent
		** via the ioctl.
		*/
		bcopy((caddr_t)&(tpdp->tpd_curterm), (caddr_t)nexttiosp,
		 sizeof(struct termios));
		nexttiosp->c_iflag = (nexttiosp->c_iflag & HI16)|tiop->c_iflag;
		nexttiosp->c_oflag = (nexttiosp->c_oflag & HI16)|tiop->c_oflag;
		nexttiosp->c_cflag = (nexttiosp->c_cflag & HI16)|tiop->c_cflag;
		nexttiosp->c_lflag = (nexttiosp->c_lflag & HI16)|tiop->c_lflag;
		bcopy((caddr_t)tiop->c_cc,(caddr_t)nexttiosp->c_cc,NCC);
		return (tp_ioctlok(tpcp, iocp->ioc_cr));
	default:
		break;
	}
	return (0);
}

/*
** The following routines manage the minor device space of the trusted
** path driver.
*/

/*
** Free a minor device and all associated storage objects.  A
** performance improvement is contemplated which would invalidate the
** storage and hold on to it. Since the total number of trusted path
** devices should remain fairly constant, this would reduce the number
** of kmem_alloc() calls without eating significant memory. For now the
** code just frees the storage.
*/
STATIC	void
tp_freechan(tpcp)
struct	tpchan	*tpcp;
{
	register int	min;

	min = geteminor(tpcp->tpc_dev);
	tpchanhead[min] = (struct tpchan *)0;
	if (tpcp->tpc_hupmsg){
		freemsg(tpcp->tpc_hupmsg);
	}
	if (tpcp->tpc_sakmsg){
		freemsg(tpcp->tpc_sakmsg);
	}
	kmem_free((caddr_t)tpcp, sizeof(struct tpchan));
}

/*
** Allocate a new minor device and its associated storage.  First
** search the current list of minor devices for a free one.  If one is
** found, use it, otherwise reallocate a larger list of minor devices
** and use the next available one.  The type  argument specifies the kind of
** channel (data, ctrl, cons, or adm) to be allocated. 
**
** Assign a unique connection id in tpc_connid.  Currently the only applicable
** use of a connection id is for data channels although a connection id is
** assigned for every type of tp channel.  Connection ids provide an advisory
** protection mechanism from ctrl channels disconnecting data channels when
** the ctrl channel receives hangup notification. Since data channels
** can be connected to a TP device via more than one way (the TP device's ctrl
** channel and the adm channel) and potentially more than one process, the
** ctrl channel may not want to disconnect the data channel if it was not
** the channel that connected the data channel to the TP device.
*/
STATIC	struct	tpchan	*
tp_allocchan(type)
ulong	type;
{
	static	ulong		connid = 0;
	register minor_t	i;
	register struct tpchan	*tpcp;

	if (type == TPC_ADM){
		i = TP_ADMDEV;
	}else if (type == TPC_CONS){
		i = TP_CONSDEV;
	}else{
		for (i = TP_NONCLONE; (i < tpchan_curcnt) && tpchanhead[i]; i++);
	}
	if (i >= tpchan_curcnt){
		if (!tp_growlist(&tpchan_curcnt,(caddr_t *)&tpchanhead)){
			return ((struct tpchan *)0);
		}
	}
	if (!tpchanhead[i]){
		tpcp =  kmem_zalloc(sizeof(struct tpchan), KM_NOSLEEP);
		if (!tpcp){
			return ((struct tpchan *)0);
		}
		tpcp->tpc_type = type;
		tpcp->tpc_flags = TPC_ISOPEN;
		tpcp->tpc_dev = makedevice(tpemaj, i);
		tpchanhead[i] = tpcp;
	}else{
		tpcp = tpchanhead[i];
	}
	if (++connid == 0)
		connid = 1;		/*Make sure it doesn't wrap to zero*/
	tpcp->tpc_connid = connid;
	return (tpcp);
}

/*
** Set the static variable (tpconsdev) to the device pointer for the current
** console.
** If the tp console channel is opened set its tpc_devp to tpconsdev.
** If the tp console channel has messages on its write queue, enable the queue.
** This can happen the tp console channel had been opened for "no delay" and
** tpconsdev was not yet set or the tp console channel had been opened and
** the 'console' device had been ulinked and subsequently re-linked.
** If the console changes before the tp console channel is
** open, the tpconsdev will be used at open to set the channel's device
** pointer.  Once the tp console channel is open its device pointer is kept up
** to date.
*/
STATIC	void
tp_setcons(devp)
struct	tpdev	*devp;
{
	queue_t *conschanrq;
	struct tpchan *conschanp;

	devp->tpd_userflags &= ~TPINF_CONSOLE;
	tpconsdev = devp;
	conschanp = tp_findchan(TP_CONSDEV);
	if (conschanp){
		conschanp->tpc_devp = tpconsdev;
		conschanrq = conschanp->tpc_rq;
		if (conschanrq){
			if (WR(conschanrq)->q_first){
				enableok(WR(conschanrq));
				qenable(WR(conschanrq));
			}
		}else{
			cmn_err(CE_WARN,
		 	 "tp_resetcons:TP console channel (cons) has no read queue.\n");
		}
	}
	/*
	** Wakeup any processes that were waiting for tpconsdev to be set.
	*/
	(void)wakeup((caddr_t)&tpconsdev);
}

/*
** If the cons channel is connected for input to the 'console' TP device, call
** the internal form of TP_CONSDISCONNECT (tp_disccnos()) to disconnect the
** read/input side of the cons channel.
**
** If the argument, tpdp, is not NULL, reset the 'console' TP device to tpdp.
**
** If tpdp is NULL:
** If the default console tp_consoledev is connected and has a real device
** linked underneath, make the default console the 'console' device.
** If the default console tp_consoledev is not connected or linked, clear
** tpconsdev and if the tp console channel is opened, clear its console device
** pointer (tpchanhead[TP_CONSDEV]->tpc_devp).
*/
STATIC	void
tp_resetcons(tpdp)
struct tpdev	*tpdp;
{
	queue_t	*dfltcondevrealrq;
	struct tpchan	*conschanp;
	struct tpdev	*dfltcondevp;

	conschanp = tp_findchan(TP_CONSDEV);
	if (conschanp && tpconsdev && (tpconsdev->tpd_inputchp == conschanp)){
		(void)tp_disccons();
	}

	if (tpdp){
		tp_setcons(tpdp);
	}else{
		dfltcondevp = tp_finddev(tp_consoledev, FINDDEVLINKED);
		if (dfltcondevp){
			tpconsdev = dfltcondevp;
			dfltcondevrealrq = dfltcondevp->tpd_realrq;
			if (dfltcondevrealrq){
				tp_setcons(dfltcondevp);
				return;
			}
		}
		tpconsdev = (struct tpdev *)NULL;
		if (conschanp){
			conschanp->tpc_devp = (struct tpdev *)NULL;
		}
	}
}

/*
** Switch the current 'console' TP device to the real/physical device indicated
** by tpinf_rdev.
**
** If the flag TPINF_ONLYIFLINKED is set, the 'console' device is changed
** to the real/physical device only if the real/physical device is linked
** under a TP device.
** If TPINF_ONLYIFINKED is not set and a TP device for real/physical device
** (indicated by tpinf_rdev) does not exist, a TP device for the
** real/physical device is created.
** tp_resetcons() is called to actually switch the 'console' TP device.
**
** NOTE: The new 'console' TP device does not inherit "connected for input"
** status from the previously current 'console' TP device.  The default for
** the new 'console' TP device is not to be "connected for input".
*/
STATIC int
tp_consset(tpinfp, newconstpdpp)
struct tp_info	*tpinfp;
struct tpdev	**newconstpdpp;
{

	struct tpdev	*tpdp = (struct tpdev *)NULL;
	dev_t		rdev = tpinfp->tpinf_rdev;


	if (tpinfp->tpinf_flags & TPINF_ONLYIFLINKED){
		if (!(tpdp = tp_finddev(rdev, FINDDEVLINKED))){ 
			return (ENXIO);
		}
	}else{
		tpdp = tp_finddev(rdev, 0);
		if (!tpdp){
			if (!(tpdp = tp_allocdev(rdev))){
				return (EAGAIN);
			}
			/*
			 * Copy in real device's stat information from
			 * tpinf to the TP device.
			 */ 
			tpdp->tpd_realdevfsdev = tpinfp->tpinf_rdevfsdev;
			tpdp->tpd_realdevino = tpinfp->tpinf_rdevino;
			tpdp->tpd_realdevmode = tpinfp->tpinf_rdevmode;
		}
	}
	tp_resetcons(tpdp);
	*newconstpdpp = tpdp;
	return (0);
}

/*
** Return a pointer to the channel information indicated by the minor
** device number provided.
*/
STATIC	struct	tpchan	*
tp_findchan(min)
minor_t	min;
{
	if (min < (minor_t)0)
		return ((struct tpchan *)0);
	return ((min < tpchan_curcnt) ? tpchanhead[min] : (struct tpchan *)0);
}


/*
** The following routines maintain the device list information.
*/

/*
** Free a device block and all associated storage objects.  A
** performance improvement is contemplated which would invalidate the
** storage and hold on to it. Since the total number of trusted path
** devices should remain fairly constant, this would reduce the number
** of kmem_alloc() calls without changing memory usage. For now the code
** just frees the storage.
*/

STATIC	void
tp_freedev(devp)
struct tpdev	*devp;
{
	register minor_t	min;

	min = devp->tpd_minordev;
	devp->tpd_realrq = (queue_t *)0;
	devp->tpd_ctrlrq = (queue_t *)0;
	devp->tpd_datarq = (queue_t *)0;
	devp->tpd_inputrq = (queue_t *)0;
	devp->tpd_ctrlchp = (struct tpchan *)0;
	devp->tpd_datachp = (struct tpchan *)0;
	devp->tpd_inputchp = (struct tpchan *)0;
	kmem_free((caddr_t)devp, (ulong)sizeof(struct tpdev));
	tpdevhead[min] = (struct tpdev *)0;
}

STATIC struct tpdev *
tp_finddev(realdev, flag)
dev_t	realdev;
int	flag;	/*
		** If set to FINDDEVLINKED, returns a tpdev only if it
		** has a real/physical device linked underneath.
		*/
{
	register struct tpdev	**tpdpp;

	if (!tpdevhead){
		return ((struct tpdev *)0);
	}
	if (realdev == NODEV){
		return ((struct tpdev *)0);
	}
	for (tpdpp = tpdevhead; tpdpp < &tpdevhead[tpdev_curcnt]; tpdpp++){
		if ((*tpdpp) && ((*tpdpp)->tpd_realdev == realdev)){
			if (flag == FINDDEVLINKED){
				if ((*tpdpp)->tpd_realrq){
					return (*tpdpp);
				}else{
					return ((struct tpdev *)0);
				}
			}else{
				return (*tpdpp);
			}
		}
	}
	return ((struct tpdev *)0);
}

/*
** Allocate a device block for the specified real device.  First search
** the the current list of TP devices for an already allocated device
** for that real device, then, if the device is not found, search for a
** a free entry.  As a last resort, reallocate a larger list of TP devices
** and use the next available one.  If a new device is allocated, fill
** out the tpd_realdev field with the specified real device number.
*/

struct	tpdev	*
tp_allocdev(rdev)
dev_t	rdev;
{
	register int		i;
	register struct tpdev	*tpdp;

	if (tpdp = tp_finddev(rdev, 0)){
		return (tpdp);
	}

	for (i = 0; (i < tpdev_curcnt) && tpdevhead[i]; i++);
	if (i >= tpdev_curcnt){
		/*
		** grow array of tp devices pointers
		*/
		if (!tp_growlist(&tpdev_curcnt,(caddr_t *)&tpdevhead)){
			return ((struct tpdev *)0);
		}
	}
	/*
	** allocate memory for a tp device and set tp device up.
	*/
	tpdp = tpdevhead[i] = kmem_zalloc(sizeof(struct tpdev),KM_NOSLEEP);
	if (!tpdp){
		return ((struct tpdev *)0);
	}
	tpdp->tpd_flags = 0;
	tpdp->tpd_minordev = (minor_t)i;
	tpdp->tpd_realdev = rdev;
	return (tpdp);
}

/*
** allocate "tp_listallocsz" more pointers in a list.
**
** NOTE: to maintain the indexed array "feel", a new set of 
** cnt + growcnt pointers are allocated and the values of the
** current set are copied into the new set of tp channel pointers.
**
** IMPORTANT!	If lists are ever to become modified at the interrupt
**		level put splstr() before the bcopy() splx() after
**		new listp and cntp have been assigned.  Since lists
**		currently are not modified from interrupt level, the
**		splstr() is not necessary.
*/

STATIC int
tp_growlist(cntp, listp)
int	*cntp;		/* pointer to current number of pointers */
caddr_t	*listp; 	/* pointer to list */
{
	register ulong		sz,osz;
	register caddr_t	tmp;
	register caddr_t	olst;

	olst = *listp;
	osz = (*cntp) * sizeof(caddr_t);
	sz = osz + (tp_listallocsz * sizeof(caddr_t));
	tmp = kmem_zalloc(sz, KM_NOSLEEP);
	if (!tmp)
		return (0);

	if (olst){
		bcopy(olst,tmp, osz);
	}
	*listp = tmp;
	(*cntp) += tp_listallocsz;
	if (olst){
		kmem_free(olst, osz);
	}
	return (1);
}


/*
** tp_allocioctl: Allocate an internal M_IOCTL message.
*/

STATIC mblk_t *
tp_allocioctlmsg(cmd, datasz)
int		cmd;		/* ioctl cmd */
size_t		datasz;		/* size of data part of message */
{

	register mblk_t		*mp;	/* pointer to message */ 
	register mblk_t		*bp;	/* pointer to message block */ 
	register mblk_t		*dbp;	/* pointer to current message data						** block
					*/ 
	register struct iocblk	*iocp;
	size_t			count;
	size_t			bufsz;


	if (!(mp = bp = allocb(sizeof(struct iocblk), BPRI_HI))){
		return (mblk_t *)0;
	}

	/*
	** Set up ioctl block.  Hold credentials, credentials will be freed
	** in tplrput when it receives the ACK/NAK.
	*/
	iocp = (struct iocblk *)(bp->b_rptr);
	iocp->ioc_count = datasz;
	iocp->ioc_cmd = cmd;
	/*
	** sys_cred, the generic credential structure for the kernel, is used
	** when the credentials may be required by functions that are executed
	** on behalf of the kernel and not the user.
	** NOTE:sys_cred has all privileges.
	*/  
	iocp->ioc_cr = sys_cred;
	iocp->ioc_error = 0;
	iocp->ioc_rval = 0;
	bp->b_datap->db_type = M_IOCTL;
	bp->b_wptr += sizeof(struct iocblk);

	/*
	** Allocate space for data, if any
	*/
	if (datasz){
		count = datasz;
		while (count){
			bufsz = (count <= MAXIOCBSZ ? count : MAXIOCBSZ);
			if (!(dbp = allocb(bufsz, BPRI_HI))){
				freemsg(mp);
				return ((mblk_t *)0);
			}
			dbp->b_datap->db_type = M_DATA;
			dbp->b_wptr += bufsz;
			bp = (bp->b_cont = dbp);
			count -= bufsz;
		}
	}else{
		bp->b_cont = (mblk_t *)0;
	}

	return (mp);
}

/*
** Disconnect data channel and channel connected for input (if they are
** different) from the TP device, given the external dev_t of a TP channel
** (typically the data channel).
** This meant to be called, when the controlling terminal process exits.
** It immediately denies access to the real/physical tty device (via the
** TP data channel {and input access if the cons channel is associated
** with the TP device}) if any other process has access to the data or cons
** channel associated with the TP device.
**
** NOTE: If the TP device number passed as an argument is the cons channel, only
** disconnect the cons channel for input if it is connected for input.  Since
** the cons channel gets registered as a tty type device at the Stream Head,
** it can become a controlling terminal.  We do not want to disconnect the
** data channel (if any) associated with the TP device that the cons channel
** is also connected to, since the controlling terminal process'es controlling
** terminal is the cons channel, not the data channel.
*/
void
tp_disconnect(tpdev)
dev_t	tpdev;
{

	struct tpchan	*tpcp;
	struct tpdev	*tpdp;
	minor_t		tpminordev;

	/* If device, tpdev, is not a TP type of device, or there is no
	** TP channel associated with the minor device number, or the
	** channel is not connected to a TP device, just return.
	*/
	if (tpemaj != getemajor(tpdev)){
		return;
	}
	tpminordev = geteminor(tpdev);
	if ((tpcp = tp_findchan(tpminordev)) == (struct tpchan *)0){
		return;
	}
	tpdp = tpcp->tpc_devp;
	if (tpdp){
		if (tpminordev == TP_CONSDEV){
			if (tpdp->tpd_inputchp == tpcp){
				tp_disccons(tpdp);
			}
		}else{
			tp_discinput(tpdp);
			tp_discdata(tpdp);
		}
	}
	return;
}

/*
** Change sak type to DATA if the sak type is NONE and there are no
** channels connected for input.
** This is called via a setup from timeout() from tp_discdata() and
** tp_disccons().  It delays the change to sak type DATA if a delay
** time interval is defined.
** NOTE: Any calls to tp_freedev() must call untimeout() if TP device
** has a pending timeout() function.  This is to insure the integrity
** of the argument, tpdp, to this function.
*/
STATIC void
tp_saktypeswitch(tpdp)
struct	tpdev *tpdp;
{
	if ((tpdp->tpd_sak.sak_type == saktypeNONE) && !(tpdp->tpd_inputchp)){
		tpdp->tpd_sak.sak_type = saktypeDATA;
	}
	tpdp->tpd_timeoutid = 0;
}
