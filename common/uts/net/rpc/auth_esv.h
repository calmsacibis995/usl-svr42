/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_AUTH_ESV_H	/* wrapper symbol for kernel use */
#define _NET_RPC_AUTH_ESV_H	/* subject to change without notice */

#ident	"@(#)uts-comm:net/rpc/auth_esv.h	1.2"
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
 * auth_esv.h, Protocol for secure style authentication parameters for RPC
 *
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _NET_RPC_TYPES_H
#include <net/rpc/types.h>	/* REQUIRED */
#endif

#ifndef _NET_RPC_AUTH_H
#include <net/rpc/auth.h>	/* REQUIRED */
#endif

#ifndef _NET_RPC_TOKEN_H
#include <net/rpc/token.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <rpc/auth.h>		/* REQUIRED */
#include <rpc/token.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The system is very weak.  The client uses no encryption for  it
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 */

/* The machine name is part of a credential; it may not exceed 255 bytes */
#define MAX_ESVMACH_NAME 255

/* gids compose part of a credential; there may not be more than 24 of them */
#define ESV_NGRPS 24

/*
 * "esv" (secure) style credentials.
 */
struct authesv_parms {
	u_long	 auc_stamp;
	char	*auc_machname;
	uid_t	 auc_uid;
	gid_t	 auc_gid;
	u_int	 auc_len;
	gid_t	*auc_gids;
	u_long	 auc_aid;
	s_token	 auc_privs;
	s_token	 auc_sens;
	s_token	 auc_info;
	s_token	 auc_integ;
	s_token	 auc_ncs;
};
extern bool_t xdr_authesv_parms();

#endif /* _NET_RPC_AUTH_ESV_H */
