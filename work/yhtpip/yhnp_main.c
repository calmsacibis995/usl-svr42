
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

/*
 * This is the main stream interface module for the YHNP.
 * This module handles primarily OS interface issues as opposed to the actual
 * protocol isuues which are addressed in yhnp_input.c and yhnp_output.c 
 */

#define STRNET

#include <util/types.h>
#include <util/param.h>
#include <svc/time.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <io/stropts.h>
#include <io/conf.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/yhtpip/nihdr.h>
#include <net/dlpi.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/yhtpip/yhif.h>
#include <net/yhtpip/strioc.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhip_str.h>
#include <net/yhtpip/yhip_var.h>

#ifdef SYSV
#include <util/cmn_err.h>
#endif SYSV

#include <mem/kmem.h>

#define yhipdevnum(q) (((struct ip_pcb *) ((q)->q_ptr)) - yhip_pcb)

int             nodev(), putq(), yhipopen(), yhipclose(), yhipuwput(), yhipuwsrv();
int             yhiplwput(),yhiplwsrv();
int             yhiplrput(), yhipintr();

static struct module_info yhipm_info[MUXDRVR_INFO_SZ]  = {
	YHIPM_ID, "yhip", 0, 8192, 8192, 1024,
	YHIPM_ID, "yhip", 0, 8192, 8192, 1024,
	YHIPM_ID, "yhip", 0, 8192, 8192, 1024,
	YHIPM_ID, "yhip", 0, 8192, 8192, 1024,
	YHIPM_ID, "yhip", 0, 8192, 8192, 1024
};
static struct qinit yhipurinit =
{NULL, NULL, yhipopen, yhipclose, NULL, &yhipm_info[IQP_RQ], NULL};

static struct qinit yhipuwinit =
{yhipuwput, yhipuwsrv, yhipopen, yhipclose, NULL, &yhipm_info[IQP_WQ], NULL};

static struct qinit yhiplrinit =
{yhiplrput, yhipintr, yhipopen, yhipclose, NULL, &yhipm_info[IQP_MUXRQ], NULL};

static struct qinit yhiplwinit =
{yhiplwput, yhiplwsrv, yhipopen, yhipclose, NULL, &yhipm_info[IQP_MUXWQ], NULL};

struct streamtab yhipinfo={&yhipurinit, &yhipuwinit, &yhiplrinit, &yhiplwinit};

extern struct ip_pcb yhip_pcb[];
extern int      yhipcnt, yhipprovcnt;
extern struct ip_provider yhprovider[];
struct ip_provider *yhlastprov;
int             yhipsubusers;
extern int      yhiptimerid;

unsigned char   yhip_protox[IPPROTO_MAX];
struct ipstat   yhipstat;
u_short         yhip_id;		/* ip packet ctr, for ids */

int             yhipinited;

/* configurable parameters */
extern struct ip_pcb yhip_pcb[];
extern struct ip_provider yhprovider[];
extern int yhipcnt;
extern int yhipprovcnt;

static int	yhipversprinted;

int yhipdevflag = D_OLD;

