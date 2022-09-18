

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_YHTP_VAR_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHTP_VAR_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>			/* REQUIRED */
#endif

#ifndef _IO_STREAM_H
#include <io/stream.h>			/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHTP_H
#include <net/yhtpip/yhtp.h>		/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHTP_TIMER_H
#include <net/yhtpip/yhtp_timer.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <netinet/yhtp.h>		/* REQUIRED */
#include <netinet/yhtp_timer.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Kernel variables for yhtp.
 */

/*
 * Tcp control block, one per yhtp; fields:
 */
struct yhtpcb {
	struct	yhtpiphdr *seg_next;	/* sequencing queue */
#ifndef STRNET
	struct	yhtpiphdr *seg_prev;
#endif
	short	t_state;		/* state of this connection */
	short	t_timer[YHTPT_NTIMERS];	/* yhtp timers */
	short	t_rxtshift;		/* log(2) of rexmt exp. backoff */
	short	t_rxtcur;		/* current retransmit value */
	short	t_dupacks;		/* consecutive dup acks recd */
	u_short	t_maxseg;		/* maximum segment size */
	char	t_force;		/* 1 if forcing out a byte */
	u_short	t_flags;
#define	TF_ACKNOW	0x01		/* ack peer immediately */
#define	TF_DELACK	0x02		/* ack, but try to delay it */
#define	TF_NODELAY	0x04		/* don't delay packets to coalesce */
#define	TF_NOOPT	0x08		/* don't use yhtp options */
#define	TF_SENTFIN	0x10		/* have sent FIN */
#ifdef STRNET
#define TF_IOLOCK       0x20
#define TF_NEEDIN       0x40
#define TF_NEEDOUT      0x80
#define	TF_NEEDTIMER	0x0100
#endif STRNET
#define	TF_SENTSYN	0x200		/* have sent SYN */
	struct	yhtpiphdr *t_template;	/* skeletal packet for transmit */
#ifdef STRNET
        mblk_t	*t_tmplhdr;		/* template back-pointer for dealloc */
#endif STRNET
	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	yhtp_seq	snd_una;		/* send unacknowledged */
	yhtp_seq	snd_nxt;		/* send next */
	yhtp_seq	snd_up;			/* send urgent pointer */
	yhtp_seq	snd_wl1;		/* window update seg seq number */
	yhtp_seq	snd_wl2;		/* window update seg ack number */
	yhtp_seq	iss;			/* initial send sequence number */
	u_long	snd_wnd;		/* send window */
/* receive sequence variables */
	u_long	rcv_wnd;		/* receive window */
	yhtp_seq	rcv_nxt;		/* receive next */
	yhtp_seq	rcv_up;			/* receive urgent pointer */
	yhtp_seq	irs;			/* initial receive sequence number */
/*
 * Additional variables for this implementation.
 */
/* receive variables */
	yhtp_seq	rcv_adv;		/* advertised window */
/* retransmit variables */
	yhtp_seq	snd_max;		/* highest sequence number sent
					 * used to recognize retransmits
					 */
	u_long	t_maxwin;		/* max window size to use */
/* congestion control (for slow start, source quench, retransmit after loss) */
	u_long	snd_cwnd;		/* congestion-controlled window */
	u_long	snd_ssthresh;		/* snd_cwnd size threshhold for
					 * for slow start exponential to
					 * linear switch */
/*
 * transmit timing stuff.
 * srtt and rttvar are stored as fixed point; for convenience in smoothing,
 * srtt has 3 bits to the right of the binary point, rttvar has 2.
 * "Variance" is actually smoothed difference.
 */
	short	t_idle;			/* inactivity time */
	short	t_rtt;			/* round trip time */
	yhtp_seq	t_rtseq;		/* sequence number being timed */
	short	t_srtt;			/* smoothed round-trip time */
	short	t_rttvar;		/* variance in round-trip time */
	u_short max_rcvd;		/* most peer has sent into window */
	u_short	max_sndwnd;		/* largest window peer has offered */
/* out-of-band data */
	char	t_oobflags;		/* have some */
	char	t_iobc;			/* input character */
#define	YHTPOOB_HAVEDATA	0x01
#define	YHTPOOB_HADDATA	0x02
#ifdef STRNET
	struct	yhtpcb *t_head;		/* back pointer to accept yhtpcb */
	struct	yhtpcb *t_q0;		/* queue of partial connections */
	struct	yhtpcb *t_q;		/* queue of incoming connections */
	short	t_q0len;		/* partials on t_q0 */
	short	t_qlen;			/* number of connections on so_q */
	short	t_qlimit;		/* max number queued connections */
	/*
	 * We keep our own count of data on the queue rather than
	 * rely on q_count because q_count is a short on non-eft
	 * systems.
	 */
	u_long	t_qsize;		/* number of data chars on outq */
	/*
	 * here we save mblks that arrive before the connection is accepted
	 * by the user and those received when the user's queue is full. 
	 */
	mblk_t	*t_qfirst;		/* beginning of queued data */
	mblk_t	*t_qlast;		/* end of queued data */
	int	t_iqsize;		/* amount of data on input queue */
	int	t_iqurp;		/* offset of urgent byte on input q */
	mblk_t	*t_inq;			/* pending input */
	short	t_linger;		/* linger flag */
#endif STRNET
};

