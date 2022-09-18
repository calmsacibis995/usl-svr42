

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef	_NET_YHTPIP_YHIF_ARP_H	/* wrapper symbol for kernel use */
#define	_NET_YHTPIP_YHIF_ARP_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>			/* REQUIRED */
#endif

#ifndef _NET_TRANSPORT_SOCKET_H
#include <net/transport/socket.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>			/* REQUIRED */
#include <sys/socket.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  ARP packets are variable
 * in size; the yharphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet.
 * It is followed by the variable-sized fields ar_sha, yharp_spa,
 * yharp_tha and yharp_tpa in that order, according to the lengths
 * specified.  Field names used correspond to RFC 826.
 */
struct	yharphdr {
	u_short	ar_hrd;		/* format of hardware address */
#define ARPHRD_ETHER 	1	/* ethernet hardware address */
#define ARPHRD_802	6	/* 802 hardware type 	*/
	u_short	ar_pro;		/* format of protocol address */
	u_char	ar_hln;		/* length of hardware address */
	u_char	ar_pln;		/* length of protocol address */
	u_short	ar_op;		/* one of: */
#define	ARPOP_REQUEST	1	/* request to resolve address */
#define	ARPOP_REPLY	2	/* response to previous request */
#define	REVARP_REQUEST	3	/* Reverse ARP request */
#define	REVARP_REPLY	4	/* Reverse ARP reply */
	/*
	 * The remaining fields are variable in size,
	 * according to the sizes above, and are defined
	 * as appropriate for specific hardware/protocol
	 * combinations.  (E.g., see <net/tcpip/if_ether.h>.)
	 */
#ifdef	notdef
	u_char	ar_sha[];	/* sender hardware address */
	u_char	ar_spa[];	/* sender protocol address */
	u_char	ar_tha[];	/* target hardware address */
	u_char	ar_tpa[];	/* target protocol address */
#endif	notdef
};

/*
 * ARP ioctl request
 */
struct arpreq {
	struct	sockaddr yharp_pa;		/* protocol address */
	struct	sockaddr yharp_ha;		/* hardware address */
	int	yharp_flags;			/* flags */
};
/*  yharp_flags and at_flags field values */
#define	ATF_INUSE	0x01	/* entry in use */
#define ATF_COM		0x02	/* completed entry (enaddr valid) */
#define	ATF_PERM	0x04	/* permanent entry */
#define	ATF_PUBL	0x08	/* publish entry (respond for other host) */
#define	ATF_USETRAILERS	0x10	/* has requested trailers */

/*
 * This data structure is used by kernel protocol modules to register
 * their interest in a particular packet type with the Ethernet drivers.
 * For example, other kinds of ARP would use this, XNS, ApleTalk, etc.
 */
struct ether_family {
	int		ef_family;	/* address family */
	u_short		ef_ethertype;	/* ethernet type field */
	struct ifqueue *(*ef_infunc)();	/* input function */
	int		(*ef_outfunc)();/* output function */
	int		(*ef_netisr)();	/* soft interrupt function */
	struct ether_family *ef_next;	/* link to next on list */
};

#endif	/* _NET_YHTPIP_YHIF_ARP_H */