/* ARGSUSED */
yhipopen(q, dev, flag, sflag)
	queue_t        *q;
{
	struct ip_pcb  *lp;
	mblk_t         *bp;
	struct stroptions *sop;

	if (!yhipversprinted) {
		yhipversion();
		yhipversprinted = 1;
	}
	if (!yhipinited && (yhipinit(), !yhipinited))
		return (OPENFAIL);
	dev = minor(dev);
	if (sflag == CLONEOPEN) {
		for (dev = 0; dev < yhipcnt; dev++)
			if (!(yhip_pcb[dev].ip_state & IPOPEN))
				break;
	}
	if ((dev < 0) || (dev >= yhipcnt)) {
		yhsetuerror(ENXIO);
		return (OPENFAIL);
	}
	STRLOG(IPM_ID, 0, 9, SL_TRACE, "yhipopen: opening dev %x", dev);

	/*
	 * Set up the correct stream head flow control parameters 
	 */
	while ((bp = allocb(sizeof(struct stroptions), BPRI_HI)) == NULL)
#if (ATT > 30) || (INTEL > 30)
		if (strwaitbuf(sizeof(struct stroptions), BPRI_HI, 3)) {
#else
		if (strwaitbuf(sizeof(struct stroptions), BPRI_HI)) {
#endif
			STRLOG(YHIPM_ID, 1, 2, SL_TRACE,
			       "yhipopen failed: no memory for stropts");
			return (OPENFAIL);
		}

	lp = &yhip_pcb[dev];
	if (!(lp->ip_state & IPOPEN)) {
		lp->ip_state = IPOPEN;
		lp->ip_rdq = q;
		q->q_ptr = (caddr_t) lp;
		WR(q)->q_ptr = (caddr_t) lp;
	} else if (q != lp->ip_rdq) {
		freeb(bp);
		yhsetuerror(EBUSY);
		return (OPENFAIL);	/* only one stream at a time! */
	}

	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof(struct stroptions);
	sop = (struct stroptions *) bp->b_rptr;
	sop->so_flags = SO_HIWAT | SO_LOWAT;
	sop->so_hiwat = yhipm_info[IQP_HDRQ].mi_hiwat;
	sop->so_lowat = yhipm_info[IQP_HDRQ].mi_lowat;
	putnext(q, bp);
	STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "Ipopen succeeded");
	return (dev);
}

yhipclose(q)
	queue_t        *q;
{
	STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "ipclose: closing dev %x",
	       yhipdevnum(q));
	if (q->q_ptr == NULL) {
#ifdef SYSV
		cmn_err(CE_WARN, "ipclose: null q_qptr");
#else
		printf ("yhipclose: null q_qptr");
#endif SYSV
		return;
	}
	((struct ip_pcb *) (q->q_ptr))->ip_state &= ~IPOPEN;
	((struct ip_pcb *) (q->q_ptr))->ip_rdq = NULL;
	flushq(WR(q), 1);
	q->q_ptr = NULL;
}


/*
 * yhipuwput is the upper write put routine.  It takes messages from transport
 * protocols and decides what to do with them, controls and ioctls get
 * processed here, actual data gets queued and we let yhnp_output handle it. 
 */

yhipuwput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	struct ip_pcb  *pcb = (struct ip_pcb *) q->q_ptr;
	int             i;

	STRLOG(YHIPM_ID,0,9,SL_TRACE, "yhipuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		yhipioctl(q, bp);
		break;

	case M_PROTO:
	case M_PCPROTO:{
			union N_primitives *op;

			op = (union N_primitives *) bp->b_rptr;
			switch (op->prim_type) {
			case N_INFO_REQ:
				op->error_ack.PRIM_type = N_ERROR_ACK;
				op->error_ack.ERROR_prim = N_INFO_REQ;
				op->error_ack.N_error = NSYSERR;
				op->error_ack.UNIX_error = ENXIO;
				qreply(q, bp);
				break;

			case N_BIND_REQ: {
				unsigned long    sap = op->bind_req.N_sap;
				
				for (i = 0; i < yhipcnt; i++) {
					if ((yhip_pcb[i].ip_state & IPOPEN)
					    && (yhip_pcb[i].ip_proto == sap)) {
						op->error_ack.PRIM_type = N_ERROR_ACK;
						op->error_ack.ERROR_prim = N_BIND_REQ;
						op->error_ack.N_error = NBADSAP;
						qreply(q, bp);
						break;
					}
				}
				if (!sap || sap >= IPPROTO_MAX) {
					op->error_ack.PRIM_type
						= N_ERROR_ACK;
					op->error_ack.ERROR_prim
						= N_BIND_REQ;
					op->error_ack.N_error = NBADSAP;
					qreply(q, bp);
					return;
				}
                                if (sap == IPPROTO_RAW) {
                                        for (i=0; i<IPPROTO_MAX; i++)
						if (yhip_protox[i] == yhipcnt)
                                                        yhip_protox[i] =
								yhipdevnum(q);
				} else
					yhip_protox[sap] = yhipdevnum(q);
				pcb->ip_proto = sap;
				op->bind_ack.PRIM_type = N_BIND_ACK;
				op->bind_ack.N_sap = sap;
				op->bind_ack.ADDR_length = 0;
				qreply(q, bp);
				break;
			}

			case N_UNBIND_REQ:
                                if (pcb->ip_proto == IPPROTO_RAW) {
                                        for (i=0; i<IPPROTO_MAX; i++)
                                                if (yhip_protox[i] == yhipdevnum(q))
                                                        yhip_protox[i] = yhipcnt;
				} else
					if (pcb->ip_proto)
						yhip_protox[pcb->ip_proto] =
							yhip_protox[IPPROTO_RAW];
				pcb->ip_proto = 0;
				op->ok_ack.PRIM_type = N_OK_ACK;
				op->ok_ack.CORRECT_prim =
					N_UNBIND_REQ;
				qreply(q, bp);
				break;

			case N_UNITDATA_REQ:
				putq(q, bp);
				break;

			default:
				STRLOG(YHIPM_ID, 3, 5, SL_ERROR,
				       "yhipuwput: unrecognized prim: %d", op->prim_type);
				freemsg(bp);
				break;
			}
		}
		break;

	case M_CTL:
		yhsendctl(bp);
		break;

	case M_FLUSH:
		if (*bp->b_rptr & FLUSHW) {
			flushq(q, FLUSHALL);
			*bp->b_rptr &= ~FLUSHW;
		}
		if (*bp->b_rptr & FLUSHR)
			qreply(q, bp);
		else
			freemsg(bp);
		break;

	default:
		STRLOG(YHIPM_ID, 0, 5, SL_ERROR,
		       "IP: unexpected type received in wput: %d.",
		       bp->b_datap->db_type);
		freemsg(bp);
		break;
	}
}


