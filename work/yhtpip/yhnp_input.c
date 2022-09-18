
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

#define STRNET

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>

#include <io/stropts.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>

#include <net/yhtpip/protosw.h>
#include <net/transport/socket.h>
#include <svc/errno.h>
#include <util/cmn_err.h>

#include <net/yhtpip/yhif.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhin_pcb.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhip.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/yhip_icmp.h>
#include <net/yhtpip/yhip_str.h>
#include <net/yhtpip/yhtp.h>
#include <net/yhtpip/insrem.h>

#include <net/yhtpip/nihdr.h>
#include <mem/kmem.h>

extern int			yhipcnt;
extern struct ip_provider	yhprovider[], *yhlastprov, *yhprov_withaddr();
extern struct ip_pcb		yhip_pcb[];
extern struct ip_provider	*yhin_onnetof();
extern unsigned char		yhip_protox[];
extern int			yhin_interfaces;


#define satosin(sa)	((struct sockaddr_in *) (sa))

/*
 * We need to save the YHNP options in case a protocol wants to respond to an
 * incoming packet over the same route if the packet got here using YHNP source
 * routing.  This allows connection establishment and maintenance when the
 * remote end is on a network that is not known to us. 
 */
int             yhip_nhops = 0;
static struct yhip_srcrt {
	char            nop;	/* one NOP to align */
	char            srcopt[IPOPT_OFFSET + 1];	/* OPTVAL, OLEN and
							 * OFFSET */
	struct in_addr  route[MAX_IPOPTLEN];
}               yhip_srcrt;

struct ipq      yhipq;
int             yhiptimerid;
u_char          yhipcksum = 1;
mblk_t         *yhip_reass();
struct route    yhipforward_rt;

/*
 * Initialize the fragmentation/reassembly structures.  Called from ipinit. 
 */

yhipq_setup()
{
	yhipq.next = &yhipq;
	yhipq.prev = &yhipq;
}

int yhipdprintf = 0;

/*
 * Input routine.  Checksum and byte swap header.  If fragmented try to
 * reassamble.  If complete and fragment queue exists, discard. Process
 * options.  Pass to next level. 
 */
