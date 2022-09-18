
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

#define STRNET

#include <util/types.h>
#include <util/param.h>
#include <util/spl.h>
#include <svc/systm.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <svc/errno.h>

#include <net/transport/tihdr.h>
#include <net/transport/tiuser.h>
#include <net/transport/socket.h>
#include <net/transport/socketvar.h>
#include <net/transport/sockio.h>
#include <net/yhtpip/byteorder.h>
#include <net/yhtpip/yhif.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhin_pcb.h>
#include <net/yhtpip/yhip.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/protosw.h>
#include <net/yhtpip/yhip_str.h>
#include <net/yhtpip/insrem.h>
#include <mem/kmem.h>

struct in_addr  yhzeroin_addr;
/*extern void     bcopy();*/

extern struct ip_provider *yhprov_withaddr(), *yhprov_withdstaddr();
extern struct ip_provider *yhin_onnetof();

extern struct ip_provider yhprovider[];
extern struct ip_provider *yhlastprov;

#define satosin(sa)	((struct sockaddr_in *) (sa))

struct inpcb   *
yhin_pcballoc(head)
	struct inpcb   *head;
{
	register struct inpcb *inp;

	inp = (struct inpcb *) kmem_alloc(sizeof(struct inpcb), KM_NOSLEEP);
	if (inp == NULL) {
		return ((struct inpcb *) NULL);
	}
	bzero((caddr_t) inp, sizeof(struct inpcb));
	inp->inp_head = head;
	inp->inp_q = NULL;
	inp->inp_addrlen = sizeof(struct sockaddr_in);
	inp->inp_family = AF_OSI;

	insque((struct vq *) inp, (struct vq *) head);
	return (inp);
}

yhin_pcbbind(inp, nam)
	register struct inpcb *inp;
	mblk_t         *nam;
{
	register struct inpcb *head = inp->inp_head;
	register struct sockaddr_in *sin;
	u_short         lport = 0;
	int		len;

	if (inp->inp_lport || inp->inp_laddr.s_addr != INADDR_ANY)
		return (EINVAL);
	if (nam == 0) {
		STRLOG(IPM_ID, 1, 5, SL_TRACE, "null yhin_pcbbind");
		goto noname;
	}
	sin = (struct sockaddr_in *) nam->b_rptr;
	len = nam->b_wptr - nam->b_rptr;
	if (!in_chkaddrlen(len))
		return (EINVAL);
	inp->inp_addrlen = len;
	inp->inp_family = sin->sin_family;
	STRLOG(IPM_ID, 1, 6, SL_TRACE, "yhin_pcbbind port %d addr %x",
	       sin->sin_port, sin->sin_addr.s_addr);
	if (sin->sin_addr.s_addr != INADDR_ANY
	    && !yhprov_withaddr(sin->sin_addr)) {
		if (inp->inp_protoopt & SO_IMASOCKET){
			return (EADDRNOTAVAIL);
		}
		else
			sin->sin_addr.s_addr = INADDR_ANY;
	}
	lport = sin->sin_port;
	if (lport) {
		u_short         aport = ntohs(lport);
		int             wild = 0;

		/* GROSS */
		if (aport < IPPORT_RESERVED && (inp->inp_state & SS_PRIV) == 0)
			return (EACCES);
		/* even GROSSER, but this is the Internet */
		if ((inp->inp_protoopt & SO_REUSEADDR) == 0 &&
		    ((inp->inp_protodef & PR_CONNREQUIRED) == 0 ||
		     (inp->inp_protoopt & SO_ACCEPTCONN) == 0))
			wild = INPLOOKUP_WILDCARD;
		if (yhin_pcblookup(head,
			      yhzeroin_addr, 0, sin->sin_addr, lport, wild)) {
			if (inp->inp_protoopt & SO_IMASOCKET)
				return (EADDRINUSE);
			else
				lport = 0;
		}
	}
	inp->inp_laddr = sin->sin_addr;
noname:
	if (lport == 0)
		do {
			if (head->inp_lport++ < IPPORT_RESERVED ||
			    head->inp_lport > IPPORT_USERRESERVED)
				head->inp_lport = IPPORT_RESERVED;
			lport = htons(head->inp_lport);
		} while (yhin_pcblookup(head,
				      yhzeroin_addr, 0, inp->inp_laddr,
				      lport, INPLOOKUP_WILDCARD));
	inp->inp_lport = lport;
	return (0);
}

