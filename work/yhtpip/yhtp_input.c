
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

#ifdef SYSV
#include <net/transport/tihdr.h>
#else
#include <net/transport/tihdr.h>
#endif SYSV

#include <net/yhtpip/yhin.h>
#include <net/yhtpip/yhin_var.h>
#include <net/yhtpip/yhroute.h>
#include <net/yhtpip/yhin_pcb.h>
#include <net/yhtpip/yhin_systm.h>
#include <net/yhtpip/yhip.h>
#include <net/yhtpip/yhip_var.h>
#include <net/yhtpip/yhtp.h>
#include <net/yhtpip/yhtp_fsm.h>
#include <net/yhtpip/yhtp_seq.h>
#include <net/yhtpip/yhtp_timer.h>
#include <net/yhtpip/yhtp_var.h>
#include <net/yhtpip/yhtpip.h>
#include <net/yhtpip/yhtp_debug.h>
#include <net/yhtpip/insrem.h>
#include <net/yhtpip/yhip_str.h>

#ifdef SYSV
#ifdef SYSV
#include <util/cmn_err.h>
#endif
#endif SYSV

#include <proc/signal.h>

int		yhtpdprintf = 0;
int             yhtpprintfs = 0;
int             yhtpcksum = 1;
int		yhtprexmtthresh = 3;
struct yhtpiphdr yhtp_saveti;
struct yhtpstat  yhtpstat;
extern          yhtpnodelack;
extern int      yhtpalldebug;
struct yhtpcb   *yhtp_newyhtpcb();
struct inpcb   *yhin_pcballoc();
extern void	yhtp_calldeq();

#define	YHTP_REASS(tp, ti, bp, q, flags) { \
	mblk_t *nbp; \
		     \
	if ((ti)->ti_seq == (tp)->rcv_nxt && \
	    (tp)->seg_next == (struct yhtpiphdr *)(tp) && \
	    (tp)->t_state == YHTPS_ESTABLISHED && \
	    !((flags) & TH_URG)) { \
		(tp)->rcv_nxt += (ti)->ti_len; \
		(flags) = (ti)->ti_flags & TH_FIN; \
		yhtpstat.yhtps_rcvpack++; \
                yhtpstat.yhtps_rcvbyte += (ti)->ti_len; \
		while (bp && !msgblen(bp)) { \
			nbp = bp->b_cont; \
			freeb(bp); \
			bp = nbp; \
		} \
		if (bp) { \
			if ((tp->t_iqsize == 0) && (q) && canput(q->q_next)) { \
				if (nbp = yhheaderize(bp)) { \
					STRLOG(YHTPM_ID, 2, 5, SL_TRACE, "yhtp sending %d, seq %d, up wq %x", (ti)->ti_len, tp->rcv_nxt%10000, WR(q)); \
					putnext(q, nbp); \
				} else { \
					yhtp_enqdata(tp, bp, -1); \
					timeout(yhtp_calldeq, q, HZ); \
				} \
			} else { \
				yhtp_enqdata(tp, bp, -1); \
			} \
		} \
	} else \
		(flags) = yhtp_reass(q, (tp), (ti), (bp)); \
}

mblk_t         *yhheaderize();

yhtp_reass(sq, tp, ti, bp)
	queue_t        *sq;
	register struct yhtpcb *tp;
	register struct yhtpiphdr *ti;
	mblk_t         *bp;
{
	register struct yhtpiphdr *q, *qprev;
	mblk_t         *nbp;
	struct inpcb   *inp = tp->t_inpcb;
	int             flags;
	struct yhtpiphdr *nti;

	/*
	 * Call with ti==0 after become established to force pre-ESTABLISHED
	 * data up to user socket. 
	 */
	if (bp == 0)
		goto present;


	/*
	 * Find a segment which begins after this one does. 
	 */
	for (qprev = (struct yhtpiphdr *)tp, q = tp->seg_next; 
	     q != (struct yhtpiphdr *) tp;
	     qprev = q, q = (struct yhtpiphdr *) q->ti_next)
		if (SEQ_GT(q->ti_seq, ti->ti_seq))
			break;

	/*
	 * If there is a preceding segment, it may provide some of our data
	 * already.  If so, drop the data from the incoming segment.  If it
	 * provides all of our data, drop us. 
	 */
	if (qprev != (struct yhtpiphdr *) tp) {
		register int    i;

		/* conversion to int (in i) handles seq wraparound */
		i = qprev->ti_seq + qprev->ti_len - ti->ti_seq;
		if (i > 0) {
			if (i >= (int)ti->ti_len) {
				yhtpstat.yhtps_rcvduppack++;
                                yhtpstat.yhtps_rcvdupbyte += ti->ti_len;
                                goto drop;
                        }
			adjmsg(bp, i);
			ti->ti_len -= i;
			ti->ti_seq += i;
			if (ti->ti_flags & TH_URG) {
				if ((int)ti->ti_urp >= i)
					ti->ti_urp -= i;
				else 
					ti->ti_flags &= ~TH_URG;
			}
		}
	}
        yhtpstat.yhtps_rcvoopack++;
        yhtpstat.yhtps_rcvoobyte += ti->ti_len;

	/*
	 * While we overlap succeeding segments trim them or, if they are
	 * completely covered, dequeue them. 
	 */
	while (q != (struct yhtpiphdr *) tp) {
		register int    i = (ti->ti_seq + ti->ti_len) - q->ti_seq;

		if (i <= 0)
			break;
		if (i < q->ti_len) {
			q->ti_seq += i;
			q->ti_len -= i;
			adjmsg(q->ti_mblk, i);
			if (q->ti_flags & TH_URG) {
				if ((int)q->ti_urp >= i)
					q->ti_urp -= i;
				else 
					q->ti_flags &= ~TH_URG;
			}
			break;
		}
		dequenxt((struct vq *) qprev);
		freemsg(q->ti_mblk);
		q = (struct yhtpiphdr *) qprev->ti_next;
	}

	/*
	 * Stick new segment in its place. 
	 */
	enque((struct vq *) ti, (struct vq *) qprev);

	if (ti->ti_seq != tp->rcv_nxt) {
		STRLOG(YHTPM_ID, 2, 4, SL_TRACE, "yhtp_reass skip got %d expect %d",
		       ti->ti_seq % 10000, tp->rcv_nxt % 10000);
	}
present:
	/*
	 * Present data to user, advancing rcv_nxt through completed sequence
	 * space. 
	 */
	if (YHTPS_HAVERCVDSYN(tp->t_state) == 0)
		return (0);
	ti = tp->seg_next;
	if (ti == (struct yhtpiphdr *) tp || ti->ti_seq != tp->rcv_nxt)
		return (0);
	if (tp->t_state == YHTPS_SYN_RECEIVED && ti->ti_len)
		return (0);
	do {
		tp->rcv_nxt += ti->ti_len;
		flags = ti->ti_flags & TH_FIN;
		bp = ti->ti_mblk;
		dequenxt((struct vq *) tp);
		nti = (struct yhtpiphdr *) ti->ti_next;
		if (inp->inp_state & SS_CANTRCVMORE)
			freemsg(bp);
		else {
			while (bp && !msgblen(bp)) {
				nbp = bp->b_cont;
				freeb(bp);
				bp = nbp;
			}
			if (bp)
				yhsendup (tp, bp, ti, sq);
		}
		ti = nti;
	} while (ti != (struct yhtpiphdr *) tp && ti->ti_seq == tp->rcv_nxt);
	return (flags);
 drop:
	freemsg(bp);
	return (0);
}


