/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:rxputm.c	1.1.2.3"
#ident  "$Header: rxputm.c 1.3 91/06/28 $"

#include <sys/byteorder.h>
#include <sys/types.h>
#include <rx.h>
#include <stdio.h>
#include <unistd.h>
#include "rxmsg.h"


/* externally defined routines */

extern	void	log();


/* externally defined global variables */

extern	char	Logmsg[];


/*
 *	rxputm()
 */

int
rxputm(netfd, type, len, msg)
int	netfd;
int	type;
long	len;
char	*msg;
{
	struct rx_msg_head	head;
	int	tobesent;
	int	totalsent, sent;

	head.msg_type = htonl(type);
	head.msg_len = htonl((int) len);

	/* write the header */
	tobesent = sizeof(head);
	totalsent = 0;

	while(tobesent > 0) {
		if ((sent = write(netfd, (char *) (&head) + totalsent,
				  (unsigned int) tobesent)) < 0)
			break;
		totalsent += sent;
		tobesent -= sent;
	}

	if (sent < 0)
		return(-1);

	/* write the message */
	tobesent = len;
	totalsent = 0;

	while(tobesent > 0) {
		if ((sent = write(netfd, msg + totalsent,
				  (unsigned int) tobesent)) < 0)
			break;
		totalsent += sent;
		tobesent -= sent;
	}

	if (sent < 0)
		return(-1);

	(void) sprintf(Logmsg, "rxputm: netfd = %d, type = %d, len = %ld, success",
		       netfd, type, len);
	log(Logmsg);

	if (type == RXM_DATA) {
		struct data_msg		*data_msg;

		log("rxputm: RXM_DATA");
		data_msg = (struct data_msg *) msg;
		(void) sprintf(Logmsg, "fd = %ld, len = %ld",
			       ntohl((int) data_msg->fd), ntohl((int) data_msg->len));
		log(Logmsg);
	}

	return(0);
}
