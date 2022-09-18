

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_YHIF_ETHER_H
#define _NET_YHTPIP_YHIF_ETHER_H

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _IO_STREAM_H
#include <io/stream.h>		/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHIN_H
#include <net/yhtpip/yhin.h>	/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHIF_H
#include <net/yhtpip/yhif.h>	/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHIF_ARP_H
#include <net/yhtpip/yhif_arp.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/stream.h>		/* REQUIRED */
#include <netinet/yhin.h>		/* REQUIRED */
#include <net/yhif.h>		/* REQUIRED */
#include <net/yhif_arp.h>		/* REQUIRED */

#else

/*
 * The following include is for compatibility with SunOS 3.x and
 * 4.3bsd.  Newly written programs should include it separately.
 */
#include <net/yhif_arp.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * Ethernet address - 6 octets
 */
typedef u_char ether_addr_t[6];

/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	ether_addr_t ether_dhost;
	ether_addr_t ether_shost;
	u_short	ether_type;
};

#define	ETHERTYPE_PUP		0x0200		/* PUP protocol */
#define	ETHERTYPE_IP		0x0800		/* IP protocol */
#define ETHERTYPE_YHIP		0x0801		/* YHIP protocol */
#define	ETHERTYPE_ARP		0x0806		/* Addr. resolution protocol */
#define ETHERTYPE_YHARP 	0x0807		/* YHARP protocol */
#define	ETHERTYPE_REVARP	0x8035		/* Reverse ARP */

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#define	ETHERMTU	1500
#define	ETHERMIN	(60-14)

/*
 * Ethernet Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to
 * RFC 826.
 */
struct	ether_yharp {
	struct	yharphdr ea_hdr;		/* fixed-size header */
	ether_addr_t yharp_sha;		/* sender hardware address */
	u_char	yharp_spa[4];		/* sender protocol address */
	ether_addr_t yharp_tha;		/* target hardware address */
	u_char	yharp_tpa[4];		/* target protocol address */
};
#define	yharp_hrd	ea_hdr.ar_hrd
#define	yharp_pro	ea_hdr.ar_pro
#define	yharp_hln	ea_hdr.ar_hln
#define	yharp_pln	ea_hdr.ar_pln
#define	yharp_op	ea_hdr.ar_op

/*
 *      multicast address structure
 *
 *      Keep a reference count for each multicast address so
 *      addresses loaded into chip are unique.
 */
struct  mcaddr {
        ether_addr_t mc_enaddr;			/* multicast address */
        u_short mc_count;                       /* reference count */
};
#define MCADDRMAX       64              /* multicast addr table length */
#define MCCOUNTMAX      4096            /* multicast addr max reference count */

/*
 * Structure shared between the ethernet driver modules and
 * the address resolution code.  For example, each ec_softc or il_softc
 * begins with this structure.
 *
 * The structure contains a pointer to an array of multicast addresses.
 * This pointer is NULL until the first successful SIOCADDMULTI ioctl
 * is issued for the interface.
 */
struct	yharpcom {
	struct		ifnet ac_if;		/* network-visible interface */
	ether_addr_t	ac_enaddr;		/* ethernet hardware address */
	struct in_addr	ac_ipaddr;		/* copy of ip address- XXX */
	struct mcaddr	*ac_mcaddr;		/* table of multicast addrs */
	u_short		ac_nmcaddr;		/* count of M/C addrs in use */
	struct in_addr	ac_lastip;      	/* cache of last ARP lookup */
	ether_addr_t	ac_lastyharp;		/* result of the last ARP */
	int		ac_mintu;		/* minimum transfer unit */
	int		ac_addrlen;		/* length of address */
	int		ac_mactype;		/* type of network */
};

/*
 * Internet to ethernet address resolution table.
 */
struct	yharptab {
	struct	in_addr at_iaddr;	/* internet address */
	union {
	    ether_addr_t atu_enaddr;	/* ethernet address */
	    long   atu_tvsec;			/* timestamp if incomplete */
	} 	at_union;
	u_char	at_timer;		/* minutes since last reference */
	u_char	at_flags;		/* flags */
#ifdef STRNET
	mblk_t	*at_hold;	/* last packet until resolved/timeout */
#else
	struct	mbuf *at_hold;	/* last packet until resolved/timeout */
#endif STRNET
};

# define at_enaddr at_union.atu_enaddr
# define at_tvsec at_union.atu_tvsec

/*
 * Compare two Ethernet addresses - assumes that the two given
 * pointers can be referenced as shorts.  On architectures
 * where this is not the case, use bcmp instead.  Note that like
 * bcmp, we return zero if they are the SAME.
 */
#if defined(sun2) || defined(sun3) || defined(sun3x)
/*
 * On 680x0 machines, we can do a longword compare that is NOT
 * longword aligned, as long as it is even aligned.
 */
#define ether_cmp(a,b) ( ((short *)a)[2] != ((short *)b)[2] || \
  *((long *)a) != *((long *)b) )
#endif

/*
 * On a sparc, functions are FAST
 */
#if defined(sparc)
#define ether_cmp(a,b) (sparc_ether_cmp((short *)a, (short *)b))
#endif 

#ifndef ether_cmp
#define ether_cmp(a,b) (bcmp((caddr_t)a,(caddr_t)b, 6))
#endif

/*
 * Copy Ethernet addresses from a to b - assumes that the two given
 * pointers can be referenced as shorts.  On architectures
 * where this is not the case, use bcopy instead.
 */
#if defined(sun2) || defined(sun3) || defined(sun3x)
#define ether_copy(a,b) { ((long *)b)[0]=((long *)a)[0]; \
 ((short *)b)[2]=((short *)a)[2]; }
#endif

#if defined(sparc)
#define ether_copy(a,b) { ((short *)b)[0]=((short *)a)[0]; \
 ((short *)b)[1]=((short *)a)[1]; ((short *)b)[2]=((short *)a)[2]; }
#endif

#ifndef ether_copy
#define ether_copy(a,b) (bcopy((caddr_t)a,(caddr_t)b, 6))
#endif

/*
 * Copy IP addresses from a to b - assumes that the two given
 * pointers can be referenced as shorts.  On architectures
 * where this is not the case, use bcopy instead.
 */
#if defined(sun2) || defined(sun3) || defined(sun3x)
#define ip_copy(a,b) { *((long *)b) = *((long *)a); }
#endif

#if defined(sparc)
#define ip_copy(a,b) { ((short *)b)[0]=((short *)a)[0]; \
 ((short *)b)[1]=((short *)a)[1]; }
#endif

#ifndef ip_copy
#define ip_copy(a,b) (bcopy((caddr_t)a,(caddr_t)b, 4))
#endif

#ifdef	_KERNEL
ether_addr_t yhetherbroadcastaddr;
struct	yharptab *yharptnew();
char *yhether_sprintf();
#endif	/* _KERNEL */

#endif	/* _NET_YHTPIP_YHIF_ETHER_H */