yhsendup (tp, bp, ti, sq)
	struct yhtpcb *tp;
	mblk_t *bp;
	struct yhtpiphdr *ti;
	queue_t *sq;
{
	mblk_t *nbp;
	int otlen = ti->ti_len;

	if ((tp->t_iqsize == 0) && sq && canput(sq->q_next)) {
		if ((ti->ti_flags & TH_URG) && 
		    ((int)ti->ti_urp < (int)ti->ti_len)) {
			if (!yhtp_passoobup(tp, bp, sq, ti->ti_urp)) 
				yhtp_enqdata(tp, bp, ti->ti_urp);
		} else {
			if (nbp = yhheaderize(bp)) {
				STRLOG(YHTPM_ID, 2, 5, SL_TRACE,
				       "yhtp sending %d, seq %d, up wq %x",
				       otlen, tp->rcv_nxt % 10000, WR(sq));
				putnext(sq, nbp);
			} else {
				yhtp_enqdata(tp, bp, -1);
				timeout(yhtp_calldeq, sq, HZ);
			}
		}
	} else {
		yhtp_enqdata(tp, bp, -1);
	}
}


/*
 * takes one mblk chain with urgent data in it and splits it up into
 * up to three mblk chains and passes these up.  The first is an M_DATA
 * message with data before the urgent mark.  The second is a T_EXDATA_IND
 * containing the urgent byte.  The third is an M_DATA message with the
 * data after the urgent mark.
 */
yhtp_passoobup(tp, bp0, q, urp)
struct yhtpcb *tp;
mblk_t *bp0;
queue_t *q;
int urp;
{
	/*
	 * bp0 points to the original mblk chain.  bp1 points
	 * to the chain of data before the urgent mark.  bp2 points to the
	 * chain with the urgent byte.  bp3 points to the chain of data
	 * after the urgent mark.  nbp is a temporary.   urp is the offset
	 * to the urgent byte that we are going to extricate.
	 */
	mblk_t *bp1, *bp2, *bp3, *nbp;
	char oobyte;
	struct T_exdata_ind *ind;

	if ((bp1 = dupmsg(bp0)) == (mblk_t *) NULL)
		goto fail;

	/* find mblk with the urgent byte in it */
	for (nbp = bp1 ; urp >= msgblen (nbp); nbp = nbp->b_cont)
		urp -= msgblen (nbp);

	oobyte = *((char *) (nbp->b_rptr + urp));

	if (msgblen(nbp) > urp + 1) {
		/* need to save rest of data for third message */
		bp3 = dupmsg (nbp);	/* can this fail? */
		bp3->b_rptr += (urp + 1);
	} else
		bp3 = nbp->b_cont;

	/* shave off oob byte and data past it */
	nbp->b_wptr -= (msgblen(nbp) - urp);
	nbp->b_cont = NULL;

	/* dump any zero-length mblks */
	while (bp1 && (msgblen (bp1) == 0)) {
		nbp = bp1->b_cont;
		freeb (bp1);
		bp1 = nbp;
	}
	if (bp1) {
		/* finish up first mblk containing data before urgent byte */
		if ((nbp = yhheaderize (bp1)) == (mblk_t *) NULL) {
			freemsg (bp1);
			if (bp3)
				freemsg (bp3);
			goto fail;
		}
		bp1 = nbp;	/* first mblk is done */
	}
	
	if ((bp2 = allocb(sizeof (struct T_exdata_ind), BPRI_HI)) == 
	    (mblk_t *) NULL) {
		if (bp1)
			freemsg (bp1);
		if (bp3)
			freemsg (bp3);
		goto fail;
	}
	bp2->b_datap->db_type = M_PROTO;
	ind = (struct T_exdata_ind *) bp2->b_rptr;
	bp2->b_wptr += sizeof (struct T_exdata_ind);
	ind->PRIM_type = T_EXDATA_IND;
	ind->MORE_flag = 0;
	if ((bp2->b_cont = allocb(1, BPRI_HI)) == (mblk_t *) NULL) {
		if (bp1)
			freemsg(bp1);
		freemsg(bp2);
		if (bp3)
			freemsg(bp3);
		goto fail;
	}
	bp2->b_cont->b_datap->db_type = M_DATA;
	bp2->b_cont->b_wptr += 1;
	*(bp2->b_cont->b_rptr) = oobyte;

	while (bp3 && (msgblen (bp3) == 0)) {
		nbp = bp3->b_cont;
		freeb (bp3);
		bp3 = nbp;
	}
	if (bp3) {
		if ((nbp = yhheaderize (bp3)) == (mblk_t *) NULL) {
			if (bp1)
				freemsg(bp1);
			freemsg(bp2);
			freemsg(bp3);
			goto fail;
		}
		bp3 = nbp;
	}

	/* pass all three up */
	if (bp1) {
		putnext(q, bp1);
	}
	putnext (q, bp2);
	if (bp3) {
		putnext(q, bp3);
	}
	/* success */
	return (1);

 fail:
	return (0);
}



