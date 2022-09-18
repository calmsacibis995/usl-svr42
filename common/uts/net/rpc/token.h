/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_TOKEN_H	/* wrapper symbol for kernel use */
#define _NET_RPC_TOKEN_H	/* subject to change without notice */

#ident	"@(#)uts-comm:net/rpc/token.h	1.2.2.2"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>
#endif

#ifndef _NET_TRANSPORT_TIUSER_H
#include <net/transport/tiuser.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/tiuser.h>

#endif /* _KERNEL_HEADERS */

/* token types, for get_remote_token call */
#define PRIVS_T		1
#define SENS_T		2
#define INFO_T		3
#define INTEG_T		4
#define NCS_T		5
#define ACL_T		6

/* the token data type */
typedef u_long	s_token;

/* the token service external interface */
#if defined(__STDC__)
extern s_token get_remote_token(struct netbuf *, u_int, caddr_t, u_int);
extern u_int map_local_token(s_token, u_int, caddr_t, u_int);
#else
extern s_token get_remote_token();
extern u_int map_local_token();
#endif

#endif	/* _NET_RPC_TOKEN_H */