/*
 * Connect from a socket to a specified address. Both address and port must
 * be specified in argument sin. If don't have a local address for this
 * socket yet, then pick one.
 */
yhin_pcbconnect(inp, nam)
	struct inpcb   *inp;
	mblk_t         *nam;
{
	register struct ip_provider *prov;
	struct ip_provider *first_prov = (struct ip_provider *) NULL, tempprov;
	register struct sockaddr_in *sin = (struct sockaddr_in *) nam->b_rptr;
	int	len;

	len = nam->b_wptr - nam->b_rptr;
	if (!in_chkaddrlen(len))
		return (EINVAL);
	if(sin->sin_family != AF_OSI && sin->sin_family != bswaps(AF_OSI)){
		return (EAFNOSUPPORT);
	}
	if (sin->sin_port == 0){
		return (EADDRNOTAVAIL);
	}
	inp->inp_addrlen = len;
	inp->inp_family = AF_OSI;

	/*
	 * If destination is INADDR_ANY, find loopback.
	 */
	if (sin->sin_addr.s_addr == INADDR_ANY) {
		for (prov = yhprovider; prov <= yhlastprov; prov++) {
			if (prov->qbot != NULL
			  && (prov->if_flags & (IFF_UP | IFF_LOOPBACK))
			     == (IFF_UP | IFF_LOOPBACK)) {
				first_prov = prov;
				break;
			}
		}
	}
	if (first_prov == NULL) {
		for (prov = yhprovider; prov <= yhlastprov; prov++) {
			if (prov->qbot != NULL && (prov->if_flags & IFF_UP)
			  && !(prov->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT))) {
				first_prov = prov;
				break;
			}
		}
	}
	if (first_prov == NULL) {
		for (prov = yhprovider; prov <= yhlastprov; prov++) {
			if (prov->qbot != NULL && (prov->if_flags & IFF_UP)
			    && !(prov->if_flags & IFF_LOOPBACK)) {
				first_prov = prov;
				break;
			}
		}
	}
	if (first_prov) {
		/*
		 * If the destination address is INADDR_ANY, use loopback
		 * or primary local address.  If the supplied address is
		 * INADDR_BROADCAST, and the primary interface supports
		 * broadcast, choose the broadcast address for that
		 * interface.
		 */
		if (sin->sin_addr.s_addr == INADDR_ANY)
			sin->sin_addr = *PROV_INADDR(first_prov);
		else if (sin->sin_addr.s_addr == (u_long) INADDR_BROADCAST &&
			 (first_prov->if_flags & IFF_BROADCAST))
			sin->sin_addr = *SOCK_INADDR(&first_prov->if_broadaddr);
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		register struct route *ro;

		prov = (struct ip_provider *) 0;
		/*
		 * If route is known or can be allocated now, our src addr is
		 * taken from the i/f, else punt.
		 */
		ro = &inp->inp_route;
		if (ro->ro_rt &&
		    (satosin(&ro->ro_dst)->sin_addr.s_addr != 
		     sin->sin_addr.s_addr ||
		     inp->inp_protoopt & SO_DONTROUTE)) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (mblk_t *) 0;
		}
		if ((inp->inp_protoopt & SO_DONTROUTE) == 0 &&	/* XXX */
		    (ro->ro_rt == (mblk_t *) 0
		   || RT(ro->ro_rt)->rt_prov == (struct ip_provider *) 0)) {
			/* No route yet, so try to acquire one */
			satosin(&ro->ro_dst)->sin_addr = sin->sin_addr;
			if (yhrtalloc(ro, SSF_SWITCH) == RT_DEFER) {
				*PROV_INADDR(&tempprov) =
					satosin (&(RT(ro->ro_rt)->rt_gateway))->sin_addr;
				prov = &tempprov;	/* switched: gateway is
							 * laddr */
				RTFREE(ro->ro_rt);
				ro->ro_rt = 0;	/* don't cache till complete */
			}
		}
		/*
		 * If we found a route, use the address corresponding to the
		 * outgoing interface unless it is the loopback (in case a
		 * route to our address on another net goes to loopback).
		 */
		if (ro->ro_rt && (prov = RT(ro->ro_rt)->rt_prov) &&
		    (prov->if_flags & IFF_LOOPBACK))
			prov = 0;
		if (prov == 0) {
			prov = yhprov_withdstaddr(sin->sin_addr);
			if (prov == 0)
				prov = yhin_onnetof(yhin_netof(sin->sin_addr));
			if (prov == 0)
				prov = first_prov;
			if (prov == 0){
				return (EADDRNOTAVAIL);
			}
		}
	}
	if (yhin_pcblookup(inp->inp_head,
			 sin->sin_addr,
			 sin->sin_port,
		inp->inp_laddr.s_addr ? inp->inp_laddr : *PROV_INADDR(prov),
			 inp->inp_lport,
			 0))
		return (EADDRINUSE);
	if (inp->inp_laddr.s_addr == INADDR_ANY && inp->inp_lport != 0) {
		inp->inp_laddr = *PROV_INADDR(prov);
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		mblk_t         *bp = allocb(inp->inp_addrlen, BPRI_HI);
		struct sockaddr_in *sin1 = (struct sockaddr_in *) bp->b_rptr;
		int             error;

		if (bp == (mblk_t *) NULL) {
			return (ENOSR);
		}
		bp->b_wptr += inp->inp_addrlen;
		sin1->sin_family = inp->inp_family;
		sin1->sin_addr = *PROV_INADDR(prov);
		sin1->sin_port = inp->inp_lport;
		inp->inp_lport = 0;
		if (error = yhin_pcbbind(inp, bp)) {
			freeb(bp);
			return (error);
		}
		freeb(bp);
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	return (0);
}

