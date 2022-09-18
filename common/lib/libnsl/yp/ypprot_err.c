/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libnsl:common/lib/libnsl/yp/ypprot_err.c	1.1.5.2"
#ident  "$Header: ypprot_err.c 1.2 91/06/26 $"

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
static  char sccsid[] = "@(#)ypprot_err.c 1.9 88/02/08 Copyr 1985 Sun Micro";
#endif

#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

/*
 * Maps a yp protocol error code (as defined in
 * yp_prot.h) to a yp client interface error code (as defined in
 * ypclnt.h).
 */
int
ypprot_err(yp_protocol_error)
	unsigned int yp_protocol_error;
{
	int reason;

	switch (yp_protocol_error) {
	case YP_TRUE: 
		reason = 0;
		break;
 	case YP_NOMORE: 
		reason = YPERR_NOMORE;
		break;
 	case YP_NOMAP: 
		reason = YPERR_MAP;
		break;
 	case YP_NODOM: 
		reason = YPERR_DOMAIN;
		break;
 	case YP_NOKEY: 
		reason = YPERR_KEY;
		break;
 	case YP_BADARGS:
		reason = YPERR_BADARGS;
		break;
 	case YP_BADDB:
		reason = YPERR_BADDB;
		break;
 	case YP_VERS:
		reason = YPERR_VERS;
		break;
	default:
		reason = YPERR_YPERR;
		break;
	}
	
  	return(reason);
}
