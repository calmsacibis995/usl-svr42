
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

#ifdef SLIP
/*
 * routines for switched slip -- streams version 
 */

#define STRNET

#include <util/types.h>
#include <util/param.h>
#include <util/spl.h>
#include <svc/systm.h>
#include <proc/signal.h>

#ifdef SYSV
#include <proc/cred.h>
#include <proc/proc.h>
#endif /* SYSV */


#include <proc/user.h>
#include <io/ioctl.h>
#include <svc/errno.h>

#ifdef SYSV
#include <util/cmn_err.h>
#endif SYSV

#include <io/stream.h>
#include <io/stropts.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <util/debug.h>
#include <net/yhtpip/protosw.h>
#include <net/transport/socket.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhif.h>
#include <net/yhtpip/af.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhip_str.h>

#define mtod(m,t) ((t)((m)->b_rptr))
/* #define ATOL(sa) (((struct sockaddr_in *)(sa))->sin_addr.s_addr) */
#define ATOL(sa) ((sa)->s_addr)

#define satosin(sa)	((struct sockaddr_in *) (sa))

#define SLIP_NODIAL	120	/* dial timeout */
#define SLIP_PEERWAIT	2	/* time to wait after call answer before
				 * sending first packet */
#define SLIP_CHECKWAIT	40	/* check connection after how long */

u_short          yhSlip_Hangwait = 300;	/* time to wait before hanging up */

 /* static */ struct {
	queue_t        *queue;
	mblk_t         *mp;
	mblk_t         *waiting;
}               yhSlreq;
mblk_t         *yhSldeferred, *yhSlevents;
int             yhSlnumqueued = 0, yhSlnumdeferred = 0, yhSlnumevents = 0;
int             yhSlmaxqueued = 10, yhSlmaxdeferred = 40, yhSlmaxevents = 15;
int             yhSlchkqueued = 0, yhSlchkdeferred = 0, yhSlchkevents = 0;
extern struct ip_provider *yhloopprov;

/* static */ void yhioc_ack(), yhioc_error();

struct slevent {
	int             (*func) ();
	caddr_t         arg;
	unsigned        time;
};

struct defstruct {
	int             (*routine) ();
	unsigned        args[8];
};

/*
 * these routines are called out of the routing code 
 */