yhipintr(q)
	queue_t        *q;
{
	register mblk_t *bp, *fbp;
	register struct ip *ip;
	register struct ipq *fp;
	mblk_t         *bp0;
	int             hlen, i;
	queue_t        *qp;	/* pointer to the next queue upstream */
	struct N_unitdata_ind *hdr;
	struct ip_provider *prov, *prov1 = (struct ip_provider *) q->q_ptr;

	while (bp = getq(q)) {
		if (yhipdprintf) {
			printf ("yhipintr: got buf.\n");
		}
		/*
		 * Toss packets coming off providers marked "down". 
		 */
		if ((prov1->if_flags & IFF_UP) == 0) {
			if (yhipdprintf) {
				printf ("yhipintr: drop - interface down\n");
			}
			freemsg(bp);
			continue;
		}
		yhipstat.ips_total++;
		bp0 = bp;	/* Drop link level header */
		bp = bp->b_cont;
		freeb(bp0);
		if (pullupmsg(bp, sizeof(struct ip)) == 0) {
			cmn_err(CE_WARN, "yhipintr: pullupmsg failed\n");
			goto bad;
		}
		if ((bp->b_wptr - bp->b_rptr) < sizeof(struct ip)) {
			yhipstat.ips_tooshort++;
			if (yhipdprintf) 
				printf ("yhipintr: too short\n");
			goto bad;
		}
		ip = (struct ip *) bp->b_rptr;
		hlen = ip->ip_hl << 2;
		if (hlen < sizeof(struct ip)) {	/* minimum header length */
			yhipstat.ips_badhlen++;
			if (yhipdprintf)
				printf ("ip_input: header too short\n");
			goto bad;
		}
		if (hlen > (bp->b_wptr - bp->b_rptr)) {
			if (pullupmsg(bp, hlen) == 0) {
				yhipstat.ips_badhlen++;
				if (yhipdprintf)
					printf ("ip_input: header too short 2\n");
				goto bad;
			}
			ip = (struct ip *) bp->b_rptr;
		}
		if (yhipcksum)
			if (ip->ip_sum = yhin_cksum(bp, hlen)) {
				yhipstat.ips_badsum++;
				if (yhipdprintf)
					printf ("ip_input: bad header checksum\n");
				goto bad;
			}
		/*
		 * Convert fields to host representation. 
		 */
		ip->ip_len = ntohs((u_short) ip->ip_len);
		if (ip->ip_len < hlen) {
			yhipstat.ips_badlen++;
			if (yhipdprintf)
				printf ("ip_input: bad length\n");
			goto bad;
		}
		ip->ip_id = ntohs(ip->ip_id);
		ip->ip_off = ntohs((u_short) ip->ip_off);

		/*
		 * Check that the amount of data in the buffers is as at
		 * least much as the IP header would have us expect. Trim
		 * buffers if longer than we expect. Drop packet if shorter
		 * than we expect. 
		 */
		i = -(u_short) ip->ip_len;
		bp0 = bp;
		for (;;) {
			i += (bp->b_wptr - bp->b_rptr);
			if (bp->b_cont == 0)
				break;
			bp = bp->b_cont;
		}
		if (i != 0) {
			if (i < 0) {
				yhipstat.ips_toosmall++;
				bp = bp0;
				if (yhipdprintf)
					printf ("ip_input: too small\n");
				goto bad;
			}
			if (i <= (bp->b_wptr - bp->b_rptr))
				bp->b_wptr -= i;
			else
				adjmsg(bp0, -i);
		}
		bp = bp0;

		/*
		 * Process options and, if not destined for us, ship it on.
		 * yhip_dooptions returns 1 when an error was detected (causing
		 * an icmp message to be sent and the original packet to be
		 * freed). 
		 */
		yhip_nhops = 0;	/* for source routed packets */
		if (hlen > sizeof(struct ip) && yhip_dooptions(bp, q))
			continue;

		/*
		 * Check our list of addresses, to see if the packet is for
		 * us. 
		 */
		for (prov = yhprovider; prov <= yhlastprov; prov++) {
			if (PROV_INADDR(prov)->s_addr == ip->ip_dst.s_addr) {
				goto ours;
			}
			if (
#ifdef	DIRECTED_BROADCAST
			    prov1 == prov &&
#endif
			    (prov->if_flags & IFF_BROADCAST)) {
				u_long          t;

				if (satosin(&prov->if_broadaddr)->sin_addr.s_addr
				    == ip->ip_dst.s_addr)
					goto ours;
				if (ip->ip_dst.s_addr ==
				    prov->ia.ia_netbroadcast.s_addr)
					goto ours;
				/*
				 * Look for all-0's host part (old broadcast
				 * addr), either for subnet or net. 
				 */
				t = ntohl(ip->ip_dst.s_addr);
				if (t == prov->ia.ia_subnet)
					goto ours;
				if (t == prov->ia.ia_net)
					goto ours;
			}
		}
		if (ip->ip_dst.s_addr == (u_long) INADDR_BROADCAST)
			goto ours;
		if (ip->ip_dst.s_addr == INADDR_ANY)
			goto ours;

		/*
		 * Not for us; forward if possible and desirable. 
		 */
		yhip_forward(q, bp);
		continue;

ours:
		/*
		** If offset or IP_MF are set, must reassemble.
		** Otherwise, nothing need be done.
		** (We could look in the reassembly queue to see
		** if the packet was previously fragmented,
		** but it's not worth the time; just let them time out.
		*/

		if (ip->ip_off &~ IP_DF) {
			/*
			 * Look for queue of fragments of this datagram. 
			 */
			for (fp = yhipq.next; fp != &yhipq; fp = fp->next) {
				if (ip->ip_id == fp->ipq_id &&
				    ip->ip_src.s_addr == fp->ipq_src.s_addr &&
				    ip->ip_dst.s_addr == fp->ipq_dst.s_addr &&
				    ip->ip_p == fp->ipq_p)
					goto found;
			}
			fp = 0;
found:

			/*
			 * Adjust ip_len to not reflect header, set ip_mff if
			 * more fragments are expected, convert offset of this
			 * to bytes. 
			 */
			ip->ip_len -= hlen;
			IPASFRAG(ip)->ipf_mff = 0;
			if (ip->ip_off & IP_MF)
				IPASFRAG(ip)->ipf_mff = 1;
			ip->ip_off <<= 3;

			/*
			 * If datagram marked as having more fragments or if
			 * this is not the first fragment, attempt reassembly;
			 * if it succeeds, proceed. 
			 */
			if (IPASFRAG(ip)->ipf_mff || ip->ip_off) {
				yhipstat.ips_fragments++;
				bp = yhip_reass(bp, fp);
				if (bp == 0)
					continue;
				ip = (struct ip *) bp->b_rptr;
			} else if (fp)
				yhip_freef(fp);

		} else
			ip->ip_len -= hlen;
		/*
		 * Switch out to protocol's input routine. 
		 */
		if (yhip_protox[ip->ip_p] == yhipcnt) {
			if (yhipdprintf) {
				printf ("yhipintr: drop - proto %d not supported\n",
					ip->ip_p);
			}
			freemsg(bp);
			continue;
		}
		qp = yhip_pcb[yhip_protox[ip->ip_p]].ip_rdq;
		if ((fbp = allocb(sizeof(struct N_unitdata_ind) +
				  sizeof(struct ip_provider *),
				  BPRI_HI)) == NULL) {
			if (yhipdprintf) {
				printf ("yhipintr: drop - allocb failed\n");
			}
			freemsg(bp);
			continue;
		}
		hdr = (struct N_unitdata_ind *) fbp->b_rptr;
		fbp->b_wptr += sizeof(struct N_unitdata_ind) +
			sizeof(struct ip_provider *);
		fbp->b_datap->db_type = M_PROTO;
		hdr->PRIM_type = N_UNITDATA_IND;
		hdr->LA_length = 0;
		hdr->RA_offset = sizeof(struct N_unitdata_ind);
		hdr->RA_length = sizeof(struct ip_provider *);
		*((struct ip_provider **) (hdr + 1)) =
			(struct ip_provider *) q->q_ptr;
		fbp->b_cont = bp;
		if (!canput(qp)) {
			/* TOM - should send source quench here ? */
			STRLOG(IPM_ID, 2, 5, SL_TRACE, "client %d full",
			       ip->ip_p);
			if (yhipdprintf) {
				printf ("yhipintr: drop - can't put\n");
			}
			freemsg(fbp);
			continue;
		}
		if (yhipdprintf) {
			printf ("yhipintr: passing buf up\n");
		}
		putnext(qp, fbp);
		continue;

bad:
		if (yhipdprintf) {
			printf ("ipinitr: drop - bad packet\n");
		}
		freemsg(bp);
	}
}

