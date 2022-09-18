/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

#define STRNET
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysmacros.h>
#include <net/transport/socket.h>
#include <svc/errno.h>
#include <net/transport/sockio.h>
#include <io/conf.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/log/log.h>
#include <io/strlog.h>
#include <net/dlpi.h>
#include <net/yhtpip/yhif.h>
#include <net/yhtpip/yhif_arp.h>
#include <net/yhtpip/strioc.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhif_ether.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhip.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhip_str.h>
#include <net/yhtpip/yharp.h>
#include <net/yhtpip/clnpapp.h>

struct yharptab *yharptnew();
char *yhether_sprintf();

/* configurable parameters */
extern struct yharptab		yharptab[];
extern ether_addr_t yhrouteaddr;
extern struct yhroute yhroutetab[10];
extern int			yhroutep;
extern int			yharptab_bsiz;
extern int			yharptab_nb;
extern int			yharptab_size;

#define	SPLNULL		(-1)	/* unused spl level */

/*
 * ARP trailer negotiation.  Trailer protocol is not IP specific, but ARP
 * request/response use IP addresses. 
 */

#define	ARPTAB_HASH(a) \
	((u_long)(a) % yharptab_nb)

#define	ARPTAB_LOOK(at,addr) { \
	register n; \
	at = &yharptab[ARPTAB_HASH(addr) * yharptab_bsiz]; \
	for (n = 0 ; n < yharptab_bsiz ; n++,at++) \
		if (at->at_iaddr.s_addr == addr) \
			break; \
	if (n >= yharptab_bsiz) \
		at = 0; \
}

#define	ARP_IDLE(a) \
	( ((a)->yharp_qtop == NULL) && !((a)->yharp_flags & ARPF_PLINKED))

/* timer values */
#define	ARPT_AGE	(60*1)	/* aging timer, 1 min. */
#define	ARPT_KILLC	20	/* kill completed entry in 20 mins. */
#define	ARPT_KILLI	3	/* kill incomplete entry in 3 minutes */

ether_addr_t yhetherbroadcastaddr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
ether_addr_t yhnull_etheraddr = {0, 0, 0, 0, 0, 0};

extern struct ifnet loif;
int	yharptimerid;
int	yharpinited;


int             yharpopen(), yharpclose(), yharpuwput(), yharplrput();

static struct module_info yharpm_info[MUXDRVR_INFO_SZ] = {
	YHARPM_ID, "yharp", 0, 8192, 8192, 1024,
	YHARPM_ID, "yharp", 0, 8192, 8192, 1024,
	YHARPM_ID, "yharp", 0, 8192, 8192, 1024,
	YHARPM_ID, "yharp", 0, 8192, 8192, 1024,
	YHARPM_ID, "yharp", 0, 8192, 8192, 1024
};

static struct qinit yharpurinit =
{NULL, NULL, yharpopen, yharpclose, NULL, &yharpm_info[IQP_RQ], NULL};

static struct qinit yharpuwinit =
{yharpuwput, NULL, yharpopen, yharpclose, NULL, &yharpm_info[IQP_WQ], NULL};

static struct qinit yharplrinit =
{yharplrput, NULL, yharpopen, yharpclose, NULL, &yharpm_info[IQP_MUXRQ], NULL};

static struct qinit yharplwinit =
{NULL, NULL, yharpopen, yharpclose, NULL, &yharpm_info[IQP_MUXWQ], NULL};

struct streamtab yharpinfo = {&yharpurinit, &yharpuwinit, &yharplrinit, &yharplwinit};

extern struct yhapp_pcb yhapp_pcb[];
struct yharp_pcb yharp_pcb[N_ARP];

#define	ARPF_PLINKED	0x1		/* persistent links */

int yharpdevflag = D_OLD;

/*
 * Below is the code implementing the actual Address Resolution Protocol
 * which is spurred by failed yharpresolve requests.  No one can ever actually
 * use yharp from user level, so all the put routine supports is an ioctl
 * interface. 
 */

yharpinit()
{
	yhipregister();
	yharpinited = 1;
}


