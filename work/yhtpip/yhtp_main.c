
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

/*
 * This is the main stream interface module for the YinHe Transmission Control
 * Protocol (YHTP).  Here, we deal with the stream setup and tear-down.  The
 * TPI state machine processing is in yhtp_state.c and the specific I/O packet
 * handling happens in yhtp_input.c and yhtp_output.c 
 */


#define STRNET

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <util/map.h>
#include <svc/errno.h>
#include <proc/signal.h>

#ifdef SYSV
#include <proc/cred.h>
#include <proc/proc.h>
#ifdef SYSV
#include <util/cmn_err.h>
#endif
#endif /* SYSV */

#include <proc/user.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <io/conf.h>
#include <util/debug.h>

#ifdef SYSV
#include <net/transport/tihdr.h>
#else
#include <net/transport/tihdr.h>
#endif SYSV

#include <net/transport/timod.h>
#include <net/yhtpip/nihdr.h>
#include <net/transport/socket.h>
#include <net/transport/socketvar.h>
#include <net/transport/sockio.h>
#include <net/yhtpip/yhif.h>
#include <net/yhtpip/strioc.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhin_pcb.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/yhip_str.h>
#include <net/yhtpip/yhtp.h>
#include <net/yhtpip/yhtp_seq.h>
#include <net/yhtpip/yhtp_timer.h>
#include <net/yhtpip/yhtp_var.h>
#include <net/yhtpip/yhtp_fsm.h>
#include <net/yhtpip/yhtpip.h>
#include <mem/kmem.h>


int             nodev(), yhtpopen(), yhtpclose(), yhtp_deqdata(), yhtpuwput(), yhtpuwsrv();
int             yhtplrput(), yhtplrsrv(), yhtplwsrv();

static struct module_info yhtpm_info[MUXDRVR_INFO_SZ] = {
	YHTPM_ID, "yhtp", 0, 5120, 5120, 1024,	/* IQP_RQ */
	YHTPM_ID, "yhtp", 0, 5120, 5120, 1024,	/* IQP_WQ */
	YHTPM_ID, "yhtp", 0, 5120, 5120, 1024,	/* IQP_HDRQ */
	YHTPM_ID, "yhtp", 0, 5120, 5120, 1024,	/* IQP_MUXRQ */
	YHTPM_ID, "yhtp", 0, 5120, 5120, 1024	/* IQP_MUXWQ */
};
static struct qinit yhtpurinit =
{NULL, yhtp_deqdata, yhtpopen, yhtpclose, NULL, &yhtpm_info[IQP_RQ], NULL};

static struct qinit yhtpuwinit =
{yhtpuwput, yhtpuwsrv, yhtpopen, yhtpclose, NULL, &yhtpm_info[IQP_WQ], NULL};

static struct qinit yhtplrinit =
{yhtplrput, yhtplrsrv, yhtpopen, yhtpclose, NULL, &yhtpm_info[IQP_MUXRQ], NULL};

static struct qinit yhtplwinit =
{NULL, yhtplwsrv, yhtpopen, yhtpclose, NULL, &yhtpm_info[IQP_MUXWQ], NULL};

struct streamtab yhtpinfo = {&yhtpurinit, &yhtpuwinit, &yhtplrinit, &yhtplwinit};

queue_t        *yhtp_qbot;
int             yhtp_index;
extern int      yhtpfastid, yhtpslowid;

extern int	yhtpdprintf;
static int      yhtpinited;

yhtp_seq         yhtp_iss;
struct inpcb    yhtcb;

mblk_t	       *yhtp_dihdr;

/* configurable parameters */
extern unsigned char   yhtp_dev[];	/* bit mask of minor devs */
extern int		nyhtp;
extern int		yhtplinger;

int yhtpdevflag = D_OLD;

#define PLINGER	PZERO+1
#undef min

/*
 * The transport level protocols in the yhnet implementation are very odd
 * beasts.  In particular, they have no real minor number, just a pointer to
 * the inpcb struct. 
 */

