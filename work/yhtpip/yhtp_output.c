
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
#include <net/yhtpip/protosw.h>
#include <net/transport/socket.h>
#include <net/transport/socketvar.h>
#include <net/yhtpip/yhif.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <net/yhtpip/nihdr.h>
#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhin_pcb.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhip.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/yhtp.h>


#define	YHTPOUTFLAGS

#include <net/yhtpip/yhtp_fsm.h>
#include <net/yhtpip/yhtp_seq.h>
#include <net/yhtpip/yhtp_timer.h>
#include <net/yhtpip/yhtp_var.h>
#include <net/yhtpip/yhtpip.h>
#include <net/yhtpip/yhtp_debug.h>
#include <net/yhtpip/yhip_str.h>

#ifdef SYSV
#ifdef SYSV
#include <util/cmn_err.h>
#endif
#endif SYSV

/*
 * Initial options. 
 */
u_char          yhtp_initopt[4] = {YHTPOPT_MAXSEG, 4, 0x0, 0x0,};
extern int      yhtpcksum;
extern int      yhtpalldebug;
extern queue_t *yhtp_qbot;
mblk_t         *yhtp_dupblks();

/*
 * Yhtp output routine: figure out what should be sent and send it. 
 */
yhtp_out(tp)
	struct yhtpcb *tp;
{
	yhtp_io(tp, TF_NEEDOUT, NULL);
}

struct yhtpcb *
yhtp_output(tp)
	register struct yhtpcb *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	queue_t        *q = inp->inp_q;
	register int    len;	/* amount of new data we have to send */
	register int	win;	/* current send or receive window */
	mblk_t         *bp0;
	int             off;	/* offset of new data from snd.una */
	int		flags;
	register mblk_t *bp;
	register struct yhtpiphdr *ti;
	u_char         *opt;
	unsigned        optlen = 0;
	int             idle;	/* flag - set if we have no outstanding
				 * unacknowledged data */
	int		sendalot; /* flag - set if we have more than one MSS 
				   * worth of data to send */
	struct ip_unitdata_req *ipreq;
	register int    s;
	u_long		recvspace;
	
	recvspace = (q ? q->q_hiwat : 5120);

	idle = (tp->snd_max == tp->snd_una);
	STRLOG(YHTPM_ID, 2, 8, SL_TRACE, "got to yhtp_output");
