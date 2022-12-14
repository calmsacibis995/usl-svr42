/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/ns_sendpass.c	1.4.8.2"
#ident  "$Header: ns_sendpass.c 1.2 91/06/26 $"
#include <stdio.h>
#include <tiuser.h>
#include <nsaddr.h>
#include "nserve.h"
#include "nslog.h"

ns_sendpass(name, oldpass, newpass)
char	*name, *oldpass, *newpass;
{
	struct nssend send;
	struct nssend *ns_getblock();
	char *malloc();

	LOG2(L_TRACE, "(%5d) enter: ns_sendpass\n", Logstamp);
	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 *	oldpass is not encrypted, newpass is.
	 */

	send.ns_code = NS_SENDPASS;
	send.ns_name = name;
	send.ns_type = 0;
	send.ns_flag = 0;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;
	send.ns_desc = malloc(strlen(oldpass)+strlen(newpass)+3);
	strcpy(send.ns_desc, oldpass);
	strcat(send.ns_desc, ":");
	strcat(send.ns_desc, newpass);

	if (ns_getblock(&send) == (struct nssend *)NULL) {
		LOG2(L_TRACE, "(%5d) leave: ns_sendpass\n", Logstamp);
		return(FAILURE);
	}

	LOG2(L_TRACE, "(%5d) leave: ns_sendpass\n", Logstamp);
	return(SUCCESS);
}
