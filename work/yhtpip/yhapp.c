
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

#define STRNET
#include <util/types.h>
#include <util/param.h>
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

extern int      yharpinited;

/* yhrouteaddr is the default router's ether_addr */
ether_addr_t yhrouteaddr = {0x8,0x0,0x20,0xa,0x4a,0xfa};

/* ----------wbs----------*/
struct yhroute  yhroutetab[10];
int	yhroutep = 1;

/*
 * The first section implements the stream interface to yhapp 
 */

int             yhappopen(), yhappclose(), yhappwput(), yhapprput();

static struct module_info yhappm_info[MODL_INFO_SZ] = {
	YHAPPM_ID, "yhapp", 0, 8192, 8192, 1024,
	YHAPPM_ID, "yhapp", 0, 8192, 8192, 1024
};

static struct qinit yhapprinit =
{yhapprput, NULL, yhappopen, yhappclose, NULL, &yhappm_info[IQP_RQ], NULL};

static struct qinit yhappwinit =
{yhappwput, NULL, yhappopen, yhappclose, NULL, &yhappm_info[IQP_WQ], NULL};

struct streamtab yhappinfo = {&yhapprinit, &yhappwinit, NULL, NULL};

struct yhapp_pcb yhapp_pcb[N_ARP];
extern struct yharp_pcb yharp_pcb[];

int yhappdevflag = D_OLD;

/*
 * These are the stream module routines for YHAPP processing 
 */

/* ARGSUSED */
yhappopen(q, dev, flag, sflag)
	queue_t        *q;
{
	STRLOG(YHAPPM_ID, 0, 9, SL_TRACE, "yhapp open called");
	if (!yharpinited)
		yharpinit();
	dev = minor(dev);
	if (sflag == MODOPEN) {
		for (dev = 0; dev < N_ARP; dev++) {
			if (yhapp_pcb[dev].yhapp_q == NULL) {
				break;
			}
		}
	}
	if (dev < 0 || dev >= N_ARP) {
		yhsetuerror(ENXIO);
		return (OPENFAIL);
	}

	if (yhapp_pcb[dev].yhapp_q == NULL) {
		yhapp_pcb[dev].yhapp_q = q;
		q->q_ptr = (char *) &yhapp_pcb[dev];
		WR(q)->q_ptr = (char *) &yhapp_pcb[dev];
	} else if (q != yhapp_pcb[dev].yhapp_q) {
		yhsetuerror(EBUSY);
		return (OPENFAIL);
	}
	STRLOG(YHAPPM_ID, 0, 9, SL_TRACE, "open succeeded");
	return (0);
}

yhappclose(q)
	queue_t        *q;
{
	struct yhapp_pcb *ap;

	STRLOG(YHAPPM_ID, 0, 9, SL_TRACE, "yhapp close called");
	ap = (struct yhapp_pcb *) q->q_ptr;
	ap->yhapp_q = NULL;
	if (ap->yharp_pcb) {
		ap->yharp_pcb->yhapp_pcb = NULL;
		ap->yharp_pcb = NULL;
	}
	ap->yhapp_uname[0] = NULL;
	ap->yhapp_ac.ac_ipaddr.s_addr = 0;
	flushq(WR(q), 1);
	STRLOG(YHAPPM_ID, 0, 9, SL_TRACE, "close succeeded");
	return;
}


