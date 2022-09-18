/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)libnsl:common/lib/libnsl/rpc/clnt_perror.c	1.3.7.4"
#ident  "$Header: clnt_perror.c 1.2 91/06/26 $"

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
/*
 * clnt_perror.c
 *
 */

#include <stdio.h>
#include <string.h>

#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <sys/tiuser.h>

extern char *t_errlist[];
extern char *malloc();
extern int t_nerr;

extern char *netdir_sperror();

static char *buf;

static char *
_buf()
{

	if (buf == NULL)
		buf = (char *)malloc(256);
	return (buf);
}

static const char *
auth_errmsg(stat)
	enum auth_stat stat;
{
	switch (stat) {
	case AUTH_OK:
		return "Authentication OK";
	case AUTH_BADCRED:
		return "Invalid client credential";
	case AUTH_REJECTEDCRED:
		return "Server rejected credential";
	case AUTH_BADVERF:
		return "Invalid client verifier";
	case AUTH_REJECTEDVERF:
		return "Server rejected verifier";
	case AUTH_TOOWEAK:
		return "Client credential too weak";
	case AUTH_INVALIDRESP:
		return "Invalid server verifier";
	case AUTH_FAILED:
		return "Failed (unspecified error)";
	}
	return "Unknown authentication error";
}

/*
 * Return string reply error info. For use after clnt_call()
 */
char *
clnt_sperror(cl, s)
	CLIENT *cl;
	char *s;
{
	struct rpc_err e;
	void clnt_perrno();
	const char *err;
	char *str = _buf();
	char *strstart = str;

	if (str == NULL)
		return (NULL);
	CLNT_GETERR(cl, &e);

	(void) sprintf(str, "%s: ", s);
	str += strlen(str);

	(void) strcpy(str, clnt_sperrno(e.re_status));
	str += strlen(str);

	switch (e.re_status) {
	case RPC_SUCCESS:
	case RPC_CANTENCODEARGS:
	case RPC_CANTDECODERES:
	case RPC_TIMEDOUT:
	case RPC_PROGUNAVAIL:
	case RPC_PROCUNAVAIL:
	case RPC_CANTDECODEARGS:
	case RPC_SYSTEMERROR:
	case RPC_UNKNOWNHOST:
	case RPC_UNKNOWNPROTO:
	case RPC_UNKNOWNADDR:
	case RPC_NOBROADCAST:
	case RPC_RPCBFAILURE:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
		break;

	case RPC_N2AXLATEFAILURE:
		(void) sprintf(str, "; %s", netdir_sperror());
		break;

	case RPC_TLIERROR:
		(void) sprintf(str, "; %s", t_errlist[e.re_terrno]);
		if (e.re_errno) {
			str += strlen(str);
			(void) sprintf(str, "; %s", strerror(e.re_errno));
		}
		break;

	case RPC_CANTSEND:
	case RPC_CANTRECV:
		if (e.re_errno) {
			(void) sprintf(str, "; errno = %s",
					strerror(e.re_errno));
		}
		if (e.re_terrno) {
			str += strlen(str);
			(void) sprintf(str, "; %s", t_errlist[e.re_terrno]);
		}
		break;

	case RPC_VERSMISMATCH:
		(void) sprintf(str, "; low version = %lu, high version = %lu",
				e.re_vers.low, e.re_vers.high);
		break;

	case RPC_AUTHERROR:
		err = auth_errmsg(e.re_why);
		if (err != NULL) {
			(void) sprintf(str, "; why = %s", err);
		} else {
			(void) sprintf(str,
				"; why = (unknown authentication error - %d)",
				(int) e.re_why);
		}
		break;

	case RPC_PROGVERSMISMATCH:
		(void) sprintf(str, "; low version = %lu, high version = %lu",
				e.re_vers.low, e.re_vers.high);
		break;

	default:	/* unknown */
		(void) sprintf(str, "; s1 = %lu, s2 = %lu",
				e.re_lb.s1, e.re_lb.s2);
		break;
	}
	return strstart;
}

void
clnt_perror(cl, s)
	CLIENT *cl;
	char *s;
{
	(void) fprintf(stderr, "%s\n", clnt_sperror(cl, s));
}

void
clnt_perrno(num)
	enum clnt_stat num;
{
	(void) fprintf(stderr, "%s\n", clnt_sperrno(num));
}

/*
 * Why a client handle could not be created
 */
