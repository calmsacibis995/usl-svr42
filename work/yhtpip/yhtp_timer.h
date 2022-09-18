

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_YHTP_TIMER_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHTP_TIMER_H	/* subject to change without notice */


/*
 * Definitions of the YHTP timers.  These timers are counted
 * down PR_SLOWHZ times a second.
 */

#define	YHTPT_NTIMERS	5

#define	YHTPT_REXMT	0	/* retransmit */
#define	YHTPT_PERSIST	1	/* retransmit persistance */
#define	YHTPT_KEEP	2	/* keep alive */
#define	YHTPT_2MSL	3	/* 2*msl quiet time timer */
#define	YHTPT_LINGER	4	/* linger on close imer */

/*
 * The YHTPT_REXMT timer is used to force retransmissions.
 * The YHTP has the YHTPT_REXMT timer set whenever segments
 * have been sent for which ACKs are expected but not yet
 * received.  If an ACK is received which advances tp->snd_una,
 * then the retransmit timer is cleared (if there are no more
 * outstanding segments) or reset to the base value (if there
 * are more ACKs expected).  Whenever the retransmit timer goes off,
 * we retransmit one unacknowledged segment, and do a backoff
 * on the retransmit timer.
 *
 * The YHTPT_PERSIST timer is used to keep window size information
 * flowing even if the window goes shut.  If all previous transmissions
 * have been acknowledged (so that there are no retransmissions in progress),
 * and the window is too small to bother sending anything, then we start
 * the YHTPT_PERSIST timer.  When it expires, if the window is nonzero,
 * we go to transmit state.  Otherwise, at intervals send a single byte
 * into the peer's window to force him to update our window information.
 * We do this at most as often as YHTPT_PERSMIN time intervals,
 * but no more frequently than the current estimate of round-trip
 * packet time.  The YHTPT_PERSIST timer is cleared whenever we receive
 * a window update from the peer.
 *
 * The YHTPT_KEEP timer is used to keep connections alive.  If a
 * connection is idle (no segments received) for YHTPTV_KEEP_INIT amount of time,
 * but not yet established, then we drop the connection.  Once the connection
 * is established, if the connection is idle for YHTPTV_KEEP_IDLE time
 * (and keepalives have been enabled on the socket), we begin to probe
 * the connection.  We force the peer to send us a segment by sending:
 *	<SEQ=SND.UNA-1><ACK=RCV.NXT><CTL=ACK>
 * This segment is (deliberately) outside the window, and should elicit
 * an ack segment in response from the peer.  If, despite the YHTPT_KEEP
 * initiated segments we cannot elicit a response from a peer in YHTPT_MAXIDLE
 * amount of time probing, then we drop the connection.
 */

#define	YHTP_TTL		30		/* default time to live for YHTP segs */
/*
 * Time constants.
 */
#define	YHTPTV_MSL	( 30*PR_SLOWHZ)		/* max seg lifetime (hah!) */
#define	YHTPTV_SRTTBASE	0			/* base roundtrip time;
						   if 0, no idea yet */
#define	YHTPTV_SRTTDFLT	(  3*PR_SLOWHZ)		/* assumed RTT if no info */

#define	YHTPTV_PERSMIN	(  5*PR_SLOWHZ)		/* retransmit persistance */
#define	YHTPTV_PERSMAX	( 60*PR_SLOWHZ)		/* maximum persist interval */

#define	YHTPTV_KEEP_INIT	( 75*PR_SLOWHZ)		/* initial connect keep alive */
#define	YHTPTV_KEEP_IDLE	(120*60*PR_SLOWHZ)	/* dflt time before probing */
#define	YHTPTV_KEEPINTVL	( 75*PR_SLOWHZ)		/* default probe interval */
#define	YHTPTV_KEEPCNT	8			/* max probes before drop */

#define	YHTPTV_MIN	(  1*PR_SLOWHZ)		/* minimum allowable value */
#define	YHTPTV_REXMTMAX	( 64*PR_SLOWHZ)		/* max allowable REXMT value */

#define	YHTP_LINGERTIME		120		/* linger at most 2 minutes */

#define	YHTP_MAXRXTSHIFT	12		/* maximum retransmits */

#ifdef	YHTPTIMERS
char           *yhtptimers[] =
{"REXMT", "PERSIST", "KEEP", "2MSL", "LINGER"};
#endif

/*
 * Force a time value to be in a certain range.
 */
#define	YHTPT_RANGESET(tv, value, tvmin, tvmax) { \
	(tv) = (value); \
	if ((tv) < (tvmin)) \
		(tv) = (tvmin); \
	else if ((tv) > (tvmax)) \
		(tv) = (tvmax); \
}

#ifdef _KERNEL
extern int yhtp_keepidle;		/* time before keepalive probes begin */
extern int yhtp_keepintvl;		/* time between keepalive probes */
extern int yhtp_maxidle;			/* time to drop after starting probes */
extern int yhtp_ttl;			/* time to live for YHTP segs */
extern int yhtp_backoff[];
#endif

#endif	/* _NET_YHTPIP_YHTP_TIMER_H */