/*
 * Take incoming datagram fragment and try to reassemble it into whole
 * datagram.  If a chain for reassembly of this datagram already exists, then
 * it is given as fp; otherwise have to make a chain. 
 */
mblk_t         *
yhip_reass(bp, fp)
	register mblk_t *bp;
	register struct ipq *fp;
{
	register struct ipasfrag *ip;
	register struct ipasfrag *q, *qprev;
	mblk_t         *nbp;
	int             hlen = ((struct ip *) bp->b_rptr)->ip_hl << 2;
	int             i, next;

	ip = (struct ipasfrag *) bp->b_rptr;
	bp->b_datap->db_type = M_DATA;	/* we only send data up */

	STRLOG(IPM_ID, 2, 7, SL_TRACE,
	       "yhip_reass fp = %x off = %d len = %d",
	       fp, ip->ip_off, ip->ip_len);
	/*
	 * Presence of header sizes in data blocks would confuse code below. 
	 */
	bp->b_rptr += hlen;

	/*
	 * If first fragment to arrive, create a reassembly queue. 
	 */
	if (fp == 0) {
		if ((fp = (struct ipq *)kmem_alloc(sizeof(struct ipq), KM_NOSLEEP)) == NULL)
			goto dropfrag;
		STRLOG(IPM_ID, 2, 9, SL_TRACE, "first frag, fp = %x", fp);
		insque((struct vq *) fp, (struct vq *) & yhipq);
		fp->ipq_ttl = IPFRAGTTL;
		fp->ipq_p = ip->ip_p;
		fp->ipq_id = ip->ip_id;
		fp->ipq_next = qprev = (struct ipasfrag *) fp;
		fp->ipq_src = IPHDR(ip)->ip_src;
		fp->ipq_dst = IPHDR(ip)->ip_dst;
		q = (struct ipasfrag *) fp;
		goto insert;
	}
	/*
	 * Find a segment which begins after this one does. 
	 */
	for (qprev = (struct ipasfrag *) fp, q = fp->ipq_next; 
	     q != (struct ipasfrag *) fp; 
	     qprev = q, q = q->ipf_next)
		if (q->ip_off > ip->ip_off)
			break;

	/*
	 * If there is a preceding segment, it may provide some of our data
	 * already.  If so, drop the data from the incoming segment.  If it
	 * provides all of our data, drop us. 
	 */
	if (qprev != (struct ipasfrag *) fp) {
		i = qprev->ip_off + qprev->ip_len - ip->ip_off;
		if (i > 0) {
			if (i >= ip->ip_len)
				goto dropfrag;
			adjmsg(bp, i);
			ip->ip_off += i;
			ip->ip_len -= i;
		}
	}
	/*
	 * While we overlap succeeding segments trim them or, if they are
	 * completely covered, dequeue them. 
	 */
	while (q != (struct ipasfrag *) fp && ip->ip_off + ip->ip_len > q->ip_off) {
		i = (ip->ip_off + ip->ip_len) - q->ip_off;
		if (i < q->ip_len) {
			STRLOG(IPM_ID, 2, 9, SL_TRACE,
			       "frag overlap adj off %d len %d",
			       q->ip_off, q->ip_len);
			q->ip_len -= i;
			q->ip_off += i;
			adjmsg(q->ipf_mblk, i);
			break;
		}
		STRLOG(IPM_ID, 2, 9, SL_TRACE,
		       "frag overlap del off %d len %d",
		       q->ip_off, q->ip_len);
		yhip_deqnxt(qprev);
		freemsg(q->ipf_mblk);
		q = qprev->ipf_next;
	}

insert:
	ip->ipf_mblk = bp;
	/*
	 * Stick new segment in its place; check for complete reassembly. 
	 */
	yhip_enq(ip, qprev);
	next = 0;
	for (q = fp->ipq_next; 
	     q != (struct ipasfrag *) fp; 
	     qprev = q, q = q->ipf_next) {
		if (q->ip_off != next)
			return (0);
		next += q->ip_len;
	}
	if (qprev->ipf_mff)
		return (0);

	/*
	 * Reassembly is complete; concatenate fragments. 
	 */
	q = fp->ipq_next;
	bp = q->ipf_mblk;
	q = q->ipf_next;
	for (; q != (struct ipasfrag *)fp; bp = nbp, q = q->ipf_next) {
		nbp = q->ipf_mblk;
		linkb(bp, nbp);
	}

	/*
	 * Create header for new clnp packet by modifying header of first
	 * packet; dequeue and discard fragment reassembly header. Make
	 * header visible. 
	 */
	ip = fp->ipq_next;
	ip->ip_len = next;
	bp = ip->ipf_mblk;
	bp->b_rptr -= (ip->ip_hl << 2);
	IPHDR(ip)->ip_src = fp->ipq_src;
	IPHDR(ip)->ip_dst = fp->ipq_dst;
	remque((struct vq *) fp);
	kmem_free((caddr_t)fp, sizeof(struct ipq));
	STRLOG(IPM_ID, 2, 5, SL_TRACE, "frag complete fp = %x", fp);
	return (bp);

dropfrag:
	STRLOG(IPM_ID, 2, 3, SL_TRACE,
	       "dropped frag fp = %x off = %d len = %d",
	       fp, ip ? ip->ip_off : 0, ip ? ip->ip_len : 0);
	yhipstat.ips_fragdropped++;
	freemsg(bp);
	return (0);
}


