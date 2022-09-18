/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:rxperror.c	1.2.2.3"
#ident  "$Header: rxperror.c 1.3 91/06/28 $"

#include <pfmt.h>
#include <sys/types.h>
#include <rx.h>


/*
 * prxerror() - print an rexec error message
 *
 */


void
rxperror(errno)
long	errno;		/* error number */
{
	switch(errno) {

	case RXE_OK:
		(void) pfmt(stderr, MM_ERROR, ":169:No error\n");
		return;

	case RXE_2MANYRX:
		(void) pfmt(stderr, MM_ERROR,
			    ":170:Too many open client rexec connections\n");
		return;

	case RXE_BADFLAGS:
		(void) pfmt(stderr, MM_ERROR,
			    ":171:Bad options/flags specified\n");
		return;

	case RXE_BADARGS:
		(void) pfmt(stderr, MM_ERROR, ":172:Too many arguments\n");
		return;

	case RXE_BADENV:
		(void) pfmt(stderr, MM_ERROR, ":173:Bad environment specified\n");
		return;

	case RXE_BADMACH:
		(void) pfmt(stderr, MM_ERROR, ":174:Unknown host\n");
		return;

	case RXE_CONNPROB:
		(void) pfmt(stderr, MM_ERROR, ":175:Connection problem\n");
		return;

	case RXE_NORXSERVER:
		(void) pfmt(stderr, MM_ERROR,
			    ":176:Host is not running rxserver\n");
		return;

	case RXE_BADVERSION:
		(void) pfmt(stderr, MM_ERROR, ":177:Unsupported version\n");
		return;

	case RXE_NOSVCFILE:
		(void) pfmt(stderr, MM_ERROR,
			    ":178:Could not open services file\n");
		return;

	case RXE_NOSVC:
		(void) pfmt(stderr, MM_ERROR, ":179:Service not available\n");
		return;

	case RXE_NOTAUTH:
		(void) pfmt(stderr, MM_ERROR,
			    ":180:Not authorized to execute service\n");
		return;

	case RXE_NOPTS:
		(void) pfmt(stderr, MM_ERROR,
			    ":181:No pseudo terminals available\n");
		return;

	case RXE_PIPE:
		(void) pfmt(stderr, MM_ERROR,
			    ":182:Cannot make pipe for stderr\n");
		return;

	case RXE_BADSTART:
		(void) pfmt(stderr, MM_ERROR,
			    ":183:Error in starting server side\n");
		return;

	case RXE_NOSPACE:
		(void) pfmt(stderr, MM_ERROR,
			    ":184:Server side memory allocation problems\n");
		return;

	case RXE_BADCNUM:
		(void) pfmt(stderr, MM_ERROR,
			    ":185:Bad rexec connection number\n");
		return;

	case RXE_AGAIN:
		(void) pfmt(stderr, MM_ERROR,
		     ":186:Write could cause process to block, try later\n");
		return;

	case RXE_BADSIG:
		(void) pfmt(stderr, MM_ERROR, ":187:Bad signal number\n");
		return;

	case RXE_BADSTATE:
		(void) pfmt(stderr, MM_ERROR,
			    ":188:Wrong state for operation/message\n");
		return;

	case RXE_TIRDWR:
		(void) pfmt(stderr, MM_ERROR,
			    ":189:Could not push tirdwr module\n");
		return;

	case RXE_WRITE:
		(void) pfmt(stderr, MM_ERROR,
			    ":190:write() handler failed\n");
		return;

	case RXE_IOCTL:
		(void) pfmt(stderr, MM_ERROR,
			    ":191:ioctl() handler failed\n");
		return;

	case RXE_PROTOCOL:
		(void) pfmt(stderr, MM_ERROR,
			    ":192:Protocol error - unexpected message received\n");
		return;

	case RXE_UNKNOWN:
		(void) pfmt(stderr, MM_ERROR, ":193:Unknown error\n");
		return;

	default:
		(void) pfmt(stderr, MM_ERROR,
			    ":194:No error message for error\n");
		return;
	} /* switch */
}