mblk_t         *
yhheaderize(bp)
	register mblk_t *bp;
{
	register mblk_t *bp0;
	extern mblk_t *yhtp_dihdr;

	if (!bp)
		return (NULL);
	if (bp0 = dupb(yhtp_dihdr))
		bp0->b_cont = bp;
	else if (bp0 = copyb(yhtp_dihdr))
		bp0->b_cont = bp;
	else
#ifdef SYSV
		cmn_err(CE_WARN, "yhheaderize: dupb/copyb failed");
#else
		printf( "yhheaderize: dupb/copyb failed");
#endif
	return (bp0);
}


/*
 * YHTP input routine.
 */
yhtp_linput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	register struct yhtpiphdr *ti;
	struct inpcb   *inp;
	int             len, tlen;
	register struct yhtpcb *tp = 0;
	register int    tiflags;

	yhtpstat.yhtps_rcvtotal++;
	/*
	 * Get IP and YHTP header together in first mblk. Note: IP leaves IP
	 * header in first mblk. Also leave room for back pointer to mblk
	 * structure 
	 */
	ti = (struct yhtpiphdr *) bp->b_rptr;
	if (((struct ip *) ti)->ip_hl > (sizeof(struct ip) >> 2))
		yhip_stripoptions(bp, (mblk_t *) 0);
	if (msgblen(bp) < sizeof(struct yhtpiphdr)) {
		if ((pullupmsg(bp, sizeof(struct yhtpiphdr))) == 0) {
			if (yhtpdprintf)
#ifdef SYSV
				cmn_err(CE_NOTE, "yhtp_linput: too short\n");
#else
				printf( "yhtp_linput: too short\n");
#endif
			yhtpstat.yhtps_rcvshort++;
			freemsg(bp);
			return;
		}
		ti = (struct yhtpiphdr *) bp->b_rptr;
	}
	bp->b_datap->db_type = M_DATA;	/* we only send data up */

	/*
	 * Checksum extended YHTP header and data. 
	 */
	tlen = ((struct ip *) ti)->ip_len;
	len = sizeof(struct ip) + tlen;
	ti->ti_len = (u_short) tlen;
	ti->ti_len = htons((u_short) ti->ti_len);
	if (yhtpcksum) {
		ti->ti_next = 0;
		ti->ti_mblk = 0;
		ti->ti_x1 = 0;
		if (ti->ti_sum = yhin_cksum(bp, len)) {
			if (yhtpprintfs)
#ifdef SYSV
				cmn_err(CE_NOTE,"yhtp sum: src %x, sum %x", ntohl(ti->ti_src), ti->ti_sum);
#else
				printf("yhtp sum: src %lx, sum %x", ntohl(ti->ti_src), ti->ti_sum);
#endif
			yhtpstat.yhtps_rcvbadsum++;
			goto drop;
		}
	}
	ti->ti_mblk = bp;
	/*
	 * Convert YHTP protocol specific fields to host format. 
	 */
	ti->ti_seq = ntohl(ti->ti_seq);
	ti->ti_ack = ntohl(ti->ti_ack);
	ti->ti_win = ntohs(ti->ti_win);
	ti->ti_urp = ntohs(ti->ti_urp);

	/*
	 * Locate pcb for segment. 
	 */
	inp = yhin_pcblookup
		(&yhtcb, ti->ti_src, ti->ti_sport, ti->ti_dst, ti->ti_dport,
		 INPLOOKUP_WILDCARD);

	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then all data in
	 * the incoming segment is discarded. 
         * If the TCB exists but is in CLOSED state, it is embryonic,
         * but should either do a listen or a connect soon.
	 */
	if (inp == 0)
		goto dropwithreset;
	tp = intoyhtpcb(inp);
	if (tp == 0) {
		STRLOG(YHTPM_ID, 3, 3, SL_TRACE,
		       "yhtp_input: inp %x but no tp", inp);
		goto dropwithreset;
	}
        if (tp->t_state == YHTPS_CLOSED) {
		if (yhtpprintfs)
#ifdef SYSV
			cmn_err(CE_NOTE, "yhtp_linput: state == CLOSED\n");
#else
			printf( "yhtp_linput: state == CLOSED\n");
#endif
                goto drop;
	}
	yhtp_io(tp, TF_NEEDIN, bp);
	return;

dropwithreset:
	/*
	 * Generate a RST, dropping incoming segment. Make ACK acceptable to
	 * originator of segment. Don't bother to respond if destination was
	 * broadcast. 
	 */
	if (yhtpdprintf)
#ifdef SYSV
		cmn_err(CE_NOTE, "yhtp_linput: drop with reset\n");
#else
		printf( "yhtp_linput: drop with reset\n");
#endif
        tiflags = ti->ti_flags;
	if ((tiflags & TH_RST) || yhin_broadcast(ti->ti_dst))
		goto drop;
	if (tiflags & TH_ACK)
		yhtp_respond(bp, tp, ti, (yhtp_seq) 0, ti->ti_ack, TH_RST);
	else {
		/* adjust ti_len to be length of data */
		ti->ti_len = ntohs(ti->ti_len) - ti->ti_off * 4;
		if (tiflags & TH_SYN)
			ti->ti_len++;
		yhtp_respond(bp, tp, ti, ti->ti_seq + ti->ti_len, (yhtp_seq) 0,
			    TH_RST | TH_ACK);
	}
	return;

drop:
	if (yhtpdprintf)
#ifdef SYSV
		cmn_err(CE_NOTE, "yhtp_linput: drop\n");
#else
		printf( "yhtp_linput: drop\n");
#endif
	freemsg(bp);
	return;
}

struct yhtpcb *
yhtp_uinput(tp0)
	struct yhtpcb *tp0;
{
	register struct yhtpcb *tp = NULL;
	register struct yhtpiphdr *ti;
	struct inpcb    *inp;
	int		len;
	int             off;
	register mblk_t *bp;
	mblk_t          *optbp;
	register int    tiflags;
	int             todrop, acked, needoutput, ourfinisacked;
	short           ostate;
	struct in_addr  laddr;
	int             dropsocket;
	int		iss = 0;
	queue_t        *usq;	/* upstream q */
	int		s;

	if (yhtpdprintf)
#ifdef SYSV
		cmn_err(CE_NOTE, "yhtp_uinput\n");
#else
		printf( "yhtp_uinput\n");
#endif
moreinput:
	tp = tp0 ? tp0 : tp;
	tp0 = NULL;

	s = splstr();
	if (!tp || !(bp = tp->t_inq)) {
		if (tp)
			tp->t_flags &= ~TF_NEEDIN;
		splx(s);
		return(tp);
	}
	tp->t_inq = bp->b_next;
	inp = tp->t_inpcb;
	splx(s);

