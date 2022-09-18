/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_CLNT_SOC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_CLNT_SOC_H	/* subject to change without notice */

#ident	"@(#)uts-comm:net/rpc/clnt_soc.h	1.3.3.5"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * clnt.h - Client side remote procedure call interface.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */
#ifdef _KERNEL_HEADERS

#ifndef _NET_RPC_CLNT_H
#include <net/rpc/clnt.h>		/* REQUIRED */
#endif

#ifndef _NET_TRANSPORT_SOCKET_H
#include <net/transport/socket.h>	/* SVR4.0COMPAT */
#endif

#ifndef _NET_TCPIP_IN_H
#include <net/tcpip/in.h>		/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <rpc/clnt.h>			/* REQUIRED */
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

#define UDPMSGSIZE	8800	/* rpc imposed limit on udp msg size */

/*
 * enum clnt_stat
 * callrpc(host, prognum, versnum, procnum, inproc, in, outproc, out)
 *	char *host;
 *	u_long prognum, versnum, procnum;
 *	xdrproc_t inproc, outproc;
 *	char *in, *out;
 *	char *nettype;
 */
extern int callrpc();

/*
 * TCP based rpc
 * CLIENT *
 * clnttcp_create(raddr, prog, vers, fdp, sendsz, recvsz)
 *	struct sockaddr_in *raddr;
 *	u_long prog;
 *	u_long version;
 *	int *fdp;
 *	u_int sendsz;
 *	u_int recvsz;
 */
extern CLIENT *clnttcp_create();

/*
 * UDP based rpc.
 * CLIENT *
 * clntudp_create(raddr, program, version, wait, fdp)
 *	struct sockaddr_in *raddr;
 *	u_long program;
 *	u_long version;
 *	struct timeval wait;
 *	int *fdp;
 *
 * Same as above, but you specify max packet sizes.
 * CLIENT *
 * clntudp_bufcreate(raddr, program, version, wait, fdp, sendsz, recvsz)
 *	struct sockaddr_in *raddr;
 *	u_long program;
 *	u_long version;
 *	struct timeval wait;
 *	int *fdp;
 *	u_int sendsz;
 *	u_int recvsz;
 *
 */
extern CLIENT *clntudp_create();
extern CLIENT *clntudp_bufcreate();

/*
 * Memory based rpc (for speed check and testing)
 * CLIENT *
 * clntraw_create(prog, vers)
 *	u_long prog;
 *	u_long vers;
 */
extern CLIENT *clntraw_create();

#endif /* _NET_RPC_CLNT_SOC_H */