char *
clnt_spcreateerror(s)
	char *s;
{
	extern int _sys_num_err;
	char *str = _buf();

	if (str == NULL)
		return (NULL);
	(void) sprintf(str, "%s: ", s);
	(void) strcat(str, clnt_sperrno(rpc_createerr.cf_stat));

	switch (rpc_createerr.cf_stat) {
	case RPC_N2AXLATEFAILURE:
		(void) strcat(str, " - ");
		(void) strcat(str, netdir_sperror());
		break;

	case RPC_RPCBFAILURE:
		(void) strcat(str, " - ");
		(void) strcat(str,
			clnt_sperrno(rpc_createerr.cf_error.re_status));
		break;

	case RPC_SYSTEMERROR:
		(void) strcat(str, " - ");
		if ((rpc_createerr.cf_error.re_errno > 0) &&
			(rpc_createerr.cf_error.re_errno < _sys_num_err))
			(void) strcat(str,
			    strerror(rpc_createerr.cf_error.re_errno));
		else
			(void) sprintf(&str[strlen(str)], "Error %d",
			    rpc_createerr.cf_error.re_errno);
		break;

	case RPC_TLIERROR:
		(void) strcat(str, " - ");
		if ((rpc_createerr.cf_error.re_terrno > 0) &&
			(rpc_createerr.cf_error.re_terrno < t_nerr)) {
			(void) strcat(str,
				t_errlist[rpc_createerr.cf_error.re_terrno]);
			if (rpc_createerr.cf_error.re_terrno == TSYSERR) {
				char *err = strerror(rpc_createerr.cf_error.re_errno);
				if (err) {
					strcat(str, " (");
					strcat(str, err);
					strcat(str, ")");
				}
			}
		}
		else
			(void) sprintf(&str[strlen(str)], "TLI Error %d",
					    rpc_createerr.cf_error.re_terrno);
		if (rpc_createerr.cf_error.re_errno > 0) {
			if (rpc_createerr.cf_error.re_errno < _sys_num_err)
				(void) strcat(str,
			    strerror(rpc_createerr.cf_error.re_errno));
			else
				(void) sprintf(&str[strlen(str)], "Error %d",
					    rpc_createerr.cf_error.re_terrno);
		}
		break;
	}
	return (str);
}

void
clnt_pcreateerror(s)
	char *s;
{
	(void) fprintf(stderr, "%s\n", clnt_spcreateerror(s));
}

/*
 * This interface for use by rpc_call() and rpc_broadcast()
 */
const char *
clnt_sperrno(stat)
	const enum clnt_stat stat;
{

	switch (stat) {
	case RPC_SUCCESS:
		return "RPC: Success";
		break;
	case RPC_CANTENCODEARGS:
		return "RPC: Can't encode arguments";
		break;
	case RPC_CANTDECODERES:
		return "RPC: Can't decode result";
		break;
	case RPC_CANTSEND:
		return "RPC: Unable to send";
		break;
	case RPC_CANTRECV:
		return "RPC: Unable to receive";
		break;
	case RPC_TIMEDOUT:
		return "RPC: Timed out";
		break;
	case RPC_VERSMISMATCH:
		return "RPC: Incompatible versions of RPC";
		break;
	case RPC_AUTHERROR:
		return "RPC: Authentication error";
		break;
	case RPC_PROGUNAVAIL:
		return "RPC: Program unavailable";
		break;
	case RPC_PROGVERSMISMATCH:
		return "RPC: Program/version mismatch";
		break;
	case RPC_PROCUNAVAIL:
		return "RPC: Procedure unavailable";
		break;
	case RPC_CANTDECODEARGS:
		return "RPC: Server can't decode arguments";
		break;
	case RPC_SYSTEMERROR:
		return "RPC: Remote system error";
		break;
	case RPC_UNKNOWNHOST:
		return "RPC: Unknown host";
		break;
	case RPC_UNKNOWNPROTO:
		return "RPC: Unknown protocol";
		break;
	case RPC_RPCBFAILURE:
		return "RPC: Rpcbind failure";
		break;
	case RPC_N2AXLATEFAILURE:
		return "RPC: Name to address translation failed";
		break;
	case RPC_NOBROADCAST:
		return "RPC: Broadcast not supported";
		break;
	case RPC_PROGNOTREGISTERED:
		return "RPC: Program not registered";
		break;
	case RPC_UNKNOWNADDR:
		return "RPC: Remote server address unknown";
		break;
	case RPC_TLIERROR:
		return "RPC: Miscellaneous tli error";
		break;
	case RPC_FAILED:
		return "RPC: Failed (unspecified error)";
		break;
	default:
		return "RPC: (unknown error code)";
		break;
	}
	/* NOTREACHED */
}