	optbp = (mblk_t *)NULL;
	needoutput = 0;
	dropsocket = 0;
	ti = (struct yhtpiphdr *)bp->b_rptr;
	ti->ti_len = ntohs((u_short)ti->ti_len);
        /*
         * Check that YHTP offset makes sense, pull out YHTP options and adjust
         * length.
         */
	off = ti->ti_off << 2;
	if (off < sizeof(struct yhtphdr) || off > ti->ti_len) {
		if (yhtpprintfs)
#ifdef SYSV
			cmn_err(CE_NOTE,"yhtp off: src %x off %d", ntohl(ti->ti_src), off);
#else
			printf("yhtp off: src %lx off %d", ntohl(ti->ti_src), off);
#endif
		yhtpstat.yhtps_rcvbadoff++;
		goto drop;
	}
	ti->ti_len -= off;
	if (off > sizeof(struct yhtphdr)) {
		if (msgblen(bp) < sizeof(struct ip) + off) {
			if (pullupmsg(bp, (int) sizeof(struct ip) + off)
			    == 0) {
				if (yhtpdprintf)
#ifdef SYSV
					cmn_err(CE_NOTE, "yhtp_uinput: too short\n");
#else
					printf( "yhtp_uinput: too short\n");
#endif
				yhtpstat.yhtps_rcvshort++;
				goto drop;
			}
			ti = (struct yhtpiphdr *) bp->b_rptr;
		}
		optbp = allocb((int) (off - sizeof(struct yhtphdr)), BPRI_HI);
		if (optbp == 0) {
			if (yhtpdprintf)
#ifdef SYSV
				cmn_err(CE_NOTE, "yhtp_uinput: can't allocb opt buf");
#else
				printf( "yhtp_uinput: can't allocb opt buf");
#endif
			goto drop;
		}
		optbp->b_wptr += off - sizeof(struct yhtphdr);
		{
			caddr_t         op = (caddr_t) bp->b_rptr + sizeof(struct yhtpiphdr);
			bcopy(op, (char *) optbp->b_rptr,
			      (unsigned) (off - sizeof(struct yhtphdr)));
			bp->b_wptr -= off - sizeof(struct yhtphdr);
			bcopy(op + (off - sizeof(struct yhtphdr)), op,
			(unsigned) (msgblen(bp) - sizeof(struct yhtpiphdr)));
		}
	}
	tiflags = ti->ti_flags;

        /*
         * Drop YHTP and IP headers; YHTP options were dropped above.
         */
        bp->b_rptr += sizeof(struct yhtpiphdr);
	if (tp->t_state == YHTPS_CLOSED) {
		STRLOG(YHTPM_ID, 3, 1, SL_TRACE,
		       "yhtp_input: CLOSED: inp %x tp %x", inp, tp);
		goto dropwithreset;
	}
	if ((inp->inp_protoopt & SO_DEBUG) || yhtpalldebug != 0) {
		ostate = tp->t_state;
		yhtp_saveti = *ti;
	}
	if (inp->inp_protoopt & SO_ACCEPTCONN) {
		inp = yhinpnewconn(inp);
		if (inp == 0)
			goto drop;
		/*
		 * This is ugly, but .... 
		 *
		 * Mark pcb as temporary until we're committed to keeping it.
		 * The code at ``drop'' and ``dropwithreset'' check the flag
		 * dropsocket to see if the temporary PCB created here should
		 * be discarded. We mark the PCB as discardable until we're
		 * committed to it below in YHTPS_LISTEN. 
		 */
		dropsocket++;
		inp->inp_laddr = ti->ti_dst;
		inp->inp_lport = ti->ti_dport;
		inp->inp_options = yhip_srcroute(0);
		tp0 = tp;
		tp = intoyhtpcb(inp);
		tp->t_state = YHTPS_LISTEN;
	}
	/*
	 * Segment received on connection. Reset idle time and keep-alive
	 * timer. 
	 */
	tp->t_idle = 0;
	tp->t_timer[YHTPT_KEEP] = yhtp_keepidle;

	/*
	 * Process options if not in LISTEN state, else do it below (after
	 * getting remote address). 
	 */
	if (optbp && tp->t_state != YHTPS_LISTEN) {
		yhtp_dooptions(tp, optbp, ti);
		optbp = 0;
	}
	usq = inp->inp_q;	/* find the way upstream */

	/*
	 * Calculate amount of space in receive window, and then do YHTP input
	 * processing. Receive window is amount of space in rcv queue, but
	 * not less than advertised window. 
	 */
	{
		int             hiwat, win;

		hiwat = (usq ? usq->q_hiwat : 5120);
		win = hiwat - tp->t_iqsize;
		if (win < 0)
			win = 0;
		if (win > hiwat) {
			win = hiwat;
		}
		tp->rcv_wnd = MAX(win, (int) (tp->rcv_adv - tp->rcv_nxt));
	}