yhin_pcbdisconnect(inp)
	struct inpcb   *inp;
{

	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;
	if (inp->inp_state & SS_NOFDREF)
		yhin_pcbdetach(inp);
}

yhin_pcbdetach(inp)
	struct inpcb   *inp;
{
	STRLOG(IPM_ID, 1, 4, SL_TRACE, "yhin_pcbdetach wq %x pcb %x",
	       inp->inp_q ? WR(inp->inp_q) : 0, inp);
	if (inp->inp_options)
		(void) freeb(inp->inp_options);
	if (inp->inp_route.ro_rt)
		yhrtfree(inp->inp_route.ro_rt);
	remque((struct vq *) inp);
	if (inp->inp_q) {
		inp->inp_q->q_ptr = (char *) NULL;
	}
	(void) kmem_free(inp, sizeof(struct inpcb));
}

yhin_setsockaddr(inp, nam)
	register struct inpcb *inp;
	mblk_t         *nam;
{
	register struct sockaddr_in *sin = (struct sockaddr_in *) nam->b_rptr;

	nam->b_wptr = nam->b_rptr + inp->inp_addrlen;
	bzero((caddr_t) sin, inp->inp_addrlen);
	sin->sin_family = inp->inp_family;
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

yhin_setpeeraddr(inp, nam)
	register struct inpcb *inp;
	mblk_t         *nam;
{
	register struct sockaddr_in *sin = (struct sockaddr_in *) nam->b_rptr;

	nam->b_wptr = nam->b_rptr + inp->inp_addrlen;
	bzero((caddr_t) sin, inp->inp_addrlen);
	sin->sin_family = inp->inp_family;
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass some notification to all connections of a protocol associated with
 * address dst.  Call the protocol specific routine (if any) to handle each
 * connection.
 * If portmatch arg is set, only pass to matching port.  Prevents denial of
 * service attacks via bogus icmp error messages.
 */
yhin_pcbnotify(head, src, dst, errno, notify, portmatch)
	struct inpcb   *head;
	register struct sockaddr_in *src, *dst;
	int             errno, (*notify) (), portmatch;
{
	register struct inpcb *inp, *oinp;

	STRLOG(IPM_ID, 3, 4, SL_TRACE,
	     "yhin_pcbnotify: sending error %d to pcbs from %x", errno, head);

	for (inp = head->inp_next; inp != head;) {
		if (inp->inp_faddr.s_addr != dst->sin_addr.s_addr) {
			inp = inp->inp_next;
			continue;
		}
		if ( portmatch ) {
		    if ( inp->inp_fport != dst->sin_port ) {
			inp = inp->inp_next;
			continue;
		    }
		    if ( src && inp->inp_lport != src->sin_port ) {
			inp = inp->inp_next;
			continue;
		    }
		}
		if (errno)
			inp->inp_error = errno;
		oinp = inp;
		inp = inp->inp_next;
		if (notify)
			(*notify) (oinp);
	}
}

/*
 * Check for alternatives when higher level complains about service problems.
 * For now, invalidate cached routing information.  If the route was created
 * dynamically (by a redirect), time to try a default gateway again.
 */
yhin_losing(inp)
	struct inpcb   *inp;
{
	register mblk_t *rt;

	if ((rt = inp->inp_route.ro_rt)) {
		if (RT(rt)->rt_flags & RTF_DYNAMIC)
			(void) yhrtrequest((int) SIOCDELRT, rt);
		yhrtfree(rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time output is
		 * attempted.
		 */
	}
}

/*
 * After a routing change, flush old routing and allocate a (hopefully)
 * better one.
 */
yhin_rtchange(inp)
	register struct inpcb *inp;
{
	if (inp->inp_route.ro_rt) {
		yhrtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time output is
		 * attempted.
		 */
	}
}

struct inpcb   *
yhin_pcblookup(head, faddr, fport, laddr, lport, flags)
	struct inpcb   *head;
	struct in_addr  faddr, laddr;
	unsigned short  fport, lport;
	int             flags;
{
	register struct inpcb *inp, *match = 0;
	int             yhmatchwild = 3, yhwildcard;
	register int    s;

	s = splstr();
	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
		if (inp->inp_lport != lport)
			continue;
		yhwildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				yhwildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				yhwildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				yhwildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
				 inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				yhwildcard++;
		}
		if (yhwildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (yhwildcard < yhmatchwild) {
			match = inp;
			yhmatchwild = yhwildcard;
			if (yhmatchwild == 0)
				break;
		}
	}
	splx(s);
	return (match);
}

/*
 * the following defines default values for most socket options
 */
static int      yhopt_on = 1;
static int      yhopt_off = 0;
static struct linger yhlingerdef = {0, 0};
static struct optdefault yhsockdefault[] = {
	SO_LINGER, (char *) &yhlingerdef, sizeof(struct linger),
	SO_DEBUG, (char *) &yhopt_off, sizeof(int),
	SO_KEEPALIVE, (char *) &yhopt_off, sizeof(int),
	SO_DONTROUTE, (char *) &yhopt_off, sizeof(int),
	SO_USELOOPBACK, (char *) &yhopt_off, sizeof(int),
	SO_BROADCAST, (char *) &yhopt_off, sizeof(int),
	SO_REUSEADDR, (char *) &yhopt_off, sizeof(int),
	SO_OOBINLINE, (char *) &yhopt_off, sizeof(int),
	SO_IMASOCKET, (char *) &yhopt_off, sizeof(int),
	/* defaults for these have to be taken from elsewhere */
	SO_SNDBUF, (char *) 0, sizeof(int),
	SO_RCVBUF, (char *) 0, sizeof(int),
	SO_SNDLOWAT, (char *) 0, sizeof(int),
	SO_RCVLOWAT, (char *) 0, sizeof(int),
	0, (char *) 0, 0
};

/*
 * yhin_pcboptmgmt handles "socket" level options management The return value
 * is 0 if ok, or a T-error, or a negative E-error. Note that if T_CHECK sets
 * T_FAILURE in the message, the return value will still be 0. The list of
 * options is built in the message pointed to by mp.
 */

int
yhin_pcboptmgmt(q, req, opt, mp)
	queue_t        *q;
	struct T_optmgmt_req *req;
	struct opthdr  *opt;
	mblk_t         *mp;
{
	struct inpcb   *inp = qtoinp(q);
	int            *optval;

	switch (req->MGMT_flags) {

	case T_NEGOTIATE:
		switch (opt->name) {
		case SO_LINGER:{
				struct linger  *l = (struct linger *) OPTVAL(opt);
				if (opt->len != OPTLEN(sizeof(struct linger)))
					return TBADOPT;
				if (l->l_onoff) {
					inp->inp_protoopt |= SO_LINGER;
					inp->inp_linger = l->l_linger;
				} else
					inp->inp_protoopt &= ~SO_LINGER;
				break;
			}

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_OOBINLINE:
		case SO_IMASOCKET:
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_PROTOTYPE:
			if (opt->len != OPTLEN(sizeof(int)))
				return TBADOPT;
			optval = (int *) OPTVAL(opt);
			switch (opt->name) {
			case SO_DEBUG:
			case SO_KEEPALIVE:
			case SO_DONTROUTE:
			case SO_USELOOPBACK:
			case SO_BROADCAST:
			case SO_REUSEADDR:
			case SO_OOBINLINE:
			case SO_IMASOCKET:
				if (*(int *) optval)
					inp->inp_protoopt |= opt->name;
				else
					inp->inp_protoopt &= ~opt->name;
				break;
			case SO_SNDBUF:
				q->q_hiwat = *(int *) optval;
				break;
			case SO_RCVBUF:
				RD(q)->q_hiwat = *(int *) optval;
				break;
			case SO_SNDLOWAT:
				q->q_lowat = *(int *) optval;
				break;
			case SO_RCVLOWAT:
				RD(q)->q_lowat = *(int *) optval;
				break;
			case SO_PROTOTYPE:
				qtoinp(q)->inp_proto = *(int *) optval;
				break;
			}
			break;

		default:
			return TBADOPT;
		}

		/* fall through to retrieve value */

	case T_CHECK:
		switch (opt->name) {
		case SO_LINGER:{
				struct linger   l;
				if (inp->inp_protoopt & SO_LINGER) {
					l.l_onoff = 1;
					l.l_linger = inp->inp_linger;
				} else
					l.l_onoff = l.l_linger = 0;
				if (!yhmakeopt(mp, SOL_SOCKET, SO_LINGER, &l, sizeof(l)))
					return -ENOSR;
				break;
			}

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_OOBINLINE:
		case SO_IMASOCKET:
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_PROTOTYPE:{
				int             val;

				switch (opt->name) {
				case SO_DEBUG:
				case SO_KEEPALIVE:
				case SO_DONTROUTE:
				case SO_USELOOPBACK:
				case SO_BROADCAST:
				case SO_REUSEADDR:
				case SO_OOBINLINE:
				case SO_IMASOCKET:
					val = (inp->inp_protoopt & opt->name) != 0;
					break;

				case SO_SNDBUF:
					val = q->q_hiwat;
					break;
				case SO_RCVBUF:
					val = RD(q)->q_hiwat;
					break;
				case SO_SNDLOWAT:
					val = q->q_lowat;
					break;
				case SO_RCVLOWAT:
					val = RD(q)->q_lowat;
					break;
				case SO_PROTOTYPE:
					val = qtoinp(q)->inp_proto;
					break;
				}

				if (!yhmakeopt(mp, SOL_SOCKET, opt->name, &val, sizeof(int)))
					return -ENOSR;
				break;
			}

		default:
			req->MGMT_flags = T_FAILURE;
			break;
		}
		break;

	case T_DEFAULT:{
			struct optdefault *o;
			int             val;

			/* get default values from table */
			for (o = yhsockdefault; o->optname; o++) {
				if (o->val) {
					if (!yhmakeopt(mp, SOL_SOCKET, o->optname,
						     o->val, o->len))
						return -ENOSR;
				}
			}

			/* add default values that aren't in the table */
			val = q->q_qinfo->qi_minfo->mi_hiwat;
			if (!yhmakeopt(mp, SOL_SOCKET, SO_SNDBUF, &val, sizeof(int)))
				return -ENOSR;
			val = RD(q)->q_qinfo->qi_minfo->mi_hiwat;
			if (!yhmakeopt(mp, SOL_SOCKET, SO_RCVBUF, &val, sizeof(int)))
				return -ENOSR;
			val = q->q_qinfo->qi_minfo->mi_lowat;
			if (!yhmakeopt(mp, SOL_SOCKET, SO_SNDLOWAT, &val, sizeof(int)))
				return -ENOSR;
			val = RD(q)->q_qinfo->qi_minfo->mi_lowat;
			if (!yhmakeopt(mp, SOL_SOCKET, SO_RCVLOWAT, &val, sizeof(int)))
				return -ENOSR;

			break;
		}

	}

	return 0;
}


/*
 * YHNP socket option processing. This function is actually called from higher
 * level protocols which have an inpcb structure (e.g., TCP).
 */

int
yhip_options(q, req, opt, mp)
	queue_t        *q;
	struct T_optmgmt_req *req;
	struct opthdr  *opt;
	mblk_t         *mp;
{
	struct inpcb   *inp = qtoinp(q);
	int             error;

	switch (req->MGMT_flags) {

	case T_NEGOTIATE:
		switch (opt->name) {
		case IP_OPTIONS:
			if ((error = yhip_pcbopts(inp, OPTVAL(opt), opt->len, 1)))
				return error;
			break;
		default:
			return TBADOPT;
		}

		/* fall through to retrieve value */

	case T_CHECK:
		switch (opt->name) {
		case IP_OPTIONS:{
				mblk_t         *opts;

				if (opts = inp->inp_options) {
					/* don't copy first 4 bytes */
					if (!yhmakeopt(mp, IPPROTO_IP, IP_OPTIONS,
						     opts->b_rptr + sizeof(struct in_addr),
					 opts->b_wptr - (opts->b_rptr + 4)))
						return -ENOSR;
				} else {
					if (!yhmakeopt(mp, IPPROTO_IP, IP_OPTIONS,
						     (char *) 0, 0))
						return -ENOSR;
				}
				break;
			}
		default:
			req->MGMT_flags = T_FAILURE;
			break;
		}
		break;

	case T_DEFAULT:
		if (!yhmakeopt(mp, IPPROTO_IP, IP_OPTIONS, (char *) 0, 0))
			return -ENOSR;
		break;
	}

	return 0;

}

/*
 * Set up YHNP options in pcb for insertion in output packets. Store in message
 * block with pointer in inp->inp_options, adding pseudo-option with
 * destination address if source routed. If set is non-zero, set new options,
 * otherwise just check.
 */
yhip_pcbopts(inp, optbuf, cnt, set)
	struct inpcb   *inp;
	char           *optbuf;
	int             cnt, set;
{
	register        optlen;
	register u_char *cp;
	register mblk_t *bp1 = (mblk_t *) NULL;
	u_char          opt;

	if (cnt == 0) {
		if (set) {
			if (inp->inp_options)
				freeb(inp->inp_options);
			inp->inp_options = NULL;
		}
		return 0;
	}
	/*
	 * IP first-hop destination address will be stored before actual
	 * options; move other options back and clear it when none present.
	 */

	if ((bp1 = allocb((int) (cnt + sizeof(struct in_addr)), BPRI_LO))
	    == NULL) {
		return -ENOSR;
	}
	cp = bp1->b_wptr += sizeof(struct in_addr);
	yhin_ovbcopy(optbuf, (char *) bp1->b_wptr, (unsigned) cnt);
	bp1->b_wptr += cnt;
	bzero((char *) bp1->b_rptr, sizeof(struct in_addr));

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= IPOPT_OLEN || optlen > cnt)
				goto bad;
		}
		switch (opt) {

		default:
			break;

		case IPOPT_LSRR:
		case IPOPT_SSRR:
			/*
			 * user process specifies route as: ->A->B->C->D D
			 * must be our final destination (but we can't check
			 * that since we may not have connected yet). A is
			 * first hop destination, which doesn't appear in
			 * actual YHNP option, but is stored before the
			 * options.
			 */
			if (optlen < IPOPT_MINOFF - 1 + sizeof(struct in_addr))
				goto bad;
			bp1->b_wptr -= sizeof(struct in_addr);
			cnt -= sizeof(struct in_addr);
			optlen -= sizeof(struct in_addr);
			cp[IPOPT_OLEN] = optlen;
			/*
			 * Move first hop before start of options.
			 */
			bcopy((caddr_t) & cp[IPOPT_OFFSET + 1],
			      (char *) bp1->b_rptr, sizeof(struct in_addr));
			/*
			 * Then copy rest of options back to close up the
			 * deleted entry.
			 */
			yhin_ovbcopy((caddr_t) (&cp[IPOPT_OFFSET + 1] +
					 sizeof(struct in_addr)),
			      (caddr_t) & cp[IPOPT_OFFSET + 1],
			      (unsigned) cnt - (IPOPT_OFFSET + 1));
			break;
		}
	}
	if (set)
		inp->inp_options = bp1;
	else
		freeb(bp1);
	return 0;

bad:
	freeb(bp1);
	return TBADOPT;
}