yhipioctl(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	struct iocblk  *iocbp;
	int             i;
	register struct ip_provider *prov;
	static void	yhifconf();

	iocbp = (struct iocblk *) bp->b_rptr;

	/* screen out routing ioctls */
	if (((iocbp->ioc_cmd >> 8) & 0xFF) == 'r') {
		if (bp->b_cont == NULL) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		iocbp->ioc_error = yhrtioctl(iocbp->ioc_cmd, bp->b_cont);
		bp->b_datap->db_type = iocbp->ioc_error ? M_IOCNAK : M_IOCACK;
		qreply(q, bp);
		return;
	}
	switch (iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
		STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
		       "yhipioctl: linking new provider");
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = 0;
		iocbp->ioc_count = 0;
		for (i = 0; i < yhipprovcnt; i++) {
			if (yhprovider[i].qbot == NULL) {
				prov = &yhprovider[i];
				break;
			}
		}
		if (i == yhipprovcnt) {
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
			       "I_LINK failed: no provider slot available");
			qreply(q, bp);
			return;
		} else {
			struct linkblk	*lp;
			dl_bind_req_t	*bindr;
			mblk_t		*nbp;

			if ((nbp = allocb(sizeof(dl_bind_req_t), BPRI_HI))
			    == NULL) {
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
				     "I_LINK failed: Can't alloc bind buf");
				qreply(q, bp);
				return;
			}

			if (prov > yhlastprov)
				yhlastprov = prov;
			lp = (struct linkblk *) bp->b_cont->b_rptr;
			bzero(&yhprovider[i], sizeof(struct ip_provider));
			prov->qbot = lp->l_qbot;
			prov->qbot->q_ptr = (char *) prov;
			RD(prov->qbot)->q_ptr = (char *) prov;
			prov->l_index = lp->l_index;
			prov->ia.ia_ifa.ifa_addr.sa_family = AF_OSI;
			prov->qtop = q;
			prov->linkbp = bp;
			/* wait to ACK until DL_BIND succeeds */

			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof(dl_bind_req_t);
			bindr = (dl_bind_req_t *) nbp->b_rptr;
			bindr->dl_primitive = DL_BIND_REQ;
			bindr->dl_max_conind = 0;
			bindr->dl_service_mode = DL_CLDLS;
			bindr->dl_conn_mgmt = 0;
			bindr->dl_sap = YHIP_SAP;
			putnext(prov->qbot, nbp);
			return;
		}
	case I_PUNLINK:
	case I_UNLINK:
		{
			struct linkblk	*lp;
			mblk_t		*nbp;
			dl_unbind_req_t	*bindr;

			iocbp->ioc_error = 0;
			iocbp->ioc_rval = 0;
			iocbp->ioc_count = 0;
			lp = (struct linkblk *) bp->b_cont->b_rptr;
			for (prov = yhprovider; prov <= yhlastprov; prov++) {
				if (prov->qbot &&
				    prov->l_index == lp->l_index) {
					break;
				}
			}
			if (prov > yhlastprov) {
				iocbp->ioc_error = ENXIO;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
				    "I_UNLINK: no provider with index = %x",
				       lp->l_index);
				qreply(q, bp);
				return;
			}

			prov->qbot = NULL;
			prov->qtop = q;
			prov->linkbp = bp;
			/* wait to ACK until DL_UNBIND succeeds */

			/* Do the link level unbind */
			if ((nbp = allocb(sizeof(dl_unbind_req_t),
					  BPRI_HI)) == NULL) {
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
				       "I_UNLINK: no buf for unbind");
				qreply(q, bp);
				return;
			}
			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof(dl_unbind_req_t);
			bindr = (dl_unbind_req_t *) nbp->b_rptr;
			bindr->dl_primitive = DL_UNBIND_REQ;
			putnext(lp->l_qbot, nbp);
			return;
		}

	case SIOCGIFCONF:	/* return provider configuration */
		yhifconf(q, bp);
		return;

	case INITQPARMS:
		if (iocbp->ioc_error = yhinitqparms(bp, yhipm_info, MUXDRVR_INFO_SZ))
			bp->b_datap->db_type = M_IOCNAK;
		else
			bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		return;

	default:
		yhin_control(q, bp);
		return;
	}
}