yhrtswitch(ro, flags)
	register struct route *ro;
{
	register struct rtentry *rt = RT(ro->ro_rt);
	void            yhrtnodial();


	if (rt->rt_flags & RTF_TOSWITCH) {
		struct route    route;

		if (flags & SSF_TOSWITCH) {
#ifdef SYSV
			cmn_err(CE_WARN,
				"yhrtswitch recursion, dest %x gateway %x\n",
				satosin(&rt->rt_dst)->sin_addr.s_addr, 
				satosin(&rt->rt_gateway)->sin_addr.s_addr);
#else
			printf ("yhrtswitch recursion, dest %x gateway %x\n",
				satosin(&rt->rt_dst)->sin_addr.s_addr, 
				satosin(&rt->rt_gateway)->sin_addr.s_addr);
#endif SYSV
			ro->ro_rt = 0;
			return (RT_FAIL);
		}
		bzero((caddr_t) & route, sizeof(route));
		route.ro_dst = rt->rt_gateway;
		yhrtalloc(&route, SSF_TOSWITCH);
		if (route.ro_rt) {
			rt = RT(ro->ro_rt);
			yhrtfree(route.ro_rt);
			if ((RT(route.ro_rt)->rt_flags & RTF_SWITCHED)
			    && !(RT(route.ro_rt)->rt_flags & RTF_TOSWITCH)) {
				ro->ro_rt = route.ro_rt;
			} else {
				ro->ro_rt = 0;
				return (RT_FAIL);
			}
		} else {
			ro->ro_rt = 0;
			return (RT_FAIL);
		}
	}
	if (flags & SSF_REMOTE)
		rt->rt_flags |= RTF_REMOTE;

	if (!(flags & SSF_SWITCH)) {
		switch (SSS_GETSTATE(rt)) {
		case SSS_NOCONN:
		case SSS_DIALING:	/* dialing already */
		case SSS_CALLFAIL:	/* clearing deferred stuff */
			rt->rt_refcnt++;
			return (RT_DEFER);
		case SSS_CLEARWAIT:	/* still have connection! */
			SSS_SETSTATE(rt, SSS_INUSE);	/* fall thru */
		case SSS_OPENWAIT:	/* already dialed but waiting */
		case SSS_INUSE:/* connected */
			rt->rt_refcnt++;
			return (RT_SWITCHED);
		default:
#ifdef SYSV
			cmn_err(CE_WARN, "yhrtswitch: bad state %x\n",
				rt->rt_flags);
#else
			printf ("yhrtswitch: bad state %x\n",
				rt->rt_flags);
#endif SYSV
			ro->ro_rt = 0;
			return (RT_FAIL);
		}
	}
	switch (SSS_GETSTATE(rt)) {
	case SSS_NOCONN:
		SSS_SETSTATE(rt, SSS_DIALING);
		yhsltimeout(yhrtnodial, (caddr_t) rt, SLIP_NODIAL);
		yhsldocall(&satosin(&rt->rt_dst)->sin_addr);
		rt->rt_refcnt++;
		return (RT_DEFER);
	case SSS_DIALING:	/* dialing already */
	case SSS_OPENWAIT:	/* already dialed but waiting */
		rt->rt_refcnt++;
		return (RT_DEFER);
	case SSS_INUSE:	/* connected */
		rt->rt_refcnt++;
		return (RT_SWITCHED);
	case SSS_CLEARWAIT:	/* still have connection! */
		SSS_SETSTATE(rt, SSS_INUSE);
		rt->rt_refcnt++;
		return (RT_SWITCHED);
	case SSS_CALLFAIL:	/* clearing deferred stuff */
		ro->ro_rt = 0;
		return (RT_FAIL);
	default:
#ifdef SYSV
		cmn_err(CE_WARN, "yhrtswitch: bad state %x\n", rt->rt_flags);
#else
		printf ("yhrtswitch: bad state %x\n", rt->rt_flags);
#endif SYSV
		ro->ro_rt = 0;
		return (RT_FAIL);
	}
}

yhrtunswitch(rt)
	register struct rtentry *rt;
{
	void            yhrthangup();

	switch (SSS_GETSTATE(rt)) {
	case SSS_INUSE:	/* connected */
	case SSS_OPENWAIT:	/* connected */
		SSS_SETSTATE(rt, SSS_CLEARWAIT);
		yhsltimeout(yhrthangup, (caddr_t) rt, yhSlip_Hangwait);
		break;
	case SSS_NOCONN:
	case SSS_DIALING:	/* dialing already */
	case SSS_CALLFAIL:	/* clearing deferred stuff */
	case SSS_CLEARWAIT:
		break;
	default:
#ifdef SYSV
		cmn_err(CE_WARN, "yhrtunswitch: bad state %x\n", rt->rt_flags);
#else
		printf ("yhrtunswitch: bad state %x\n", rt->rt_flags);
#endif SYSV
		break;
	}
}

/*
 * these routines are called at yhsltimein (pf_timeout) time 
 */
/*
 * done with active connection 
 */
 /* static */ void
yhrthangup(rt)
	register struct rtentry *rt;
{

	if (SSS_GETSTATE(rt) != SSS_CLEARWAIT) {
		return;		/* somebody wants it now */
	}
	SSS_SETSTATE(rt, SSS_NOCONN);
	yhslhangup(rt);		/* tell daemon to detach */
}

/*
 * call timed out (daemon died) 
 */
 /* static */ void
yhrtnodial(rt)
	register struct rtentry *rt;
{
	if (SSS_GETSTATE(rt) != SSS_DIALING)
		return;
	SSS_SETSTATE(rt, SSS_CALLFAIL);
	yhslsenddeferred();
	SSS_SETSTATE(rt, SSS_NOCONN);
}

 /* static */ void
yhslstartconn(rt)
	register struct rtentry *rt;
{
	void            yhslcheckuse();

	if (SSS_GETSTATE(rt) != SSS_OPENWAIT)
		return;
	SSS_SETSTATE(rt, SSS_INUSE);
	yhslsenddeferred();
	yhsltimeout(yhslcheckuse, (caddr_t) rt, SLIP_CHECKWAIT);
}

