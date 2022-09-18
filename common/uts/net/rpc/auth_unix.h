/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_AUTH_UNIX_H	/* wrapper symbol for kernel use */
#define _NET_RPC_AUTH_UNIX_H	/* subject to change without notice */

#ident	"@(#)uts-comm:net/rpc/auth_unix.h	1.2.3.5"
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
 * auth_unix.h, Protocol for UNIX style authentication parameters for RPC
 */

/*
 * This file is now obsolete. Users should switch to auth_sys.h .
 */

#ifdef _KERNEL_HEADERS

#ifndef _NET_RPC_AUTH_SYS_H
#include <net/rpc/auth_sys.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <rpc/auth_sys.h>	/* REQUIRED */

#else

#include <rpc/auth_sys.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#endif /* _NET_RPC_AUTH_UNIX_H */
