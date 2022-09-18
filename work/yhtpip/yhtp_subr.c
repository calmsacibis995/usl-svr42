
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
#include <io/stropts.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/transport/socket.h>
#include <net/transport/socketvar.h>
#include <net/yhtpip/protosw.h>
#include <net/yhtpip/yhif.h>
#include <svc/errno.h>

#ifdef SYSV
#include <net/transport/tihdr.h>
#else
#include <net/transport/tihdr.h>
#endif SYSV

#include <net/yhtpip/nihdr.h>

#ifdef SYSV
#ifdef SYSV
#include <util/cmn_err.h>
#endif
#endif SYSV

#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhin_pcb.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhip.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/yhip_icmp.h>
#include <net/yhtpip/yhip_str.h>
#include <net/yhtpip/yhtp.h>
#include <net/yhtpip/yhtp_fsm.h>
#include <net/yhtpip/yhtp_seq.h>
#include <net/yhtpip/yhtp_timer.h>
#include <net/yhtpip/yhtp_var.h>
#include <net/yhtpip/yhtpip.h>
#include <net/yhtpip/insrem.h>
#include <mem/kmem.h>

int	yhtp_ttl = YHTP_TTL;

extern queue_t *yhtp_qbot;

/*
 * Create template to be used to send yhtp packets on a connection. Call after
 * host entry created, allocates an mblk and fills in a skeletal yhtp/yhnp
 * header, minimizing the amount of work necessary when the connection is
 * used. 
 */
struct yhtpiphdr *
yhtp_template(tp)
	struct yhtpcb   *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	register mblk_t *bp;
	register struct yhtpiphdr *n;

	if ((n = tp->t_template) == 0) {
		bp = allocb(sizeof(struct yhtpiphdr), BPRI_HI);
		if (bp == NULL)
			return (0);
		bp->b_rptr = bp->b_datap->db_lim - sizeof(struct yhtpiphdr);
		bp->b_wptr = bp->b_datap->db_lim;
		n = (struct yhtpiphdr *) bp->b_rptr;
		tp->t_tmplhdr = bp;
	}
	n->ti_next = 0;
	n->ti_mblk = 0;
	n->ti_x1 = 0;
	n->ti_pr = IPPROTO_YHTP;
	n->ti_len = htons(sizeof(struct yhtpiphdr) - sizeof(struct ip));
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_faddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return (n);
}

/*
 * Send a single message to the YHTP at address specified by the given YHTP/IP
 * header.  If flags==0, then we make a copy of the yhtpiphdr at ti and send
 * directly to the addressed host. This is used to force keep alive messages
 * out using the YHTP template for a connection tp->t_template.  If flags are
 * given then we send a message back to the YHTP which originated the segment
 * ti, and discard the mbuf containing it and any other attached mblocks. 
 *
 * In any case the ack and sequence number of the transmitted segment are as
 * specified by the parameters. 
 */
yhtp_respond(bp, tp, ti, ack, seq, flags)
	mblk_t         *bp;
	struct yhtpcb   *tp;
	register struct yhtpiphdr *ti;
	yhtp_seq         ack, seq;
	int             flags;
{
	mblk_t         *bp0;
	int             win = 0, tlen;
	struct route    yhtproute, *ro = 0;
	struct ip_unitdata_req *ipreq;

	if (tp) {
		if (tp->t_inpcb->inp_q && !(tp->t_inpcb->inp_state & SS_CANTRCVMORE)) {
			win = tp->t_inpcb->inp_q->q_hiwat - tp->t_iqsize;
			if (win < (long)(tp->rcv_adv - tp->rcv_nxt))
				win = (long)(tp->rcv_adv - tp->rcv_nxt);
		}
		ro = &tp->t_inpcb->inp_route;
	} else {
		ro = &yhtproute;
		bzero((caddr_t) ro, sizeof(*ro));
	}
	if (flags == 0) {
		bp = allocb(sizeof(struct yhtpiphdr) + 1, BPRI_HI);
		if (bp == NULL)
			return;
#ifdef YHTP_COMPAT_42
		tlen = 1;
#else
		tlen = 0;
#endif
		bp->b_wptr += sizeof(struct yhtpiphdr) + tlen;
		bcopy((char *) ti, (char *) bp->b_rptr, sizeof(*ti));
		ti = (struct yhtpiphdr *) bp->b_rptr;
		flags = TH_ACK;
	} else {
		freemsg(bp->b_cont);
		bp->b_cont = NULL;
		bp->b_rptr = (u_char *) ti;
		tlen = 0;
		bp->b_wptr = bp->b_rptr + sizeof(struct yhtpiphdr);
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
		xchg(ti->ti_dst.s_addr, ti->ti_src.s_addr, u_long);
		xchg(ti->ti_dport, ti->ti_sport, u_short);
#undef xchg
	}
	ti->ti_next = 0;
	ti->ti_mblk = 0;
	ti->ti_x1 = 0;
	ti->ti_len = htons((u_short) (sizeof(struct yhtphdr) + tlen));
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	ti->ti_off = sizeof(struct yhtphdr) >> 2;
	ti->ti_flags = flags;
	ti->ti_win = htons((u_short) win);
	ti->ti_urp = 0;
	ti->ti_sum = 0;
	ti->ti_sum = yhin_cksum(bp, (int) (sizeof(struct yhtpiphdr) + tlen));
	((struct ip *) ti)->ip_len = sizeof(struct yhtpiphdr) + tlen;
	((struct ip *) ti)->ip_ttl = yhtp_ttl;
	bp0 = allocb(sizeof(struct ip_unitdata_req), BPRI_HI);
	if (bp0 == NULL) {
		freeb(bp);
		return;
	}
	bp0->b_cont = bp;
	bp = bp0;
	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += sizeof(struct ip_unitdata_req);
	ipreq = (struct ip_unitdata_req *) bp->b_rptr;
	ipreq->dl_primitive = N_UNITDATA_REQ;
	ipreq->options = 0;
	ipreq->route = *ro;
	ipreq->flags = 0;
	ipreq->dl_dest_addr_length = sizeof(ti->ti_dst);
	ipreq->dl_dest_addr_offset = sizeof(struct ip_unitdata_req)
		- sizeof(struct in_addr);
	ipreq->ip_addr = ti->ti_dst;
	if (yhtp_qbot) {
		if (ro->ro_rt)
			RT(ro->ro_rt)->rt_refcnt++;
		putnext(yhtp_qbot, bp);
	} else {
		freemsg(bp);
	}
}

