/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/clnt_perr.c	1.4.2.5"
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
 * clnt_perr.c	- a kernel only version of clnt_perror.c
 */
#include <util/types.h>
#include <net/rpc/types.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>

/*
 * This interface for use by clntrpc
 */
const char *
clnt_sperrno(stat)
	const enum clnt_stat stat;
{
	switch (stat) {
	case RPC_SUCCESS: 
		return ("RPC: Success"); 
	case RPC_CANTENCODEARGS: 
		return ("RPC: Can't encode arguments");
	case RPC_CANTDECODERES: 
		return ("RPC: Can't decode result");
	case RPC_CANTSEND: 
		return ("RPC: Unable to send");
	case RPC_CANTRECV: 
		return ("RPC: Unable to receive");
	case RPC_TIMEDOUT: 
		return ("RPC: Timed out");
	case RPC_INTR:
		return ("RPC: Interrupted");
	case RPC_VERSMISMATCH: 
		return ("RPC: Incompatible versions of RPC");
	case RPC_AUTHERROR: 
		return ("RPC: Authentication error");
	case RPC_PROGUNAVAIL: 
		return ("RPC: Program unavailable");
	case RPC_PROGVERSMISMATCH: 
		return ("RPC: Program/version mismatch");
	case RPC_PROCUNAVAIL: 
		return ("RPC: Procedure unavailable");
	case RPC_CANTDECODEARGS: 
		return ("RPC: Server can't decode arguments");
	case RPC_SYSTEMERROR: 
		return ("RPC: Remote system error");
	case RPC_UNKNOWNHOST: 
		return ("RPC: Unknown host");
	case RPC_UNKNOWNPROTO:
		return ("RPC: Unknown protocol");
	case RPC_PMAPFAILURE: 
		return ("RPC: Port mapper failure");
	case RPC_PROGNOTREGISTERED: 
		return ("RPC: Program not registered");
	case RPC_FAILED: 
		return ("RPC: Failed (unspecified error)");
	}
	return ("RPC: (unknown error code)");
}


