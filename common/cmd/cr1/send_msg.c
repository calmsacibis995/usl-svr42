/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cr1:send_msg.c	1.4.2.3"
#ident  "$Header: send_msg.c 1.2 91/06/25 $"

/*
 *  Send a message to the keymaster and wait for a reply
 */

#include "cr1.h"
#include "keymaster.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stropts.h>
#include <unistd.h>

#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/times.h>

extern int errno;
extern char *scheme;

int
send_msg(char * scheme, Kmessage *kmsg)
{
	int fd;			/*  fd of connection to keymaster  */
	XDR xdrs;		/*  XDR stream for messages to/from keymaster */
	char message[MLEN];	/*  message to/from keymaster  */
	char pipe_name[128];	/*  name of well known keymaster pipe */
	int pos;		/*  position in message buffer  */
	int nchar;		/*  #characters read or written  */
	int request;		/*  remember what we asked for  */
	Kmessage reply;

	request = kmsg->type;

	/*  Open keymaster's well-known STREAMS pipe  */

	sprintf(pipe_name, "%s/%s/%s", DEF_KEYDIR, scheme, DEF_KMPIP);
	if ((fd = open(pipe_name, O_RDWR)) == -1) {
		return(CR_PIPE);
	}

	/*  Create XDR stream for message to keymaster  */

	xdrmem_create(&xdrs, message, sizeof(message), XDR_ENCODE);

	if (!xdr_kmessage(&xdrs, kmsg)) {
		return(CR_MSGOUT);
	}

	pos = xdr_getpos(&xdrs);

	/*  Send message to keymaster  */

	if ((nchar = write(fd, message, (u_int) pos)) != pos) {
		return(CR_MSGOUT);
	}

	/*  Destroy XDR stream for message to keymaster	*/

	xdr_destroy(&xdrs);
	
	/*  Get response from keymaster  */

	nchar = read(fd, message, sizeof(message));
	(void) close(fd);

	if (nchar < 0)
		return(CR_MSGIN);
	else if (nchar == 0) {
		if (request == STOP)
			return(0);
		else
			return(CR_MSGIN);
	}

	/*  Create XDR stream for message from keymaster  */

	xdrmem_create(&xdrs, message, sizeof(message), XDR_DECODE);

	/*  Decode message from keymaster  */

	if (!xdr_kmessage(&xdrs, &reply)) {
		return(CR_MSGIN);
	}
	pos = xdr_getpos(&xdrs);

	/*  Report result from keymaster  */

	switch(reply.type) {

	case SEND_KEY:
		if (request != GET_KEY)
			return(CR_BADREPLY);
		break;
	case CONFIRM:
		break;
	case REJECT:
		return(CR_REJECT);
		/* NOTREACHED */
		break;
	default:
		return(CR_BADREPLY);
		/* NOTREACHED */
		break;
	}

	/*  Terminate  */

	return(0);
}

int
principal_copy(char *to, char *from, int flag)
{
	static char *part1 = NULL;
	char *part2, *logname, *sysname;

	logname = sysname = NULL;
	*to = '\0';

	if (!from && (flag & P_LOCAL)) {
		struct passwd *entry;

		if ( (entry = getpwuid(geteuid())) == NULL )
			return(-1);
		from = entry->pw_name;
	}

	if (strlen(from) > sizeof(Principal))
		return(-1);

	if (part1)
		free(part1);
	part1 = strdup(from);

	if (flag & P_LOCAL)
		logname = part1;
	else
		sysname = part1;

	if ((part2 = strpbrk(part1, "!@")) != NULL) {
	
		/* make sure thare are at most two parts */

		if (strpbrk(part2+1, "!@") != NULL)
			return(-1);

		switch (*part2) {

		case '!':	/* BNU notation */

			*(part2++) = '\0';
			logname = part2;
			sysname = part1;
			break;

		case '@':	/* Internet notation */

			*(part2++) = '\0';
			sysname = part2;
			logname = part1;
			break;

		default:
	
			return(-1);
			/* NOTREACHED */
			break;

		}

	}

	if (logname && (*logname == '\0'))
		logname = NULL;

	if (logname) {	/* make sure it is valid characters */
		char *ptr = logname;
		while (*ptr)
			if (!isprint(*ptr++))
				return(-1);
	}

	if (sysname && (*sysname == '\0'))
		sysname = NULL;
	
	if (sysname) {	/* make sure it is valid characters */
		char *ptr = sysname;
		while (*ptr)
			if (!isprint(*ptr++))
				return(-1);
	}

	if (logname)
		(void) strcpy(to, logname);
	(void) strcat(to, "@");
	if (sysname)
		(void)strcat(to, sysname);

	return(0);

}