/*
 * Create a new YHTP control block, making an empty reassembly queue and
 * hooking it to the argument protocol control block. 
 */
struct yhtpcb   *
yhtp_newyhtpcb(inp)
	struct inpcb   *inp;
{
	register struct yhtpcb *tp;

	tp = (struct yhtpcb *) kmem_alloc(sizeof(struct yhtpcb), KM_NOSLEEP);
	if (tp == NULL) {
		STRLOG(YHTPM_ID, 1, 2, SL_TRACE, "newyhtpcb no mem inp %x",
		       inp);
		return ((struct yhtpcb *) 0);
	}
	bzero((char *) tp, sizeof(struct yhtpcb));
	tp->seg_next = (struct yhtpiphdr *) tp;
	tp->t_q = tp->t_q0 = (struct yhtpcb *) NULL;
	tp->t_maxseg = IP_MAXPACKET;	/* large number */
	tp->t_flags = 0;	/* sends options! */
	tp->t_inpcb = inp;
	/*
	 * Init srtt to YHTPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = YHTPTV_SRTTBASE;
	tp->t_rttvar = YHTPTV_SRTTDFLT << 2;
	YHTPT_RANGESET(tp->t_rxtcur, 
	    ((YHTPTV_SRTTBASE >> 2) + (YHTPTV_SRTTDFLT << 2)) >> 1,
	    YHTPTV_MIN, YHTPTV_REXMTMAX);
	tp->t_linger = 0;
	tp->t_inq = NULL;
	tp->snd_cwnd = 65535;
	tp->snd_ssthresh = 65535;		/* XXX */
	tp->t_maxwin = (inp->inp_q ? inp->inp_q->q_hiwat : 5120);
	tp->t_iqurp = -1;			/* no urgent data present */
	inp->inp_ppcb = (caddr_t) tp;
	STRLOG(YHTPM_ID, 1, 5, SL_TRACE, "newyhtpcb yhtcb %x inp %x", tp, inp);
	return (tp);
}

/*
 * Drop a YHTP connection, reporting the specified error.  If connection is
 * synchronized, then send a ER to peer. 
 */

struct yhtpcb   *
yhtp_drop(tp, errno)
	register struct yhtpcb *tp;
	int             errno;
{

	STRLOG(YHTPM_ID, 1, 4, SL_TRACE, "yhtp_drop yhtcb %x inp %x err %d.",
	       tp, tp->t_inpcb, errno);

	if (YHTPS_HAVERCVDSYN(tp->t_state)) {
		yhtp_cancelinger(tp);
		tp->t_state = YHTPS_CLOSED;
		yhtp_io(tp, TF_NEEDOUT, NULL);
		yhtpstat.yhtps_drops++;
	} else
		yhtpstat.yhtps_conndrops++;
	tp->t_inpcb->inp_error = errno;
	return (yhtp_close(tp, errno));
}

/*
 * Close a YHTP control block: discard all space held by the yhtp discard
 * internet protocol block, if not still referenced 
 */
