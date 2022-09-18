/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_NETTYPE_H	/* wrapper symbol for kernel use */
#define _NET_RPC_NETTYPE_H	/* subject to change without notice */

#ident	"@(#)uts-comm:net/rpc/nettype.h	1.4.3.5"
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
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
*          All rights reserved.
*/ 

/*
 * nettype.h, Nettype definitions.
 * All for the topmost layer of rpc
 *
 */
#ifdef _KERNEL_HEADERS

#ifndef _NET_NETCONFIG_H
#include <net/netconfig.h>	/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/netconfig.h>	/* SVR4.0COMPAT */

#else

#include <netconfig.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define _RPC_NONE	0
#define _RPC_NETPATH	1
#define _RPC_VISIBLE	2
#define _RPC_CIRCUIT_V	3
#define _RPC_DATAGRAM_V	4
#define _RPC_CIRCUIT_N	5
#define _RPC_DATAGRAM_N	6
#define _RPC_TCP	7
#define _RPC_UDP	8

extern void *_rpc_setconf();
extern void _rpc_endconf();
extern struct netconfig *_rpc_getconf();
extern struct netconfig *_rpc_getconfip();

#endif /* _NET_RPC_NETTYPE_H */