/* ARGSUSED */
yhtpopen(q, dev, flag, sflag)
	queue_t        *q;
{
	mblk_t         *bp;
	struct stroptions *sop;
	short           error;
	struct inpcb   *inp;
	register short  i, j, n;
	int             min;

	STRLOG(YHTPM_ID, 1, 9, SL_TRACE, "yhtpopen: wq %x dev %x", WR(q), dev);

	if (!yhtpinited && (yhtpinit(), !yhtpinited))
		return (OPENFAIL);
	if (sflag == CLONEOPEN) {
		n = (nyhtp + 7) / 8;
		for (i = 0; i < n; i++) {
			if (yhtp_dev[i] != 0xFF) {
				break;
			}
		}
		if (i == n) {
			setuerror(ENXIO);
			return (OPENFAIL);
		}
		for (j = 0; j < 8; j++) {
			if ((yhtp_dev[i] & (1 << j)) == 0) {
				break;
			}
		}
		min = (i * 8) + j;
	} else {
		if (q->q_ptr)
			return 0;
		setuerror(EINVAL);
		return (OPENFAIL);
	}

	if (error = yhtp_attach(q)) {
		setuerror((unsigned short) error);
		return (OPENFAIL);
	}
	/*
	 * Set up the correct stream head flow control parameters 
	 */
	while ((bp = allocb(sizeof(struct stroptions), BPRI_HI)) == NULL)
#if (ATT > 30) || (INTEL > 30)
		if (strwaitbuf(sizeof(struct stroptions), BPRI_HI, 3)) {
#else
		if (strwaitbuf(sizeof(struct stroptions), BPRI_HI)) {
#endif
			STRLOG(YHTPM_ID, 1, 2, SL_TRACE,
			       "yhtpopen failed: no memory for stropts");
			yhtp_freespc(qtoyhtpcb(q));
			return (OPENFAIL);
		}
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof(struct stroptions);
	sop = (struct stroptions *) bp->b_rptr;
	sop->so_flags = SO_HIWAT | SO_LOWAT;
	sop->so_hiwat = yhtpm_info[IQP_HDRQ].mi_hiwat;
	sop->so_lowat = yhtpm_info[IQP_HDRQ].mi_lowat;
	putnext(q, bp);
	inp = (struct inpcb *) q->q_ptr;
	inp->inp_minor = min;
#ifdef SYSV
	if (!pm_denied(u.u_cred, P_FILESYS)) {
#else
	if (suser() != 0) {
#endif SYSV
		inp->inp_state |= SS_PRIV;
	} else {
		setuerror(0);	/* suser sets u_error, so clear */
	}
	if (yhtplinger) {
		/*
		 * XXX workaround for bug with losing data on close
		 */
		inp->inp_protoopt |= SO_LINGER;
		inp->inp_linger = yhtplinger;
	} else if ((inp->inp_protoopt & SO_LINGER) && inp->inp_linger == 0)
		inp->inp_linger = YHTP_LINGERTIME;
	STRLOG(YHTPM_ID, 1, 5, SL_TRACE, "yhtpopen succeeded wq %x yhtcb %x",
	       WR(q), inp->inp_ppcb);
	yhtp_dev[i] |= 1 << j;
	return (min);
}

yhtpclose(q)
	queue_t        *q;
{
	struct yhtpcb   *tp;
	struct inpcb   *inp;
	short		saveminor;
	short           i, j;
	extern void     yhlingertimer();
	int		ss;

	ss = splstr();
	ASSERT(q != NULL);
	inp = qtoinp(q);
	STRLOG(YHTPM_ID, 1, 5, SL_TRACE, "yhtpclose: wq %x pcb %x",
	       WR(q), inp);
	ASSERT(inp != NULL);
	ASSERT(inp == qtoinp(WR(q)));
	tp = (struct yhtpcb *) inp->inp_ppcb;
	ASSERT(tp->t_inpcb == inp);
	saveminor = tp->t_inpcb->inp_minor;
	if (inp->inp_protoopt & SO_ACCEPTCONN) {
		struct yhtpcb   *ctp;

		inp->inp_protoopt &= ~SO_ACCEPTCONN;
		while (ctp = tp->t_q0) {
			yhtpqremque(ctp, 0);
			(void) yhtp_disconnect(ctp);
		}
		while (ctp = tp->t_q) {
			yhtpqremque(ctp, 1);
			(void) yhtp_disconnect(ctp);
		}
	}
	inp->inp_state |= SS_NOFDREF | SS_CANTRCVMORE;
	inp->inp_tstate = TS_UNBND;
	if (tp->t_state > YHTPS_LISTEN)
		tp = yhtp_disconnect(tp);
	else
		tp = yhtp_close(tp, 0);
	if (tp && tp->t_qsize) {
		if (inp->inp_protoopt & SO_LINGER && inp->inp_linger) {
			yhlingerstart(tp);

			/*
			 * In case sleep returns prematurely, which it can,
			 * check that we're still doing the linger boogie.
			 */

			while (tp->t_linger) {
				if (sleep((caddr_t) tp, PLINGER | PCATCH)) {
					/*
					 * Caught signal, so later.
					 */
					tp->t_linger = 0;
					tp->t_timer[YHTPT_LINGER] = 0;
				 	yhtpstat.yhtps_lingerabort++;
					tp->t_qsize = 0;
					break;
				}
			}
		} else {
			tp->t_qsize = 0;
		}
	}
	if (q->q_ptr == (caddr_t) inp) {
		inp->inp_q = NULL;
		q->q_ptr = NULL;
	}
	i = saveminor;
	j = i % 8;
	i = i / 8;
	yhtp_dev[i] &= ~(1 << j);
	splx(ss);
}

/*
 * yhtpuwput is the upper write put routine.  It takes messages from user
 * level for processing.  Protocol requests can fed into the state machine in
 * yhtp_state. 
 */

yhtpuwput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{

	STRLOG(YHTPM_ID, 3, 8, SL_TRACE, "yhtpuwput wq %x pcb %x", q, q->q_ptr);

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		yhtpioctl(q, bp);
		break;

	case M_IOCDATA:
		yhtpiocdata(q, bp);
		break;

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:
		yhtp_state(q, bp);
		break;

	case M_FLUSH:
		/*  
		* When flushing the write queue we must update the transmit
		* queue size stored in the PCB.  
		*/

		if (*bp->b_rptr & FLUSHW) {
			int s;

			ASSERT(qtoinp(q));
			ASSERT(intoyhtpcb(qtoinp(q)));
			s = splstr();
			(intoyhtpcb(qtoinp(q)))->t_qsize = 0;
			flushq(q, FLUSHALL);
			splx(s);
			*bp->b_rptr &= ~FLUSHW;
		}
		if (*bp->b_rptr & FLUSHR)
			qreply(q, bp);
		else
			freemsg(bp);
		break;

	case M_CTL:		/* No control messages understood */
	default:
		freemsg(bp);
		break;
	}
}

yhtpuwsrv(q)
	queue_t        *q;
{
	register struct yhtpcb *tp = qtoyhtpcb(q);

	if (tp == NULL) {
		STRLOG(YHTPM_ID, 3, 1, SL_TRACE,
		       "yhtpuwsrv: null tp; q %x inp %x", q, q->q_ptr);
		return;
	}
	yhtp_io(tp, TF_NEEDOUT, NULL);
}

/*
 * yhtpiocdata handles M_IOCDATA messages for transparent ioctls.
 */
yhtpiocdata(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	struct inpcb   *inp;

	inp = qtoinp(q);
	if (inp)
		switch (inp->inp_iocstate) {
		case INP_IOCS_DONAME:
			yhinet_doname(q, bp);
			break;
			
		default:
			break;
		}
}

yhtpioctl(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	struct iocblk  *iocbp;
	struct sockaddr_in *sin;
	struct inpcb   *inp;

	iocbp = (struct iocblk *) bp->b_rptr;

	switch (iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
		STRLOG(YHTPM_ID, 4, 9, SL_TRACE,
		       "yhtpioctl: linking new provider");
		iocbp->ioc_count = 0;
		if (yhtp_qbot != NULL) {
			iocbp->ioc_error = EBUSY;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(YHTPM_ID, 4, 3, SL_TRACE,
			       "I_LINK failed: yhtp already linked");
			qreply(q, bp);
			return;
		} else {
			struct linkblk *lp;
			struct N_bind_req *bindr;
			mblk_t         *nbp;

			lp = (struct linkblk *) bp->b_cont->b_rptr;
			yhtp_qbot = lp->l_qbot;
			yhtp_index = lp->l_index;
			/* make sure buffer is large enough to hold response */
			if ((nbp = allocb(sizeof(union N_primitives), BPRI_HI))
			    == NULL) {
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(YHTPM_ID, 0, 2, SL_TRACE,
				     "I_LINK failed: Can't alloc bind buf");
				qreply(q, bp);
				return;
			}
			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof(struct N_bind_req);
			bindr = (struct N_bind_req *) nbp->b_rptr;
			bindr->PRIM_type = N_BIND_REQ;
			bindr->N_sap = IPPROTO_YHTP;
			putnext(yhtp_qbot, nbp);
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_error = 0;
			STRLOG(YHTPM_ID, 0, 5, SL_TRACE, "I_LINK succeeded");
			qreply(q, bp);
			return;
		}
	case I_PUNLINK:
	case I_UNLINK:
		{
			struct linkblk *lp;
			mblk_t         *nbp;
			struct N_unbind_req *bindr;

			iocbp->ioc_count = 0;
			lp = (struct linkblk *) bp->b_cont->b_rptr;

			if (yhtp_qbot == NULL) {
				iocbp->ioc_error = ENXIO;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(YHTPM_ID, 0, 3, SL_TRACE,
				    "I_UNLINK: spurious unlink, index = %x",
				       lp->l_index);
				qreply(q, bp);
				return;
			}
			/* Do the IP unbind */

			/* make sure buffer is large enough to hold response */
			if ((nbp = allocb(sizeof(union N_primitives),
					  BPRI_HI)) == NULL) {
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(YHTPM_ID, 0, 2, SL_TRACE,
				       "I_UNLINK: no buf for unbind");
				qreply(q, bp);
				return;
			}
			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof(struct N_unbind_req);
			bindr = (struct N_unbind_req *) nbp->b_rptr;
			bindr->PRIM_type = N_UNBIND_REQ;
			putnext(lp->l_qbot, nbp);
			yhtp_qbot = NULL;
			yhtp_index = 0;
			bp->b_datap->db_type = M_IOCACK;
			STRLOG(YHTPM_ID, 0, 5, SL_TRACE, "I_UNLINK succeeded");
			qreply(q, bp);
			return;
		}

	case SIOCGETNAME:	/* obsolete - replaced by TI_GETMYNAME */
		iocbp->ioc_count = 0;
		inp = qtoinp(q);
		if ((bp->b_cont = allocb(inp->inp_addrlen, BPRI_LO))
		    == NULL) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOSR;
			qreply(q, bp);
			return;
		}
		in_setsockaddr(inp, bp->b_cont);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = inp->inp_addrlen;
		qreply(q, bp);
		return;

	case SIOCGETPEER:	/* obsolete - replaced by TI_GETPEERNAME */
		iocbp->ioc_count = 0;
		inp = qtoinp(q);
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOTCONN;
			qreply(q, bp);
			return;
		}
		if ((bp->b_cont = allocb(inp->inp_addrlen, BPRI_LO))
		    == NULL) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOSR;
			qreply(q, bp);
			return;
		}
		in_setpeeraddr(inp, bp->b_cont);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = inp->inp_addrlen;
		qreply(q, bp);
		return;

	case INITQPARMS:
		if (iocbp->ioc_error = initqparms(bp, yhtpm_info, MUXDRVR_INFO_SZ))
			bp->b_datap->db_type = M_IOCNAK;
		else
			bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		return;

	case TI_GETMYNAME:
		/* 
		 * yhinet_doname sets and clears inp_iocstate, so we 
		 * don't have to.
		 */
		yhinet_doname(q, bp);
		return;

	case TI_GETPEERNAME:
		inp = qtoinp(q);
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOTCONN;
			qreply(q, bp);
			return;
		}
		yhinet_doname(q, bp);
		return;
		
	default:
		if (yhtp_qbot == NULL) {
			iocbp->ioc_error = ENXIO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(YHTPM_ID, 4, 3, SL_TRACE,
			       "yhtpioctl: not linked");
			iocbp->ioc_count = 0;
			qreply(q, bp);
			return;
		}
		if (msgblen(bp) < sizeof(struct iocblk_in)) {
			if (bpsize(bp) < sizeof(struct iocblk_in)) {
				mblk_t         *nbp;

				nbp = allocb(sizeof(struct iocblk_in), BPRI_MED);
				if (!nbp) {
					iocbp->ioc_error = ENOSR;
					bp->b_datap->db_type = M_IOCNAK;
					STRLOG(YHTPM_ID, 4, 3, SL_TRACE,
					  "yhtpioctl: can't enlarge iocblk");
					qreply(q, bp);
					return;
				}
				bcopy((caddr_t)bp->b_rptr, (caddr_t)nbp->b_rptr, sizeof(struct iocblk));
				nbp->b_cont = bp->b_cont;
				nbp->b_datap->db_type = bp->b_datap->db_type;
				freeb(bp);
				bp = nbp;
				iocbp = (struct iocblk *) bp->b_rptr;
			}
			bp->b_wptr = bp->b_rptr + sizeof(struct iocblk_in);
		}
		((struct iocblk_in *) iocbp)->ioc_transport_client = RD(q);
		putnext(yhtp_qbot, bp);
		return;
	}
}

