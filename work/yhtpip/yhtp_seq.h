

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_YHTP_SEQ_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHTP_SEQ_H	/* subject to change without notice */



/*
 * YHTP sequence numbers are 32 bit integers operated
 * on with modular arithmetic.  These macros can be
 * used to compare such integers.
 */

#ifdef vax
#define	SEQ_LT(a,b)	((int)((a)-(b)) < 0)
#define	SEQ_LEQ(a,b)	((int)((a)-(b)) <= 0)
#define	SEQ_GT(a,b)	((int)((a)-(b)) > 0)
#define	SEQ_GEQ(a,b)	((int)((a)-(b)) >= 0)
#else
#define	SEQ_LT(a,b)	(((a)-(b))&0x80000000)
#define	SEQ_LEQ(a,b)	(!SEQ_GT(a,b))
#define	SEQ_GT(a,b)	SEQ_LT(b,a)
#define	SEQ_GEQ(a,b)	(!SEQ_LT(a,b))
#endif

/*
 * Macros to initialize yhtp sequence numbers for
 * send and receive from initial send and receive
 * sequence numbers.
 */
#define	yhtp_rcvseqinit(tp) \
	(tp)->rcv_adv = (tp)->rcv_nxt = (tp)->irs + 1

#define	yhtp_sendseqinit(tp) \
	(tp)->snd_una = (tp)->snd_nxt = (tp)->snd_max = (tp)->snd_up = \
	    (tp)->iss

#define	YHTP_ISSINCR	(125*1024)	/* increment for yhtp_iss each second */

#ifdef _KERNEL

#ifdef	_KERNEL_HEADERS

#ifndef _NET_YHTPIP_YHTP_H
#include <net/yhtpip/yhtp.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <netinet/yhtp.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

extern yhtp_seq	yhtp_iss;		/* yhtp initial send seq # */
#endif

#endif	/* _NET_YHTPIP_YHTP_SEQ_H */