static void
yhifconf(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	struct iocblk  *ioc = (struct iocblk *) bp->b_rptr;
	register struct ifreq *ifr;
	register int    space;
	register struct ip_provider *prov;

	if (bp->b_cont == NULL) {
		bp->b_datap->db_type = M_IOCNAK;
		ioc->ioc_error = EINVAL;
		qreply(q, bp);
		return;
	}
	ifr = (struct ifreq *) bp->b_cont->b_rptr;
	space = msgblen(bp->b_cont);

	for (prov = yhprovider; prov <= yhlastprov && space > sizeof(struct ifreq);
	     prov++) {
		if (prov->qbot == NULL) {
			continue;
		}
		bcopy(prov->name, ifr->ifr_name, sizeof(ifr->ifr_name));
		ifr->ifr_addr = prov->if_addr;
		space -= sizeof(struct ifreq);
		ifr++;
	}
	bp->b_datap->db_type = M_IOCACK;
	bp->b_cont->b_wptr -= space;
	ioc->ioc_count = msgblen(bp->b_cont);
	qreply(q, bp);
}

yhipinit()
{
	register int    i;

	STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "ipinit starting");

	if (yhipinited)
		return;
#ifdef LATER
	if (!yhipcnt)
		yhipcnt = NIP;
	if (!yhipprovcnt)
		yhipprovcnt = IP_PROVIDERS;
	yhip_pcb = (struct ip_pcb *)
		kmem_alloc((int) (yhipcnt * sizeof(struct ip_pcb)), KM_NOSLEEP);
	yhprovider = (struct ip_provider *) kmem_alloc((int) (yhipprovcnt *
					       sizeof(struct ip_provider)),
					       KM_NOSLEEP);
#endif
	yhlastprov = yhprovider;
	if (yhip_pcb == NULL || yhprovider == NULL) {
		/* == NULL if space unavailable */
		STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
		       "yhipinit: Can't get memory for control structures");
		yhsetuerror(ENOSR);
		return;
	}
	bzero(yhprovider, yhipprovcnt * sizeof(struct ip_provider));
	yhip_id = hrestime.tv_sec & 0xffff;
	for (i = 0; i < IPPROTO_MAX; i++) {
		yhip_protox[i] = yhipcnt;
	}
	yhipq_setup();
	yhip_slowtimo();
	yhipinited = 1;
	icmpinit();		/* piggyback module */
	STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "ipinit succeeded");
}