yhtpinit()
{
	struct T_data_ind *di;

	STRLOG(YHTPM_ID, 0, 9, SL_TRACE, "yhtpinit starting");


	/* allocate header for T_DATA_IND messages */
	if (!(yhtp_dihdr = allocb(sizeof(struct T_data_ind), BPRI_HI))) {
		setuerror(ENOSR);
		return;
	}
	yhtp_dihdr->b_datap->db_type = M_PROTO;
	yhtp_dihdr->b_wptr += sizeof(struct T_data_ind);
	di = (struct T_data_ind *) yhtp_dihdr->b_rptr;
	di->PRIM_type = T_DATA_IND;
	di->MORE_flag = 0;

	yhtp_iss = 1;		/* wrong */
	yhtcb.inp_next = yhtcb.inp_prev = &yhtcb;

	yhtp_slowtimo();
	yhtp_fasttimo();

	ipinit();
	ipregister();
	yhtpinited = 1;

	STRLOG(YHTPM_ID, 0, 5, SL_TRACE, "yhtpinit succeeded");
}

/*
 * yhtplrput is the lower read put routine.  It takes packets and examines
 * them.  Control packets are dealt with right away and data packets are
 * queued for yhtp_input to deal with.  The message formats understood by the
 * M_PROTO messages here are those used by the link level interface (see
 * dlpi.h). 
 */