/*
 * make sure connection actually is used (ie user did not time out) 
 */
 /* static */ void
yhslcheckuse(rt)
	register struct rtentry *rt;
{
	void            yhrthangup();

	switch (SSS_GETSTATE(rt)) {
	case SSS_INUSE:
	case SSS_CLEARWAIT:
		break;

	default:
		return;
	}
	if (rt->rt_refcnt || (rt->rt_flags & RTF_REMOTE))
		return;
	SSS_SETSTATE(rt, SSS_CLEARWAIT);
	yhrthangup(rt);
}

/*
 * yhsldocall and yhslhangup queue requests for the daemon 
 */
/* static */
yhsldocall(addr)
	register struct in_addr *addr;
{
	struct sockaddr_in sock;
	register mblk_t *m, **mpp;
	register int    s;

	bzero((caddr_t) & sock, sizeof(struct sockaddr_in));
	sock.sin_family = AF_OSI;
	sock.sin_addr = *addr;
	s = splstr();
	if (yhSlreq.mp) {
		((struct ifreq *) (yhSlreq.mp->b_cont->b_rptr))->ifr_addr =
			*(struct sockaddr *) & sock;
		yhioc_ack(yhSlreq.queue, yhSlreq.mp);
		yhSlreq.mp = 0;
		splx(s);
		return;
	}
	if (yhSlnumqueued >= yhSlmaxqueued) {
		splx(s);
#ifdef SYSV
		cmn_err(CE_WARN, "switched slip: dropping request\n");
#else
		printf ("switched slip: dropping request\n");
#endif SYSV
		return;
	}
	if ((m = allocb(sizeof(struct sockaddr_in), BPRI_MED)) == NULL) {
		splx(s);
#ifdef SYSV
		cmn_err(CE_WARN, "switched slip: no space\n");
#else
		printf ("switched slip: no space\n");
#endif SYSV
		return;
	}
	m->b_wptr += sizeof(struct sockaddr_in);
	*(struct sockaddr_in *) (m->b_wptr) = sock;
	m->b_cont = 0;
	for (mpp = &yhSlreq.waiting; *mpp; mpp = &((*mpp)->b_cont));
	*mpp = m;
	yhSlnumqueued++;
	if (yhSlnumqueued > yhSlchkqueued)
		yhSlchkqueued = yhSlnumqueued;
	splx(s);
	return;
}

/*
 * restart sleeping daemon child or slave by sending ioctl ack 
 */
/* static */
yhslhangup(rt)
	register struct rtentry *rt;
{
	if (!(rt->rt_prov) || !(rt->rt_prov->unswitch)) {
#ifdef SYSV
		cmn_err(CE_WARN, "yhrtunswitch: null pointer");
#else
		printf ("yhrtunswitch: null pointer");
#endif SYSV
		return;
	}
	putnext(rt->rt_prov->qbot, rt->rt_prov->unswitch);
	rt->rt_prov->unswitch = 0;
	if (!(rt->rt_flags & RTF_SLAVE))
		rt->rt_prov = yhloopprov;
	return;
}

/*
 * called when yhslstat interrupted 
 */
yhswdetach(prov)
	struct ip_provider *prov;
{
	struct route    route;
	struct rtentry *rt;

	if (prov->unswitch == NULL)
		return;

	bzero((caddr_t)&route, sizeof(route));
	bcopy((caddr_t)&(prov->if_dstaddr), (caddr_t)&(route.ro_dst), 
	  sizeof(route.ro_dst));
	yhrtalloc(&route, 0);
	if (route.ro_rt) {
		rt = RT(route.ro_rt);
		yhrtfree(route.ro_rt);
		SSS_SETSTATE(rt, SSS_CLEARWAIT);
		yhrthangup(rt);
		if (!(rt->rt_flags & RTF_SLAVE))
			rt->rt_prov = yhloopprov;
	}
	prov->unswitch = NULL;
}

/*
 * get a connection request (SIOCSLGETREQ ioctl): daemon only 
 */