/*
 *yhiplrput is the lower read put routine.  It takes packets and examines
 * them.  Control packets are dealt with right away and data packets are
 * queued for yhnp_input to deal with.  The message formats understood by the
 * M_PROTO messages here are those used by the link level interface (see
 * dlpi.h). 
 */

yhiplrput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	union DL_primitives *op;
	struct ip_provider *prov;

	switch (bp->b_datap->db_type) {
	case M_DATA:		/* Can't send pure data through LLC layer */
		freemsg(bp);
		break;

	case M_IOCACK:		/* ioctl's returning from link layer */
	case M_IOCNAK:
		yhin_upstream(q, bp);
		break;

	case M_PROTO:
	case M_PCPROTO:
		op = (union DL_primitives *) bp->b_rptr;
		prov = (struct ip_provider *) q->q_ptr;
		switch (op->dl_primitive) {
		case DL_INFO_ACK:
			((struct ip_provider *) q->q_ptr)->if_maxtu =
				op->info_ack.dl_max_sdu;
			((struct ip_provider *) q->q_ptr)->if_mintu =
				op->info_ack.dl_min_sdu;
			STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "Got Info ACK");
			freemsg(bp);
			break;

		case DL_BIND_ACK:
			STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "got bind ack");

			if ( !prov || !prov->qtop || !prov->linkbp ) {
				STRLOG(YHIPM_ID, 3, 5, SL_ERROR,
				"yhiplrput: no provider for BIND/LINK ack");
				break;
			}
			/* ack the I_PLINK/I_LINK */
			STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "I_LINK succeeded");
			prov->linkbp->b_datap->db_type = M_IOCACK;
			qreply(prov->qtop, prov->linkbp);
			prov->qtop = NULL;
			prov->linkbp = NULL;

			bp->b_datap->db_type = M_PCPROTO;
			bp->b_wptr = bp->b_rptr + DL_INFO_REQ_SIZE;
			op->dl_primitive = DL_INFO_REQ;
			qreply(q, bp);
			break;

		case DL_ERROR_ACK:
			STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
			       "ERROR ACK: prim = %d, net error = %d, unix error = %d",
			       op->error_ack.dl_error_primitive,
			       op->error_ack.dl_errno,
			       op->error_ack.dl_unix_errno);
			freemsg(bp);

			if (op->error_ack.dl_error_primitive == DL_BIND_REQ &&
			prov && OTHERQ(prov->qbot) == q &&
			prov->qtop && prov->linkbp) {
				prov->linkbp->b_datap->db_type = M_IOCNAK;
				qreply(prov->qtop, prov->linkbp);
				/* undo setup */
				prov->qbot->q_ptr = 0;
				RD(prov->qbot)->q_ptr = 0;
				bzero(prov, sizeof(struct ip_provider));
			}
			else if (op->error_ack.dl_error_primitive ==
			DL_UNBIND_REQ && prov && !prov->qbot &&
			prov->qtop && prov->linkbp) {
				prov->linkbp->b_datap->db_type = M_IOCNAK;
				qreply(prov->qtop, prov->linkbp);
				/* undo teardown */
				prov->qbot = OTHERQ(q);
			}
			break;

		case DL_OK_ACK:
			STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
			       "Got OK ack, prim = %x",
			       op->ok_ack.dl_correct_primitive);
			freemsg(bp);

			if (op->ok_ack.dl_correct_primitive == DL_UNBIND_REQ &&
			prov && !prov->qbot) {
			    /* finish teardown */
			    prov->l_index = 0;
			    if (prov->ia.ia_ifa.ifa_ifs != NULL) {
			        if (prov->ia.ia_ifa.ifa_ifs->ifs_addrs ==
				    &prov->ia.ia_ifa) {
					prov->ia.ia_ifa.ifa_ifs->ifs_addrs =
					    prov->ia.ia_ifa.ifa_next;
				} else {
				    struct ifaddr  *ifa;

				    for (ifa =
					 prov->ia.ia_ifa.ifa_ifs->ifs_addrs;
				    ifa && ifa->ifa_next != &prov->ia.ia_ifa;
				    ifa = ifa->ifa_next)
					continue;
				    if (ifa == NULL) {
				        STRLOG(YHIPM_ID, 0, 9, SL_ERROR,
					    "ifaddr chain corrupt");
				    } else {
					ifa->ifa_next =
					    prov->ia.ia_ifa.ifa_next;
				    }
				}
			    }
			    /* ack the I_PUNLINK/I_UNLINK */
			    if (!prov->qtop || !prov->linkbp) {
				STRLOG(YHIPM_ID, 3, 5, SL_ERROR,
				    "yhiplrput: no provider of UNLINK ack");
				break;
			    }
			    STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
			        "I_UNLINK succeeded");
			    prov->linkbp->b_datap->db_type = M_IOCACK;
			    qreply(prov->qtop, prov->linkbp);
			    prov->qtop = NULL;
			    prov->linkbp = NULL;
			}
			break;

		case DL_UNITDATA_IND:
			putq(q, bp);
			break;

		case DL_UDERROR_IND:
			STRLOG(YHIPM_ID, 0, 9, SL_TRACE,
			       "Link level error, type = %x",
			       op->uderror_ind.dl_errno);
			freemsg(bp);
			break;

		default:
			STRLOG(YHIPM_ID, 3, 5, SL_ERROR,
			   "yhiplrput: unrecognized prim: %d", op->dl_primitive);
			freemsg(bp);
			break;
		}
		break;

	case M_FLUSH:
		/*
		 * Flush read queue free msg (can't route upstream) 
		 */
		STRLOG(YHIPM_ID, 0, 9, SL_TRACE, "Got flush message type = %x",
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

	case M_ERROR: {
		/*
		 * Fatal error - mark interface down
		 */
		struct ip_provider *prov =
			(struct ip_provider *) q->q_ptr;
		prov->if_flags &= ~IFF_UP;
#ifdef SYSV
		cmn_err(CE_NOTE,
		    "IP: Fatal error (%d) on interface %s, marking down.\n",
		    (int) *bp->b_rptr, prov->name);
#else
		printf (
		    "IP: Fatal error (%d) on interface %s, marking down.\n",
		    (int) *bp->b_rptr, prov->name);
#endif SYSV
		freemsg(bp);
		break;
	}
	default:
		STRLOG(YHIPM_ID, 3, 5, SL_ERROR,
		       "IP: unexpected type received in rput: %d.",
		       bp->b_datap->db_type);
		freemsg(bp);
		break;
	}
}