yhtplrput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	union N_primitives *op;
	mblk_t         *head;

	switch (bp->b_datap->db_type) {

	case M_PROTO:
	case M_PCPROTO:
		op = (union N_primitives *) bp->b_rptr;
		switch (op->prim_type) {
		case N_INFO_ACK:
			STRLOG(YHTPM_ID, 4, 5, SL_TRACE, "Got Info ACK?");
			freemsg(bp);
			break;

		case N_BIND_ACK:
			STRLOG(YHTPM_ID, 0, 5, SL_TRACE, "got bind ack");
			freemsg(bp);
			break;

		case N_ERROR_ACK:
			STRLOG(YHTPM_ID, 3, 3, SL_TRACE,
			       "ERROR ACK: prim = %d, net error = %d, unix error = %d",
			       op->error_ack.ERROR_prim,
			       op->error_ack.N_error,
			       op->error_ack.UNIX_error);
			freemsg(bp);
			break;

		case N_OK_ACK:
			STRLOG(YHTPM_ID, 3, 8, SL_TRACE,
			       "Got OK ack, prim = %x",
			       op->ok_ack.CORRECT_prim);
			freemsg(bp);
			break;

		case N_UNITDATA_IND:
			if (yhtpdprintf)
#ifdef SYSV
				cmn_err(CE_NOTE, "yhtplrput: got N_UNITDATA_IND\n");
#else
				printf( "yhtplrput: got N_UNITDATA_IND\n");
#endif
			head = bp;
			bp = bp->b_cont;
			freeb(head);
			putq(q, bp);
			break;

		case N_UDERROR_IND:
			STRLOG(YHTPM_ID, 2, 1, SL_TRACE,
			       "IP level error, type = %x",
			       op->error_ind.ERROR_type);
			yhtp_uderr(bp);
			freemsg(bp);
			break;

		default:
			STRLOG(YHTPM_ID, 3, 9, SL_ERROR,
			   "yhtplrput: unrecognized prim %d", op->prim_type);
			freemsg(bp);
			break;
		}
		break;

	case M_IOCACK:
	case M_IOCNAK:{
			struct iocblk_in *iocbp = (struct iocblk_in *) bp->b_rptr;

			if (yhtpdprintf)
#ifdef SYSV
				cmn_err(CE_NOTE, "yhtplrput: got M_IOCACK/NAK\n");
#else
				printf( "yhtplrput: got M_IOCACK/NAK\n");
#endif
			putnext(iocbp->ioc_transport_client, bp);
			break;
		}

	case M_CTL:
		if (yhtpdprintf)
#ifdef SYSV
			cmn_err(CE_NOTE, "yhtplrput: got M_CTL\n");
#else
			printf( "yhtplrput: got M_CTL\n");
#endif
		yhtp_ctlinput(bp);
		freemsg(bp);
		break;

	case M_FLUSH:
		/*
		 * Flush read queue free msg (can't route upstream) 
		 */
		STRLOG(YHTPM_ID, 4, 5, SL_TRACE, "Got flush message type = %x",
		       *bp->b_rptr);
		if (*bp->b_rptr & FLUSHR)
			flushq(q, FLUSHALL);
		if (*bp->b_rptr & FLUSHW) {
			*bp->b_rptr &= ~FLUSHR;
			flushq(WR(q), FLUSHALL);
			qreply(q, bp);
		} else
			freemsg(bp);
		return;

	default:
		STRLOG(YHTPM_ID, 3, 9, SL_ERROR,
		"yhtplrput: unexpected block type %d", bp->b_datap->db_type);
		freemsg(bp);
		break;
	}
}

/*
 * yhtplrsrv feeds mblks to yhtp_input. 
 */

yhtplrsrv(q)
	queue_t        *q;
{
	mblk_t         *bp;

	ASSERT(q == RD(yhtp_qbot));
	while (bp = getq(q)) {
		yhtp_linput(q, bp);
	}
}

/*
 * yhtplwsrv will only be called to back enable the queues after flow control
 * blockage from below.
 */

/*ARGSUSED*/
yhtplwsrv(q)
	queue_t        *q;
{
#ifdef lint
	q = q;
#endif
}
