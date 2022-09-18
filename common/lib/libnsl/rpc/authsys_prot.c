/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)libnsl:common/lib/libnsl/rpc/authsys_prot.c	1.3.6.3"
#ident  "$Header: authsys_prot.c 1.2 91/06/26 $"

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
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)authsys_prot.c 1.24 89/02/07 Copyr 1984 Sun Micro";
#endif

/*
 * authsys_prot.c
 * XDR for UNIX style authentication parameters for RPC
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_sys.h>

/*
 * XDR for unix authentication parameters.
 */
bool_t
xdr_authsys_parms(xdrs, p)
	register XDR *xdrs;
	register struct authsys_parms *p;
{
	if (xdr_u_long(xdrs, &(p->aup_time)) &&
	    xdr_string(xdrs, &(p->aup_machname), MAX_MACHINE_NAME) &&
	    xdr_uid_t(xdrs, (uid_t *)&(p->aup_uid)) &&
	    xdr_gid_t(xdrs, (gid_t *)&(p->aup_gid)) &&
	    xdr_array(xdrs, (caddr_t *)&(p->aup_gids),
		    &(p->aup_len), NGRPS, sizeof (gid_t), xdr_gid_t)) {
		return (TRUE);
	}
	return (FALSE);
}

/*
 * XDR user id types (uid_t)
 */
bool_t
xdr_uid_t(xdrs, ip)
	XDR *xdrs;
	int *ip;
{
#ifdef lint
	(void) (xdr_short(xdrs, (short *)ip));
	return (xdr_long(xdrs, (long *)ip));
#else
	if (sizeof (uid_t) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ip));
	} else {
		return (xdr_short(xdrs, (short *)ip));
	}
#endif
}

/*
 * XDR group id types (gid_t)
 */
bool_t
xdr_gid_t(xdrs, ip)
	XDR *xdrs;
	int *ip;
{
#ifdef lint
	(void) (xdr_short(xdrs, (short *)ip));
	return (xdr_long(xdrs, (long *)ip));
#else
	if (sizeof (gid_t) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ip));
	} else {
		return (xdr_short(xdrs, (short *)ip));
	}
#endif
}
