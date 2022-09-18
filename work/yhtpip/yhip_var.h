

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/




#ifndef _NET_YHTPIP_YHIP_VAR_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHIP_VAR_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>			/* REQUIRED */
#endif

#ifndef _IO_STREAM_H
#include <io/stream.h>			/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHIN_H
#include <net/yhtpip/yhin.h>		/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_BYTEORDER_H
#include <net/yhtpip/byteorder.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <netinet/in.h>			/* REQUIRED */
#include <sys/byteorder.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Overlay for ip header used by other protocols (tcp, udp).
 */
struct ipovly {
	caddr_t	ih_next;		/* for protocol sequence q's */
#ifdef STRNET
	mblk_t *ih_mblk;
#else
	caddr_t ih_prev;
#endif STRNET
	u_char	ih_x1;			/* (unused) */
	u_char	ih_pr;			/* protocol */
	short	ih_len;			/* protocol length */
	struct	in_addr ih_src;		/* source internet address */
	struct	in_addr ih_dst;		/* destination internet address */
};

/*
 * Ip reassembly queue structure.  Each fragment
 * being reassembled is attached to one of these structures.
 * They are timed out after ipq_ttl drops to 0, and may also
 * be reclaimed if memory becomes tight.
 */
struct ipq {
	struct	ipq *next,*prev;	/* to other reass headers */
	u_char	ipq_ttl;		/* time for reass q to live */
	u_char	ipq_p;			/* protocol of this fragment */
	u_short	ipq_id;			/* sequence id for reassembly */
	struct	ipasfrag *ipq_next;
					/* to ip headers of fragments */
	struct	in_addr ipq_src,ipq_dst;
};

/*
 * Ip header, when holding a fragment.
 *
 * Note: ipf_next must be at same offset as ipq_next above
 */
struct	ipasfrag {
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	ip_hl:4,
		ip_v:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_char	ip_v:4,
		ip_hl:4;
#endif
	u_char	ipf_mff;		/* copied from (ip_off&IP_MF) */
	short	ip_len;
	u_short	ip_id;
	short	ip_off;
	u_char	ip_ttl;
	u_char	ip_p;
	u_short	ip_sum;
	struct	ipasfrag *ipf_next;	/* next fragment */
#ifdef STRNET
	mblk_t	*ipf_mblk;	/* The mblk header for this data */
#else
	struct	ipasfrag *ipf_prev;	/* previous fragment */
#endif STRNET
};

#ifdef STRNET
/* get a pointer to an IP header from a pointer to a fragment */

#define IPHDR(ip) ((struct ip *) (ip))
#define IPASFRAG(ip) ((struct ipasfrag *) (ip))

#endif STRNET


/*
 * Structure stored in mbuf in inpcb.ip_options
 * and passed to ip_output when ip options are in use.
 * The actual length of the options (including ipopt_dst)
 * is in m_len.
 */
#define MAX_IPOPTLEN	40

struct ipoption {
	struct	in_addr ipopt_dst;	/* first-hop dst if source routed */
	char	ipopt_list[MAX_IPOPTLEN];	/* options proper */
};

struct	ipstat {
	long	ips_total;		/* total packets received */
	long	ips_badsum;		/* checksum bad */
	long	ips_tooshort;		/* packet too short */
	long	ips_toosmall;		/* not enough data */
	long	ips_badhlen;		/* ip header length < data size */
	long	ips_badlen;		/* ip length < ip header length */
	long	ips_fragments;		/* fragments received */
	long	ips_fragdropped;	/* frags dropped (dups, out of space) */
	long	ips_fragtimeout;	/* fragments timed out */
	long	ips_forward;		/* packets forwarded */
	long	ips_cantforward;	/* packets rcvd for unreachable dest */
	long	ips_redirectsent;	/* packets forwarded on same net */
};

#ifdef _KERNEL
/* flags passed to ip_output as last parameter */
#define	IP_FORWARDING		0x1		/* most of ip header exists */
#define	IP_ROUTETOIF		SO_DONTROUTE	/* bypass routing tables */
#define	IP_ALLOWBROADCAST	SO_BROADCAST	/* can send broadcast packets */

struct	ipstat	yhipstat;
struct	ipq	yhipq;			/* ip reass. queue */
u_short	yhip_id;				/* ip packet ctr, for ids */

#ifdef STRNET
mblk_t	*yhip_srcroute();
#else
struct	mbuf *yhip_srcroute();
#endif STRNET
#endif
#endif	/* _NET_YHTPIP_YHIP_VAR_H */