/*
 * Free a fragment reassembly header and all associated datagrams. 
 */
yhip_freef(fp)
	struct ipq     *fp;
{
	register struct ipasfrag *q;

	STRLOG(IPM_ID, 2, 9, SL_TRACE, "yhip_freef fp %x", fp);
	while((q = fp->ipq_next) != (struct ipasfrag *)fp) {
		yhip_deqnxt((struct ipasfrag *)fp);
		freemsg(q->ipf_mblk);
	}
	remque((struct vq *) fp);
	kmem_free((caddr_t)fp, sizeof(struct ipq));
}


/*
 * Put an ip fragment on a reassembly chain. 
 * This chain is no longer doubly linked.
 */
yhip_enq(p, prev)
	register struct ipasfrag *p, *prev;
{

	p->ipf_next = prev->ipf_next;
	prev->ipf_next = p;
}

/*
 * Replaces old remque-like ip_deq which operated on doubly linked lists
 */
yhip_deqnxt(p)
	register struct ipasfrag *p;
{

	p->ipf_next = p->ipf_next->ipf_next;
}

/*
 * IP timer processing; if a timer expires on a reassembly queue, discard it. 
 */
void
yhip_slowtimo()
{
	register struct ipq *fp;

#ifdef SLIP
	yhsltimein();
#endif /* SLIP */
	fp = yhipq.next;
	if (fp == 0) {
		goto newtimer;
	}
	while (fp != &yhipq) {
		--fp->ipq_ttl;
		fp = fp->next;
		if (fp->prev->ipq_ttl == 0) {
			yhipstat.ips_fragtimeout++;
			yhip_freef(fp->prev);
		}
	}
newtimer:
	yhiptimerid = timeout(yhip_slowtimo, (caddr_t) 0, HZ / 2);
}

/*
 * Drain off all datagram fragments. 
 */
yhip_drain()
{

	while (yhipq.next != &yhipq) {
		yhipstat.ips_fragdropped++;
		yhip_freef(yhipq.next);
	}
}