/* ARGSUSED */
yharpopen(q, dev, flag, sflag)
	queue_t        *q;
{
	mblk_t *bp;
	register struct yharp_pcb *ar;

	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "yharp open called");
	if (!yharpinited)
		yharpinit();
	dev = minor(dev);
	if (sflag == CLONEOPEN) {
		for (dev = 0; dev < N_ARP; dev++) {
			if (ARP_IDLE(&yharp_pcb[dev])) {
				break;
			}
		}
	}
	if (dev < 0 || dev >= N_ARP) {
		return (OPENFAIL);
	}
	ar = (struct yharp_pcb *) &yharp_pcb[dev];
	if (ARP_IDLE(ar)) {
		ar->yharp_qtop = q;
		ar->yharp_qbot = NULL;
		q->q_ptr = (char *) ar;
		WR(q)->q_ptr = (char *) ar;
	} else if (q != ar->yharp_qtop) {
		return (OPENFAIL);
	}
	/*
	 * Set up the correct stream head flow control parameters 
	 */
	if (bp = allocb(sizeof(struct stroptions), BPRI_MED)) {
		struct stroptions *sop = (struct stroptions *) bp->b_rptr;

		bp->b_datap->db_type = M_SETOPTS;
		bp->b_wptr += sizeof(struct stroptions);
		sop->so_flags = SO_HIWAT | SO_LOWAT;
		sop->so_hiwat = yharpm_info[IQP_HDRQ].mi_hiwat;
		sop->so_lowat = yharpm_info[IQP_HDRQ].mi_lowat;
		putnext(q, bp);
	}
	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "open succeeded");
	return (dev);
}

yharpclose(q)
	queue_t        *q;
{
	register struct yharp_pcb *ar;
	register int    i;

	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "yharp close called");
	for (i = 0; i < N_ARP; i++) {
		if (yharp_pcb[i].yharp_qtop == q) {
			ar = (struct yharp_pcb *) &yharp_pcb[i];
			ar->yharp_qtop = NULL;
			if (ar->yharp_flags & ARPF_PLINKED)
				continue;
			if (ar->yhapp_pcb) {
				ar->yhapp_pcb->yharp_pcb = NULL;
				ar->yhapp_pcb = NULL;
			}
			ar->yharp_uname[0] = NULL;
		}
	}
	if (!(ar->yharp_flags & ARPF_PLINKED)) {
        	q->q_ptr = NULL;
		WR(q)->q_ptr = NULL;
		flushq(WR(q), 1);
	}
	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "close succeeded");
	return;
}


/*
 * Handle downstream requests.  At the moment, this only means ioctls, since
 * we don't let the user send yharp requests, etc. 
 */

