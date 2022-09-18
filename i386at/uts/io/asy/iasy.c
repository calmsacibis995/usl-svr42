/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/asy/iasy.c	1.17"
#ident	"$Header: $"

#ifndef lint
static char iasy_copyright[] = "Copyright 1991 Intel Corporation xxxxxx";
#endif /*lint*/

/*
 * Generic Terminal Driver	(STREAMS version)
 * This forms the hardware independent part of a serial driver.
*/
#include "util/param.h"
#include "fs/fcntl.h"
#include "util/types.h"
#include "proc/signal.h"
#include "io/stream.h"
#include "svc/errno.h"
#include "io/termio.h"
#include "util/cmn_err.h"
#include "io/stropts.h"
#include "util/debug.h"
#include "proc/cred.h"
#include "io/strtty.h"
#include "fs/file.h"
#include "io/ldterm/eucioctl.h"
#include "io/asy/iasy.h"
#include "io/asy/asy.h"
#include "io/ddi.h"
#ifdef VPIX
#include "proc/proc.h"
#include "proc/tss.h"
#include "vpix/v86.h"
#endif /* VPIX */

#ifdef DEBUGSTR /* To turn on/off the strlog messages */
#include "io/strlog.h"
#include "io/log/log.h"
#define	DSTRLOG(x) strlog x
#else
#define	DSTRLOG(x)
#endif /* DEBUGSTR */

extern struct strtty asy_tty[];	/* tty structs for each device */
				/* changed from iasy_tty to asy_tty for merge */
extern struct iasy_hw iasy_hw[];/* hardware info per device */
extern int iasy_num;

#define	CL_TIME  8*HZ		/* Timeout to remove block state while closing */

/* Values for iasy_opened */
#define IASY_NOT_OPEN    0
#define IASY_OPEN        1
#define IASY_EXCL_OPEN   2

#ifdef MERGE386
extern  int     merge386enable;
#endif /* MERGE386 */

#ifdef VPIX
extern v86_t *iasystash[];	/* save proc.p_v86 here for psuedorupts */
extern int iasyintmask[];	/* which pseudorupt to give */
extern struct termss iasyss[];	/* start/stop characters */
extern int   iasy_closing[];
extern char iasy_opened[];
extern int iasy_v86_prp_pd[];
extern  int     validproc();

/* In DOSMODE and PARMRK we store MSR and LSR in the ring when they
 * change.  Borrow existing FRERROR and PERROR bits for these. */
#define MSRWORD FRERROR
#define LSRWORD PERROR
#endif /* VPIX */

#if defined(MB1)||defined(MB2)
#define IASYUNIT(minor)		(minor)
#else
#define IASYUNIT(minor)		((minor) / 2)
#endif
#define TP_TO_Q(tp)			((tp)->t_rdqp)
#define Q_TO_TP(q)			((struct strtty *)q->q_ptr)
#define TP_TO_HW(tp)		(&iasy_hw[IASYUNIT((tp)->t_dev)])
#define HW_PROC(tp, func)	((*TP_TO_HW(tp)->proc)((tp), func))
#define VM_PROC(tp, func, argp1, argp2)  \
						((*TP_TO_HW(tp)->vmproc)((tp), func, (argp1), (argp2)))

int iasydevflag = 0; 	/* SVR4.0 requirement */
int iasy_cnt = 0;	 	/* /etc/crash requirement */

int iasyopen(), iasyclose(), iasyoput();
int iasyisrv(), iasyosrv();

int (*iasygetcharp)() = 0;	/* ptr to hardware fn getchar	*/
void (*iasyputcharp)() = 0;	/* ptr to hardware fn putchar	*/

struct module_info iasy_info = {
	901, "iasy", 0, INFPSZ, IASY_HIWAT, IASY_LOWAT };
static struct qinit iasy_rint = {
	putq, iasyisrv, iasyopen, iasyclose, NULL, &iasy_info, NULL};
static struct qinit iasy_wint = {
	iasyoput, iasyosrv, iasyopen, iasyclose, NULL, &iasy_info, NULL};
struct streamtab iasyinfo = {
	&iasy_rint, &iasy_wint, NULL, NULL};

/*
 * Wakeup sleep function calls sleeping for a STREAMS buffer
 * to become available
 */
STATIC void
iasybufwake( tp)
register struct strtty *tp;
{
 	wakeup( (caddr_t)&tp->t_in.bu_bp);
}

iasy_drain(tp)
struct strtty	*tp;
{
	HW_PROC(tp, T_RESUME);
	tp->t_state &= ~TTIOW;
	wakeup((caddr_t)&tp->t_oflag);
	tp->t_dstat = 0;
}

/*
 * void
 * iasy_carrier(struct strtty *)
 *      wakeup/signal a process to indicate the establishment of connection.
 *
 * Calling/Exit State:
 *      None.
 */
void
iasy_carrier(struct strtty *tp)
{
	(void)wakeup((caddr_t) &tp->t_rdqp);
}


