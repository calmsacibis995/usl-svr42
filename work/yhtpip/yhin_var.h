

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_IN_VAR_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_IN_VAR_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _NET_YHTPIP_YHIF_H
#include <net/yhtpip/yhif.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <net/if.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Interface address, Internet version.  One of these structures
 * is allocated for each interface with an Internet address.
 * The ifaddr structure contains the protocol-independent part
 * of the structure and is assumed to be first.
 */
struct in_ifaddr {
	struct	ifaddr ia_ifa;		/* protocol-independent info */
#define	ia_addr	ia_ifa.ifa_addr
#define	ia_broadaddr	ia_ifa.ifa_broadaddr
#define	ia_dstaddr	ia_ifa.ifa_dstaddr
#define	ia_ifp		ia_ifa.ifa_ifp
	u_long	ia_net;			/* network number of interface */
	u_long	ia_netmask;		/* mask of net part */
	u_long	ia_subnet;		/* subnet number, including net */
	u_long	ia_subnetmask;		/* mask of net + subnet */
	struct	in_addr ia_netbroadcast; /* broadcast addr for (logical) net */
	int	ia_flags;
	struct	in_ifaddr *ia_next;	/* next in list of internet addresses */
};
/*
 * Given a pointer to an in_ifaddr (ifaddr),
 * return a pointer to the addr as a sockadd_in.
 */
#define	IA_SIN(ia) ((struct sockaddr_in *)(&((struct in_ifaddr *)ia)->ia_addr))
/*
 * ia_flags
 */
#define	IFA_ROUTE	0x01		/* routing entry installed */

#ifdef	_KERNEL
struct	in_ifaddr *in_ifaddr;
struct	in_ifaddr *in_iaonnetof();
struct	ifqueue	ipintrq;		/* ip packet input queue */
#endif
#endif	/* _NET_YHTPIP_YHIN_VAR_H */