yharpuwput(q, bp)
	queue_t        	*q;
	mblk_t         	*bp;
{
	struct linkblk	*lp;
	dl_bind_req_t	*bindr;
	dl_unbind_req_t *unbindr;
	mblk_t         	*nbp;
	struct iocblk  	*iocbp;
	struct ifreq   	*ifr;
	struct yharp_pcb 	*ar;
	register int	i;
	int		s;

	if (bp->b_datap->db_type != M_IOCTL) {
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "Bad request for ARP, %d\n",
		       bp->b_datap->db_type);
		freemsg(bp);
		return;
	}
	ar = (struct yharp_pcb *) q->q_ptr;
	iocbp = (struct iocblk *) bp->b_rptr;
	if (msgblen(bp) >= sizeof (struct iocblk_in))
		/* It probably came from from IP.  Pass our flags back up. */
		((struct iocblk_in *) iocbp)->ioc_ifflags |= IFF_BROADCAST;

	switch (iocbp->ioc_cmd) {
	case I_PLINK:
		ar->yharp_flags |= ARPF_PLINKED;
		/* no break */
	case I_LINK:
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = 0;
		iocbp->ioc_count = 0;

		/*
		 * If we've already used this bottom, clone a new pcb. 
		 */

		if (ar->yharp_qbot != NULL) {
			for (i = 0; i < N_ARP; i++) {
				if (ARP_IDLE(&yharp_pcb[i])) {
					break;
				}
			}
			if (i == N_ARP) {
				ar->yharp_flags &= ~ARPF_PLINKED;
				iocbp->ioc_error = EBUSY;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(ARPM_ID, 0, 9, SL_TRACE,
				       "I_LINK failed: No free devices");
				qreply(q, bp);
				return;
			}
			yharp_pcb[i].yharp_qtop = ar->yharp_qtop;
			ar = &yharp_pcb[i];
		}
		lp = (struct linkblk *) bp->b_cont->b_rptr;
		ar->yharp_qbot = lp->l_qbot;
		ar->yharp_qbot->q_ptr = (char *) ar;
		OTHERQ(ar->yharp_qbot)->q_ptr = (char *) ar;
		ar->yharp_index = lp->l_index;
		if ((nbp = allocb(sizeof(dl_bind_req_t), BPRI_HI))
		    == NULL) {
			ar->yharp_flags &= ~ARPF_PLINKED;
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "I_LINK failed: Can't alloc bind buf");
			qreply(q, bp);
			return;
		}
		nbp->b_datap->db_type = M_PROTO;
		nbp->b_wptr += sizeof(dl_bind_req_t);
		bindr = (dl_bind_req_t *) nbp->b_rptr;
		bindr->dl_primitive = DL_BIND_REQ;
		bindr->dl_max_conind = 0;
		bindr->dl_service_mode = DL_CLDLS;
		bindr->dl_conn_mgmt = 0;
		bindr->dl_sap = YHARP_SAP;
		putnext(ar->yharp_qbot, nbp);
		bp->b_datap->db_type = M_IOCACK;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "I_LINK succeeded");
		qreply(q, bp);
		return;

	case I_PUNLINK:
		ar->yharp_flags &= ~ARPF_PLINKED;
		/* no break */
	case I_UNLINK:
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = 0;
		iocbp->ioc_count = 0;
		lp = (struct linkblk *) bp->b_cont->b_rptr;

		for (i = 0; i < N_ARP; i++) {
			if (!ARP_IDLE(&yharp_pcb[i]) &&
			    yharp_pcb[i].yharp_index == lp->l_index) {
				ar = (struct yharp_pcb *) &yharp_pcb[i];
				break;
			}
		}
		if (i == N_ARP || ar->yharp_qbot == NULL) {
			iocbp->ioc_error = EPROTO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "I_UNLINK: bad unlink req");
			qreply(q, bp);
			return;
		}
		/* Do the link level unbind */

		if ((nbp = allocb(sizeof(dl_unbind_req_t),
				  BPRI_HI)) == NULL) {
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "I_UNLINK: no buf for unbind");
			qreply(q, bp);
			return;
		}
		nbp->b_datap->db_type = M_PROTO;
		nbp->b_wptr += sizeof(dl_unbind_req_t);
		unbindr = (dl_unbind_req_t *) nbp->b_rptr;
		unbindr->dl_primitive = DL_UNBIND_REQ;
		putnext(lp->l_qbot, nbp);
		ar->yharp_qbot = NULL;
		ar->yharp_index = 0;
		ar->yharp_flags &= ~ARPF_PLINKED;
		bp->b_datap->db_type = M_IOCACK;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "I_UNLINK succeeded");
		qreply(q, bp);
		return;
	case SIOCSARP:
	case SIOCDARP:
	case SIOCGARP:
		yharpioctl(q, bp);
		return;
	case SIOCSROUTETAB:
		yharprouteset(q, bp);
		return;
	case SIOCSIFNAME:
		ifr = (struct ifreq *) bp->b_cont->b_rptr;
		strcpy(ar->yharp_uname, ifr->ifr_name);
		for (i = 0; i < N_ARP; i++) {
			if (yhapp_pcb[i].yhapp_q &&
			    !strcmp(yhapp_pcb[i].yhapp_uname, ar->yharp_uname)) {
				yhapp_pcb[i].yharp_pcb = ar;
				ar->yhapp_pcb = &yhapp_pcb[i];
			}
		}
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		s = splstr();
		if (ar->yharp_saved) {
			bp = ar->yharp_saved;
			ar->yharp_saved = NULL;
			yhin_arpinput(ar, bp);
		}
		splx(s);
		break;

	case INITQPARMS:
		if (iocbp->ioc_error = initqparms(bp, yharpm_info, MUXDRVR_INFO_SZ))
			bp->b_datap->db_type = M_IOCNAK;
		else
			bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		return;

	default:
		/*
		 * Send everything else downstream. 
		 */
		if (ar->yharp_qbot) {
			putnext(ar->yharp_qbot, bp);
		} else {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
		}
		break;

	}
}

/*
 * yharplrput accepts packets from downstream, and updates the ARP tables
 * yhapproriately and generates responses to lower requests.  
 */

yharplrput(q, bp)
	queue_t        *q;
	register mblk_t *bp;
{
	union DL_primitives *prim;
	register struct hdrarp *ah;
	register struct yharp_pcb *ar = (struct yharp_pcb *) q->q_ptr;

	switch (bp->b_datap->db_type) {
	case M_FLUSH:
		if (*bp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);
	/* fall through */
	default:
		if (ar->yharp_qtop)
			putnext(ar->yharp_qtop, bp);
		else
			freemsg(bp);
		break;

	case M_CTL:
		freemsg(bp);
		break;

	case M_PCPROTO:
	case M_PROTO:
		prim = (union DL_primitives *) bp->b_rptr;
		switch (prim->dl_primitive) {
		default:
			STRLOG(ARPM_ID, 0, 9, SL_ERROR,
			       "yharplrput: unexpected prim type (%d)", 
			       prim->dl_primitive);
			break;

		case DL_UNITDATA_IND:
			if (ar->yhapp_pcb == NULL) {
				break;
			}
			if (ar->yhapp_pcb->yhapp_ac.ac_if.if_flags & IFF_NOARP) {
				break;
			}
			if (pullupmsg(bp->b_cont, -1) == 0)
				break;
			if (bp->b_cont->b_wptr - bp->b_cont->b_rptr <
			    sizeof(struct hdrarp)) {
				break;
			}
			ah = (struct hdrarp *) bp->b_cont->b_rptr;
			if ( ah->yharp_pid != ARP_PID || ah->yharp_v != ARP_VER)
				break;
			yhin_arpinput(ar,bp);
				return;
			break;
		}
		freemsg(bp);
		break;
	}
}