struct yhtpcb   *
yhtp_close(tp, error)
	register struct yhtpcb *tp;
	int             error;
{
	register struct yhtpiphdr *t;
	struct inpcb   *inp = tp->t_inpcb;

	STRLOG(YHTPM_ID, 1, 5, SL_TRACE, "yhtp_close yhtcb %x err %d",
	       tp, error);

        for (t = tp->seg_next; t != (struct yhtpiphdr *)tp; t = tp->seg_next) {
		dequenxt((struct vq *) tp);
		freemsg(t->ti_mblk);
	}
	yhinpisdisconnected(tp->t_inpcb, error);
	inp->inp_laddr.s_addr = INADDR_ANY;
	inp->inp_lport = 0;
	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;

	yhtpstat.yhtps_closed++;
	if (inp->inp_state & SS_NOFDREF) {
		yhtp_freespc(tp);
		return ((struct yhtpcb *) 0);
	}
	return (tp);
}

yhtp_freespc(tp)
	struct yhtpcb   *tp;
{
	struct inpcb   *inp = tp->t_inpcb;
	mblk_t         *bp, *bp0;
	int 		s;

	STRLOG(YHTPM_ID, 1, 9, SL_TRACE, "yhtp_freespc pcb %x", inp);
	s = splstr();
	tp->t_flags &= ~(TF_NEEDIN|TF_NEEDOUT);
	splx(s);
	if (tp->t_template)
		(void) freeb(tp->t_tmplhdr);

	bp = tp->t_qfirst;
	while (bp) {
		bp0 = bp->b_next;
		freemsg(bp);
		bp = bp0;
	}

	if (tp->t_head) {
		if (!yhtpqremque(tp, 0) && !yhtpqremque(tp, 1))
#ifdef SYSV
			cmn_err(CE_PANIC, "yhtp_freespc remque");
#else
			panic( "yhtp_freespc remque");
#endif
	}
	bp = tp->t_inq;
	while (bp) {
		bp0 = bp->b_next;
		freemsg(bp);
		bp = bp0;
	}
	inp->inp_ppcb = 0;
	(void) kmem_free(tp, sizeof(struct yhtpcb));
	yhin_pcbdetach(inp);
}

yhtp_drain()
{

}

yhtp_ctlinput(bp)
	mblk_t         *bp;
{
	struct ip_ctlmsg *ctl;
	extern u_char   yhinetctlerrmap[];
	int             yhtp_quench(), yhin_rtchange();
	struct sockaddr_in src,dst;
	int yhtp_errdiscon();

	ctl = (struct ip_ctlmsg *) bp->b_rptr;
	if ((unsigned) ctl->command > PRC_NCMDS)
		return;
	if (ctl->src_addr.s_addr == INADDR_ANY)
		return;
	dst.sin_family = htons(AF_OSI);
	dst.sin_addr.s_addr = ctl->src_addr.s_addr;
	dst.sin_port = ctl->src_port;
	src.sin_family = htons(AF_OSI);
	src.sin_addr.s_addr = ctl->dst_addr.s_addr;
	src.sin_port = ctl->dst_port;

	switch (ctl->command) {

	case PRC_QUENCH:
		yhin_pcbnotify(&yhtcb, &dst, &src, 0, yhtp_quench, 0);
		break;

	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		yhin_pcbnotify(&yhtcb, &dst, &src, 0, yhin_rtchange, 0);
		break;

	default:
		if (yhinetctlerrmap[ctl->command] == 0)
			return;	/* XXX */
		yhin_pcbnotify(&yhtcb, &dst, &src,
			 (int) yhinetctlerrmap[ctl->command], yhtp_errdiscon, 1);
	}
}

/*
 * When a source quench is received, close congestion window
 * to one segment.  We will gradually open it again as we proceed.
 */
yhtp_quench(inp)
	struct inpcb *inp;
{
	struct yhtpcb *tp = intoyhtpcb(inp);

	if (tp)
		tp->snd_cwnd = tp->t_maxseg;
}

/*
 * Save data that arrives before the user has accepted the connection (and
 * therefore given us a stream queue).  As well as data which arrives when
 * there is no room upstream.  
 */
yhtp_enqdata(tp, bp, urp)
	struct yhtpcb   *tp;
	mblk_t         *bp;
	int		urp;	/* pointer to urgent data; -1 if none */
{
	if (!bp)
		return;
	STRLOG(YHTPM_ID, 2, 5, SL_TRACE, "yhtp_enqdata q %x", tp->t_inpcb->inp_q);
	if (tp->t_qfirst) {
		tp->t_qlast->b_next = bp;
		tp->t_qlast = bp;
	} else {
		tp->t_qfirst = tp->t_qlast = bp;
	}

	if (urp != -1)
		tp->t_iqurp = tp->t_iqsize + urp;
	tp->t_iqsize += msgdsize(bp);
	bp->b_next = NULL;
	return;
}

