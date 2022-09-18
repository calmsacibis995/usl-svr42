/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/svc_soc.h	1.3.3.5"
#ident	"$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
*          All rights reserved.
*/ 

/*
 * svc.h, Server-side remote procedure call interface.
 */

#ifndef _NET_RPC_SVC_SOC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_SVC_SOC_H	/* subject to change without notice */

#ifdef _KERNEL_HEADERS

#ifndef _NET_RPC_TYPES_H
#include <net/rpc/types.h>		/* REQUIRED */
#endif

#ifndef _NET_RPC_SVC_H
#include <net/rpc/svc.h>		/* REQUIRED */
#endif

#ifndef _NET_TRANSPORT_SOCKET_H
#include <net/transport/socket.h>	/* SVR4.0COMPAT */
#endif

#ifndef _NET_TCPIP_IN_H
#include <net/tcpip/in.h>		/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <rpc/types.h>			/* REQUIRED */
#include <rpc/svc.h>			/* REQUIRED */
#include <sys/socket.h>			/* SVR4.0COMPAT */
#include <netinet/in.h>			/* SVR4.0COMPAT */

#else

/*
 * All the following declarations are only for backward compatibility
 * with SUNOS 4.0.
 */
#include <sys/socket.h>			/* SVR4.0COMPAT */
#include <netinet/in.h>			/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 *  Approved way of getting address of caller
 */
#define svc_getcaller(x) ((struct sockaddr_in *)(x)->xp_rtaddr.buf)

/*
 * Service registration
 *
 * svc_register(xprt, prog, vers, dispatch, protocol)
 *	SVCXPRT *xprt;
 *	u_long prog;
 *	u_long vers;
 *	void (*dispatch)();
 *	int protocol;  /* like TCP or UDP, zero means do not register 
 */
extern bool_t svc_register();

/*
 * Service un-registration
 *
 * svc_unregister(prog, vers)
 *	u_long prog;
 *	u_long vers;
 */
extern void svc_unregister();

/*
 * Memory based rpc for testing and timing.
 */
extern SVCXPRT *svcraw_create();

/*
 * Udp based rpc. For compatibility reasons
 */
extern SVCXPRT *svcudp_create();
extern SVCXPRT *svcudp_bufcreate();

/*
 * Tcp based rpc.
 */
extern SVCXPRT *svctcp_create();
extern SVCXPRT *svcfd_create();

#else

#endif /* !_NET_RPC_SVC_SOC_H */
