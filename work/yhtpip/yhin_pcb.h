

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/



#ifndef _NET_YHTPIP_YHIN_PCB_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_YHIN_PCB_H	/* subject to change without notice */


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

#ifndef _NET_YHTPIP_YHROUTE_H
#include <net/yhtpip/yhroute.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/stream.h>		/* REQUIRED */
#include <netinet/in.h>		/* REQUIRED */
#include <net/route.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
#ifdef STRNET
struct inpcb {
	struct inpcb   *inp_next, *inp_prev;
	/* pointers to other pcb's */
	struct inpcb   *inp_head;	/* pointer back to chain of inpcb's
					 * for this protocol */
	short           inp_state;	/* old so_state from sockets */
	short           inp_tstate;	/* TLI state for this endpoint */
	short           inp_error;	/* error on this pcb */
	short           inp_minor;	/* minor device number allocated */
	queue_t        *inp_q;	/* queue for this minor dev */
	struct in_addr  inp_faddr;	/* foreign host table entry */
	struct in_addr  inp_laddr;	/* local host table entry */
	u_short         inp_fport;	/* foreign port */
	u_short         inp_lport;	/* local port */
#define inp_proto	inp_lport       /* overload port field for protocol */
	caddr_t         inp_ppcb;	/* pointer to per-protocol pcb */
	struct route    inp_route;	/* placeholder for routing entry */
	mblk_t         *inp_options;	/* IP options */
	ushort          inp_protoopt;	/* old so_options from sockets */
	ushort          inp_linger;	/* time to linger while closing */
	ushort          inp_protodef;	/* old pr_flags from sockets */
	ushort		inp_iocstate;	/* state for transparent ioctls */
	int		inp_addrlen;	/* address length client likes */
	int		inp_family;	/* address family client likes */
};
/*
 * inp_iocstate tells us which transparent ioctl we are in the process
 * of handling.	 inp_iocstate is usually set when the M_IOCTL message
 * for a transparent ioctl first seen.	It is used to decide what to do
 * when the subsequent associated M_IOCDATA message(s) arrive.
 * inp_iocstate == 0 means we are not currently processing any
 * transparent ioctls.
 */
#define INP_IOCS_DONAME 1

#else
struct inpcb {
	struct	inpcb *inp_next,*inp_prev;
					/* pointers to other pcb's */
	struct	inpcb *inp_head;	/* pointer back to chain of inpcb's
					   for this protocol */
	struct	in_addr inp_faddr;	/* foreign host table entry */
	u_short	inp_fport;		/* foreign port */
	struct	in_addr inp_laddr;	/* local host table entry */
	u_short	inp_lport;		/* local port */
	struct	socket *inp_socket;	/* back pointer to socket */
	caddr_t	inp_ppcb;		/* pointer to per-protocol pcb */
	struct	route inp_route;	/* placeholder for routing entry */
	struct	mbuf *inp_options;	/* IP options */
};
#endif /* STRNET */

#define	INPLOOKUP_WILDCARD	1
#define	INPLOOKUP_SETLOCAL	2

#define	sotoinpcb(so)	((struct inpcb *)(so)->so_pcb)

#ifdef _KERNEL
#ifdef STRNET
#define qtoinp(q) ((struct inpcb *) (q)->q_ptr)
struct inpcb *yhinpnewconn();
#endif /* STRNET */
struct	inpcb *yhin_pcblookup();
#endif

#endif	/* _NET_YHTPIP_YHIN_PCB_H */