void
yhtp_calldeq(q)
	queue_t		*q;
{
	extern struct streamtab yhtpinfo;
	struct yhtpcb *tp;

	/*
	 * Make sure this is in fact a yhtp queue (since we use bufcall,
	 * it is possible that this routine will be called after the yhtp
	 * stream has been closed).  Also make sure this should be done
	 * for this connection, in case the above happens and the queue
	 * has been reused for a new YHTP endpoint.
	 */
	if (q->q_qinfo == yhtpinfo.st_rdinit
	    && (q->q_ptr)
	    && (tp = qtoyhtpcb(q))
	    && (tp->t_iqsize || YHTPS_HAVERCVDFIN(tp->t_state)))
		qenable(q);
}

yhtp_deqdata(q)
	queue_t        *q;
{
	register struct yhtpcb *tp;
	register mblk_t *bp, *bp2;
	extern mblk_t  *yhheaderize();
	int             win;

	if (q->q_ptr == NULL) {
#ifdef SYSV
		cmn_err(CE_WARN, "yhtp_deqdata: null q_ptr wq %x", WR(q));
#else
		printf( "yhtp_deqdata: null q_ptr wq %x", WR(q));
#endif
		return;
	}

	tp = qtoyhtpcb(q);

	while ((bp = tp->t_qfirst) && canput(q->q_next)) {

		if ((tp->t_iqurp >= 0) && (tp->t_iqurp < msgdsize(bp))) {
			if (yhtp_passoobup(tp, bp, q, tp->t_iqurp)) {
				tp->t_iqsize -= msgdsize(bp);
				tp->t_iqurp = -1;
			} else {
				timeout(yhtp_calldeq,q,HZ);
				break;
			}
		} else {
			if (bp2 = yhheaderize(bp)) {
				STRLOG(YHTPM_ID, 2, 5, SL_TRACE, "yhtp_deq up q %x", q);
				tp->t_qfirst = bp->b_next;
				bp->b_next = NULL;
				if (tp->t_qfirst == NULL) {
					tp->t_qlast = NULL;
				}
				tp->t_iqsize -= msgdsize(bp);
				if (tp->t_iqurp >= 0)
					tp->t_iqurp -= msgdsize(bp);
				putnext(q, bp2);
			}
			else {
				timeout(yhtp_calldeq,q,HZ);
				break;
			}
		}
	}

	if (YHTPS_HAVERCVDFIN(tp->t_state) && !tp->t_qfirst) {
		if (!yhinpordrelind((struct inpcb *) q->q_ptr)) {
			if (!bufcall(sizeof(struct T_ordrel_ind),BPRI_HI,yhtp_calldeq,q))
				timeout(yhtp_calldeq,q,HZ);
		}
	}

	if (!YHTPS_HAVERCVDFIN(tp->t_state)) {
		int	hiwat;

		hiwat = (q ? q->q_hiwat : 5120);
		win = hiwat - tp->t_iqsize;
		if (win > 0) {
			int adv = win - (tp->rcv_adv - tp->rcv_nxt);

			if ((tp->t_iqsize == 0 && adv >= (int)(2 * tp->t_maxseg))
			    || (100 * adv / hiwat >= 35))
				yhtp_io(tp, TF_NEEDOUT, 0);
		}
	}
}

struct yhtpcb *yhtp_output(), *yhtp_uinput(), *yhtp_dotimers();

yhtp_io(tp, flag, bp)
	register struct yhtpcb *tp;
	int flag;
	mblk_t *bp;
{
	int s;
	int oflag, tflag;
	struct yhtpcb *(*func)();
	mblk_t **bpp;
 
	s = splstr();
	if (flag == TF_NEEDIN) {
		for (bpp = &tp->t_inq; *bpp; bpp = &(*bpp)->b_next)
			;
		*bpp = bp;
		bp->b_next = NULL;
	}
	tp->t_flags |= flag;
	if (!(tp->t_flags & TF_IOLOCK)) {
                tp->t_flags |= TF_IOLOCK;
                for (;;) {
			/* TF_NEED* flags cleared by service routines */
			if (tp->t_flags & TF_NEEDTIMER)
				func = yhtp_dotimers;
			else if (tp->t_flags & TF_NEEDOUT)
				func = yhtp_output;
			else if (tp->t_flags & TF_NEEDIN)
				func = yhtp_uinput;
			else {
				tp->t_flags &= ~TF_IOLOCK;
				break;
			}
			splx(s);
			if (!(*func)(tp))
				return;
			s = splstr();
		}
	}
	splx(s);
}
