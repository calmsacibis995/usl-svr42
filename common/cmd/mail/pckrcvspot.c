/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pckrcvspot.c	1.6.2.3"
#ident "@(#)pckrcvspot.c	2.8 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	pckrcvspot - pick the spot to place the Received: lines

    SYNOPSIS
	int pckrcvspot(Hdrinfo *phdrinfo);

    DESCRIPTION
	If any H_RECEIVED lines in msg, decide where to put them.
	Returns :
		 -1 ==> No H_RECEIVED lines to be printed.
		> 0 ==> Header line type after (before) which to place H_RECEIVED lines
*/

int pckrcvspot(phdrinfo)
Hdrinfo *phdrinfo;
{
	static const char pn[] = "pckrcvspot";
	register int	rc;	

	if (phdrinfo->hdrs[H_RECEIVED] == (Hdrs *)NULL) {
		rc = -1;
	} else if (phdrinfo->orig_rcv) {
		rc = H_RECEIVED;
	} else if (phdrinfo->orig_aff) {
		rc = H_AFWDFROM;
	} else if (phdrinfo->fnuhdrtype == H_RVERS) {
		if (phdrinfo->hdrs[H_EOH] != (Hdrs *)NULL) {
			if (phdrinfo->hdrs[H_DATE] != (Hdrs *)NULL) {
				rc = H_DATE;
			} else {
				rc = H_EOH;
			}
		} else
			rc = H_RVERS;
	} else if ((phdrinfo->fnuhdrtype == H_MVERS) &&
	    (phdrinfo->hdrs[H_EOH] != (Hdrs *)NULL)) {
		rc = H_EOH;
	} else {
		rc = H_CTYPE;
	}
	Dout(pn, 3, "'%s'\n", (rc == -1 ? "No Received: lines": header[rc].tag));
	return (rc);
}
