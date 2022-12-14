/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/ns_getblock.c	1.3.7.2"
#ident  "$Header: ns_getblock.c 1.2 91/06/26 $"
#include <stdio.h>
#include <tiuser.h>
#include <nsaddr.h>
#include "nserve.h"
#include "nslog.h"

struct nssend *
ns_getblock(send)
struct nssend *send;
{
	struct nssend *rcv;

	/*
	 *	Setup the communication path to the name server.
	 */
	
	LOG2(L_TRACE, "(%5d) enter: ns_getblock\n", Logstamp);
	if (ns_setup() == FAILURE) {
		LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
		return((struct nssend *)NULL);
	}
	
	if (ns_send(send) == FAILURE) {
		ns_close();
		LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
		return((struct nssend *)NULL);
	}

	/*
	 *	Get a return structure and check the return code
	 *	from the name server.
	 */
	
	if ((rcv = ns_rcv()) == (struct nssend *)NULL) {
		ns_close();
		LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
		return((struct nssend *)NULL);
	}

	if ((rcv->ns_code&(~MORE_DATA)) != SUCCESS) {
		ns_close();
		LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
		return((struct nssend *)NULL);
	}

	ns_close();
	LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);

	return(rcv);
}

struct nssend *
ns_getblocks(send)
struct nssend *send;
{
	struct nssend *rcv;

	/*
	 *	Setup the communication path to the name server.
	 */
	
	LOG2(L_TRACE, "(%5d) enter: ns_getblock\n", Logstamp);
	if (send)
	{
		if (ns_setup() == FAILURE) {
			LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
			return((struct nssend *)NULL);
		}
	
		if (ns_send(send) == FAILURE) {
			ns_close();
			LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
			return((struct nssend *)NULL);
		}
	}

	/*
	 *	Get a return structure and check the return code
	 *	from the name server.
	 */
	
	if ((rcv = ns_rcv()) == (struct nssend *)NULL) {
		ns_close();
		LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
		return((struct nssend *)NULL);
	}

	if ((rcv->ns_code&(~MORE_DATA)) != SUCCESS) {
		ns_close();
		LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
		return((struct nssend *)NULL);
	}

	if (!(rcv->ns_code & MORE_DATA))
		ns_close();

	LOG2(L_TRACE, "(%5d) leave: ns_getblock\n", Logstamp);
	return(rcv);
}