/*
 *	Open an iasy line
*/
/* ARGSUSED */
iasyopen(q, devp, flag, sflag, crp)
queue_t *q;		/* Read queue pointer */
dev_t *devp;
int flag;
int sflag;
cred_t *crp;
{	register struct strtty *tp;
	register struct stroptions *sop;
	minor_t ldev, dev;
	int oldpri;
	mblk_t *mop;
	int	ret;

	ldev = getminor(*devp);
	dev = IASYUNIT(ldev);
	if (dev >= iasy_num)
		return(ENXIO);
	if (iasy_hw[dev].proc == 0)
		return(ENXIO);	/* No hardware for this minor number */

#ifdef VPIX
	/* enforce exclusive access */
	if (iasy_opened[iasychan(dev)] == IASY_EXCL_OPEN ||
	   ((flag & O_EXCL) && iasy_opened[iasychan(dev)] != IASY_NOT_OPEN)) {
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasy: VPIX open failed"));
		return(EBUSY);
	}
#endif /* VPIX */

	tp = &asy_tty[dev];

	oldpri = SPL();

	/* 
	 * Do the required things on first open 
	 */
	if ((tp->t_state & (ISOPEN | WOPEN)) == 0) {

		tp->t_dev = ldev;
		tp->t_rdqp = q;
		q->q_ptr = (caddr_t) tp;
		WR(q)->q_ptr = (caddr_t) tp;

		/*set process group on first tty open*/
		while ((mop = allocb(sizeof(struct stroptions),BPRI_MED)) 
											== NULL){
			if ( flag & (FNDELAY | FNONBLOCK)) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return( EAGAIN);
			}
			bufcall( (uint)sizeof( struct stroptions), BPRI_MED, 
						iasybufwake, tp);
			if ( sleep( (caddr_t)&tp->t_in.bu_bp, TTIPRI | PCATCH)) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return( EINTR);
			}
		}

		mop->b_datap->db_type = M_SETOPTS;
		mop->b_wptr += sizeof(struct stroptions);
		sop = (struct stroptions *)mop->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
		sop->so_hiwat = IASY_HIWAT;
		sop->so_lowat = IASY_LOWAT;
		(void) putnext(q, mop);
	
		/* Set water marks on write q */
		strqset(WR(q), QHIWAT,  0, IASY_HIWAT);
		strqset(WR(q), QLOWAT,  0, IASY_LOWAT);

#ifdef VPIX
		iasy_opened[iasychan(dev)] = (flag & O_EXCL) ? IASY_EXCL_OPEN : IASY_OPEN;
		iasy_v86_prp_pd[2*iasychan(dev)] = 0;
		iasy_v86_prp_pd[(2*iasychan(dev)) + 1] = 0;
		iasyss[iasychan(dev)].ss_start = CSTART;
                iasyss[iasychan(dev)].ss_stop = CSTOP;
#else
                tp->t_cc[VSTART] = CSTART;
                tp->t_cc[VSTOP] = CSTOP;
#endif /* VPIX */

		tp->t_iflag = 0;
		tp->t_oflag = 0;
		tp->t_cflag = B9600|CS8|CREAD|HUPCL;
		tp->t_lflag = 0;

		/* allocate RX buffer */
		while ((tp->t_in.bu_bp = 
						allocb(IASY_BUFSZ, BPRI_MED)) == NULL){
			if ( flag & (FNDELAY | FNONBLOCK)) {
				tp->t_rdqp = NULL;
				splx (oldpri);
				return( EAGAIN);
			}
			bufcall( (uint)sizeof( struct stroptions), BPRI_MED, 
						iasybufwake, tp);
			if ( sleep( (caddr_t)&tp->t_in.bu_bp, TTIPRI | PCATCH)) {
				tp->t_rdqp = NULL;
				splx (oldpri);
				return( EINTR);
			}
		}

		tp->t_in.bu_cnt = IASY_BUFSZ;
		tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
		tp->t_out.bu_bp = 0;
		tp->t_out.bu_cnt = 0;
		tp->t_out.bu_ptr = 0;
	}else{
		if (ldev != tp->t_dev){
			return(EBUSY);
		}
	}
	
	/* Init HW and SW state */
	if (ret=HW_PROC(tp, T_CONNECT)){ /* T_CONNECT must compute CARR_ON */
		tp->t_rdqp = NULL;
		(void) HW_PROC(tp, T_DISCONNECT);
		if (tp->t_in.bu_bp){
			freeb(tp->t_in.bu_bp);
			tp->t_in.bu_bp = 0;
		}
		splx(oldpri);
		return(ret);
	}

	if (tp->t_cflag & CLOCAL)
		tp->t_state |= CARR_ON;

	/* wait for carrier */
	if (!(flag & (FNDELAY | FNONBLOCK))) {
		while ((tp->t_state & CARR_ON) == 0) {
			tp->t_state |= WOPEN;
			if (sleep((caddr_t) &tp->t_rdqp, TTIPRI|PCATCH)) {
				if (!(tp->t_state & ISOPEN)) {
					q->q_ptr=NULL;
					WR(q)->q_ptr=NULL;
					tp->t_rdqp = NULL;
					if (tp->t_in.bu_bp){
						freeb(tp->t_in.bu_bp);
						tp->t_in.bu_bp = 0;
					}
#ifdef VPIX
					iasy_opened[iasychan(dev)] = IASY_NOT_OPEN;
#endif /* VPIX */
					(void) HW_PROC(tp, T_DISCONNECT);
				}
				tp->t_state &= ~WOPEN;
				splx(oldpri);
				return(EINTR);
			}
		}
	}
	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;
	splx(oldpri);
	return(0);
}

