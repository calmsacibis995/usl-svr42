

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_YHTP_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHTP_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>			/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_BYTEORDER_H
#include <net/yhtpip/byteorder.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>			/* REQUIRED */
#include <sys/byteorder.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	u_long	yhtp_seq;

/*
 * YHTP header.
 * Per RFC 793, September, 1981.
 */
struct yhtphdr {
	u_short	th_sport;		/* source port */
	u_short	th_dport;		/* destination port */
	yhtp_seq	th_seq;			/* sequence number */
	yhtp_seq	th_ack;			/* acknowledgement number */
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	th_x2:4,		/* (unused) */
		th_off:4;		/* data offset */
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_char	th_off:4,		/* data offset */
		th_x2:4;		/* (unused) */
#endif
	u_char	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
	u_short	th_win;			/* window */
	u_short	th_sum;			/* checksum */
	u_short	th_urp;			/* urgent pointer */
};

#define	YHTPOPT_EOL	0
#define	YHTPOPT_NOP	1
#define	YHTPOPT_MAXSEG	2

/*
 * Default maximum segment size for YHTP.
 * With an IP MSS of 576, this is 536,
 * but 512 is probably more convenient.
 */
#ifdef	lint
#define	YHTP_MSS	536
#else
#ifndef IP_MSS
#define IP_MSS	576
#endif
#define	YHTP_MSS	MIN(512, IP_MSS - sizeof (struct yhtpiphdr))
#endif

/*
 * User-settable options (used with setsockopt).
 */
#define	YHTP_NODELAY	0x01	/* don't delay send to coalesce packets */
#define	YHTP_MAXSEG	0x02	/* set maximum segment size */

#endif	/* _NET_YHTPIP_YHTP_H */