/*
 * Timeout routine.  Age yharp_tab entries once a minute. 
 */
yharptimer()
{
	register struct yharptab *at;
	register        i;
	int		s;

	yharptimerid = timeout(yharptimer, (caddr_t) 0, ARPT_AGE * HZ);
	at = &yharptab[0];
	s = splstr();	/* just in case yharptimer called < splstr */
	for (i = 0; i < yharptab_size; i++, at++) {
		if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
			continue;
		if (++at->at_timer < (u_char)((at->at_flags & ATF_COM) ?
						ARPT_KILLC : ARPT_KILLI))
			continue;
		/* timer has expired, clear entry */
		yharptfree(at);
	}
	splx(s);
}


yharpresolve(ap, m, desten, usetrailers)
	struct yhapp_pcb *ap;
	mblk_t         *m;
	register u_char *desten;
	int            *usetrailers;
{
	register struct in_addr *destip;
	dl_unitdata_req_t *req = (dl_unitdata_req_t *) m->b_rptr;
	register struct yharpcom *ac = &ap->yhapp_ac;
	register struct yharptab *at;
	int             lna;
	int		s;

	destip = (struct in_addr *) (m->b_rptr + req->dl_dest_addr_offset);
	*usetrailers = 0;
	if (yhin_broadcast(*destip)) {	/* broadcast address */
		bcopy((caddr_t) &yhetherbroadcastaddr, (caddr_t) desten,
		      sizeof(yhetherbroadcastaddr));
		return (1);
	}
	lna = yhin_lnaof(*destip);
	/* if for us, U-turn */
	if (destip->s_addr == ac->ac_ipaddr.s_addr) {
		dl_unitdata_ind_t *ind;
		mblk_t         *hdr;

		hdr = allocb((int) (sizeof(dl_unitdata_ind_t)
				    + 2 * req->dl_dest_addr_length),
			     BPRI_HI);
		if (!hdr) {
			freemsg(m);
			return (0);
		}
		hdr->b_datap->db_type = M_PROTO;
		hdr->b_wptr += sizeof(dl_unitdata_ind_t) +
			2 * req->dl_dest_addr_length;
		ind = (dl_unitdata_ind_t *) hdr->b_rptr;
		ind->dl_primitive = DL_UNITDATA_IND;
		ind->dl_dest_addr_offset = sizeof(dl_unitdata_ind_t);
		ind->dl_dest_addr_length = req->dl_dest_addr_length;
		ind->dl_src_addr_offset = sizeof(dl_unitdata_ind_t) +
			req->dl_dest_addr_length;
		ind->dl_src_addr_length = req->dl_dest_addr_length;
		bcopy((char *) m->b_rptr + req->dl_dest_addr_offset,
		      (char *) hdr->b_rptr + ind->dl_dest_addr_offset,
		      (unsigned) req->dl_dest_addr_length);
		bcopy((char *) m->b_rptr + req->dl_dest_addr_offset,
		      (char *) hdr->b_rptr + ind->dl_src_addr_offset,
		      (unsigned) req->dl_dest_addr_length);
		hdr->b_cont = m->b_cont;
		freeb(m);
		putnext(ap->yhapp_q, hdr);
		/*
		 * The packet has already been sent and freed. 
		 */
		return (0);
	}
	s = splstr();
	ARPTAB_LOOK(at, destip->s_addr);
	if (at == 0) {		/* not found */
		if (ac->ac_if.if_flags & IFF_NOARP) {
			bcopy((caddr_t) &ac->ac_enaddr, (caddr_t) desten, 3);
			desten[3] = (lna >> 16) & 0x7f;
			desten[4] = (lna >> 8) & 0xff;
			desten[5] = lna & 0xff;
			splx(s);
			return (1);
		} else {
			at = yharptnew(destip);
			at->at_hold = m;
			yharpwhohas(ap, destip);
			splx(s);
			return (0);
		}
	}
	at->at_timer = 0;	/* restart the timer */
	if (at->at_flags & ATF_COM) {	/* entry IS complete */
		bcopy((caddr_t) &at->at_enaddr, (caddr_t) desten,
		      sizeof(at->at_enaddr));
		if (at->at_flags & ATF_USETRAILERS)
			*usetrailers = 1;
		splx(s);
		return (1);
	}
	/*
	 * There is an yharptab entry, but no ethernet address response yet.
	 * Replace the held mblk with this latest one. 
	 */
	if (at->at_hold)
		freemsg(at->at_hold);
	at->at_hold = m;
	splx(s);
	yharpwhohas(ap, destip);	/* ask again */
	return (0);
}