struct ip_provider *yhip_rtaddr();

/*
 * Do option processing on a datagram, possibly discarding it if bad options
 * are encountered. 
 */
yhip_dooptions(bp, q)
	mblk_t         *bp;
	queue_t        *q;
{
	register struct ip *ip;
	register u_char *cp;
	int             opt, optlen, cnt, off, code, type = ICMP_PARAMPROB;
	register struct ip_timestamp *ipt;
	register struct ip_provider *prov;
	struct in_addr *sin, in;
	n_time          ntime;

	ip = (struct ip *) bp->b_rptr;
	cp = (u_char *) (ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof(struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (u_char *) ip;
				goto bad;
			}
		}
		switch (opt) {

		default:
			break;

			/*
			 * Source routing with record. Find interface with
			 * current destination address. If none on this
			 * machine then drop if strictly routed, or do
			 * nothing if loosely routed. Record interface
			 * address and bring up next address component.  If
			 * strictly routed make sure next address on directly
			 * accessible net. 
			 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *) ip;
				goto bad;
			}
			prov = yhprov_withaddr(ip->ip_dst);
			if (prov == 0) {
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward. 
				 */
				break;
			}
			off--;	/* 0 origin */
			if (off > optlen - sizeof(struct in_addr)) {
				/*
				 * End of source route.  Should be for us. 
				 */
				yhsave_rte(cp, ip->ip_src);
				break;
			}
			/*
			 * locate outgoing interface 
			 */
			bcopy((caddr_t) (&ip->ip_dst), (caddr_t) & in,
			      sizeof(in));
			if ((opt == IPOPT_SSRR &&
			     yhin_onnetof(yhin_netof(in)) == 0) ||
			    (prov = yhip_rtaddr(in)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			ip->ip_dst = in;
			bcopy((caddr_t) PROV_INADDR(prov),
			      (caddr_t) (cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_RR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *) ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore. 
			 */
			off--;	/* 0 origin */
			if (off > optlen - sizeof(struct in_addr))
				break;
			bcopy((caddr_t) (&ip->ip_dst), (caddr_t) & in,
			      sizeof(in));
			/*
			 * locate outgoing interface 
			 */
			if ((prov = yhip_rtaddr(in)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy((caddr_t) PROV_INADDR(prov),
			      (caddr_t) (cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (u_char *) ip;
			ipt = (struct ip_timestamp *) cp;
			if (ipt->ipt_len < 5)
				goto bad;
			if (ipt->ipt_ptr > ipt->ipt_len - sizeof(long)) {
				if (++ipt->ipt_oflw == 0)
					goto bad;
				break;
			}
			sin = (struct in_addr *) (cp + ipt->ipt_ptr - 1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				prov = (struct ip_provider *) q->q_ptr;
				bcopy((caddr_t) PROV_INADDR(prov),
				      (caddr_t) sin, sizeof(struct in_addr));
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			case IPOPT_TS_PRESPEC:
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				bcopy((caddr_t) sin, (caddr_t) & in,
				      sizeof(struct in_addr));
				if (yhprov_withaddr(in) == 0)
					continue;
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			default:
				goto bad;
			}
			ntime = iptime();
			bcopy((caddr_t) & ntime, (caddr_t)cp + ipt->ipt_ptr - 1,
				sizeof(n_time));
			ipt->ipt_ptr += sizeof(n_time);
		}
	}
	return (0);
bad:
	icmp_error(ip, type, code, q, 0);
	freemsg(bp);
	return (1);
}

/*
 * Given address of next destination (final or next hop), return internet
 * address info of interface to be used to get there. 
 */
struct ip_provider *
yhip_rtaddr(dst)
	struct in_addr  dst;
{
	register struct in_addr *sin;
	struct ip_provider *prov;
	int             rtret = RT_OK;

	sin = &satosin(&yhipforward_rt.ro_dst)->sin_addr;

	if (yhipforward_rt.ro_rt == 0 || dst.s_addr != sin->s_addr) {
		if (yhipforward_rt.ro_rt) {
			RTFREE(yhipforward_rt.ro_rt);
			yhipforward_rt.ro_rt = 0;
		}
		*sin = dst;
		/* SLIP: dial in yhip_output, dont cache route */
		if ((rtret = yhrtalloc(&yhipforward_rt, 0)) == RT_DEFER) {
			RTFREE(yhipforward_rt.ro_rt);
			yhipforward_rt.ro_rt = 0;
		}
	}
	if (yhipforward_rt.ro_rt == 0)
		return ((struct ip_provider *) 0);

	prov = RT(yhipforward_rt.ro_rt)->rt_prov;
	if (rtret == RT_SWITCHED) {
		RTFREE(yhipforward_rt.ro_rt);
		yhipforward_rt.ro_rt = 0;
	}
	return (prov);
}

/*
 * Save incoming source route for use in replies, to be picked up later by
 * yhip_srcroute if the receiver is interested. 
 */
yhsave_rte(option, dst)
	u_char         *option;
	struct in_addr  dst;
{
	unsigned        olen;
	extern          yhipprintfs;

	olen = option[IPOPT_OLEN];
	if (olen > sizeof(yhip_srcrt) - 1) {
		if (yhipprintfs)
			printf("yhsave_rte: olen %d\n", olen);
		return;
	}
	bcopy((caddr_t) option, (caddr_t) yhip_srcrt.srcopt, olen);
	yhip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);
	yhip_srcrt.route[yhip_nhops++] = dst;
}

/*
 * Retrieve incoming source route for use in replies, in the same form used
 * by setsockopt. The first hop is placed before the options, will be removed
 * later. 
 */
mblk_t         *
yhip_srcroute(len)
int	len;
{
	register struct in_addr *p, *q;
	register mblk_t *bp;

	if (yhip_nhops == 0)
		return ((mblk_t *) 0);
	bp = allocb((int) (yhip_nhops * sizeof(struct in_addr) +
			   IPOPT_OFFSET + 1 + 1 + len), BPRI_HI);
	if (bp == 0) {
		return ((mblk_t *) 0);
	}
	bp->b_wptr += yhip_nhops * sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1;

	/*
	 * First save first hop for return route 
	 */
	p = &yhip_srcrt.route[yhip_nhops - 1];
	*((struct in_addr *) (bp->b_rptr)) = *p--;

	/*
	 * Copy option fields and padding (nop) to data block. 
	 */
	yhip_srcrt.nop = IPOPT_NOP;
	bcopy((caddr_t) & yhip_srcrt, (char *) (bp->b_rptr +
					      sizeof(struct in_addr)),
	      IPOPT_OFFSET + 1 + 1);
	q = (struct in_addr *) (bp->b_rptr +
			     sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1);
	/*
	 * Record return path as an IP source route, reversing the path
	 * (pointers are now aligned). 
	 */
	while (p >= yhip_srcrt.route)
		*q++ = *p--;
	return (bp);
}

/*
 * Strip out YHNP options, at higher level protocol in the kernel. Second
 * argument is buffer to which options will be moved, and return value is
 * their length. 
 */
yhip_stripoptions(bp, moptbp)
	mblk_t         *bp, *moptbp;
{
	struct ip      *ip;
	register int    i;
	register caddr_t opts;
	int             olen, optsoff = 0;;

	ip = (struct ip *) bp->b_rptr;
	olen = (ip->ip_hl << 2) - sizeof(struct ip);
	opts = (caddr_t) (ip + 1);
	/*
	 * Copy the options if there's a place to put them.
	 */

	if (moptbp) {

		/*
		 * If rptr == wptr, we're dealing with an option set
		 * that yhip_srcroute found no source routing in.
		 * So, we've got an empty mblk, into the beginning
		 * of which we have to coerce a "first hop" address.
		 * In a packet with no source routing, this would be the
		 * destination address.  Otherwise, wptr != rptr, and
		 * we're just appending to the mblk coming out of
		 * yhip_srcroute.
		 */

		if (moptbp->b_wptr == moptbp->b_rptr) {
			bcopy((caddr_t)&ip->ip_dst, (caddr_t)moptbp->b_wptr,
				sizeof(struct in_addr));
			moptbp->b_wptr += sizeof(struct in_addr);
		}

		/*
		 * Push the rest of the options in.  We don't have
		 * to worry about the other IP level options like
		 * we do the source routing, so just search for them
		 * and insert them into the mblk.  Notice that anything dealing
		 * with source routing is ignored, since you would want to
		 * do that in yhip_srcroute instead.
		 */
		
		while (optsoff + 1 <= olen) {
			switch((u_char)opts[optsoff]) {
			case IPOPT_LSRR:
			case IPOPT_SSRR:
				optsoff += opts[optsoff + IPOPT_OLEN];
				break;
			case IPOPT_EOL:
			case IPOPT_NOP:
				*moptbp->b_wptr = opts[optsoff++];
				moptbp->b_wptr++;
				break;
			default:
				bcopy((caddr_t)&opts[optsoff],
				      (caddr_t)moptbp->b_wptr,
				      opts[optsoff + IPOPT_OLEN]);
				moptbp->b_wptr += opts[optsoff + IPOPT_OLEN];
				optsoff += opts[optsoff + IPOPT_OLEN];
				break;
			}
		}
	}

	/*
	 * Actually strip out the old options data.
	 */
	i = (bp->b_wptr - bp->b_rptr) - (sizeof(struct ip) + olen);
	bcopy(opts + olen, opts, (unsigned) i);
	bp->b_wptr -= olen;
	ip->ip_hl = sizeof(struct ip) >> 2;
}

u_char          yhinetctlerrmap[PRC_NCMDS] = {
					    0, 0, 0, 0,
					    0, 0, EHOSTDOWN, EHOSTUNREACH,
		      ENETUNREACH, EHOSTUNREACH, ECONNREFUSED, ECONNREFUSED,
					    EMSGSIZE, EHOSTUNREACH, 0, 0,
					    0, 0, 0, 0,
					    ENOPROTOOPT
};

int             yhipprintfs = 0;

extern int		yhipforwarding;		/* from ip.cf */
extern int		yhipsendredirects;	/* from ip.cf */

/*
 * Forward a packet.  If some error occurs return the sender an icmp packet.
 * Note we can't always generate a meaningful icmp message because icmp
 * doesn't have a large enough repertoire of codes and types. 
 *
 * If not forwarding (possibly because we have only a single external network),
 * just drop the packet.  This could be confusing if yhipforwarding was zero
 * but some routing protocol was advancing us as a gateway to somewhere.
 * However, we must let the routing protocol deal with that. 
 */
yhip_forward(q, bp)
	queue_t        *q;
	register mblk_t *bp;
{
	register struct ip *ip;
	register int    error = 0, type = 0, code;
	struct in_addr *in;
	register struct ip_provider *prov;
	mblk_t         *mcopy, *fbp;
	register int    i;
	struct in_addr  dest;
	struct ip_unitdata_req *ip_req;
	int             rtret = RT_OK;

	mcopy = bp;		/* in case we call icmp_error */
	dest.s_addr = 0;
	ip = (struct ip *) bp->b_rptr;
	if (yhipprintfs)
		printf("forward: src %x dst %x ttl %x\n", ntohl(ip->ip_src),
		       ntohl(ip->ip_dst), ip->ip_ttl);
	ip->ip_id = htons(ip->ip_id);
	if (yhipforwarding == 0 || yhin_interfaces <= 1) {
		if (yhipdprintf) {
			printf ("yhip_forward: cant forward\n");
		}
		yhipstat.ips_cantforward++;
		freemsg(bp);
		return;
	}
	if (yhin_canforward(ip->ip_dst) == 0) {
		if (yhipdprintf) {
			printf ("yhip_forward: can't in_forward\n");
		}
		freemsg(bp);
		return;
	}
	if (ip->ip_ttl <= IPTTLDEC) {
		type = ICMP_TIMXCEED, code = ICMP_TIMXCEED_INTRANS;
		goto sendicmp;
	}
	ip->ip_ttl -= IPTTLDEC;

	/*
	 * Save at most 64 bytes of the packet in case we need to generate an
	 * control message to the src. 
	 */
	mcopy = dupmsg(bp);

	in = &satosin(&yhipforward_rt.ro_dst)->sin_addr;
	if (yhipforward_rt.ro_rt == 0 ||
	    ip->ip_dst.s_addr != in->s_addr) {
		if (yhipforward_rt.ro_rt) {
			RTFREE(yhipforward_rt.ro_rt);
			yhipforward_rt.ro_rt = 0;
		}
		*in = ip->ip_dst;

		/* SLIP: dial in yhip_output, dont cache route */
		if ((rtret = yhrtalloc(&yhipforward_rt, SSF_REMOTE)) == RT_DEFER) {
			RTFREE(yhipforward_rt.ro_rt);
			yhipforward_rt.ro_rt = 0;
		}
	}
	/*
	 * If forwarding packet using same interface that it came in on,
	 * perhaps should send a redirect to sender to shortcut a hop. Only
	 * send redirect if source is sending directly to us, and if packet
	 * was not source routed (or has any options). 
	 * Also, don't send redirect if forwarding using a default route
	 * or a route modified by a redirect.
	 */
	prov = (struct ip_provider *) q->q_ptr;
	if (yhipforward_rt.ro_rt && RT(yhipforward_rt.ro_rt)->rt_prov == prov &&
	   (RT(yhipforward_rt.ro_rt)->rt_flags & RTF_DYNAMIC|RTF_MODIFIED) == 0 &&
	    satosin(&(RT(yhipforward_rt.ro_rt)->rt_dst))->sin_addr.s_addr != 0 &&
	    yhipsendredirects && ip->ip_hl == (sizeof(struct ip) >> 2)) {
		u_long          src = ntohl(ip->ip_src.s_addr);
		u_long          dst = ntohl(ip->ip_dst.s_addr);

		if ((src & prov->ia.ia_subnetmask) == prov->ia.ia_subnet) {
			if (RT(yhipforward_rt.ro_rt)->rt_flags & RTF_GATEWAY)
				dest = satosin(&(RT(yhipforward_rt.ro_rt)->rt_gateway))->sin_addr;
			else
				dest = ip->ip_dst;
			/*
			 * If the destination is reached by a route to host,
			 * is on a subnet of a local net, or is directly on
			 * the attached net (!), use host redirect. (We may
			 * be the correct first hop for other subnets.) 
			 */
			type = ICMP_REDIRECT;
			code = ICMP_REDIRECT_NET;
			if ((RT(yhipforward_rt.ro_rt)->rt_flags & RTF_HOST) ||
			    (RT(yhipforward_rt.ro_rt)->rt_flags & RTF_GATEWAY) == 0)
				code = ICMP_REDIRECT_HOST;
			else
				for (prov = yhprovider; prov <= yhlastprov; prov++) {
					if (prov->qbot == 0)
						continue;
					if ((dst & prov->ia.ia_netmask) == prov->ia.ia_net) {
						if (prov->ia.ia_subnetmask != prov->ia.ia_netmask)
							code = ICMP_REDIRECT_HOST;
						break;
					}
				}
			if (yhipprintfs)
				printf("redirect (%d) to %x\n", code, dest);
		}
	}
	if ((fbp = allocb(sizeof(struct ip_unitdata_req), BPRI_MED)) == NULL) {
		freemsg(bp);
		if (mcopy) {
			freemsg(mcopy);
		}
		error = ENOSR;
		return;
	}
	ip_req = (struct ip_unitdata_req *) fbp->b_rptr;
	fbp->b_wptr += sizeof(struct ip_unitdata_req);
	fbp->b_cont = bp;
	fbp->b_datap->db_type = M_PROTO;
	ip_req->dl_primitive = N_UNITDATA_REQ;
	ip_req->dl_dest_addr_length = 0;
	ip_req->route = yhipforward_rt;
	ip_req->flags = IP_FORWARDING;
	ip_req->options = (mblk_t *) NULL;
	for (i = 0; i < yhipcnt; i++) {
		if (yhip_pcb[i].ip_state & IPOPEN) {
			break;
		}
	}
	if (i >= yhipcnt) {
		freemsg(fbp);
		if (mcopy) {
			freemsg(mcopy);
		}
		error = EINVAL;
		return;
	}
	if (yhipforward_rt.ro_rt)
		RT(yhipforward_rt.ro_rt)->rt_refcnt++;
	error = yhip_output(WR(yhip_pcb[i].ip_rdq), fbp);

	if (rtret == RT_SWITCHED) {
		RTFREE(yhipforward_rt.ro_rt);
		yhipforward_rt.ro_rt = 0;
	}
	if (error)
		yhipstat.ips_cantforward++;
	else if (type)
		yhipstat.ips_redirectsent++;
	else {
		if (mcopy)
			freemsg(mcopy);
		yhipstat.ips_forward++;
		return;
	}
	if (mcopy == NULL)
		return;
	ip = (struct ip *) mcopy->b_rptr;
	type = ICMP_UNREACH;
	switch (error) {

	case 0:		/* forwarded, but need redirect */
		type = ICMP_REDIRECT;
		/* code set above */
		break;

	case ENETUNREACH:
	case ENETDOWN:
		if (yhin_localaddr(ip->ip_dst))
			code = ICMP_UNREACH_HOST;
		else
			code = ICMP_UNREACH_NET;
		break;

	case EMSGSIZE:
		code = ICMP_UNREACH_NEEDFRAG;
		break;

	case EPERM:
		code = ICMP_UNREACH_PORT;
		break;

	case ENOSR:	/* same as ENOBUFS */
		type = ICMP_SOURCEQUENCH;
		code = 0;
		break;

	case EHOSTDOWN:
	case EHOSTUNREACH:
		code = ICMP_UNREACH_HOST;
		break;

	default:
		STRLOG(IPM_ID, 3, 9, SL_ERROR,
		       "yhip_forward: unrecognized error %d", error);
		break;
	}
sendicmp:
	icmp_error(ip, type, code, q, dest);
	if (mcopy)
		freemsg(mcopy);
}