#define	intoyhtpcb(ip)	((struct yhtpcb *)(ip)->inp_ppcb)
#ifdef STRNET
#define qtoyhtpcb(q) ((struct yhtpcb *) ((struct inpcb *) (q)->q_ptr)->inp_ppcb)
#else
#define	sotoyhtpcb(so)	(intoyhtpcb(sotoinpcb(so)))
#endif STRNET

/*
 * YHTP statistics.
 * Many of these should be kept per connection,
 * but that's inconvenient at the moment.
 */
struct	yhtpstat {
	u_long	yhtps_connattempt;	/* connections initiated */
	u_long	yhtps_accepts;		/* connections accepted */
	u_long	yhtps_connects;		/* connections established */
	u_long	yhtps_drops;		/* connections dropped */
	u_long	yhtps_conndrops;		/* embryonic connections dropped */
	u_long	yhtps_closed;		/* conn. closed (includes drops) */
	u_long	yhtps_segstimed;		/* segs where we tried to get rtt */
	u_long	yhtps_rttupdated;	/* times we succeeded */
	u_long	yhtps_delack;		/* delayed acks sent */
	u_long	yhtps_timeoutdrop;	/* conn. dropped in rxmt timeout */
	u_long	yhtps_rexmttimeo;	/* retransmit timeouts */
	u_long	yhtps_persisttimeo;	/* persist timeouts */
	u_long	yhtps_keeptimeo;		/* keepalive timeouts */
	u_long	yhtps_keepprobe;		/* keepalive probes sent */
	u_long	yhtps_keepdrops;		/* connections dropped in keepalive */

	u_long	yhtps_sndtotal;		/* total packets sent */
	u_long	yhtps_sndpack;		/* data packets sent */
	u_long	yhtps_sndbyte;		/* data bytes sent */
	u_long	yhtps_sndrexmitpack;	/* data packets retransmitted */
	u_long	yhtps_sndrexmitbyte;	/* data bytes retransmitted */
	u_long	yhtps_sndacks;		/* ack-only packets sent */
	u_long	yhtps_sndprobe;		/* window probes sent */
	u_long	yhtps_sndurg;		/* packets sent with URG only */
	u_long	yhtps_sndwinup;		/* window update-only packets sent */
	u_long	yhtps_sndctrl;		/* control (SYN|FIN|RST) packets sent */

	u_long	yhtps_rcvtotal;		/* total packets received */
	u_long	yhtps_rcvpack;		/* packets received in sequence */
	u_long	yhtps_rcvbyte;		/* bytes received in sequence */
	u_long	yhtps_rcvbadsum;		/* packets received with ccksum errs */
	u_long	yhtps_rcvbadoff;		/* packets received with bad offset */
	u_long	yhtps_rcvshort;		/* packets received too short */
	u_long	yhtps_rcvduppack;	/* duplicate-only packets received */
	u_long	yhtps_rcvdupbyte;	/* duplicate-only bytes received */
	u_long	yhtps_rcvpartduppack;	/* packets with some duplicate data */
	u_long	yhtps_rcvpartdupbyte;	/* dup. bytes in part-dup. packets */
	u_long	yhtps_rcvoopack;		/* out-of-order packets received */
	u_long	yhtps_rcvoobyte;		/* out-of-order bytes received */
	u_long	yhtps_rcvpackafterwin;	/* packets with data after window */
	u_long	yhtps_rcvbyteafterwin;	/* bytes rcvd after window */
	u_long	yhtps_rcvafterclose;	/* packets rcvd after "close" */
	u_long	yhtps_rcvwinprobe;	/* rcvd window probe packets */
	u_long	yhtps_rcvdupack;		/* rcvd duplicate acks */
	u_long	yhtps_rcvacktoomuch;	/* rcvd acks for unsent data */
	u_long	yhtps_rcvackpack;	/* rcvd ack packets */
	u_long	yhtps_rcvackbyte;	/* bytes acked by rcvd acks */
	u_long	yhtps_rcvwinupd;		/* rcvd window update packets */
	u_long	yhtps_linger;		/* connections that lingered */
	u_long	yhtps_lingerabort;	/* lingers aborted by signal */
	u_long	yhtps_lingerexp;		/* linger timer expired */
	u_long	yhtps_lingercan;		/* linger timer cancelled */
};

#ifdef STRNET
#ifdef TLI_PRIMS
char	*yhtli_primitives[] = {"CONNECT", "ACCEPT", "DISCONNECT", "DATA",
			     "EX_DATA", "INFORMATION", "BIND", "UNBIND",
			     "UNITDATA", "OPTIONS", "ORDERLY RELEASE",
			     };
#endif TLI_PRIMS
#endif STRNET

#ifdef _KERNEL
extern struct	inpcb yhtcb;		/* head of queue of active yhtpcb's */
extern struct	yhtpstat yhtpstat;	/* yhtp statistics */
struct	yhtpiphdr *yhtp_template();
struct	yhtpcb *yhtp_close(), *yhtp_drop();
struct	yhtpcb *yhtp_timers(), *yhtp_disconnect(), *yhtp_usrclosed();
#endif
#ifdef sun
#define YHTP_COMPAT_42
#endif
#endif	/* _NET_YHTPIP_YHTP_VAR_H */