/*
 *	Close an iasy line
*/
/* ARGSUSED */
iasyclose(q, flag, cred_p)
queue_t *q;		/* Read queue pointer */
int flag;
cred_t *cred_p;
{	register struct strtty *tp;
	register int	oldpri;
	int	dev;

	tp = Q_TO_TP(q);
	oldpri = SPL();
	dev = IASYUNIT(tp->t_dev);
	

	/* Drain queued output to the user's terminal. */
	while (tp->t_state & CARR_ON){
		tp->t_dstat = 0;
		if ((tp->t_out.bu_bp == 0) && (WR(q)->q_first == NULL))
			break;


		tp->t_state |= TTIOW;
		tp->t_dstat = timeout(iasy_drain, (caddr_t)tp, CL_TIME);
		if (sleep((caddr_t) &tp->t_oflag, TTOPRI|PCATCH)) {
			tp->t_state &= ~TTIOW;
			if (tp->t_dstat){
				untimeout(tp->t_dstat);
				tp->t_dstat = 0;
			}
			break;
		}
	}

#ifdef VPIX
	iasy_closing[iasychan(dev)] = 1;
	iasy_opened[iasychan(dev)] = IASY_NOT_OPEN;
	iasyintmask[iasychan(dev)] = 0;
	iasystash[iasychan(dev)] = 0;
	tp->t_iflag &= ~DOSMODE;
	iasy_v86_prp_pd[2*iasychan(dev)] = 0;
	iasy_v86_prp_pd[(2*iasychan(dev)) + 1] = 0;
#endif /* VPIX */

	ASSERT(WR(q)->q_first == NULL);
	if (!(tp->t_state & ISOPEN)) {	/* See if it's closed already */
		splx(oldpri);
		return;
	}
	if (tp->t_cflag & HUPCL){
		timeout(iasy_hup, tp, 1);
		(void) HW_PROC(tp, T_DISCONNECT);
	}
#ifdef VPIX
	iasy_closing[iasychan(dev)] = 0;
#endif /* VPIX */

	tp->t_state &= ~(ISOPEN | BUSY | TIMEOUT | TTSTOP);
	iasyflush(WR(q), FLUSHR);
	if (tp->t_in.bu_bp) {
		freeb((mblk_t *)tp->t_in.bu_bp);	
		tp->t_in.bu_bp  = 0;
		tp->t_in.bu_ptr = 0;
		tp->t_in.bu_cnt = 0;
	}
	if (tp->t_out.bu_bp) {
		freeb((mblk_t *)tp->t_out.bu_bp);	
		tp->t_out.bu_bp  = 0;
		tp->t_out.bu_ptr = 0;
		tp->t_out.bu_cnt = 0;
	}
	tp->t_rdqp = NULL;
	tp->t_iflag = 0;
	tp->t_oflag = 0;
	tp->t_cflag = 0;
	tp->t_lflag = 0;
	q->q_ptr = WR(q)->q_ptr = NULL;
	splx(oldpri);
}

/*
 *	Resume output after a delay
*/
void
iasydelay(tp)
struct strtty *tp;
{	int s;

	s=SPL();
	tp->t_state &= ~TIMEOUT;
	(void) HW_PROC(tp, T_OUTPUT);
	splx(s);
}

/*
 * ioctl handler for output PUT procedure
*/
void
iasyputioc(q, bp)
queue_t *q;		/* Write queue pointer */
mblk_t *bp;		/* Ioctl message pointer */
{	struct strtty *tp;
	struct iocblk *iocbp;
	mblk_t *bp1;

	iocbp = (struct iocblk *)bp->b_rptr;
	tp = Q_TO_TP(q);

	switch (iocbp->ioc_cmd) {
#ifdef VPIX
	case AIOCDOSMODE:
	case AIOCNONDOSMODE:
	case AIOCINFO:
	case AIOCSERIALOUT:
	case AIOCSERIALIN:
	case AIOCINTTYPE:
	case AIOCSETSS:
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasyputioc: VPIX ioctl 0x%x", 
													iocbp->ioc_cmd));
#endif /* VPIX */
	case SETRTRLVL:
	case TCSETSW:
	case TCSETSF:
	case TCSETAW:
	case TCSETAF:
	case TCSBRK: /* run these now, if possible */
		if (q->q_first != NULL || (tp->t_state & (BUSY|TIMEOUT|TTSTOP))) {
			(void) putq(q, bp);		/* queue ioctl behind output */
			break;
		}
		iasysrvioc(q, bp);			/* No output, do it now */
		break;

	case TCSETA:	/* immediate parm set */
	case TCSETS:
	case TCGETA:
	case TCGETS:	/* immediate parm retrieve */
		iasysrvioc (q, bp);			/* Do these anytime */
		break;

	case TIOCSTI: { /* Simulate typing of a character at the terminal. */
		register mblk_t *mp;

		/*
		 * The permission checking has already been done at the stream
		 * head, since it has to be done in the context of the process
		 * doing the call.
		 */
		if ((mp = allocb(1, BPRI_MED)) != NULL) {
			if (!canput(RD(q)->q_next))
				freemsg(mp);
			else {
				*mp->b_wptr++ = *bp->b_cont->b_rptr;
				putq( tp->t_rdqp, mp);
			}
		}

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		putnext( RD(q), bp);
		break;
	}

	case EUC_MSAVE:
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	default:
		if ((iocbp->ioc_cmd & 0xff00) != LDIOC) { /* An IOCTYPE ? */
#ifdef MERGE386
			if (merge386enable) {
				if( q->q_first != NULL )
				{
					putq(q, bp);
					return;
				}
				iasysrvioc(q, bp);
				return;
			}
#endif /* MERGE386 */
/*
			/*
			 *	Unknown ioctls are either intended for the hardware dependant
			 *	code or an upstream module that is not present.  Pass the
			 *	request to the HW dependant code to handle it.
			 */
			(*(TP_TO_HW(tp)->hwdep))(q, bp);
			return;
		}
		/* ignore LDIOC cmds */
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;
	}
}