	switch (tp->t_state) {

	case YHTPS_LISTEN:{
			mblk_t         *am;
			register struct sockaddr_in *sin;

			if (tiflags & TH_RST)
				goto drop;
			if (tiflags & TH_ACK)
				goto dropwithreset;
			if ((tiflags & TH_SYN) == 0)
				goto drop;
			if (yhin_broadcast(ti->ti_dst))
				goto drop;
			am = allocb(sizeof(struct sockaddr_in), BPRI_HI);
			if (am == NULL)
				goto drop;
			am->b_wptr += sizeof(struct sockaddr_in);
			sin = (struct sockaddr_in *) am->b_rptr;
			sin->sin_family = AF_OSI;
			sin->sin_addr = ti->ti_src;
			sin->sin_port = ti->ti_sport;
			laddr = inp->inp_laddr;
			if (inp->inp_laddr.s_addr == INADDR_ANY)
				inp->inp_laddr = ti->ti_dst;
			if (yhin_pcbconnect(inp, am)) {
				inp->inp_laddr = laddr;
				(void) freemsg(am);
				goto drop;
			}
			(void) freemsg(am);
			tp->t_template = yhtp_template(tp);
			if (tp->t_template == 0) {
				tp = yhtp_drop(tp, ENOBUFS);
				dropsocket = 0;	/* socket is already gone */
				goto drop;
			}
			if (optbp) {
				yhtp_dooptions(tp, optbp, ti);
				optbp = 0;
			}
			if (iss)
				tp->iss = iss;
			else
				tp->iss = yhtp_iss;
			yhtp_iss += YHTP_ISSINCR / 2;
			tp->irs = ti->ti_seq;
			yhtp_sendseqinit(tp);
			yhtp_rcvseqinit(tp);
			tp->t_flags |= TF_ACKNOW;
			tp->t_state = YHTPS_SYN_RECEIVED;
			tp->t_timer[YHTPT_KEEP] = YHTPTV_KEEP_INIT;
			dropsocket = 0;	/* committed to socket */
			yhtpstat.yhtps_accepts++;
			goto trimthenstep6;
		}

	case YHTPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     SEQ_GT(ti->ti_ack, tp->snd_max)))
			goto dropwithreset;
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK)
				tp = yhtp_drop(tp, ECONNREFUSED);
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (tiflags & TH_ACK) {
			tp->snd_una = ti->ti_ack;
			STRLOG(YHTPM_ID, 2, 9, SL_TRACE,
			       "snd_una %x", tp->snd_una);
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
		}
		tp->t_timer[YHTPT_REXMT] = 0;
		tp->irs = ti->ti_seq;
		yhtp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
                if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
			yhtpstat.yhtps_connects++;
			yhinpisconnected(inp);
			tp->t_state = YHTPS_ESTABLISHED;
			tp->t_maxseg = MIN(tp->t_maxseg, (ushort_t)yhtp_mss(tp));
			(void) yhtp_reass(usq, tp, (struct yhtpiphdr *) 0,
					 (mblk_t *) 0);
                        /*
                         * if we didn't have to retransmit the SYN,
                         * use its rtt as our initial srtt & rtt var.
                         */
                        if (tp->t_rtt) {
                                tp->t_srtt = tp->t_rtt << 3;
                                tp->t_rttvar = tp->t_rtt << 1;
                                YHTPT_RANGESET(tp->t_rxtcur,
                                    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
                                    YHTPTV_MIN, YHTPTV_REXMTMAX);
                                tp->t_rtt = 0;
			}
		} else
			tp->t_state = YHTPS_SYN_RECEIVED;

