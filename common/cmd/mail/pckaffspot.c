/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pckaffspot.c	1.6.2.3"
#ident "@(#)pckaffspot.c	2.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	pckaffspot - pick the spot to place the Auto-Forward lines

    SYNOPSIS
	int pckaffspot(Hdrinfo *phdrinfo, int check4afwdfrom);

    DESCRIPTION
	If any H_AFWDFROM lines in msg, decide where to put them.
	If check4afwdfrom and no H_AFWDFROM headers exist, return -1.
	Returns :
		 -1 ==> No H_AFWDFROM lines to be printed.
		> 0 ==> Header line type after (before) which to place H_AFWDFROM
			lines and H_AFWDCNT
*/

int pckaffspot(phdrinfo, check4afwdfrom)
Hdrinfo *phdrinfo;
int check4afwdfrom;
{
	static const char pn[] = "pckaffspot";
	register int	rc;	

	if (phdrinfo->hdrs[H_AFWDFROM] == (Hdrs *)NULL && check4afwdfrom) {
		rc = -1;
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
	Dout(pn, 3, "'%s'\n", (rc == -1 ? "No Auto-Forward-From lines" : header[rc].tag));
	return (rc);
}
