/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/dumprcv.c	1.9.2.3"
#ident "@(#)dumprcv.c	2.12 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	dumprcv - dump H_RECEIVED fields

    SYNOPSIS
	void dumprcv(Hdrinfo *phdrinfo, CopyLetFlags type, int htype,
	    int *didrcvlines, int *suppress, FILE *f, int Pflg);

    DESCRIPTION
	Put out H_RECEIVED lines if necessary, or
	suppress their printing from the calling routine.
*/

void dumprcv(phdrinfo, type, htype, didrcvlines, suppress, f, Pflg)
Hdrinfo			*phdrinfo;
register CopyLetFlags	type;
register int		htype;
register int		*didrcvlines;
register int		*suppress;
register FILE		*f;
int			Pflg;
{
	static const char pn[] = "dumprcv";
	int		rcvspot;	/* Place to put H_RECEIVED lines */
	Hdrs		*hptr;

	Dout(pn, 15, "type=%d, htype=%d/%s, *didrcvlines=%d, *suppress=%d\n",
		(int)type, htype, htype >= 0 ? header[htype].tag : "None", *didrcvlines, *suppress);

	rcvspot = pckrcvspot(phdrinfo);
	if (rcvspot == -1) {
		Dout(pn, 15, "\trcvspot==-1\n");
		return;
	}

	if (htype == H_RECEIVED) {
		*suppress = TRUE;
	}

	if (*didrcvlines == TRUE) {
		Dout(pn, 15, "\tdidrcvlines == TRUE\n");
		return;
	}
	if ((htype >= 0) && (rcvspot != htype)) {
		Dout(pn, 15, "\thtype < 0 || rcvspot != htype, *suppress=%d\n", *suppress);
		return;
	}

	*didrcvlines = TRUE;
	for (hptr = phdrinfo->hdrs[H_RECEIVED];
	     hptr != (Hdrs *)NULL;
	     hptr = hptr->next) {
		printhdr(type, H_RECEIVED, hptr, f, Pflg);
	}
	Dout(pn, 15, "\t*didrcvlines=%d, *suppress=%d\n", *didrcvlines, *suppress);
}