trimthenstep6:
		/*
		 * Advance ti->ti_seq to correspond to first data byte. If
		 * data, trim to stay within window, dropping FIN if
		 * necessary. 
		 */
		ti->ti_seq++;
		if (ti->ti_len > tp->rcv_wnd) {
			todrop = ti->ti_len - tp->rcv_wnd;
			adjmsg(bp, -todrop);
			ti->ti_len = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
                        yhtpstat.yhtps_rcvpackafterwin++;
                        yhtpstat.yhtps_rcvbyteafterwin += todrop;
		}
		tp->snd_wl1 = ti->ti_seq - 1;
		tp->rcv_up = ti->ti_seq;
		goto step6;

	default:
		break;
	}

	todrop = tp->rcv_nxt - ti->ti_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			ti->ti_seq++;
			if (ti->ti_urp > 1) 
				ti->ti_urp--;
			else
				tiflags &= ~TH_URG;
			todrop--;
		}
		if (todrop > ti->ti_len ||
		    todrop == ti->ti_len && (tiflags&TH_FIN) == 0) {
			yhtpstat.yhtps_rcvduppack++;
			yhtpstat.yhtps_rcvdupbyte += ti->ti_len;
			/*
			 * If segment is just one to the left of the window,
			 * check two special cases:
			 * 1. Don't toss RST in response to 4.2-style keepalive.
			 * 2. If the only thing to drop is a FIN, we can drop
			 *    it, but check the ACK or we will get into FIN
			 *    wars if our FINs crossed (both CLOSING).
			 * In either case, send ACK to resynchronize,
			 * but keep on processing for RST or ACK.
			 */
			if ((tiflags & TH_FIN && todrop == ti->ti_len + 1)
#ifdef YHTP_COMPAT_42
			  || (tiflags & TH_RST && ti->ti_seq == tp->rcv_nxt - 1)
#endif
			   ) {
				todrop = ti->ti_len;
				tiflags &= ~TH_FIN;
				tp->t_flags |= TF_ACKNOW;
			} else
				goto dropafterack;
		} else {
			yhtpstat.yhtps_rcvpartduppack++;
			yhtpstat.yhtps_rcvpartdupbyte += todrop;
		}
		adjmsg(bp, todrop);
		ti->ti_seq += todrop;
		ti->ti_len -= todrop;
		if ((int)ti->ti_urp > todrop)
			ti->ti_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			ti->ti_urp = 0;
		}
	}

	if ((inp->inp_state & SS_NOFDREF) &&
	    tp->t_state > YHTPS_CLOSE_WAIT && ti->ti_len) {
		tp = yhtp_close(tp, 0);
		yhtpstat.yhtps_rcvafterclose++;
		goto dropwithreset;
	}

	todrop = (ti->ti_seq + ti->ti_len) - (tp->rcv_nxt + tp->rcv_wnd);
	if (todrop > 0) {
		yhtpstat.yhtps_rcvpackafterwin++;
		if (todrop >= ti->ti_len) {
			yhtpstat.yhtps_rcvbyteafterwin += ti->ti_len;
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && ti->ti_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				yhtpstat.yhtps_rcvwinprobe++;
			} else
				goto dropafterack;
		} else
			yhtpstat.yhtps_rcvbyteafterwin += todrop;
		adjmsg(bp, -todrop);
		ti->ti_len -= todrop;
		tiflags &= ~(TH_PUSH | TH_FIN);
	}

	if (tiflags & TH_RST) {
		int error = 0;

		switch (tp->t_state) {

		case YHTPS_SYN_RECEIVED:
			error = ECONNREFUSED;
			goto close;

		case YHTPS_ESTABLISHED:
		case YHTPS_FIN_WAIT_1:
		case YHTPS_FIN_WAIT_2:
		case YHTPS_CLOSE_WAIT:
			STRLOG(YHTPM_ID, 1, 7, SL_TRACE, "rcvd RST yhtcb %x pcb %x",
			       tp, tp->t_inpcb);
			error = ECONNRESET;
		close:
			tp->t_state = YHTPS_CLOSED;
			yhtpstat.yhtps_drops++;
			yhtp_cancelinger(tp);
			tp = yhtp_close(tp, error);
			goto drop;

		case YHTPS_CLOSING:
		case YHTPS_LAST_ACK:
		case YHTPS_TIME_WAIT:
			tp->t_state = YHTPS_CLOSED;
			yhtp_cancelinger(tp);
			tp = yhtp_close(tp, 0);
			goto drop;

		default:
			break;
		}
	}

	/*
	 * If a SYN is in the window, then this is an error and we send an
	 * RST and drop the connection. 
	 */
	if (tiflags & TH_SYN) {
		tp = yhtp_drop(tp, ECONNRESET);
		goto dropwithreset;
	}
	/*
	 * If the ACK bit is off we drop the segment and return. 
	 */
	if ((tiflags & TH_ACK) == 0)
		goto drop;

	/*
	 * Ack processing. 
	 */
	switch (tp->t_state) {

	case YHTPS_SYN_RECEIVED:
		if ( ! (SEQ_GT(tp->snd_max, tp->iss)))  /* TLI WRES_CIND */
			goto drop;
		if (SEQ_GT(tp->snd_una, ti->ti_ack) ||
		    SEQ_GT(ti->ti_ack, tp->snd_max))
			goto dropwithreset;
		yhtpstat.yhtps_connects++;
		yhinpisconnected(inp);
		tp->t_state = YHTPS_ESTABLISHED;
		tp->t_maxseg = MIN(tp->t_maxseg, (ushort_t)yhtp_mss(tp));
		(void) yhtp_reass(usq, tp, (struct yhtpiphdr *) 0, (mblk_t *) 0);
		tp->snd_wl1 = ti->ti_seq - 1;
		/* fall into ... */

	case YHTPS_ESTABLISHED:
	case YHTPS_FIN_WAIT_1:
	case YHTPS_FIN_WAIT_2:
	case YHTPS_CLOSE_WAIT:
	case YHTPS_CLOSING:
	case YHTPS_LAST_ACK:
	case YHTPS_TIME_WAIT:

		if (SEQ_LEQ(ti->ti_ack, tp->snd_una)) {
			if (ti->ti_len == 0 && ti->ti_win == tp->snd_wnd) {
				yhtpstat.yhtps_rcvdupack++;
				if (tp->t_timer[YHTPT_REXMT] == 0 ||
				    ti->ti_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks == yhtprexmtthresh) {
					yhtp_seq onxt = tp->snd_nxt;
					u_int win =
					    MIN(tp->snd_wnd, tp->snd_cwnd) / 2 /
						tp->t_maxseg;

					if (win < 2)
						win = 2;
					tp->snd_ssthresh = win * tp->t_maxseg;

					tp->t_timer[YHTPT_REXMT] = 0;
					tp->t_rtt = 0;
					tp->snd_nxt = ti->ti_ack;
					tp->snd_cwnd = tp->t_maxseg;
					(void) yhtp_output(tp);

					if (SEQ_GT(onxt, tp->snd_nxt))
						tp->snd_nxt = onxt;
					goto drop;
				}
			} else
				tp->t_dupacks = 0;
			break;
		}
		tp->t_dupacks = 0;
		if (SEQ_GT(ti->ti_ack, tp->snd_max)) {
			yhtpstat.yhtps_rcvacktoomuch++;
			goto dropafterack;
		}
		acked = ti->ti_ack - tp->snd_una;
		yhtpstat.yhtps_rcvackpack++;
		yhtpstat.yhtps_rcvackbyte += acked;

		if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq)) {
			yhtpstat.yhtps_rttupdated++;
			if (tp->t_srtt != 0) {
				register short delta;

				/*
				 * srtt is stored as fixed point with 3 bits
				 * after the binary point (i.e., scaled by 8).
				 * The following magic is equivalent
				 * to the smoothing algorithm in rfc793
				 * with an alpha of .875
				 * (srtt = rtt/8 + srtt*7/8 in fixed point).
				 * Adjust t_rtt to origin 0.
				 */
				delta = tp->t_rtt - 1 - (tp->t_srtt >> 3);
				if ((tp->t_srtt += delta) <= 0)
					tp->t_srtt = 1;
				if (delta < 0)
					delta = -delta;
				delta -= (tp->t_rttvar >> 2);
				if ((tp->t_rttvar += delta) <= 0)
					tp->t_rttvar = 1;
			} else {
				tp->t_srtt = tp->t_rtt << 3;
				tp->t_rttvar = tp->t_rtt << 1;
			}
			tp->t_rtt = 0;
			tp->t_rxtshift = 0;
			YHTPT_RANGESET(tp->t_rxtcur, 
			    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
			    YHTPTV_MIN, YHTPTV_REXMTMAX);
		}

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (ti->ti_ack == tp->snd_max) {
			tp->t_timer[YHTPT_REXMT] = 0;
			needoutput = 1;
		} else if (tp->t_timer[YHTPT_PERSIST] == 0)
			tp->t_timer[YHTPT_REXMT] = tp->t_rxtcur;
		{
		u_int incr = tp->t_maxseg;
		register tmp;

		if (tp->snd_cwnd > tp->snd_ssthresh)
			incr = MAX(incr * incr / tp->snd_cwnd, 1);

		tmp = (int)tp->snd_cwnd + incr;
		tp->snd_cwnd = MIN(tmp, IP_MAXPACKET); /* XXX */
		}
		ourfinisacked = (tp->t_flags & TF_SENTFIN)
				&& (ti->ti_ack == tp->snd_max);
		if (acked > tp->t_qsize) {
			int             qsize = tp->t_qsize;

			tp->snd_wnd -= qsize;
			tp->t_qsize = 0;
			if (usq)
				yhtp_qdrop(WR(usq), qsize);
			STRLOG(YHTPM_ID, 2, 9, SL_TRACE,
			       "setting q size to 0");
		} else if (acked) {
			tp->snd_wnd -= acked;
			tp->t_qsize -= acked;
			if (usq)
				yhtp_qdrop(WR(usq), acked);
			STRLOG(YHTPM_ID, 2, 9, SL_TRACE,
			    "subtracting %d from q size, new value is %d",
			       acked, tp->t_qsize);
			acked = 0;
		}
		tp->snd_una = ti->ti_ack;
		STRLOG(YHTPM_ID, 2, 9, SL_TRACE,
		       "snd_una %x", tp->snd_una);
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		case YHTPS_FIN_WAIT_1:
			if (ourfinisacked) {
				/*
				 * If we can't receive any more data, then
				 * closing user can proceed. Starting the
				 * timer is contrary to the specification,
				 * but if we don't get a FIN we'll hang
				 * forever. 
				 */
				yhtp_cancelinger(tp);
				if (inp->inp_state & SS_CANTRCVMORE) {
					STRLOG(YHTPM_ID, 1, 7, SL_TRACE,
					"FINack, FW1, CANTRCV, inp %x", inp);
					tp->t_timer[YHTPT_2MSL] = yhtp_maxidle;
				}
				tp->t_state = YHTPS_FIN_WAIT_2;
			}
			break;

		case YHTPS_CLOSING:
			if (ourfinisacked) {
				tp->t_state = YHTPS_TIME_WAIT;
				yhtp_canceltimers(tp);
				yhtp_cancelinger(tp);
				tp->t_timer[YHTPT_2MSL] = 2 * YHTPTV_MSL;
			}
			break;

		case YHTPS_LAST_ACK:
			if (ourfinisacked) {
				yhtp_cancelinger(tp);
				tp->t_state = YHTPS_CLOSED;
				tp = yhtp_close(tp, 0);
				goto drop;
			}
			break;

		case YHTPS_TIME_WAIT:
			tp->t_timer[YHTPT_2MSL] = 2 * YHTPTV_MSL;
			goto dropafterack;
		default:
			break;
		}
	default:
		break;
	}