/* ARGSUSED */
yhslgetreq(wrq, mp, prov)
	queue_t        *wrq;
	mblk_t         *mp;
	struct ip_provider *prov;
{
	register struct ifreq *ifr = (struct ifreq *) (mp->b_cont->b_rptr);
	register mblk_t *mp1;
	register int    s;

	s = splstr();
	if (ifr->ifr_name[0]) {
		yhSlreq.mp = NULL;
		yhioc_ack(wrq, mp);
	} else if (yhSlreq.mp) {
		yhioc_error(wrq, mp, EBUSY);
	} else if (mp1 = yhSlreq.waiting) {
		ifr->ifr_addr = *(struct sockaddr *) (mp1->b_rptr);
		yhSlreq.waiting = mp1->b_cont;
		mp1->b_cont = 0;
		freemsg(mp1);
		yhSlnumqueued--;
		yhioc_ack(wrq, mp);
	} else {
		yhSlreq.mp = mp;
		yhSlreq.queue = wrq;
	}
	splx(s);
	return;
}

/*
 * stat ioctl (SIOSLSTAT ioctl): inform kernel of call status 
 */
yhslstat(wrq, mp, prov)
	queue_t        *wrq;
	mblk_t         *mp;
	struct ip_provider *prov;
{
	register struct ifreq *ifr;
	struct route    route;
	register struct rtentry *rt;
	void            yhslstartconn();
	register int    s;

	ifr = (struct ifreq *) mp->b_cont->b_rptr;
	route.ro_dst = ifr->ifr_addr;
	route.ro_rt = 0;
	yhrtalloc(&route, 0);
	if (!route.ro_rt) {
		yhioc_error(wrq, mp, ENOENT);
		return;
	}
	rt = RT(route.ro_rt);
	yhrtfree(route.ro_rt);
	s = splstr();
	if (prov == NULL) {
		if ((rt->rt_flags & RTF_SWITCHED)	/* call failed */
		    &&SSS_GETSTATE(rt) == SSS_DIALING) {
			yhrtnodial(rt);
			yhioc_ack(wrq, mp);
		} else {	/* slave cant fail */
			yhioc_error(wrq, mp, EINVAL);
		}
		return;
	}
	switch (SSS_GETSTATE(rt) | (rt->rt_flags & RTF_SWITCHED)) {
	case RTF_SWITCHED | SSS_DIALING:	/* master */
		break;
	case SSS_NOCONN:	/* 1-way slave */
		rt->rt_flags |= (RTF_SWITCHED | RTF_SLAVE);
	case RTF_SWITCHED | SSS_NOCONN:	/* 2-way slave */
		if (rt->rt_prov && rt->rt_prov != yhloopprov && rt->rt_prov != prov) {
			yhioc_error(wrq, mp, EEXIST);
			return;
		}
		break;
	default:
		yhioc_error(wrq, mp, EBADF);
		return;
	}
	rt->rt_prov = prov;
	SSS_SETSTATE(rt, SSS_OPENWAIT);
	yhsltimeout(yhslstartconn, rt, SLIP_PEERWAIT);
	prov->unswitch = mp;
	return;
}

/*
 * higher level routines call yhsldefer with packets to be retried 
 */
yhsldefer(routine, a0, a1, a2, a3, a4, a5, a6, a7)
	int             (*routine) ();
{
	register mblk_t *m;
	struct defstruct *dp;
	register mblk_t **mpp;
	register int    s;

	if (yhSlnumdeferred >= yhSlmaxdeferred) {
#ifdef SYSV
		cmn_err(CE_WARN, "switched slip: dropping deferral\n");
#else
		printf ("switched slip: dropping deferral\n");
#endif SYSV
		return;
	}
	if ((m = allocb(sizeof(struct defstruct), BPRI_MED)) == NULL) {
#ifdef SYSV
		cmn_err(CE_WARN, "switched slip: no space\n");
#else
		printf ("switched slip: no space\n");
#endif SYSV
		return;
	}
	m->b_wptr += sizeof(struct defstruct);
	m->b_cont = 0;
	dp = mtod(m, struct defstruct *);
	dp->routine = routine;
	dp->args[0] = a0;
	dp->args[1] = a1;
	dp->args[2] = a2;
	dp->args[3] = a3;
	dp->args[4] = a4;
	dp->args[5] = a5;
	dp->args[6] = a6;
	dp->args[7] = a7;
	s = splstr();
	for (mpp = &yhSldeferred; *mpp; mpp = &((*mpp)->b_cont));
	*mpp = m;
	yhSlnumdeferred++;
	if (yhSlnumdeferred > yhSlchkdeferred)
		yhSlchkdeferred = yhSlnumdeferred;
	splx(s);
}