yharpgetroute(sin, desten)
	struct in_addr *sin;
	register u_char *desten;
{
	int i;

	for (i=0; i<10; i++){
		if((yhroutetab[i].netid.s_host == sin->s_host) &&
		   (yhroutetab[i].netid.s_net == sin->s_net))
			break;
	}
	if (i < 10){
		bcopy((caddr_t) &yhroutetab[i].routeraddr, (caddr_t) desten,
	      		sizeof(ether_addr_t));
		return (1);
	} else 	return (0);
}

/*
 * Free an yharptab entry. 
 */
yharptfree(at)
	register struct yharptab *at;
{
	register int    s;

	s = splstr();

	if (at->at_hold)
		freemsg(at->at_hold);
	at->at_hold = 0;
	at->at_timer = at->at_flags = 0;
	at->at_iaddr.s_addr = 0;
	splx(s);
}

/*
 * Enter a new address in yharptab, pushing out the oldest entry from the
 * bucket if there is no room. This always succeeds since no bucket can be
 * completely filled with permanent entries (except from yharpioctl when
 * testing whether another permanent entry will fit). 
 */
struct yharptab  *
yharptnew(addr)
	struct in_addr *addr;
{
	register        n;
	u_char		oldest = 0;
	register struct yharptab *at, *ato = NULL;

	static int      first = 1;

	if (first) {
		first = 0;
		yharptimerid = timeout(yharptimer, (caddr_t) 0, HZ);
	}
	at = &yharptab[ARPTAB_HASH(addr->s_addr) * yharptab_bsiz];
	for (n = 0; n < yharptab_bsiz; n++, at++) {
		if (at->at_flags == 0)
			goto out;	/* found an empty entry */
		if (at->at_flags & ATF_PERM)
			continue;
		if (at->at_timer > oldest || !oldest) {
			oldest = at->at_timer;
			ato = at;
		}
	}
	if (ato == NULL)
		return (NULL);
	at = ato;
	yharptfree(at);
out:
	at->at_iaddr = *addr;
	at->at_flags = ATF_INUSE;
	return (at);
}

yharprouteset(q, bp)
	queue_t	       *q;
	mblk_t	       *bp;
{
	struct iocblk  *iocbp;
	int		i, s, error;
	register struct arpreq *ar;
	register struct sockaddr_in *sin;

	if (msgblen(bp->b_cont) != sizeof(struct arpreq)) {
		error = ENXIO;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "yharpcom: bad size for arpreq");
		goto nak;
	}
	ar = (struct arpreq *) bp->b_cont->b_rptr;

	if (( ar->yharp_pa.sa_family != AF_OSI) ||
	    ar->yharp_ha.sa_family != AF_UNSPEC) {
		error = EAFNOSUPPORT;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "yharpcom: bad addr family");
		goto nak;
	}
	sin = (struct sockaddr_in *) & ar->yharp_pa;
	for (i=0; i<10; i++){
		if((yhroutetab[i].netid.s_host == sin->s_host) &&
		   (yhroutetab[i].netid.s_net == sin->s_net))
			break;
	}
	if (i < 10){
		goto ack;
	}
	s = splstr();
	yhroutetab[yhroutep].netid.s_addr = sin->sin_addr.s_addr;
	/* set router's ether_addr */
	bcopy((caddr_t) ar->yharp_ha.sa_data, 
	      (caddr_t) &(yhroutetab[yhroutep].routeraddr),
	         sizeof(yhrouteaddr));
	/* finish set router's addr*/
	yhroutep++;
	if(yhroutep > 9)
		yhroutep = 0;
  
	splx(s);
ack:
	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
	return;
nak:
	splx(s);
	iocbp->ioc_error = error;
	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
	return;
}