yhappwput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{

	STRLOG(YHAPPM_ID, 0, 9, SL_TRACE,
	       "yhappuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		yhappioctl(q, bp);
		break;

	case M_PROTO:
	case M_PCPROTO:
		STRLOG(YHAPPM_ID, 0, 9, SL_TRACE, "passing data through yhapp");
		yhapp_doproto(q, bp);
		break;

	case M_CTL:
		freemsg(bp);
		break;

	case M_FLUSH:
		if (*bp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		if (*bp->b_rptr & FLUSHR)
			flushq(RD(q), FLUSHDATA);
		putnext(q, bp);
		break;

	default:
		STRLOG(YHAPPM_ID, 0, 5, SL_ERROR,
		       "yhapp: unexpected type received in wput: %d.\n",
		       bp->b_datap->db_type);
		freemsg(bp);
		break;
	}
}


yhappioctl(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	struct iocblk  *iocbp;
	struct ifreq   *ifr;
	struct yhapp_pcb *ap;
	int             i;

	ap = (struct yhapp_pcb *) q->q_ptr;
	iocbp = (struct iocblk *) bp->b_rptr;
	if (msgblen(bp) >= sizeof (struct iocblk_in))
		/* It probably came from from IP.  Pass our flags back up. */
		((struct iocblk_in *) iocbp)->ioc_ifflags |= IFF_BROADCAST;

	switch (iocbp->ioc_cmd) {

	case SIOCSIFNAME:
		ifr = (struct ifreq *) bp->b_cont->b_rptr;
		strcpy(ap->yhapp_uname, ifr->ifr_name);
		for (i = 0; i < N_ARP; i++) {
			if (yharp_pcb[i].yharp_qbot &&
			    !strcmp(yharp_pcb[i].yharp_uname, ap->yhapp_uname)) {
				yharp_pcb[i].yhapp_pcb = ap;
				ap->yharp_pcb = &yharp_pcb[i];
			}
		}
		putnext(q, bp);
		return;

	case SIOCSIFFLAGS:
		ifr = (struct ifreq *) bp->b_cont->b_rptr;
		ifr->ifr_flags |= IFF_BROADCAST;
		ap->yhapp_ac.ac_if.if_flags = ifr->ifr_flags;
		putnext(q, bp);
		return;

	case SIOCSIFADDR:
		ifr = (struct ifreq *) bp->b_cont->b_rptr;
		ap->yhapp_ac.ac_ipaddr = *SOCK_INADDR(&ifr->ifr_addr);
		break;

	case SIOCGIFMETRIC:
	case SIOCSIFMETRIC:
	case SIOCGIFADDR:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case SIOCGIFDSTADDR:
	case SIOCSIFDSTADDR:
	case SIOCIFDETACH:
		break;

		/*
		 * This will be acked from enet level. 
		 */

	case SIOCGIFFLAGS:
	case IF_UNITSEL:
		putnext(q, bp);
		return;

	case SIOCSARP:
	case SIOCDARP:
	case SIOCGARP:
		yharpioctl(q, bp);
		return;

	case SIOCSROUTETAB:
		yharprouteset(q, bp);

	case INITQPARMS:
		if (iocbp->ioc_error = initqparms(bp, yhappm_info, MODL_INFO_SZ))
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
		putnext(q, bp);
		return;
	}
	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
}

yhapp_doproto(q, bp)
	queue_t		*q;
	mblk_t		*bp;
{
	union DL_primitives	*prim;
	dl_unitdata_req_t	*req;
	mblk_t         	*tempbp;
	int		temp;
	ether_addr_t	enaddr;
	struct in_addr  *destosi;
	struct yhapp_pcb 	*ap = (struct yhapp_pcb *) q->q_ptr;
	int		addrlen = ap->yhapp_ac.ac_addrlen;

	prim = (union DL_primitives *) bp->b_rptr;

	switch (prim->dl_primitive) {
	case DL_UNITDATA_REQ:
		req = (dl_unitdata_req_t *) bp->b_rptr;
		destosi = (struct in_addr *)
			  (bp->b_rptr + req->dl_dest_addr_offset);
		if (!yhinet_netmatch(* destosi,ap->yhapp_ac.ac_ipaddr)){
			if(yharpgetroute(destosi, (char *)&enaddr[0]) == 0){
				break;
			}
/*			bcopy((caddr_t)&yhrouteaddr,
			      (caddr_t)&enaddr[0],sizeof(enaddr));
 */
		} else if (yharpresolve(ap, bp, (char *) &enaddr[0], &temp)
		    == 0) {
			break;
		}
		if (bpsize(bp) < sizeof(dl_unitdata_req_t) +
		    addrlen) {
			tempbp = allocb(sizeof(dl_unitdata_req_t) +
					addrlen, BPRI_HI);
			if (tempbp == NULL) {
				freemsg(bp);
				break;
			}
			tempbp->b_cont = bp->b_cont;
			freeb(bp);
			bp = tempbp;
			bp->b_datap->db_type = M_PROTO;
			req = (dl_unitdata_req_t *) bp->b_rptr;
			req->dl_primitive = DL_UNITDATA_REQ;
		}
		bp->b_wptr = bp->b_rptr +
			sizeof(dl_unitdata_req_t) + addrlen;
		req->dl_dest_addr_length = addrlen;
		req->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
		bcopy((caddr_t) &enaddr[0],
		      (caddr_t) bp->b_rptr + sizeof(dl_unitdata_req_t),
		      sizeof(enaddr));
		/* if addrlen is 8 (ethernet address length+2), copy in type */
		if (addrlen == 8)
			*((ushort *) (bp->b_wptr - 2)) = htons(YHIP_SAP);
		/* fall through ... */
        	
	{	struct 	ip 	*ip;
		struct 	clnp	*clnp;
		struct  osi_addr *dst, *src;
 		mblk_t  * bp0;
		bp0 = allocb(sizeof(struct clnp), BPRI_HI);
		if(bp0 == NULL) {
                       	freemsg(bp);
 			printf("alloc is NULL ");
			return;
          	}
		bp0->b_wptr = bp0->b_rptr + sizeof(struct clnp);
		bp0->b_datap->db_type = bp->b_cont->b_datap->db_type;
		ip = (struct ip*) bp->b_cont->b_rptr;
		clnp = (struct clnp*) bp0->b_rptr;
		bzero((caddr_t)clnp,sizeof(*clnp));
  		dst = (struct osi_addr *)&clnp->clnp_dst;
		src = (struct osi_addr *)&clnp->clnp_src;
		dst->addrlen = src->addrlen = OSI_ADDR_LEN; 
		dst->afi = src->afi = OSI_ADDR_AFI;
		*(u_short *)dst->idi = *(u_short *)src->idi
				     = htons(OSI_ADDR_IDI);
		dst->rdi[0] = src->rdi[0] = OSI_ADDR_RDI;
		dst->rdi[1] = ip->ip_dst.s_net & OSI_ADDR_MASK;
		dst->sni[1] = ip->ip_dst.s_host;
                dst->ntn[2] = ip->ip_dst.s_lh;
                dst->ntn[3] = ip->ip_dst.s_impno;
		src->rdi[1] = ip->ip_src.s_net & OSI_ADDR_MASK;
		src->sni[1] = ip->ip_src.s_host;
		src->ntn[2] = ip->ip_src.s_lh;
		src->ntn[3] = ip->ip_src.s_impno;
 
       		clnp->clnp_p = CLNP_PID;
		clnp->clnp_v = CLNP_VER;
		clnp->clnp_typ = TP_PDU;
		clnp->clnp_hl = sizeof(struct clnp);
		clnp->clnp_ttl = CLNP_MAXTTL;
		clnp->clnp_typ |= CLNP_ER; 
		*(u_short *)clnp->clnp_sl = htons(ip->ip_len + clnp->clnp_hl);
		*(u_short *)clnp->clnp_sum = 0;

		bp0->b_cont = bp->b_cont;
		bp->b_cont = bp0;
	} 
	default:
		putnext(q, bp);
		break;
	}
}

/*
 * Upstream put routine. 
 */

yhapprput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	struct yhapp_pcb *ap = (struct yhapp_pcb *) q->q_ptr;

	switch (bp->b_datap->db_type) {

	case M_PROTO:
	case M_PCPROTO:
		switch (((union DL_primitives *) bp->b_rptr)->dl_primitive) {
		case DL_BIND_ACK:{
				dl_bind_ack_t *b_ack;

				/* on a bind ack, save the ethernet address */
				b_ack = (dl_bind_ack_t *) bp->b_rptr;
				bcopy((caddr_t) bp->b_rptr + 
				      b_ack->dl_addr_offset,
				      (caddr_t) &ap->yhapp_ac.ac_enaddr,
				      sizeof(ap->yhapp_ac.ac_enaddr));
				break;
			}

		case DL_INFO_ACK: {
			dl_info_ack_t *info_ack =
				(dl_info_ack_t *) bp->b_rptr;
			info_ack->dl_max_sdu -= sizeof(struct clnp);
			ap->yhapp_ac.ac_mintu = info_ack->dl_min_sdu;
			ap->yhapp_ac.ac_addrlen = info_ack->dl_addr_length;
			ap->yhapp_ac.ac_mactype = info_ack->dl_mac_type;
			break;
			}
		case DL_UNITDATA_IND: {
			if((bp->b_cont->b_wptr - bp->b_cont->b_rptr) >
			   sizeof(struct clnp)){
				bp->b_cont->b_rptr += sizeof(struct clnp);
			} else{
				mblk_t  *bp0;

				if(pullupmsg(bp->b_cont, sizeof(struct clnp))
				   ==0) {
					freemsg(bp);
                              		printf(" pullupmsg failed !\n");
					return;
				}
                       		bp0 = bp->b_cont;
				bp->b_cont = bp->b_cont->b_cont;
				freeb(bp0);
			}
			break;
			}
		}
		break;

	case M_FLUSH:
		if (*bp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);
		break;
	case M_CTL:
		freemsg(bp);
		return;		/* don't send this message upstream */
	case M_IOCNAK:
		/*
		 * must intercept nak's of SIOCGIFFLAGS, SIOCSIFNAME,
		 * and SIOCSIFFLAGS
		 * since some drivers may not handle ip-specific ioctls
		 */
		if ( ((struct iocblk *)bp->b_rptr)->ioc_cmd == SIOCGIFFLAGS ||
			((struct iocblk *)bp->b_rptr)->ioc_cmd == SIOCSIFNAME ||
			((struct iocblk *)bp->b_rptr)->ioc_cmd == SIOCSIFFLAGS ) {
				bp->b_datap->db_type = M_IOCACK;
				((struct iocblk *)bp->b_rptr)->ioc_error = 0;
		}
		break;
	default:
		break;
	}
	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "yhapp passing data up stream");
	putnext(q, bp);
}
