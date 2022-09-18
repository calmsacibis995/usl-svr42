/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/rpc.h	1.4.2.3"
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
 * rpc.h, Just includes the billions of rpc header files necessary to 
 * do remote procedure calling.
 *
 */
#ifndef _NET_RPC_RPC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPC_H	/* subject to change without notice */

#ifdef _KERNEL_HEADERS

#ifndef _NET_RPC_TYPES_H
#include <net/rpc/types.h>	
#endif

#ifndef _NET_TRANSPORT_TIUSER_H
#include <net/transport/tiuser.h>	
#endif
#ifndef _FS_FCNTL_H
#include <fs/fcntl.h>	
#endif
#ifndef _NET_TCPIP_IN_H
#include <net/tcpip/in.h>	
#endif
#ifndef _NET_KTLI_T_KUSER_H
#include <net/ktli/t_kuser.h>	
#endif

#ifndef _NET_RPC_XDR_H
#include <net/rpc/xdr.h>	
#endif
#ifndef _NET_RPC_AUTH_H
#include <net/rpc/auth.h>	
#endif
#ifndef _NET_RPC_CLNT_H
#include <net/rpc/clnt.h>	
#endif

#ifndef _NET_RPC_RPC_MSG_H
#include <net/rpc/rpc_msg.h>	
#endif
#ifndef _NET_RPC_AUTH_SYS_H
#include <net/rpc/auth_sys.h>	
#endif
#ifndef _NET_RPC_AUTH_DES_H
#include <net/rpc/auth_des.h>	
#endif
#ifndef _NET_RPC_AUTH_ESV_H
#include <net/rpc/auth_esv.h>
#endif

#ifndef _NET_RPC_SVC_H
#include <net/rpc/svc.h>	
#endif
#ifndef _NET_RPC_SVC_AUTH_H
#include <net/rpc/svc_auth.h>	
#endif

#elif defined(_KERNEL)

#include <rpc/types.h>	
#include <sys/tiuser.h>	
#include <sys/fcntl.h>	
#include <netinet/in.h>	
#include <sys/t_kuser.h>	
#include <rpc/xdr.h>	
#include <rpc/auth.h>	
#include <rpc/clnt.h>	
#include <rpc/rpc_msg.h>	
#include <rpc/auth_sys.h>	
#include <rpc/auth_des.h>	
#include <rpc/auth_esv.h>
#include <rpc/svc.h>	
#include <rpc/svc_auth.h>	

#else

#include <rpc/types.h>		/* some typedefs */

#include <tiuser.h>
#include <fcntl.h>
#include <memory.h>

#include <rpc/xdr.h>		/* generic (de)serializer */
#include <rpc/auth.h>		/* generic authenticator (client side) */
#include <rpc/clnt.h>		/* generic client side rpc */

#include <rpc/rpc_msg.h>	/* protocol for rpc messages */
#include <rpc/auth_sys.h>	/* protocol for unix style cred */
#include <rpc/auth_des.h>	/* protocol for des style cred */

#include <rpc/svc.h>		/* service manager and multiplexer */
#include <rpc/svc_auth.h>	/* service side authenticator */

#include <rpc/rpcb_clnt.h>	/* rpcbind interface functions */

#endif /* _KERNEL_HEADERS */


#endif /* ! _NET_RPC_RPC_H */
