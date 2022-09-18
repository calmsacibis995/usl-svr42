
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

#define STRNET
#include <util/param.h>
#include <util/spl.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/transport/socket.h>
#include <net/transport/socketvar.h>
#include <net/yhtpip/protosw.h>
#include <svc/errno.h>

#include <net/yhtpip/yhin.h>
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

#ifdef SYSV
#include <util/cmn_err.h>
#endif

int             yhtpnodelack = 0;
int		yhtp_keepidle = YHTPTV_KEEP_IDLE;
int		yhtp_keepintvl = YHTPTV_KEEPINTVL;
int		yhtp_maxidle;
int             yhtpfastid, yhtpslowid;
extern queue_t *yhtp_qbot;
extern int      yhtpalldebug;

/*
 * Fast timeout routine for processing delayed acks 
 */
yhtp_fasttimo()
{
	register struct inpcb *inp;
	register struct yhtpcb *tp;

	inp = yhtcb.inp_next;
	if (inp)
		for (; inp != &yhtcb; inp = inp->inp_next)
			if ((tp = (struct yhtpcb *) inp->inp_ppcb) &&
			    (tp->t_flags & TF_DELACK)) {
				tp->t_flags &= ~TF_DELACK;
				tp->t_flags |= TF_ACKNOW;
				yhtpstat.yhtps_delack++;
				yhtp_io(tp, TF_NEEDOUT, NULL);
			}
	yhtpfastid = timeout(yhtp_fasttimo, 0, HZ / 5);
}

/*
 * Yhtp protocol timeout routine called every 500 ms. Updates the timers in
 * all active yhtcb's and causes finite state machine actions if timers expire. 
 * (yhtp_slowtimo now processes timers by calling yhtp_dotimers under the
 * I/O lock.)
 */
struct yhtpcb *
yhtp_dotimers(tp)
struct yhtpcb *tp;
{
	register int    i;

	i = splstr();
	tp->t_flags &= ~TF_NEEDTIMER;
	splx(i);
	for (i = 0; i < YHTPT_NTIMERS; i++) {
		if (tp->t_timer[i] && --tp->t_timer[i] == 0) {
			if (!yhtp_timers(tp, i))
				return (struct yhtpcb *) 0;
			if ((tp->t_inpcb->inp_protoopt & SO_DEBUG)
			    || yhtpalldebug != 0)
				yhtp_trace(TA_TIMER, tp->t_state, tp,
					  (struct yhtpiphdr *) 0, i);
		}
	}
	tp->t_idle++;
	if (tp->t_rtt)
		tp->t_rtt++;
	return tp;
}

yhtp_slowtimo()
{
	register struct inpcb *ip, *ipnxt;
	register struct yhtpcb *tp;

	yhtp_maxidle = YHTPTV_KEEPCNT * yhtp_keepintvl;
	/*
	 * Search through yhtcb's and update active timers. 
	 */
	ip = yhtcb.inp_next;
	if (ip == 0) {
		return;
	}
	for (; ip != &yhtcb; ip = ipnxt) {
		ipnxt = ip->inp_next;
		tp = intoyhtpcb(ip);
		if (tp)
			yhtp_io(tp, TF_NEEDTIMER, NULL);
	}
	yhtp_iss += YHTP_ISSINCR / PR_SLOWHZ;	/* increment iss */
#ifdef YHTP_COMPAT_42
	if ((int) yhtp_iss < 0)
		yhtp_iss = 0;	/* XXX */
#endif
	yhtpslowid = timeout(yhtp_slowtimo, 0, HZ / 2);
}

/*
 * Cancel all timers for YHTP tp. 
 */
yhtp_canceltimers(tp)
	struct yhtpcb   *tp;
{
	register int    i;