yharpioctl(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	int             cmd;
	struct iocblk  *iocbp;
	register struct arpreq *ar;
	register struct yharptab *at;
	register struct sockaddr_in *sin;
	int		error;
	register int    i;
	register int    s = SPLNULL;

	iocbp = (struct iocblk *) bp->b_rptr;
	cmd = iocbp->ioc_cmd;
	if (msgblen(bp->b_cont) != sizeof(struct arpreq)) {
		error = ENXIO;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "yharpcom: bad size for arpreq");
		goto nak;
	}
	ar = (struct arpreq *) bp->b_cont->b_rptr;


	if ((ar->yharp_pa.sa_family != AF_INET &&
	    ar->yharp_pa.sa_family != AF_OSI) ||
	    ar->yharp_ha.sa_family != AF_UNSPEC) {
		error = EAFNOSUPPORT;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "yharpcom: bad addr family");
		goto nak;
	}
	sin = (struct sockaddr_in *) & ar->yharp_pa;
	s = splstr();
	ARPTAB_LOOK(at, sin->sin_addr.s_addr);
	if (at == NULL) {	/* not found */
		if (cmd != SIOCSARP) {
			error = ENXIO;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "yharpcom: no yharptab entry");
			goto nak;
		}
		for (i = 0; i < N_ARP; i++) {
			if (yhinet_netmatch(sin->sin_addr,
					  yhapp_pcb[i].yhapp_ac.ac_ipaddr))
				break;
		}
		if (i == N_ARP) {
			error = ENETUNREACH;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "yharpcom: no such network assigned");
			goto nak;
		}
	}
	switch (cmd) {

	case SIOCSARP:		/* set entry */
		/* set router's ether_addr */
		bcopy((caddr_t) ar->yharp_ha.sa_data, (caddr_t) &yhrouteaddr,
		      sizeof(yhrouteaddr));
		/* finish set router's addr*/

		if (drv_priv(iocbp->ioc_cr) != 0) {
			error = EPERM;
			yhsetuerror(0);
			goto nak;
		}
		if (at == NULL) {
			at = yharptnew(&sin->sin_addr);
			if (ar->yharp_flags & ATF_PERM) {
				/*
				 * never make all entries in a bucket
				 * permanent 
				 */
				register struct yharptab *tat;

				/* try to re-allocate */
				tat = yharptnew(&sin->sin_addr);
				if (tat == NULL) {
					yharptfree(at);
					splx(s);
					iocbp->ioc_error = EADDRNOTAVAIL;
					bp->b_datap->db_type = M_IOCNAK;
					STRLOG(ARPM_ID, 0, 9, SL_TRACE,
					       "yharpcom: can't add entry");
					qreply(q, bp);
					return;
				}
				yharptfree(tat);
			}
		}
		bcopy((caddr_t) ar->yharp_ha.sa_data, (caddr_t) &at->at_enaddr,
		      sizeof(at->at_enaddr));
		at->at_flags = ATF_COM | ATF_INUSE |
			(ar->yharp_flags & (ATF_PERM | ATF_PUBL | ATF_USETRAILERS));
		at->at_timer = 0;
		break;

	case SIOCDARP:		/* delete entry */
		if (drv_priv(iocbp->ioc_cr) != 0) {
			error = EPERM;
			yhsetuerror(0);
			goto nak;
		}
		yharptfree(at);
		break;

	case SIOCGARP:		/* get entry */
		bcopy((caddr_t) &at->at_enaddr, (caddr_t) ar->yharp_ha.sa_data,
		      sizeof(at->at_enaddr));
		ar->yharp_flags = at->at_flags;
		iocbp->ioc_count = sizeof(struct arpreq);
		break;

	default:
		error = EINVAL;
		goto nak;
	}
	splx(s);
	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
	return;
nak:
	if (s != SPLNULL)
		splx(s);
	iocbp->ioc_error = error;
	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
	return;
}

/*
 * Convert Ethernet address to printable (loggable) representation. 
 */