again:
	s = splstr();
	tp->t_flags &= ~TF_NEEDOUT;
	splx(s);
	sendalot = 0;
	off = tp->snd_nxt - tp->snd_una;
	win = MIN(tp->snd_wnd, tp->snd_cwnd);

	/*
	 * If in persist timeout with window of 0, send 1 byte. Otherwise, if
	 * window is small but nonzero and timer expired, we will send what
	 * we can and go to transmit state. 
	 */
	if (tp->t_force) {
		if (win == 0)
			win = 1;
		else {
			tp->t_timer[YHTPT_PERSIST] = 0;
			tp->t_rxtshift = 0;
		}
	}
	if (q) {
		len = MIN(tp->t_qsize, win) - off;
	} else {
		len = 0;
	}
	flags = yhtp_outflags[tp->t_state];

	if (len < 0) {
		len = 0;
		if (win == 0) {
			tp->t_timer[YHTPT_REXMT] = 0;
			tp->snd_nxt = tp->snd_una;
		}
	}
	if (len > (int)tp->t_maxseg) {
		len = tp->t_maxseg;
		sendalot = 1;
	}
	if (SEQ_LT(tp->snd_nxt + len, tp->snd_una + tp->t_qsize))
		flags &= ~TH_FIN;
	else if (flags & TH_FIN)
		STRLOG(YHTPM_ID, 1, 9, SL_TRACE,
		       "FIN not suppressed: %d >= %d",
		       tp->snd_nxt + len, tp->snd_una + tp->t_qsize);

	/* calculate receive window */
	win = recvspace - tp->t_iqsize;
	if (win < 0)
		/* module above us can over-run our high water mark */
		win = 0;

	if (flags & TH_FIN &&
	    ((tp->t_flags & TF_SENTFIN) == 0 || tp->snd_nxt == tp->snd_una))
		goto send;
	/*
	 * Send if we owe peer an ACK. 
	 */
	if (tp->t_flags & TF_ACKNOW)
		goto send;
	if (flags & (TH_SYN | TH_RST))
		goto send;
	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;

	/*
	 * Sender silly window avoidance.  If connection is idle and can send
	 * all data, a maximum segment, at least a maximum default-size
	 * segment do it, or are forced, do it; otherwise don't bother. If
	 * peer's buffer is tiny, then send when window is at least half
	 * open. If retransmitting (possibly after persist timer forced us to
	 * send into a small window), then must resend. 
	 */
	if (len) {
		if (len == tp->t_maxseg)
			goto send;
		if ((idle || tp->t_flags & TF_NODELAY) &&
		    len + off >= tp->t_qsize)
			goto send;
		if (tp->t_force)
			goto send;
		if (len >= (int)(tp->max_sndwnd / 2))
			goto send;
		if (SEQ_LT(tp->snd_nxt, tp->snd_max))
			goto send;
	}
	if (win > 0) {
		int adv = win - (tp->rcv_adv - tp->rcv_nxt);

		if (tp->t_iqsize == 0 && adv >= 2 * (int)tp->t_maxseg)
			goto send;
		if (100 * adv / recvspace >= 35)
			goto send;
	}

	/*
	 * YHTP window updates are not reliable, rather a polling protocol
	 * using ``persist'' packets is used to insure receipt of window
	 * updates.  The three ``states'' for the output side are: idle ot
	 * doing retransmits or persists persisting		to move a
	 * small or zero window (re)transmitting	and thereby not
	 * persisting 
	 *
	 * tp->t_timer[YHTPT_PERSIST] is set when we are in persist state.
	 * tp->t_force is set when we are called to send a persist packet.
	 * tp->t_timer[YHTPT_REXMT] is set when we are retransmitting The
	 * output side is idle when both timers are zero. 
	 *
	 * If send window is too small, there is data to transmit, and no
	 * retransmit or persist is pending, then go to persist state. If
	 * nothing happens soon, send when timer expires: if window is
	 * nonzero, transmit what we can, otherwise force out a byte. 
	 */
	if (tp->t_qsize && tp->t_timer[YHTPT_REXMT] == 0 &&
	    tp->t_timer[YHTPT_PERSIST] == 0) {
		tp->t_rxtshift = 0;
		yhtp_setpersist(tp);
	}
	/*
	 * No reason to send a segment, just return. 
	 */
	return(tp);

send:
	/*
	 * Grab a header buffer, attaching a duplicate of data to be
	 * transmitted, and initialize the header from the template for sends
	 * on this connection. 
	 */
	bp = allocb(sizeof(struct yhtpiphdr), BPRI_HI);
	if (bp == NULL) {
		bufcall(sizeof(struct yhtpiphdr), BPRI_HI, yhtp_out, (long) tp);
		return(tp);
	}
	bp->b_rptr = bp->b_datap->db_lim - sizeof(struct yhtpiphdr);
	bp->b_wptr = bp->b_datap->db_lim;
	if (len) {
		if (tp->t_force && len == 1)
			yhtpstat.yhtps_sndprobe++;
		else if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
			yhtpstat.yhtps_sndrexmitpack++;
			yhtpstat.yhtps_sndrexmitbyte += len;
		} else {
			yhtpstat.yhtps_sndpack++;
			yhtpstat.yhtps_sndbyte += len;
		}
		ASSERT(q);
		bp->b_cont = (q ? yhtp_dupblks(WR(q), off, len) : 0);
		if (bp->b_cont == 0) {
			len = 0;
		}
	} else if (tp->t_flags & TF_ACKNOW)
		yhtpstat.yhtps_sndacks++;
	else if (flags & (TH_SYN|TH_FIN|TH_RST))
		yhtpstat.yhtps_sndctrl++;
	else if (SEQ_GT(tp->snd_up, tp->snd_una))
		yhtpstat.yhtps_sndurg++;
	else
		yhtpstat.yhtps_sndwinup++;

	ti = (struct yhtpiphdr *) bp->b_rptr;
	if (tp->t_template == 0)