step6:
	if ((tiflags & TH_ACK) &&
	    (SEQ_LT(tp->snd_wl1, ti->ti_seq) || tp->snd_wl1 == ti->ti_seq &&
	     (SEQ_LT(tp->snd_wl2, ti->ti_ack) ||
	      tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (ti->ti_len == 0 &&
		    tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd)
			yhtpstat.yhtps_rcvwinupd++;
		tp->snd_wnd = ti->ti_win;
		tp->snd_wl1 = ti->ti_seq;
		tp->snd_wl2 = ti->ti_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}
	/*
	 * Process segments with URG.
	 */
	if ((tiflags & TH_URG) && ti->ti_urp &&
	    YHTPS_HAVERCVDFIN(tp->t_state) == 0) {

		/*
		 * If this segment advances the known urgent pointer, then
		 * mark the data stream.  This should not happen in
		 * CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT STATES since a
		 * FIN has been received from the remote side. In these
		 * states we ignore the URG. 
		 *
		 * XXX - Current YHTP interpretations say that the urgent
		 * pointer points to the last octet of urgent data.  For
		 * compatibility with previous releases, we continue to use 
		 * the obsolete interpretation where the urgent pointer 
		 * points to the first octet of data past the urgent section.  
		 * This decision means that we only recognize
		 * segments as urgent when the urgent pointer is > 0,
		 * which may cause problems interoperating with other systems.
		 */
		if (SEQ_GT(ti->ti_seq + ti->ti_urp, tp->rcv_up)) {
			tp->rcv_up = ti->ti_seq + ti->ti_urp;
			tp->t_oobflags &= ~(YHTPOOB_HAVEDATA | YHTPOOB_HADDATA);
		}

		ti->ti_urp--;		/* XXX - 4.2 BSD compatibility */
	} else
		/*
		 * If no out of band data is expected, pull receive urgent
		 * pointer along with the receive window. 
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;

	/*
	 * Process the segment text, merging it into the YHTP sequencing
	 * queue, and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data is
	 * presented to the user (this happens in yhtp_usrreq.c, case
	 * PRU_RCVD). 
	 */
	if ((ti->ti_len || (tiflags & TH_FIN)) &&
	    YHTPS_HAVERCVDFIN(tp->t_state) == 0) {
		YHTP_REASS(tp, ti, bp, usq, tiflags);
		if (yhtpnodelack == 0)
			tp->t_flags |= TF_DELACK;
		else
			tp->t_flags |= TF_ACKNOW;
		/*
		 * Note the amount of data that peer has sent into our
		 * window, in order to estimate the sender's buffer size. 
		 */
		if (usq)
			len = usq->q_hiwat - (tp->rcv_adv - tp->rcv_nxt);
		else
			len = 5120 - (tp->rcv_adv - tp->rcv_nxt);
		if (len > (int)tp->max_rcvd)
			tp->max_rcvd = len;
	} else {
		freemsg(bp);
		tiflags &= ~TH_FIN;
	}

	if (tiflags & TH_FIN) {
		STRLOG(YHTPM_ID, 1, 7, SL_TRACE, "rcvd FIN yhtcb %x pcb %x",
		       tp, tp->t_inpcb);
		if (YHTPS_HAVERCVDFIN(tp->t_state) == 0) {
			inp->inp_state |= SS_CANTRCVMORE;
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		switch (tp->t_state) {


		case YHTPS_SYN_RECEIVED:
		case YHTPS_ESTABLISHED:
			tp->t_state = YHTPS_CLOSE_WAIT;
			/* yhtp_deqdata takes care of sending T_ORDREL_IND */
			if (usq) qenable(usq);
			break;

		case YHTPS_FIN_WAIT_1:
			tp->t_state = YHTPS_CLOSING;
			if (inp->inp_q) 
				qenable(inp->inp_q);
			break;

		case YHTPS_FIN_WAIT_2:
			tp->t_state = YHTPS_TIME_WAIT;
			yhtp_canceltimers(tp);
			tp->t_timer[YHTPT_2MSL] = 2 * YHTPTV_MSL;
			if (inp->inp_q)
				qenable(inp->inp_q);
			break;

		case YHTPS_TIME_WAIT:
			tp->t_timer[YHTPT_2MSL] = 2 * YHTPTV_MSL;
			break;

		default:
			break;
		}
	}
	if ((inp->inp_protoopt & SO_DEBUG) || yhtpalldebug != 0)
		yhtp_trace(TA_INPUT, ostate, tp, &yhtp_saveti, 0);

	/*
	 * Return any desired output. 
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW))
		(void) yhtp_output(tp);		/* Already got the IO lock */
	goto moreinput;

dropafterack:
	if (tiflags & TH_RST)
		goto drop;
	freemsg(bp);
	tp->t_flags |= TF_ACKNOW;
	(void) yhtp_output(tp);
	goto moreinput;

dropwithreset:
	if (optbp) {
		(void) freemsg(optbp);
		optbp = 0;
	}
	if ((tiflags & TH_RST) || yhin_broadcast(ti->ti_dst))
		goto drop;
	if (tiflags & TH_ACK)
		yhtp_respond(bp, tp, ti, (yhtp_seq) 0, ti->ti_ack, TH_RST);
	else {
		if (tiflags & TH_SYN)
			ti->ti_len++;
		yhtp_respond(bp, tp, ti, ti->ti_seq + ti->ti_len, (yhtp_seq) 0,
			    TH_RST | TH_ACK);
	}
	/* destroy temporarily created socket */
	if (dropsocket)
		tp = yhtp_drop(tp, ECONNABORTED);
	goto moreinput;

drop:
	if (yhtpdprintf)
#ifdef SYSV
		cmn_err(CE_NOTE, "yhtp_uinput: drop\n");
#else
		printf( "yhtp_uinput: drop\n");
#endif
	if (optbp)
		(void) freemsg(optbp);
	/*
	 * Drop space held by incoming segment and return. 
	 */
	if (tp && ((tp->t_inpcb->inp_protoopt & SO_DEBUG) || yhtpalldebug != 0))
		yhtp_trace(TA_DROP, ostate, tp, &yhtp_saveti, 0);
	freemsg(bp);
	/* destroy temporarily created socket */
	if (dropsocket)
		tp = yhtp_drop(tp, ECONNABORTED);
	goto moreinput;
}

yhtp_dooptions(tp, optbp, ti)
	struct yhtpcb   *tp;
	mblk_t         *optbp;
	struct yhtpiphdr *ti;
{
	register u_char *cp;
	int             opt, optlen, cnt;

	cp = (u_char *) optbp->b_rptr;
	cnt = msgblen(optbp);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == YHTPOPT_EOL)
			break;
		if (opt == YHTPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {

		default:
			break;

		case YHTPOPT_MAXSEG:
			if (optlen != 4)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			tp->t_maxseg = *(u_short *) (cp + 2);
			tp->t_maxseg = ntohs((u_short) tp->t_maxseg);
			tp->t_maxseg = MIN(tp->t_maxseg, (ushort_t)yhtp_mss(tp));
			break;
		}
	}
	(void) freemsg(optbp);
}

/*
 * Determine a reasonable value for maxseg size. If the route is known, use
 * one that can be handled on the given interface without forcing IP to
 * fragment. If interface pointer is unavailable, or the destination isn't
 * local, use a conservative size (512 or the default IP max size, but no
 * more than the maxtu of the interface through which we route), as we can't
 * discover anything about intervening gateways or networks. 
 *
 */
#define satosin(x) ((struct sockaddr_in *) (x))
yhtp_mss(tp)
	register struct yhtpcb *tp;
{
	struct route   *ro;
	struct ip_provider *prov;
	int             mss;
	struct inpcb   *inp;
	int             rtret = RT_OK;

	inp = tp->t_inpcb;
	ro = &inp->inp_route;
	if ((ro->ro_rt == (mblk_t *) 0) ||
	    (prov = RT(ro->ro_rt)->rt_prov) == (struct ip_provider *) 0) {
		if (ro->ro_rt)
			yhrtfree(ro->ro_rt);
		/* No route yet, so try to acquire one */
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			satosin(&ro->ro_dst)->sin_addr.s_addr = 
				inp->inp_faddr.s_addr;
			rtret = yhrtalloc(ro, 0);	/* no dial */
		}
		if (rtret == RT_DEFER) {
			yhrtfree(ro->ro_rt);
			ro->ro_rt = 0;
		}
		if ((ro->ro_rt == 0) || (prov = RT(ro->ro_rt)->rt_prov) == 0)
			return (YHTP_MSS);
		if (rtret == RT_SWITCHED) {
			yhrtfree(ro->ro_rt);
			ro->ro_rt = 0;
		}
	}
	mss = prov->if_maxtu - sizeof(struct yhtpiphdr);
	if (mss > 1024)
		mss = (mss / 1024) * 1024;
	if (yhin_localaddr(inp->inp_faddr))
		return (mss);
	mss = MIN(mss, YHTP_MSS);
	tp->snd_cwnd = mss;
	return (mss);
}

