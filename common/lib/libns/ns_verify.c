/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/ns_verify.c	1.4.6.2"
#ident  "$Header: ns_verify.c 1.2 91/06/26 $"
#include <stdio.h>
#include <tiuser.h>
#include <nsaddr.h>
#include <nserve.h>
#include "stdns.h"
#include "nsdb.h"
#include "nslog.h"

char *
ns_verify(name, passwd)
char *name, *passwd;
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	LOG2(L_TRACE, "(%5d) enter: ns_verify\n", Logstamp);
	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	send.ns_code = NS_VERIFY;
	send.ns_type = 0;
	send.ns_flag = 0;
	send.ns_name = name;
	send.ns_desc = passwd;
	send.ns_mach = NULL;
	send.ns_addr = NULL;
	send.ns_path = NULL;

	while ((rtn = ns_getblock(&send)) == (struct nssend *)NULL &&
		ns_errno == R_INREC)
			sleep(1);

	if (rtn == (struct nssend *) NULL) {
		LOG2(L_TRACE, "(%5d) leave: ns_verify\n", Logstamp);
		return((char *)NULL);
	}

	LOG2(L_TRACE, "(%5d) leave: ns_verify\n", Logstamp);
	return(rtn->ns_desc);
}