	for (i = 0; i < YHTPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

int             yhtp_backoff[YHTP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };
/*
 * YHTP timer processing. 
 */
struct yhtpcb   *
yhtp_timers(tp, timer)
	register struct yhtpcb *tp;
	int             timer;
{
	register int    rexmt;

	extern void	yhlingtimer();

	switch (timer) {

		/*
		 * 2 MSL timeout in shutdown went off.  If we're closed but
		 * still waiting for peer to close and connection has been
		 * idle too long, or if 2MSL time is up from TIME_WAIT,
		 * delete connection control block.  Otherwise, check again
		 * in a bit. 
		 */
	case YHTPT_2MSL:
		if (tp->t_state != YHTPS_TIME_WAIT &&
		    tp->t_idle <= yhtp_maxidle)
			tp->t_timer[YHTPT_2MSL] = yhtp_keepintvl;
		else {
			tp->t_state = YHTPS_CLOSED;
			tp = yhtp_close(tp, 0);
		}
		break;

		/*
		 * Retransmission timer went off.  Message has not been acked
		 * within retransmit interval.  Back off to a longer
		 * retransmit interval and retransmit one segment. 
		 */
	case YHTPT_REXMT:
		if (++tp->t_rxtshift > YHTP_MAXRXTSHIFT) {
			tp->t_rxtshift = YHTP_MAXRXTSHIFT;
			yhtpstat.yhtps_timeoutdrop++;
			tp = yhtp_drop(tp, ETIMEDOUT);
			break;
		}
		yhtpstat.yhtps_rexmttimeo++;
		rexmt = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
		rexmt *= yhtp_backoff[tp->t_rxtshift];
		YHTPT_RANGESET(tp->t_rxtcur, rexmt, YHTPTV_MIN, YHTPTV_REXMTMAX);
		tp->t_timer[YHTPT_REXMT] = tp->t_rxtcur;
		/*
		 * If losing, let the lower level know and try for
		 * a better route.  Also, if we backed off this far,
		 * our srtt estimate is probably bogus.  Clobber it
		 * so we'll take the next rtt measurement as our srtt;
		 * move the current srtt into rttvar to keep the current
		 * retransmit times until then.
		 */
		if (tp->t_rxtshift > YHTP_MAXRXTSHIFT / 4) {
			yhin_losing(tp->t_inpcb);
			tp->t_rttvar += (tp->t_srtt >> 2);
			tp->t_srtt = 0;
		}
		tp->snd_nxt = tp->snd_una;
		/*
		 * If timing a segment in this window, stop the timer.
		 */
		tp->t_rtt = 0;
		/*
		 * Close the congestion window down to one segment
		 * (we'll open it by one segment for each ack we get).
		 * Since we probably have a window's worth of unacked
		 * data accumulated, this "slow start" keeps us from
		 * dumping all that data as back-to-back packets (which
		 * might overwhelm an intermediate gateway).
		 *
		 * There are two phases to the opening: Initially we
		 * open by one mss on each ack.  This makes the window
		 * size increase exponentially with time.  If the
		 * window is larger than the path can handle, this
		 * exponential growth results in dropped packet(s)
		 * almost immediately.  To get more time between 
		 * drops but still "push" the network to take advantage
		 * of improving conditions, we switch from exponential
		 * to linear window opening at some threshhold size.
		 * For a threshhold, we use half the current window
		 * size, truncated to a multiple of the mss.
		 *
		 * (the minimum cwnd that will give us exponential
		 * growth is 2 mss.  We don't allow the threshhold
		 * to go below this.)
		 */
		{
		u_int win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
		if (win < 2)
			win = 2;
		tp->snd_cwnd = tp->t_maxseg;
		tp->snd_ssthresh = win * tp->t_maxseg;
		}
		yhtp_output(tp);
		break;

		/*
		 * Persistance timer into zero window. Force a byte to be
		 * output, if possible. 
		 */
	case YHTPT_PERSIST:
		yhtpstat.yhtps_persisttimeo++;
		yhtp_setpersist(tp);
		tp->t_force = 1;
		yhtp_output(tp);
		break;

		/*
		 * Keep-alive timer went off; send something or drop
		 * connection if idle for too long. 
		 */
	case YHTPT_KEEP:
		yhtpstat.yhtps_keeptimeo++;
		if (tp->t_state < YHTPS_ESTABLISHED)
			goto dropit;
		if (tp->t_inpcb->inp_protoopt & SO_KEEPALIVE &&
		    tp->t_state <= YHTPS_CLOSE_WAIT) {
		    	if (tp->t_idle >= yhtp_keepidle + yhtp_maxidle)
				goto dropit;
			/*
			 * Send a packet designed to force a response
			 * if the peer is up and reachable:
			 * either an ACK if the connection is still alive,
			 * or an RST if the peer has closed the connection
			 * due to timeout or reboot.
			 * Using sequence number tp->snd_una-1
			 * causes the transmitted zero-length segment
			 * to lie outside the receive window;
			 * by the protocol spec, this requires the
			 * correspondent YHTP to respond.
			 */
			yhtpstat.yhtps_keepprobe++;
#ifdef YHTP_COMPAT_42
			/*
			 * The keepalive packet must have nonzero length
			 * to get a 4.2 host to respond.
			 */
			yhtp_respond((mblk_t *) 0, tp, tp->t_template,
			    tp->rcv_nxt - 1, tp->snd_una - 1, 0);
#else
			yhtp_respond((mblk_t *) 0, tp, tp->t_template,
			    tp->rcv_nxt, tp->snd_una - 1, 0);
#endif
			tp->t_timer[YHTPT_KEEP] = yhtp_keepintvl;
		} else
			tp->t_timer[YHTPT_KEEP] = yhtp_keepidle;
		break;
dropit:
		yhtpstat.yhtps_keepdrops++;
		tp = yhtp_drop(tp, ETIMEDOUT);
		break;

	case YHTPT_LINGER:
		yhlingtimer(tp);
		break;

	default:
		panic("yhtp_timers");
		break;
	}
	return (tp);
}

yhlingerstart(tp)
	struct	yhtpcb	*tp;
{
	if (tp) {
		tp->t_linger = 1;
		yhtpstat.yhtps_linger++;
		tp->t_timer[YHTPT_LINGER] = (tp->t_inpcb->inp_linger * PR_SLOWHZ);
	} else {
#ifdef SYSV
		cmn_err(CE_WARN, "yhlingerstart: null tp");
#else
		printf( "yhlingerstart: null tp");
#endif
	}
}

void
yhlingtimer(tp)
	struct yhtpcb   *tp;
{
	if (!tp) {
#ifdef SYSV
		cmn_err(CE_WARN, "yhlingtimer: null tp");
#else
		printf( "yhlingtimer: null tp");
#endif
		return;
	}
		
	tp->t_qsize = 0;
	tp->t_linger = 0;
	tp->t_timer[YHTPT_LINGER] = 0;
	yhtpstat.yhtps_lingerexp++;
	wakeup((caddr_t) tp);
	yhtp_io(tp, TF_NEEDOUT, NULL);
	return;
}

yhtp_cancelinger(tp)
	struct yhtpcb *tp;
{
	if (!tp) {
#ifdef SYSV
		cmn_err(CE_WARN, "yhtp_cancelinger: null tp");
#else
		printf( "yhtp_cancelinger: null tp");
#endif
		return;
	}
	if (tp->t_linger) {
		tp->t_linger = 0;
		tp->t_timer[YHTPT_LINGER] = 0;
		wakeup((caddr_t) tp);
		yhtpstat.yhtps_lingercan++;
	}
}