/*
 * When an attempt at a new connection is noted on a YHTP endpoint which
 * accepts connections, yhinpnewconn is called.  If the connection is possible
 * (subject to space constraints, etc.) then we allocate a new structure,
 * properly linked into the data structure of the original yhtpcb, and return
 * this. 
 */
struct inpcb   *
yhinpnewconn(head)
	register struct inpcb *head;
{
	register struct inpcb *inp;
	struct yhtpcb   *htp, *tp;

	htp = intoyhtpcb(head);
	if (htp->t_qlen + htp->t_q0len > 3 * htp->t_qlimit / 2)
		goto bad;

	inp = yhin_pcballoc(&yhtcb);
	if (inp == NULL)
		goto bad;
	tp = yhtp_newyhtpcb(inp);
	if (tp == 0) {
		yhin_pcbdetach(inp);
		goto bad;
	}
	inp->inp_q = (queue_t *) NULL;
	inp->inp_tstate = TS_DATA_XFER;

	inp->inp_protoopt = head->inp_protoopt & ~SO_ACCEPTCONN;
	inp->inp_linger = head->inp_linger;
	inp->inp_state = head->inp_state | SS_NOFDREF;
	yhtpqinsque(htp, tp, 0);
	STRLOG(YHTPM_ID, 1, 5, SL_TRACE, "yhinpnewconn head %x inp %x", head, inp);
	return (inp);
bad:
	STRLOG(YHTPM_ID, 1, 3, SL_TRACE, "yhinpnewconn failed head %x", head);
	return ((struct inpcb *) 0);
}

/*
 * yhtp_qdrop trims data from the front of the queue.  It is used when acks
 * for the data come in. 
 */

yhtp_qdrop(q, len)
	register queue_t *q;
	register int    len;
{
	register mblk_t *bp = NULL, *obp;
	int		n;

	STRLOG(YHTPM_ID, 2, 9, SL_TRACE, "yhtp_qdrop q %x len %d", q, len);

	while (len && (bp = getq(q))) {
		do {
                	if (len >= (n=msgblen(bp))) {
                        	obp = bp;
                        	bp = bp->b_cont;
                        	freeb(obp);
                        	len -= n;
                	} else {
                        	bp->b_rptr += len;
				goto out;
                	}
        	} while (bp);
	}
out:
	if (bp)
		putbq(q, bp);
	q->q_flag |= QWANTR;
}