char           *
yhether_sprintf(addr)
	register ether_addr_t *addr;
{
	register u_char *ap = (u_char *)addr;
	register        i;
	static char     yhetherbuf[18];
	register char  *cp = yhetherbuf;
	static char     yhdigits[] = "0123456789abcdef";

	for (i = 0; i < 6; i++) {
		if (*ap > 16)
			*cp++ = yhdigits[*ap >> 4];
		*cp++ = yhdigits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = 0;
	return (yhetherbuf);
}

static int
yhmemcmp(cp1, cp2, n)
	caddr_t         cp1, cp2;
{
	while (n--)
		if (*cp1++ != *cp2++)
			return (n + 1);
	return (0);
}

/*
 * Broadcast an OSIARP packet, asking who has addr on interface ac. 
 */
yharpwhohas(ap, addr)
	struct yhapp_pcb *ap;
	struct in_addr *addr;
{
	register struct yharpcom *ac = &ap->yhapp_ac;
	register mblk_t *mp;
	register struct hdrarp *ea;
	register dl_unitdata_req_t *req;
	int addrlen = ac->ac_addrlen;

	if (ap->yharp_pcb == NULL) {
		return;
	}
	if (!yhmemcmp(&yhnull_etheraddr, &ac->ac_enaddr, sizeof(yhnull_etheraddr))) {
		STRLOG(IPM_ID, 0, 9, SL_ERROR, "null ethernet address, yhapp %x", ap);
		return;
	}
	mp = allocb(sizeof(dl_unitdata_req_t)+addrlen, BPRI_HI);
	if (mp == NULL) {
		return;
	}
	mp->b_cont = allocb(max(ac->ac_mintu,sizeof(struct hdrarp)), BPRI_HI);
	if (mp->b_cont == NULL) {
		freemsg(mp);
		return;
	}
	req = (dl_unitdata_req_t *) mp->b_rptr;
	mp->b_wptr += sizeof(dl_unitdata_req_t) + addrlen;
	mp->b_datap->db_type = M_PROTO;
	req->dl_primitive = DL_UNITDATA_REQ;
	req->dl_dest_addr_length = addrlen;
	req->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
	bcopy((caddr_t) &yhetherbroadcastaddr,
	      (caddr_t) mp->b_rptr + sizeof(dl_unitdata_req_t),
	      sizeof(yhetherbroadcastaddr));
	/* if addrlen is 8 (ethernet address length + 2), copy in type */
	if (addrlen == 8)
		*((ushort *) (mp->b_wptr - 2)) = htons(YHARP_SAP);
	ea = (struct hdrarp *) (
	     mp->b_cont->b_rptr =
	    mp->b_cont->b_datap->db_lim - max(ac->ac_mintu,sizeof(*ea)));
	mp->b_cont->b_wptr = mp->b_cont->b_datap->db_lim;
	bzero ((caddr_t)ea,sizeof(*ea));
	ea->yharp_pid = ARP_PID;
	ea->yharp_v = ARP_VER; 
	ea->yharp_ahln = sizeof(ea->yharp_sha);	/* hardware address length */
	ea->yharp_apln = sizeof(ea->yharp_spa);	/* protocol address length */
	ea->yharp_aop = ARPOP_REQUEST;
        ea->yharp_spa.addrlen = ea->yharp_tpa.addrlen = OSI_ADDR_LEN; 
        ea->yharp_spa.afi = ea->yharp_tpa.afi = OSI_ADDR_AFI;
        *(u_short *)ea->yharp_spa.idi 
	   =* (u_short *)ea->yharp_tpa.idi = htons(OSI_ADDR_IDI);
	ea->yharp_spa.rdi[0] = ea->yharp_tpa.rdi[0] = OSI_ADDR_RDI;
	ea->yharp_spa.rdi[1] = ac->ac_ipaddr.s_net & OSI_ADDR_MASK;
        ea->yharp_spa.sni[1] = ac->ac_ipaddr.s_host;
        ea->yharp_spa.ntn[2] = ac->ac_ipaddr.s_lh;
        ea->yharp_spa.ntn[3] = ac->ac_ipaddr.s_impno;
	
	ea->yharp_tpa.rdi[1] = addr->s_net & OSI_ADDR_MASK;
	ea->yharp_tpa.sni[1] = addr->s_host;
        ea->yharp_tpa.ntn[2] = addr->s_lh;
        ea->yharp_tpa.ntn[3] = addr->s_impno;
	bcopy((caddr_t) &ac->ac_enaddr, (caddr_t) &ea->yharp_sha,
	      sizeof(ea->yharp_sha));
	putnext(ap->yharp_pcb->yharp_qbot, mp);
}

yhin_arpinput(ar, bp)
	struct yharp_pcb *ar;
	mblk_t         *bp;
{
	register struct yharpcom *ac = &ar->yhapp_pcb->yhapp_ac;
	register struct hdrarp *ea;
	register struct yharptab *at = 0;	/* same as "merge" flag */
	mblk_t         *mcopy = 0, *newbp;
	struct in_addr  isaddr, itaddr, myaddr;
	struct osi_addr tmpaddr;
	int             proto, op;
	dl_unitdata_req_t *req;
	int addrlen = ac->ac_addrlen;
	int s = SPLNULL;

	if (!yhmemcmp(&yhnull_etheraddr, &ac->ac_enaddr, sizeof(yhnull_etheraddr))) {
		STRLOG(IPM_ID, 0, 9, SL_ERROR,
			"null ethernet address, yharp %x", ar);
		if (ar->yharp_saved) {
			freemsg(ar->yharp_saved);
		}
		ar->yharp_saved = bp;
		return;
	}
	myaddr = ac->ac_ipaddr;
	ea = (struct hdrarp *) bp->b_cont->b_rptr;
	op = ea->yharp_aop;
	isaddr.s_net = 0x80 | ea->yharp_spa.rdi[1];
	isaddr.s_host = ea->yharp_spa.sni[1];
        isaddr.s_lh = ea->yharp_spa.ntn[2];
        isaddr.s_impno = ea->yharp_spa.ntn[3];
	bcopy((caddr_t)&ea->yharp_tpa, (caddr_t)&tmpaddr,
		sizeof(ea->yharp_tpa));
	itaddr.s_net = 0x80 | ea->yharp_tpa.rdi[1];
	itaddr.s_host = ea->yharp_tpa.sni[1];
        itaddr.s_lh = ea->yharp_tpa.ntn[2];
        itaddr.s_impno = ea->yharp_tpa.ntn[3];
	STRLOG(ARPM_ID, 0, 8, SL_TRACE,
	     "yharp req: spa: %x tpa: %x sha:", isaddr.s_addr, itaddr.s_addr);
	STRLOG(ARPM_ID, 0, 8, SL_TRACE, yhether_sprintf(&ea->yharp_sha));
	if (!yhmemcmp((caddr_t) &ea->yharp_sha, (caddr_t) &ac->ac_enaddr,
		    sizeof(ea->yharp_sha)))
		goto out;	/* it's from me, ignore it. */
	if (!yhmemcmp((caddr_t) &ea->yharp_sha, (caddr_t) &yhetherbroadcastaddr,
		    sizeof(ea->yharp_sha))) {
		STRLOG(IPM_ID, 0, 9, SL_ERROR,
		     "yharp: ether address is broadcast for IP address %x!\n",
		       ntohl(isaddr.s_addr));
		goto out;
	}
	if (isaddr.s_addr == myaddr.s_addr) {
		itaddr = myaddr;
		if (op == ARPOP_REQUEST)
			goto reply;
		goto out;
	}
	s = splstr();
	ARPTAB_LOOK(at, isaddr.s_addr);
	if (at) {
		bcopy((caddr_t) &ea->yharp_sha, (caddr_t) &at->at_enaddr,
		      sizeof(ea->yharp_sha));
		at->at_flags |= ATF_COM;
		if (at->at_hold) {
			yhappwput(WR(ar->yhapp_pcb->yhapp_q), at->at_hold);
			at->at_hold = 0;
		}
	}
	if (at == 0 && itaddr.s_addr == myaddr.s_addr) {
		/* ensure we have a table entry */
		at = yharptnew(&isaddr);
		bcopy((caddr_t) &ea->yharp_sha, (caddr_t) &at->at_enaddr,
		      sizeof(ea->yharp_sha));
		at->at_flags |= ATF_COM;
	}
reply:
	if (op != ARPOP_REQUEST )
			goto out;
	if (itaddr.s_addr == myaddr.s_addr) {
		/* I am the target */
		bcopy((caddr_t) &ea->yharp_sha, (caddr_t) &ea->yharp_tha,
		      sizeof(ea->yharp_sha));
		bcopy((caddr_t) &ac->ac_enaddr, (caddr_t) &ea->yharp_sha,
		      sizeof(ea->yharp_sha));
	} else {
		ARPTAB_LOOK(at, itaddr.s_addr);
		if (at == NULL || (at->at_flags & ATF_PUBL) == 0)
			goto out;
		bcopy((caddr_t) &ea->yharp_sha, (caddr_t) &ea->yharp_tha,
		      sizeof(ea->yharp_sha));
		bcopy((caddr_t) &at->at_enaddr, (caddr_t) &ea->yharp_sha,
		      sizeof(ea->yharp_sha));

	}
	if (s != SPLNULL) {
		splx(s);
		s = SPLNULL;
	}
	bcopy((caddr_t) &ea->yharp_spa, (caddr_t) &ea->yharp_tpa,
	      sizeof(ea->yharp_spa));
	bcopy((caddr_t) &tmpaddr , (caddr_t) &ea->yharp_spa,
	      sizeof(ea->yharp_spa));
	ea->yharp_aop = ARPOP_REPLY;
	newbp = allocb(sizeof(dl_unitdata_req_t) + addrlen,
		       BPRI_MED);
	if (newbp == NULL)
		goto out;
	newbp->b_datap->db_type = M_PROTO;
	newbp->b_wptr += sizeof(dl_unitdata_req_t) + addrlen;
	req = (dl_unitdata_req_t *) newbp->b_rptr;
	newbp->b_cont = bp->b_cont;
	freeb(bp);
	bp = newbp;
	req->dl_primitive = DL_UNITDATA_REQ;
	req->dl_dest_addr_length = addrlen;
	req->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
	bcopy((caddr_t) &ea->yharp_tha,
	      (caddr_t) (bp->b_rptr + sizeof(dl_unitdata_req_t)),
	      sizeof(ea->yharp_tha));
	/* if addrlen is 8 (ethernet address length + 2), copy in type */
	if (addrlen == 8)
		*((ushort *) (newbp->b_wptr - 2)) = htons(YHARP_SAP);

	STRLOG(ARPM_ID, 0, 8, SL_TRACE, "yharp rply: spa: %x sha:", itaddr.s_addr);
	STRLOG(ARPM_ID, 0, 8, SL_TRACE, yhether_sprintf(&ea->yharp_sha));
	putnext(ar->yharp_qbot, bp);
	return;
out:
	if (s != SPLNULL)
		splx(s);
	freemsg(bp);
	return;
}
