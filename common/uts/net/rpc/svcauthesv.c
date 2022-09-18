/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/svcauthesv.c	1.1"
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
 * svcauthesv.c
 * Handles ESV flavor authentication parameters on the service side of rpc.
 */

#ifdef _KERNEL
#include <util/param.h>
#include <util/types.h>
#include <svc/time.h>
#include <net/tcpip/in.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc_msg.h>
#include <net/transport/tiuser.h>
#include <net/transport/tihdr.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <net/rpc/auth_esv.h>
#include <net/rpc/svc_auth.h>
#include <net/rpc/token.h>
#else
#include <stdio.h>
#include <svc/time.h>
#include <net/rpc/rpc.h>
#include <io/syslog.h>
#endif

/*
 * ESV authenticator
 */
enum auth_stat
_svcauth_esv(rqst, msg)
	register struct svc_req *rqst;
	register struct rpc_msg *msg;
{
	register enum auth_stat stat;
	XDR xdrs;
	register struct authesv_parms *auc;
	register long *buf;
	struct area {
		struct authesv_parms area_auc;
		char area_machname[MAX_ESVMACH_NAME+1];
		u_int area_gids[ESV_NGRPS];
	} *area;
	u_int auth_len;
	int str_len, gid_len;
	register int i;

	/* LINTED pointer alignment */
	area = (struct area *) rqst->rq_clntcred;
	auc = &area->area_auc;
	auc->auc_machname = area->area_machname;
	auc->auc_gids = (gid_t *)area->area_gids;
	auth_len = (u_int)msg->rm_call.cb_cred.oa_length;
	xdrmem_create(&xdrs, msg->rm_call.cb_cred.oa_base, auth_len,XDR_DECODE);
	buf = XDR_INLINE(&xdrs, auth_len);
	if (buf != NULL) {
		auc->auc_stamp = IXDR_GET_LONG(buf);
		str_len = IXDR_GET_U_LONG(buf);
		if (str_len > MAX_ESVMACH_NAME) {
			stat = AUTH_BADCRED;
			goto done;
		}
		bcopy((caddr_t)buf, auc->auc_machname, (u_int)str_len);
		auc->auc_machname[str_len] = 0;
		str_len = RNDUP(str_len);
		buf += str_len / sizeof (long);
		auc->auc_uid = IXDR_GET_LONG(buf);
		auc->auc_gid = IXDR_GET_LONG(buf);
		gid_len = IXDR_GET_U_LONG(buf);
		if (gid_len > ESV_NGRPS) {
			stat = AUTH_BADCRED;
			goto done;
		}
		auc->auc_len = gid_len;
		for (i = 0; i < gid_len; i++) {
			auc->auc_gids[i] = IXDR_GET_LONG(buf);
		}
		auc->auc_aid = IXDR_GET_LONG(buf);
		auc->auc_privs = (s_token)IXDR_GET_LONG(buf);
		auc->auc_sens = (s_token)IXDR_GET_LONG(buf);
		auc->auc_info = (s_token)IXDR_GET_LONG(buf);
		auc->auc_integ = (s_token)IXDR_GET_LONG(buf);
		auc->auc_ncs = (s_token)IXDR_GET_LONG(buf);
		/*
		 * 11 is the smallest unix credentials structure -
		 * timestamp, hostname len (0), uid, gid, gids len (0), and
		 * the 6 secure fields: auditid, privs, sens, info, integ, ncs.
		 */
		if ((11 + gid_len) * BYTES_PER_XDR_UNIT + str_len > auth_len) {
#ifdef	_KERNEL
			printf("bad auth_len gid %d str %d auth %d",
			    gid_len, str_len, auth_len);
#else
			(void) syslog(LOG_ERR, "bad auth_len gid %d str %d auth %d",
			    gid_len, str_len, auth_len);
#endif
			stat = AUTH_BADCRED;
			goto done;
		}
	} else if (! xdr_authesv_parms(&xdrs, auc)) {
		xdrs.x_op = XDR_FREE;
		(void)xdr_authesv_parms(&xdrs, auc);
		stat = AUTH_BADCRED;
		goto done;
	}
	rqst->rq_xprt->xp_verf.oa_flavor = AUTH_NULL;
	rqst->rq_xprt->xp_verf.oa_length = 0;
	stat = AUTH_OK;
done:
	XDR_DESTROY(&xdrs);
	return (stat);
}
