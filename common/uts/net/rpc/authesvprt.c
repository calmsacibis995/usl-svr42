/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/authesvprt.c	1.1"
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
 * authesv_prot.c
 * XDR for ESV style authentication parameters for RPC
 */

#include <net/rpc/types.h>

#ifdef _KERNEL
/* #include <util/param.h> */
/* #include <svc/time.h> */
#endif

#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_esv.h>
/* #include <svc/utsname.h> */

/*
 * XDR for secure authentication parameters.
 */
bool_t
xdr_authesv_parms(xdrs, p)
	register XDR *xdrs;
	register struct authesv_parms *p;
{

	if (xdr_u_long(xdrs, &(p->auc_stamp))
	    && xdr_string(xdrs, &(p->auc_machname), MAX_ESVMACH_NAME)
	    && xdr_int(xdrs, (int *)&(p->auc_uid))
	    && xdr_int(xdrs, (int *)&(p->auc_gid))
	    && xdr_array(xdrs, (caddr_t *)&(p->auc_gids),
		    &(p->auc_len), ESV_NGRPS, sizeof(int), xdr_int)
	    && xdr_u_long(xdrs, &(p->auc_aid))
	    && xdr_u_long(xdrs, (u_long *)&(p->auc_privs))
	    && xdr_u_long(xdrs, (u_long *)&(p->auc_sens))
	    && xdr_u_long(xdrs, (u_long *)&(p->auc_info))
	    && xdr_u_long(xdrs, (u_long *)&(p->auc_integ))
	    && xdr_u_long(xdrs, (u_long *)&(p->auc_ncs)) ) {
		return (TRUE);
	}
	return (FALSE);
}
