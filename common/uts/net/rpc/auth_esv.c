/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/auth_esv.c	1.1"
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

#ifdef _KERNEL

/*
 * auth_esv.c, implements secure-style authentication parameters in the kernel. 
 * Interfaces with svcauthesv on the server.
 */

#include <util/param.h>
#include <svc/time.h>
#include <util/types.h>
#include <net/rpc/types.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_esv.h>
#include <net/rpc/token.h>
#include <svc/utsname.h>
#include <proc/cred.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#include <util/debug.h>

/*
 * Unix authenticator operations vector
 */
STATIC	void	authesv_nextverf();
STATIC	bool_t	authesv_marshal();
STATIC	bool_t	authesv_validate();
STATIC	bool_t	authesv_refresh();
STATIC	void	authesv_destroy();

static struct auth_ops auth_esv_ops = {
	authesv_nextverf,
	authesv_marshal,
	authesv_validate,
	authesv_refresh,
	authesv_destroy
};


/*
 * Create a kernel secure style authenticator.
 * Returns an auth handle.
 */
AUTH *
authesv_create()
{
	register AUTH *auth;

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)kmem_alloc((u_int)sizeof(*auth), KM_SLEEP);
	auth->ah_ops = &auth_esv_ops;
	auth->ah_cred.oa_flavor = AUTH_ESV;
	auth->ah_verf = _null_auth;
	return (auth);
}

/*
 * authesv operations
 */
/*ARGSUSED*/
STATIC void
authesv_nextverf(auth)
	AUTH *auth;
{

	/* no action necessary */
}

STATIC bool_t
authesv_marshal(auth, xdrs, haddr)
	AUTH *auth;
	XDR *xdrs;
	struct netbuf *haddr;	/* address of remote host for tokens */
{

	char	*sercred;
	XDR	xdrm;
	struct	opaque_auth *cred;
	bool_t	ret = FALSE;
	register gid_t *gp, *gpend;
	register int gidlen, credsize;
	register long *ptr;
	struct authesv_parms authp;

	/*
	 * First we try a fast path to get through
	 * this very common operation.
	 */
	gp	= u.u_procp->p_cred->cr_groups;
	gidlen	= u.u_procp->p_cred->cr_ngroups;
	gpend	= &gp[gidlen-1];
	authp.auc_aid	= u.u_procp->p_pid;
	authp.auc_privs	= get_remote_token(haddr, PRIVS_T, (caddr_t)&u.u_cred->cr_wkgpriv, sizeof(pvec_t));
	authp.auc_sens	= get_remote_token(haddr, SENS_T, (caddr_t)&u.u_cred->cr_lid, sizeof(lid_t));
	authp.auc_info	= 0;		/* unsupported */
	authp.auc_integ	= 0;		/* unsupported */
	authp.auc_ncs	= 0;		/* unsupported */
	credsize = 4 + 4 + roundup(strlen(utsname.nodename), 4) + 4 + 4 + 4 +
		   gidlen * 4 + 4 + 4 + 4 + 4 + 4 + 4;
	ptr = XDR_INLINE(xdrs, 4 + 4 + credsize + 4 + 4);
	if (ptr) {
		/*
		 * We can do the fast path.
		 */
		IXDR_PUT_LONG(ptr, AUTH_ESV);	/* cred flavor */
		IXDR_PUT_LONG(ptr, credsize);	/* cred len */
		IXDR_PUT_LONG(ptr, hrestime.tv_sec);	/* auc_stamp */
		IXDR_PUT_LONG(ptr, strlen(utsname.nodename));
		bcopy(utsname.nodename, (caddr_t)ptr, strlen(utsname.nodename));
		ptr += roundup(strlen(utsname.nodename), 4) / 4;
		IXDR_PUT_LONG(ptr, u.u_procp->p_cred->cr_uid);
		IXDR_PUT_LONG(ptr, u.u_procp->p_cred->cr_gid);
		IXDR_PUT_LONG(ptr, gidlen);
		while (gp <= gpend) {
			IXDR_PUT_LONG(ptr, *gp++);
		}
		IXDR_PUT_LONG(ptr, authp.auc_aid);
		IXDR_PUT_LONG(ptr, authp.auc_privs);
		IXDR_PUT_LONG(ptr, authp.auc_sens);
		IXDR_PUT_LONG(ptr, authp.auc_info);
		IXDR_PUT_LONG(ptr, authp.auc_integ);
		IXDR_PUT_LONG(ptr, authp.auc_ncs);
		IXDR_PUT_LONG(ptr, AUTH_NULL);	/* verf flavor */
		IXDR_PUT_LONG(ptr, 0);	/* verf len */
		return (TRUE);
	}
	sercred = (char *)kmem_alloc((u_int)MAX_AUTH_BYTES, KM_SLEEP);
	/*
	 * serialize u struct stuff into sercred
	 */
	xdrmem_create(&xdrm, sercred, MAX_AUTH_BYTES, XDR_ENCODE);
	/* finish filling authesv_parms struct */
	authp.auc_stamp = hrestime.tv_sec;
	authp.auc_machname = utsname.nodename;
	authp.auc_uid = u.u_cred->cr_uid;
	authp.auc_gid = u.u_cred->cr_gid;
	authp.auc_len = u.u_cred->cr_ngroups;
	authp.auc_gids = u.u_cred->cr_groups;
	if (! xdr_authesv_parms(&xdrm, &authp)) {
		printf("authesv_marshal: xdr_authesv failed\n");
		ret = FALSE;
		goto done;
	}

	/*
	 * Make opaque auth credentials that point at serialized u struct
	 */
	cred = &(auth->ah_cred);
	cred->oa_length = XDR_GETPOS(&xdrm);
	cred->oa_base = sercred;

	/*
	 * serialize credentials and verifiers (null)
	 */
	if ((xdr_opaque_auth(xdrs, &(auth->ah_cred)))
	    && (xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}
done:
	kmem_free((caddr_t)sercred, (u_int)MAX_AUTH_BYTES);
	return (ret);
}

/*ARGSUSED*/
STATIC bool_t
authesv_validate(auth, verf)
	AUTH *auth;
	struct opaque_auth verf;
{

	return (TRUE);
}

/*ARGSUSED*/
STATIC bool_t
authesv_refresh(auth)
	AUTH *auth;
{
	return (FALSE);
}

STATIC void
authesv_destroy(auth)
	register AUTH *auth;
{

	kmem_free((caddr_t)auth, (u_int)sizeof(*auth));
}
#endif	/* _KERNEL */