/* static */
yhslsenddeferred()
{
	register mblk_t *m, *m0;
	register struct defstruct *dp;
	register int    i;
	register int    s;

	s = splstr();
	m = yhSldeferred;
	yhSldeferred = 0;
	yhSlnumdeferred = 0;
	splx(s);
	while (m) {
		dp = mtod(m, struct defstruct *);
		(*(dp->routine)) (dp->args[0], dp->args[1], dp->args[2],
				  dp->args[3], dp->args[4], dp->args[5],
				  dp->args[6], dp->args[7]);
#ifdef undef
		if (dp->freemask)
			for (i = 0; i < 8; i++)
				if (dp->freemask & 1 << i)
					freemsg(dp->args[i]);
#endif
		m0 = m->b_cont;
		m->b_cont = 0;
		freemsg(m);
		m = m0;
	}
}

/*
 * timeout that heeds the network semaphore and allows only one call with
 * given func and arg to be queued 
 */
/* static */
yhsltimeout(func, arg, secs)
	int             (*func) ();
caddr_t         arg;
{
	register mblk_t *m;
	register struct slevent *sep;
	register int    s;

	s = splstr();
	for (m = yhSlevents; m; m = m->b_cont) {
		sep = mtod(m, struct slevent *);
		if (sep->func == func && sep->arg == arg) {
			sep->time = lbolt + secs * HZ;
			splx(s);
			return;
		}
	}
	if (yhSlnumevents >= yhSlmaxevents) {
		splx(s);
#ifdef SYSV
		cmn_err(CE_WARN, "switched slip: dropping timeout\n");
#else
		printf ("switched slip: dropping timeout\n");
#endif SYSV
		return;
	}
	if ((m = allocb(sizeof(struct slevent), BPRI_MED)) == NULL) {
		splx(s);
#ifdef SYSV
		cmn_err(CE_WARN, "switched slip: no space\n");
#else
		printf ("switched slip: no space\n");
#endif SYSV
		return;
	}
	m->b_wptr += sizeof *sep;
	sep = mtod(m, struct slevent *);
	sep->time = lbolt + secs * HZ;
	sep->func = func;
	sep->arg = arg;
	m->b_cont = yhSlevents;
	yhSlevents = m;
	yhSlnumevents++;
	if (yhSlnumevents > yhSlchkevents)
		yhSlchkevents = yhSlnumevents;
	splx(s);
}

/*
 * called from ip_slowtimo to process slip timeouts 
 */
yhsltimein()
{
	register mblk_t *m, *mnext, **mprev = &yhSlevents;
	register struct slevent *sep;

	for (m = yhSlevents; m; m = mnext) {
		mnext = m->b_cont;
		sep = mtod(m, struct slevent *);
		if (sep->time > lbolt) {
			mprev = &(m->b_cont);
			continue;
		}
		*mprev = mnext;
		(*(sep->func)) (sep->arg);
		m->b_cont = 0;
		freemsg(m);
		yhSlnumevents--;
	}
}

 /* static */ void
yhioc_error(wrq, mp, errno)
	register queue_t *wrq;
	register mblk_t *mp;
{
	register struct iocblk *iocpb = (struct iocblk *) mp->b_rptr;

	iocpb->ioc_error = errno;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(wrq, mp);
}

 /* static */ void
yhioc_ack(wrq, mp)
	register queue_t *wrq;
	register mblk_t *mp;
{
	mp->b_datap->db_type = M_IOCACK;
	qreply(wrq, mp);
}

#endif /* SLIP */