/*
 *	A message has arrived for the output q
*/
iasyoput(q, bp)
queue_t *q;		/* Write queue pointer */
mblk_t *bp;
{	register mblk_t *bp1;
	register struct strtty *tp;
	int s;

	tp = Q_TO_TP(q);
	s = SPL();
	switch (bp->b_datap->db_type) {
	case M_DATA:
		if (!(tp->t_state & CARR_ON)) {
			freemsg(bp);	/* Output without carrier is lost */
			splx(s);
			return(0);
		}
		while (bp) {		/* Normalize the messages */
			bp->b_datap->db_type = M_DATA;
			bp1 = unlinkb(bp);
			bp->b_cont = NULL;
			if ((bp->b_wptr - bp->b_rptr) <= 0) {
				freeb(bp);
			} else {
				(void) putq(q, bp);
			}
			bp = bp1;
		}
		(void) HW_PROC(tp, T_OUTPUT);	/* Start output */
		break;

	case M_IOCTL:
		iasyputioc(q, bp);				/* Queue it or do it */
		(void) HW_PROC(tp, T_OUTPUT);	/* just in case */
		break;

	case M_FLUSH:
#if FLUSHRW != (FLUSHR|FLUSHW)
		cmn_err(CE_PANIC, "iasy: implementation assumption botched\n");
#endif
		switch (*(bp->b_rptr)) {

		case FLUSHRW:
			iasyflush(q, (FLUSHR|FLUSHW));
			freemsg(bp);	/* iasyflush has done FLUSHR */
			break;

		case FLUSHR:
			iasyflush(q, FLUSHR);
			freemsg(bp);	/* iasyflush has done FLUSHR */
			break;

		case FLUSHW:
			iasyflush(q, FLUSHW);
			freemsg(bp);
			break;

		default:
			freemsg(bp);
			break;
		}
		break;

	case M_START:
		(void) HW_PROC(tp, T_RESUME);
		freemsg(bp);
		break;

	case M_STOP:
		(void) HW_PROC(tp, T_SUSPEND);
		freemsg(bp);
		break;

	case M_BREAK:
		if (q->q_first != NULL || (tp->t_state & BUSY)) {
			(void) putq(q, bp);	/* Device busy, queue for later */
			break;
		}
		(void) HW_PROC(tp, T_BREAK); /* Do break now */
		freemsg(bp);
		break;

	case M_DELAY:
		tp->t_state |= TIMEOUT;
		(void) timeout(iasydelay, (caddr_t)tp, (int)*(bp->b_rptr));
		freemsg (bp);
		break;

	case M_STARTI:
		(void) HW_PROC(tp, T_UNBLOCK);
		freemsg(bp);
		break;

	case M_STOPI:
		(void) HW_PROC(tp, T_BLOCK);
		freemsg(bp);
		break;

       case M_CTL:
                { 
                        struct termios *termp;
                        struct iocblk *iocbp;
                        if (( bp->b_wptr - bp->b_rptr) != sizeof(struct iocblk)
) {
                                freeb(bp);
                                break;
                        }
                        iocbp = (struct iocblk *)bp->b_rptr;
                        if ( iocbp->ioc_cmd  == MC_CANONQUERY ) {
                                if ((bp1 = allocb(sizeof(struct termios),BPRI_HI)) == (mblk_t *) NULL) {
                                        freeb(bp);
                                        break;
                                }
                                bp->b_datap->db_type = M_CTL;
                                iocbp->ioc_cmd = MC_PART_CANON;
				bp->b_cont = bp1;
				bp1->b_wptr += sizeof(struct termios);
				termp = (struct termios *)bp1->b_rptr;
				termp->c_iflag = ISTRIP | IXON | IXANY;
				termp->c_cflag = 0;
				termp->c_oflag = 0;
				termp->c_lflag = 0;

				qreply(q, bp);

			}else
				freemsg(bp);

		}
		break;

	case M_IOCDATA:
		/* HW dep ioctl data has arrived */
		(*(TP_TO_HW(tp)->hwdep))(q, bp);
		break;

	default:
		freemsg(bp);
		break;
	}
	splx(s);
	return(0);
}

/*
 *	Return the next data block -- if none, return NULL
*/
mblk_t *
iasygetoblk(q)
struct queue *q;	/* Write queue pointer */
{	register struct strtty *tp;
	register int s;
	register mblk_t *bp;