/*
 * sendctl sends control messages (generated by icmp) to any or all of our
 * clients.  It does this by dup'ing the message a whole bunch of times. 
 */

yhsendctl(bp)
	mblk_t         *bp;
{
	mblk_t         *newbp;
	struct ip_ctlmsg *ipctl;
	register int    i;
	queue_t        *qp;

	ipctl = (struct ip_ctlmsg *) bp->b_rptr;
	if (ipctl->proto == -1) {
		for (i = 0; i < yhipcnt; i++) {
			if ((yhip_pcb[i].ip_state & IPOPEN)
			    && (newbp = dupmsg(bp))) {
				putnext(yhip_pcb[i].ip_rdq, newbp);
			}
		}
		freemsg(bp);
	} else {
		if (yhip_protox[ipctl->proto] == yhipcnt) {
			freemsg(bp);
			return;
		}
		qp = yhip_pcb[yhip_protox[ipctl->proto]].ip_rdq;
		if (!canput(qp->q_next)) {
			freemsg(bp);
			return;
		}
		putnext(qp, bp);
	}
}

void
yhipregister()
{
	yhipsubusers++;
}

void
yhipderegister()
{
	yhipsubusers--;
}


#ifndef SYSV
int strlogprintf = 0;

/*VARARGS*/
strlog (mid, sid, level, flags, fmt, a0, a1, a2, a3, a4, a5, a6,a7, a8, a9)
	int mid, sid, level, flags;
	char *fmt;
	char *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	if (strlogprintf) {
		printf (fmt, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		printf ("\n");
	}
}
#endif SYSV
