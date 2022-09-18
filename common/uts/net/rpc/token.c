/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/token.c	1.1.2.1"
#ident	"$Header: $"

#include <net/rpc/token.h>
#include <net/transport/tiuser.h>
#include <svc/errno.h>
#include <util/types.h>

#ifndef NULL
#define NULL 0
#endif

/*ARGSUSED*/
s_token
get_remote_token(addr, type, val, size)
	struct netbuf *addr;	/* address of remote host */
	u_int type;		/* attribute type to map */
	caddr_t val;		/* address of local representation */
	u_int size;		/* size of area in val */
{
	if (val == NULL || size == 0)
		return(0);
	switch(type) {
	case PRIVS_T:
		return (s_token)(*(pvec_t *)val);
	case SENS_T:
		return (s_token)(*(lid_t *)val);
	case INFO_T:
		return (s_token)(*(lid_t *)val);
	case INTEG_T:
		return (0);
	case NCS_T:
		return (0);
	case ACL_T:
		return (0);	/* for now; will need general token svc */
	default:
		return (0);
	}
}

u_int
map_local_token(token, type, resp, size)
	s_token token;	/* the token to map */
	u_int type;	/* attribute type */
	caddr_t resp;	/* ptr to area to write local representation in */
	u_int size;	/* size of response area */
{
	lid_t *tmplid = (lid_t *)resp;
	pvec_t *tpvec = (pvec_t *)resp;

	if (resp == NULL || size == 0)
		return(0);
	switch(type) {
	case PRIVS_T:
		*tpvec = (pvec_t)token;
		return (sizeof(pvec_t));
	case SENS_T:
		*tmplid = (lid_t)token;
		return (sizeof(lid_t));
	case INFO_T:
		*resp = 0;
		return (0);
	case INTEG_T:
		*resp = 0;
		return (0);
	case NCS_T:
		*resp = 0;
		return (0);
	case ACL_T:
		*resp = 0;
		return (0);	/* for now; will need general token svc */
	default:
		*resp = 0;
		return (0);
	}
}
