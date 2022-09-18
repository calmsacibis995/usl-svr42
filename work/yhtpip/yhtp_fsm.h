

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_YHTP_FSM_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHTP_FSM_H	/* subject to change without notice */


/*
 * YHTP FSM state definitions.
 */

#define	YHTP_NSTATES	11

#define	YHTPS_CLOSED		0	/* closed */
#define	YHTPS_LISTEN		1	/* listening for connection */
#define	YHTPS_SYN_SENT		2	/* active, have sent syn */
#define	YHTPS_SYN_RECEIVED	3	/* have send and received syn */
/* states <YHTPS_ESTABLISHED are those where connections not established */
#define	YHTPS_ESTABLISHED	4	/* established */
#define	YHTPS_CLOSE_WAIT		5	/* rcvd fin, waiting for close */
/* states >YHTPS_CLOSE_WAIT are those where user has closed */
#define	YHTPS_FIN_WAIT_1		6	/* have closed, sent fin */
#define	YHTPS_CLOSING		7	/* closed xchd FIN; await FIN ACK */
#define	YHTPS_LAST_ACK		8	/* had fin and close; await FIN ACK */
/* states >YHTPS_CLOSE_WAIT && <YHTPS_FIN_WAIT_2 await ACK of FIN */
#define	YHTPS_FIN_WAIT_2		9	/* have closed, fin is acked */
#define	YHTPS_TIME_WAIT		10	/* in 2*msl quiet wait after close */

#define	YHTPS_HAVERCVDSYN(s)	((s) >=YHTPS_SYN_RECEIVED)

#ifdef STRNET
#define sbit(s) (1 << (s))
#define YHTPS_HAVERCVDFIN(s)	(sbit(s) & ( \
					    sbit(YHTPS_CLOSE_WAIT) | \
					    sbit(YHTPS_CLOSING) | \
					    sbit(YHTPS_LAST_ACK) | \
					    sbit(YHTPS_TIME_WAIT) \
					    ))
#else
#define	YHTPS_HAVERCVDFIN(s)	((s) >=YHTPS_TIME_WAIT)
#endif STRNET

#ifdef	YHTPOUTFLAGS
/*
 * Flags used when sending segments in yhtp_output.
 * Basic flags (TH_RST,TH_ACK,TH_SYN,TH_FIN) are totally
 * determined by state, with the proviso that TH_FIN is sent only
 * if all data queued for output is included in the segment.
 */
u_char	yhtp_outflags[YHTP_NSTATES] = {
    TH_RST|TH_ACK, 0, TH_SYN, TH_SYN|TH_ACK,
    TH_ACK, TH_ACK,
    TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_ACK, TH_ACK,
};
#endif

#ifdef	YHTPSTATES
char *yhtpstates[] = {
	"CLOSED",	"LISTEN",	"SYN_SENT",	"SYN_RCVD",
	"ESTABLISHED",	"CLOSE_WAIT",	"FIN_WAIT_1",	"CLOSING",
	"LAST_ACK",	"FIN_WAIT_2",	"TIME_WAIT",
};
#endif
#endif	/* _NET_YHTPIP_YHTP_FSM_H */