#ifdef SYSV
		cmn_err(CE_PANIC, "yhtp_output: null t_template");
#else
		panic( "yhtp_output: null t_template");
#endif

	bcopy((caddr_t) tp->t_template, (caddr_t) ti, sizeof(struct yhtpiphdr));

	if (flags & TH_FIN && tp->t_flags & TF_SENTFIN &&
	    tp->snd_nxt == tp->snd_max)
		tp->snd_nxt--;
	if (flags & TH_SYN)
		if (!(tp->t_flags & TF_SENTSYN))
			tp->t_flags |= TF_SENTSYN;
		else
			tp->snd_nxt = tp->snd_una;
	ti->ti_seq = htonl(tp->snd_nxt);
	ti->ti_ack = htonl(tp->rcv_nxt);
	/*
	 * Before ESTABLISHED, force sending of initial options unless YHTP
	 * set to not do any options. 
	 */
	opt = NULL;
	if (flags & TH_SYN && (tp->t_flags & TF_NOOPT) == 0) {
		u_short         mss;

		mss = MIN(recvspace / 2, yhtp_mss(tp));
		if (mss > IP_MSS - sizeof(struct yhtpiphdr)) {
			opt = (u_char *) yhtp_initopt;
			optlen = sizeof(yhtp_initopt);
			opt[2] = mss >> 8 & 0xff;
			opt[3] = mss & 0xff;
		}
		else
			mss = IP_MSS - sizeof(struct yhtpiphdr);
	}
	if (opt) {
		bp0 = bp->b_cont;
		bp->b_cont = allocb((int) (optlen + 4), BPRI_HI);
		if (bp->b_cont == 0) {
			(void) freeb(bp);
			freemsg(bp0);
			bufcall(optlen + 4, BPRI_HI, yhtp_out, (long) tp);
			return(tp);
		}
		bp->b_cont->b_cont = bp0;
		bp0 = bp->b_cont;
		bp0->b_wptr += optlen;
		bcopy((caddr_t) opt, (caddr_t) bp0->b_rptr, optlen);
		opt = (u_char *) (bp0->b_rptr + optlen);
		while (msgblen(bp0) & 0x3) {
			*opt++ = YHTPOPT_EOL;
			bp0->b_wptr++;
		}
		optlen = msgblen(bp0);
		ti->ti_off = (sizeof(struct yhtphdr) + optlen) >> 2;
	}
	if (flags & TH_FIN)
		STRLOG(YHTPM_ID, 1, 7, SL_TRACE, "sent FIN yhtcb %x pcb %x",
		       tp, tp->t_inpcb);
	ti->ti_flags = flags;
	/*
	 * Calculate receive window.  Don't shrink window, but avoid silly
	 * window syndrome. 
	 */
	if (win < (long)(recvspace / 4) && win < (long)tp->t_maxseg)
		win = 0;
	if (win < (long)(tp->rcv_adv - tp->rcv_nxt))
		win = (long)(tp->rcv_adv - tp->rcv_nxt);
	if (win > IP_MAXPACKET)
		win = IP_MAXPACKET;
	ti->ti_win = htons((u_short) win);
	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		ti->ti_urp = htons((u_short) (tp->snd_up - tp->snd_nxt));
		ti->ti_flags |= TH_URG;
	} else
		/*
		 * If no urgent pointer to send, then we pull the urgent
		 * pointer to the left edge of the send window so that it
		 * doesn't drift into the send window on sequence number
		 * wraparound. 
		 */
		tp->snd_up = tp->snd_una;	/* drag it along */
	/*
	 * If anything to send and we can send it all, set PUSH. (This will
	 * keep happy those implementations which only give data to the user
	 * when a buffer fills or a PUSH comes in.) 
	 */
	if (len && off + len == tp->t_qsize)
		ti->ti_flags |= TH_PUSH;

	/*
	 * Put YHTP length in extended header, and then checksum extended
	 * header and data. 
	 */
	if (len + optlen)
		ti->ti_len = htons((u_short) (sizeof(struct yhtphdr) +
							optlen + len));
	ti->ti_sum = yhin_cksum(bp, (int) (sizeof(struct yhtpiphdr)
					 + (int) optlen + len));

	/*
	 * In transmit state, time the transmission and arrange for the
	 * retransmit.  In persist state, just set snd_max. 
	 */
	if (tp->t_force == 0 || tp->t_timer[YHTPT_PERSIST] == 0) {
		yhtp_seq startseq = tp->snd_nxt;

		/*
		 * Advance snd_nxt over sequence space of this segment. 
		 */
		if (flags & TH_SYN)
			tp->snd_nxt++;
		if (flags & TH_FIN) {
			tp->snd_nxt++;
			tp->t_flags |= TF_SENTFIN;
		}
		tp->snd_nxt += len;
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			/*
			 * Time this transmission if not a retransmission and
			 * not currently timing anything. 
			 */
			if (tp->t_rtt == 0) {
				tp->t_rtt = 1;
				tp->t_rtseq = startseq;
				yhtpstat.yhtps_segstimed++;
			}
		}
		/*
		 * Set retransmit timer if not currently set,
		 * and not doing an ack or a keep-alive probe.
		 * Initial value for retransmit timer is smoothed
		 * round-trip time + 2 * round-trip time variance.
		 * Initialize shift counter which is used for backoff
		 * of retransmit time.
		 */
		if (tp->t_timer[YHTPT_REXMT] == 0 &&
		    tp->snd_nxt != tp->snd_una) {
			tp->t_timer[YHTPT_REXMT] = tp->t_rxtcur;
			if (tp->t_timer[YHTPT_PERSIST]) {
				tp->t_timer[YHTPT_PERSIST] = 0;
				tp->t_rxtshift = 0;
			}
		}
	} else if (SEQ_GT(tp->snd_nxt + len, tp->snd_max))
		tp->snd_max = tp->snd_nxt + len;

	/*
	 * Trace. 
	 */
	if ((inp->inp_protoopt & SO_DEBUG) || yhtpalldebug != 0)
		yhtp_trace(TA_OUTPUT, tp->t_state, tp, ti, 0);

	/*
	 * Fill in IP length and desired time to live and send to IP level. 
	 */
	((struct ip *) ti)->ip_len = sizeof(struct yhtpiphdr) + optlen + len;
	((struct ip *) ti)->ip_ttl = YHTP_TTL;
	bp0 = allocb(sizeof(struct ip_unitdata_req), BPRI_HI);
	if (bp0 == NULL) {
		freemsg(bp);
		bufcall(sizeof(struct ip_unitdata_req), BPRI_HI,
			yhtp_out, (long) tp);
		return(tp);
	}
	bp0->b_cont = bp;
	bp = bp0;
	bp->b_wptr += sizeof(struct ip_unitdata_req);
	bp->b_datap->db_type = M_PROTO;
	ipreq = (struct ip_unitdata_req *) bp->b_rptr;
	ipreq->dl_primitive = N_UNITDATA_REQ;
	ipreq->options = inp->inp_options;
	ipreq->route = inp->inp_route;
	ipreq->flags = inp->inp_protoopt & SO_DONTROUTE;
	ipreq->dl_dest_addr_length = sizeof(ti->ti_dst);
	ipreq->dl_dest_addr_offset = sizeof(struct ip_unitdata_req)
		- sizeof(struct in_addr);
	ipreq->ip_addr = ti->ti_dst;
	if (yhtp_qbot) {
		if (inp->inp_route.ro_rt)
			RT(inp->inp_route.ro_rt)->rt_refcnt++;
		putnext(yhtp_qbot, bp);
	} else {
		freemsg(bp);
	}
	yhtpstat.yhtps_sndtotal++;

	/*
	 * Data sent (as far as we can tell). If this advertises a larger
	 * window than any other segment, then remember the size of the
	 * advertised window. Any pending ACK has now been sent. 
	 */
	if (win > 0 && SEQ_GT(tp->rcv_nxt + win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + win;
	tp->t_flags &= ~(TF_ACKNOW | TF_DELACK);
	if (sendalot)
		goto again;
	tp->t_force = 0;
	return(tp);
}

yhtp_setpersist(tp)
	register struct yhtpcb *tp;
{
	register t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;

	if (tp->t_timer[YHTPT_REXMT])
#ifdef SYSV
		cmn_err(CE_PANIC,"yhtp_output REXMT");
#else
		panic("yhtp_output REXMT");
#endif
	/*
	 * Start/restart persistance timer.
	 */
	YHTPT_RANGESET(tp->t_timer[YHTPT_PERSIST],
	    t * yhtp_backoff[tp->t_rxtshift],
	    YHTPTV_PERSMIN, YHTPTV_PERSMAX);
	if (tp->t_rxtshift < YHTP_MAXRXTSHIFT)
		tp->t_rxtshift++;
}

/*
 * yhtp_dupblks duplicates a range of blocks, then adjusts the headers to make
 * it exactly the offset and length we want. 
 */

mblk_t         *
yhtp_dupblks(q, off, len)
	queue_t        *q;
	register int    off, len;
{
	register mblk_t *bp, *nbp;
	mblk_t         *head, *newhead;
	int		n; 
	int		origoff = off;
	int		blocks = 0;	/* count of data blocks we've dup'd */

	STRLOG(YHTPM_ID, 2, 9, SL_TRACE, "yhtp_dupblks q %x off %d len %d",
	       q, off, len);


	head = bp = q->q_first;
	do {
		while (bp && off >= (n=msgblen(bp))) {
			off -= n;
			bp = bp->b_cont;
		}
	} while (!bp && head && (bp = head = head->b_next));
	if (!bp) {
		STRLOG(YHTPM_ID, 2, 1, SL_TRACE,
		       "yhtp_dupblks: not enough data: wq %x off %d",
		       q, origoff);
		return ((mblk_t *) NULL);
	}
	nbp = newhead = dupb(bp);
	blocks++;
	if (!nbp) {
		STRLOG(YHTPM_ID, 2, 2, SL_TRACE,
		       "yhtp_dupblks: alloc failure: wq %x", q);
		return ((mblk_t *)NULL);
	}
	nbp->b_rptr += off;
	len -= msgblen(nbp);
	while (len > 0 && ((bp = bp->b_cont) || head && (bp = head = head->b_next))) {
		nbp = nbp->b_cont = dupb(bp);
		blocks++;
		if (!nbp) {
			freemsg(newhead);
			STRLOG(YHTPM_ID, 2, 2, SL_TRACE,
			       "yhtp_dupblks: alloc failure: wq %x", q);
			return ((mblk_t *) NULL);
		}
		len -= msgblen(nbp);
	}
	if (len > 0) {
#ifdef SYSV
		cmn_err(CE_PANIC, "yhtp_dupblks");
#else
		panic( "yhtp_dupblks");
#endif
	}
	nbp->b_wptr += len;	/* adjust to correct size  (len <= 0) */
	nbp->b_cont = NULL;
#define MAXDUPBLOCKS 3
	/* XXX */
	if (blocks > MAXDUPBLOCKS)
		(void) pullupmsg(newhead, -1);
#undef MAXDUPBLOCKS
	return (newhead);
}