	s = SPL();
	tp = Q_TO_TP(q);
	if (!tp) {		/* This can happen only if closed while no carrier */
		splx(s);
		return(0);
	}
	while ((bp = getq(q)) != NULL) {
		/* wakeup close write queue drain */
		switch (bp->b_datap->db_type) {
		case M_DATA:
			splx(s);
			return(bp);
		case M_IOCTL:
			if (!(tp->t_state & (TTSTOP|BUSY|TIMEOUT))) {
				iasysrvioc(q, bp);	/* Do ioctl, then return output */
			}
			else{
				(void)putbq(q,bp);
				splx(s);
				return(0);
			}
			break;
		case M_BREAK:
			if (!(tp->t_state & (TTSTOP|BUSY|TIMEOUT))) {
				(void) HW_PROC(tp, T_BREAK);	/* Do break now */
				freemsg(bp);
			}
			else{
				(void)putbq(q,bp);
				splx(s);
				return(0);
			}
			break;
		default:
			freemsg(bp);			/* Ignore junk mail */
			break;
		}
	} /*	part of while loop AMS */
	splx(s);
	return(0);
}

/*
 *	Routine to execute ioctl messages.
*/
iasysrvioc(q, bp)
queue_t *q;			/* Write queue pointer */
mblk_t *bp;			/* Ioctl message pointer */
{	struct strtty *tp;
	struct iocblk *iocbp;
	int arg, s,rq;
	mblk_t *bpr;
	mblk_t *bp1;
	char *argp;
	minor_t dev;
	int	return_val;

	iocbp = (struct iocblk *)bp->b_rptr;
	tp = Q_TO_TP(q);
	dev = IASYUNIT(tp->t_dev);
	switch (iocbp->ioc_cmd) {

#ifdef VPIX
	case AIOCDOSMODE:
	{
		struct v86blk *p_v86;
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasysrvioc: VPIX ioctl 0x%x", 
								iocbp->ioc_cmd));
		if ((tp->t_iflag & DOSMODE) == 0) {       
			p_v86 = (struct v86blk *)bp->b_cont->b_rptr;
			iasystash[iasychan(dev)] = p_v86->v86_p_v86;
			iasy_v86_prp_pd[2*iasychan(dev)] = (int)(p_v86->v86_u_procp);
			iasy_v86_prp_pd[(2*iasychan(dev))+1] = (int)(p_v86->v86_p_pid);
			tp->t_iflag |= DOSMODE;

			/*
			 * DOSMODE should be equal to 
			 *			CLOCAL TRUE
			 *			MODEM INTS ENABLED
			 *			CARR_ON TRUE
			 *	And MIEN should not be allowed off while
			 *      in DOSMODE (see standard ioctl stuff below)
			 *
			 */
			tp->t_cflag |= CLOCAL;	/* No hang up stuff */
			tp->t_state |= CARR_ON;	/* Let data flow */ 

			VM_PROC(tp, AIOCDOSMODE, NULL, NULL);

			/* Program should already have done AIOCINTTYPE.
			 * Assume SERIAL0 if has not.
			 */
			if (iasyintmask[iasychan(dev)] == 0) {
				iasyintmask[iasychan(dev)] = V86VI_SERIAL0;
			}
		}
		iocbp->ioc_rval = 0;
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;
	}
	case AIOCNONDOSMODE:
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasysrvioc: VPIX ioctl 0x%x", 
								iocbp->ioc_cmd));
		if (tp->t_iflag & DOSMODE)
		{       
			iasystash[iasychan(dev)] = 0;
			tp->t_iflag &= ~DOSMODE;
			iasyintmask[iasychan(dev)] = 0;
			iasy_v86_prp_pd[2*iasychan(dev)] = 0;
			iasy_v86_prp_pd[(2*iasychan(dev)) + 1] = 0;
			tp->t_cflag &= ~CLOCAL;	
			VM_PROC(tp, AIOCNONDOSMODE, NULL, NULL);
		}
       	iocbp->ioc_rval = 0;
       	bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;

	case AIOCINFO:
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasysrvioc: VPIX ioctl 0x%x", 
								iocbp->ioc_cmd));
		VM_PROC(tp, AIOCINFO, NULL, NULL);
		iocbp->ioc_rval = ('a' << 8) | (dev);
        bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;


	case AIOCSERIALOUT:
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasysrvioc: VPIX ioctl 0x%x", 
								iocbp->ioc_cmd));
		if ((tp->t_iflag & DOSMODE) && iasystash[iasychan(dev)]) {
                        /* The first byte in bp->b_cont->b_cont indicates */
                        /* the argument for this command which is	  */
                        /* the string we want to copy                     */
			argp = (char *)bp->b_cont->b_rptr;
			VM_PROC(tp, AIOCSERIALOUT, argp, NULL);
		}
		iocbp->ioc_rval = 0;
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;

	case AIOCSERIALIN:
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasysrvioc: VPIX ioctl 0x%x", 
								iocbp->ioc_cmd));
		if ((tp->t_iflag & DOSMODE) && iasystash[iasychan(dev)]) {
			argp = (char *)bp->b_cont->b_rptr;
			VM_PROC(tp, AIOCSERIALIN, argp, NULL);
			rq = fubyte(argp);
			if (rq & SIO_MASK(SI_MSR)) {
                bp->b_datap->db_type = M_IOCACK;
                iocbp->ioc_count = SI_MSR;
                qreply(q, bp);
			}
		}
		iocbp->ioc_rval = 0;
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;

	case AIOCINTTYPE:
		DSTRLOG((16450, 0, 0, SL_TRACE, "iasysrvioc: VPIX ioctl 0x%x", 
								iocbp->ioc_cmd));
		switch (* (int *)bp->b_cont->b_cont->b_rptr)
		{ 
		case V86VI_KBD:
			iasyintmask[iasychan(dev)] = V86VI_KBD;
			break;
		case V86VI_SERIAL1:
			iasyintmask[iasychan(dev)] = V86VI_SERIAL1;
			break;
		case V86VI_SERIAL0:
		default:
			iasyintmask[iasychan(dev)] = V86VI_SERIAL0;
			break;
		}
        VM_PROC(tp, AIOCINTTYPE, NULL, NULL);
        iocbp->ioc_rval = 0;
		bp->b_datap->db_type = M_IOCACK;
       	iocbp->ioc_count = 0;
       	qreply(q, bp);
		return;

	case AIOCSETSS:
	{
		struct termios *termp;

		DSTRLOG((16450, 0, 0, SL_TRACE, "iasysrvioc: VPIX ioctl 0x%x", 
								iocbp->ioc_cmd));
		iasyss[iasychan(dev)] = *(struct termss *)bp->b_cont->b_rptr;
        VM_PROC(tp, AIOCSETSS, NULL, NULL);
		bp->b_datap->db_type = M_IOCACK;
       	iocbp->ioc_count = 0;
       	qreply(q, bp);
		return;
	}
#endif /* VPIX */

	/* The output has drained now. */
	case TCSETAF:
		iasyflush(q, FLUSHR);
		/* FALLTHROUGH */
	case TCSETA:
	case TCSETAW: {
		register struct termio *cb;

		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		cb = (struct termio *)bp->b_cont->b_rptr;
		tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);
		bcopy ((caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCC);

		s = SPL();
		if (HW_PROC(tp, T_PARM)) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		splx(s);
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	}
	case TCSETSF:
		iasyflush(q, FLUSHR);
		/* FALLTHROUGH */
	case TCSETS:
	case TCSETSW:{
		register struct termios *cb;

		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		cb = (struct termios *)bp->b_cont->b_rptr;

		tp->t_cflag = cb->c_cflag;
		tp->t_iflag = cb->c_iflag;
		bcopy ((caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCCS);

#ifdef VPIX
		iasyss[iasychan(dev)].ss_start = tp->t_cc[VSTART];
		iasyss[iasychan(dev)].ss_stop = tp->t_cc[VSTOP];
#endif /* VPIX */

		s = SPL();
		if (HW_PROC(tp, T_PARM)) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		splx(s);
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	}
	case TCGETA: {	/* immediate parm retrieve */
		register struct termio *cb;

		if (bp->b_cont)	/* Bad user supplied parameter */
			freemsg(bp->b_cont);

		if ((bpr = allocb(sizeof(struct termio), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			(void) bufcall((ushort)sizeof(struct termio),
							BPRI_MED, iasydelay, (long)tp);
			return;
		}
		bp->b_cont = bpr;
		cb = (struct termio *)bp->b_cont->b_rptr;

		cb->c_iflag = (ushort)tp->t_iflag;
		cb->c_cflag = (ushort)tp->t_cflag;
		bcopy ((caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCC);

		bp->b_cont->b_wptr += sizeof(struct termio);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termio);
		(void) putnext(RD(q), bp);
		break;

	}
	case TCGETS: {	/* immediate parm retrieve */
		register struct termios *cb;

		if (bp->b_cont)	/* Bad user supplied parameter */
			freemsg(bp->b_cont);

		if ((bpr = allocb(sizeof(struct termios), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			(void) bufcall((ushort)sizeof(struct termios),
							BPRI_MED, iasydelay, (long)tp);
			return;
		}
		bp->b_cont = bpr;
		cb = (struct termios *)bp->b_cont->b_rptr;

		cb->c_iflag = tp->t_iflag;
		cb->c_cflag = tp->t_cflag;
		bcopy ((caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCCS);

#ifdef VPIX
		cb->c_cc[VSTART] = iasyss[iasychan(dev)].ss_start;
		cb->c_cc[VSTOP] = iasyss[iasychan(dev)].ss_stop;
#endif /* VPIX */

		bp->b_cont->b_wptr += sizeof(struct termios);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termios);
		(void) putnext(RD(q), bp);
		break;

	}
	case TCSBRK:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		arg = *(int *)bp->b_cont->b_rptr;
		if (arg == 0) {
			s = SPL();
			(void) HW_PROC(tp, T_BREAK);
			splx(s);
		}
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	case SETRTRLVL:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		s = SPL();
		switch (*(int *)bp->b_cont->b_rptr)
		{

		case T_TRLVL1: 
			return_val = HW_PROC(tp, T_TRLVL1);
			break;
		case T_TRLVL2: 
			return_val = HW_PROC(tp, T_TRLVL2);
			break;
		case T_TRLVL3: 
			return_val = HW_PROC(tp, T_TRLVL3);
			break;
		case T_TRLVL4: 
			return_val = HW_PROC(tp, T_TRLVL4);
			break;
		default:
			return_val = 1;
			break;
		}

		if (return_val){
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		splx(s);
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	case EUC_MSAVE:	/* put these here just in case... */
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	default: /* unexpected ioctl type */
		if (( iocbp->ioc_cmd&IOCTYPE) == LDIOC) {
			/*
			 * ACK LDIOC ioctls
			 */
			bp->b_datap->db_type = M_IOCACK;
			bp1 = unlinkb( bp);
			if ( bp1)
				freeb( bp1);
			iocbp->ioc_count = 0;
		} else {
#ifdef MERGE386
			if(merge386enable) {
				DSTRLOG((16450, 0, 0, SL_TRACE, 
							"iasysrvioc: Unknown ioctl 0x%x", iocbp->ioc_cmd));
				if (VM_PROC(tp, COMPPIIOCTL, bp, (char *)(&iocbp->ioc_cmd)))
						return;
			}
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
#endif /* MERGE386  */
		}
		if (canput(RD(q)->q_next) == 1) {
			(void) putnext(RD(q), bp);
		} else {
			(void) putbq(q, bp);
		}
		break;
	}
	return;
}

/*
 *	Flush input and/or output queues
*/
iasyflush(q, cmd)
queue_t *q;			/* Write queue pointer */
register cmd;		/* FLUSHR, FLUSHW, or both */
{	struct strtty *tp;
	int s;

	s = SPL();
	tp = Q_TO_TP(q);
	if (cmd & FLUSHW) {
		flushq(q, FLUSHDATA);
		(void) HW_PROC(tp, T_WFLUSH);
	}
	if (cmd & FLUSHR) {
		q = RD(q);
		(void) HW_PROC(tp, T_RFLUSH);
		flushq(q, FLUSHDATA);
		(void) putctl1(q->q_next, M_FLUSH, FLUSHR);
	}
	splx(s);
}

/*
 * New service procedure.  Pass everything upstream.
 */
iasyisrv(q)
queue_t *q;		/* Read queue pointer */
{	register mblk_t *mp;
	register struct strtty *tp;
	int s;

	tp = Q_TO_TP(q);
	s = SPL();
	while ((mp = getq(q)) != NULL) {
		/*
		 * If we can't put, then put it back if it's not
		 * a priority message.  Priority messages go up
		 * whether the queue is "full" or not.  This should
		 * allow an interrupt in, even if the queue is hopelessly
		 * backed up.
		 */
		if (!canput(q->q_next)) {
			(void) putbq(q, mp);
			splx(s);
			return;
		}
		(void) putnext(q, mp);
		if (tp->t_state & TBLOCK) {
			(void) HW_PROC(tp, T_UNBLOCK);
		}
	}
	splx(s);
}

/* ARGSUSED */
iasyosrv(q)
queue_t *q;		/* Write queue pointer */
{
#ifdef lint
	void iasyctime();
	void iasyhwdep();

	if (iasyinfo.st_rdinit)	/* Bogus references to keep lint happy */
		iasyosrv(q);
	iasyhwdep(q, (mblk_t *)0);
	iasy_ctime((struct strtty *)0, 1);
#endif
	return;
}

/*
 *	Modify your interrupt thread to use this routine instead of l_input.  It
 *	takes the data from tp->t_in, ships it upstream to the line discipline,
 *	and allocates another buffer for tp->t_in.
*/
int
iasy_input(tp, cmd)
struct strtty *tp;		/* Device with input to report */
int cmd;				/* L_BUF or L_BREAK */
{	queue_t *q;
	mblk_t *bp;
	int cnt;

	q = TP_TO_Q(tp);
	if (!q)
		return;
	switch (cmd) {
	case L_BUF:
		cnt = IASY_BUFSZ - tp->t_in.bu_cnt;
		if (cnt && canput(q->q_next)) {
			bp = allocb(IASY_BUFSZ, BPRI_MED);
			if (bp) {	/* pass up old bp contents */
				tp->t_in.bu_bp->b_wptr += cnt;
				tp->t_in.bu_bp->b_datap->db_type = M_DATA;
				(void) putnext(q, tp->t_in.bu_bp);
				tp->t_in.bu_bp = bp;
				tp->t_in.bu_cnt = IASY_BUFSZ;	/* Reset to go again */
				tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
			}else{
				return(1);
			}
		}else{
			return(cnt);
		}
		return(0);
	case L_BREAK:
		(void) putctl(q->q_next, M_BREAK);	/* signal "break detected" */
		break;
	default:
		cmn_err(CE_WARN, "iasy_input: unknown command\n");
	}
}

/*
 *	Modify your interrupt thread to use this routine instead of l_output.
 *	It retrieves the next output block from the stream and hooks it into
 *	tp->t_out.
*/
int
iasy_output(tp)
struct strtty *tp;		/* Device desiring to get more output */
{	queue_t *q;
	mblk_t *bp;

	if (tp->t_out.bu_bp) {
		freeb((mblk_t *)tp->t_out.bu_bp);	/* As stashed by previous call */
		tp->t_out.bu_bp = 0;
		tp->t_out.bu_ptr = 0;
		tp->t_out.bu_cnt = 0;
	}
	q = TP_TO_Q(tp);
	if (!q)
		return(0);
	q = WR(q);
	bp = iasygetoblk(q);
	if (bp) {
		/*
		 *	Our put procedure insures each message consists of one
		 *	block.  Give the block to the user.
		*/
		tp->t_out.bu_ptr = bp->b_rptr;
		tp->t_out.bu_cnt = bp->b_wptr - bp->b_rptr;
		tp->t_out.bu_bp = bp;
		return(CPRES);
	}

	if ((q->q_first == NULL) && (tp->t_state & TTIOW)) {
		tp->t_state &= ~TTIOW;
		if (tp->t_dstat){
			untimeout(tp->t_dstat);
			tp->t_dstat = 0;
		}
		(void) wakeup((caddr_t)&tp->t_oflag);
	}
	return(0);
}

/*
 *	Register a terminal server.  This makes an interrupt thread
 *	available via the iasy major number.
*/
int
iasy_register(fmin, count, proc, hwdep, getchar, putchar, vmproc)
minor_t fmin; 		/* Starting minor number */
int count;			/* Number of minor numbers requested */
int  (*proc)();		/* proc routine */
void (*hwdep)();	/* Hardware dependant ioctl routine */
int (*getchar)();
void (*putchar)();
int  (*vmproc)();		/* Do VPIX or MERGE stuff */
{	struct iasy_hw *hp;
	minor_t i;
	minor_t lmin;

	if (count == 0)
		return(-1);
	lmin = fmin + count - 1;
	/*
	 *	Scan for allocation problems
	*/
	hp = iasy_hw + fmin;
	for (i = fmin; i <= lmin; i++, hp++) {
		if (i >= iasy_num) {
			cmn_err(CE_WARN,
					"iasy_register: minor %d is out of range\n", i);
			return(-1);
		}
		if (hp->proc) {
			cmn_err(CE_WARN,
					"iasy_register: minor %d conflict  0x%x vs 0x%x\n",
					i, hp->proc, proc);
			return(-1);
		}
	}
	/*
	 *	Allocate the range of minor numbers
	*/
	hp = iasy_hw + fmin;
	for (i = fmin; i <= lmin; i++, hp++) {
		hp->proc = proc;
		hp->hwdep = hwdep;
		hp->vmproc = vmproc;
	}

	if (getchar != (int (*)())NULL){
		iasygetcharp = getchar;
	}
	if (putchar != (void (*)())NULL){
		iasyputcharp = putchar;
	}

/*
	if (iasy_cnt < lmin)
		iasy_cnt = lmin;
*/
	iasy_cnt = iasy_num;
	return(fmin);
}

/*
 *	Default Hardware dependant ioctl support (i.e. none).
 *	Use this routine as your hwdep() routine if you don't have any
 *	special ioctls to implement.
*/
/* ARGSUSED */
void
iasyhwdep(q, bp)
queue_t *q;	/* Write queue pointer */
mblk_t *bp;	/* This is an ioctl not understood by the DI code */
{	struct iocblk *ioc;

	ioc = (struct iocblk *)bp->b_rptr;
	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		ioc->ioc_error = EINVAL;		/* NACK unknown ioctls */
		ioc->ioc_rval = -1;
		bp->b_datap->db_type = M_IOCNAK;
		(void) putnext(RD(q), bp);
		return;
	default:
		cmn_err(CE_PANIC, "iasyhwdep: illegal message type");
	}
}

/*
 *	Send a hangup upstream to indicate loss of the connection.
*/
void
iasy_hup(tp)
struct strtty *tp;
{	queue_t *q;
	int	s;

	q = TP_TO_Q(tp);
	if (!q)
		return;
	s=SPL();
	iasyflush(WR(q), FLUSHR|FLUSHW);
	(void) putctl(q->q_next, M_HANGUP);
	splx(s);
}

/*
 * 	Do parameter settings now.
 */
void
iasyparam(tp)
struct strtty	*tp;
{	int s;

	s=SPL();
	tp->t_state &= ~TIMEOUT;
	(void) HW_PROC(tp, T_PARM);
	splx(s);
}
/*
 *	Delay "count" character times to allow for devices which prematurely
 *	clear BUSY.
*/
int
iasy_ctime(tp, count)
struct strtty *tp;
int count;
{	register int	oldpri;
	static	int	rate[] = {
		HZ+1,	/* avoid divide-by-zero, as well as unnecessary delay */
		50,
		75,
		110,
		134,
		150,
		200,
		300,
		600,
		1200,
		1800,
		2400,
		4800,
		9600,
		19200,
		38400,
	};
	/*
	 *	Delay 11 bit times to allow uart to empty.
	 *	Add one to allow for truncation and one to
	 *	allow for partial clock tick.
	*/
	count *= 1 + 1 + 11*HZ/rate[tp->t_cflag&CBAUD];
	oldpri = SPL();
	tp->t_state |= TIMEOUT;
	(void) timeout(iasyparam, (caddr_t)tp, count);
	splx(oldpri);
	return(0);
}

int
iasygetchar()
{
	if (iasygetcharp != (int (*)())NULL){
		return((iasygetcharp)());
	}
}

void
iasyputchar(c)
unsigned char	c;
{
	if (iasyputcharp != (void (*)())NULL){
		(iasyputcharp)(c);
	}
}

