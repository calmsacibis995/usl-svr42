/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/dumpaff.c	1.9.2.3"
#ident "@(#)dumpaff.c	2.15 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	dumpaff - dump Auto-Forward-From/-Count fields

    SYNOPSIS
	void dumpaff(CopyLetFlags type, CopyLetFlags htype,
	    int *didafflines, int *suppress, FILE *f, int Pflg)

    DESCRIPTION
	Put out H_AFWDFROM and H_AFWDCNT lines if necessary, or
	suppress their printing from the calling routine.
*/

void dumpaff(phdrinfo, type, htype, didafflines, suppress, f, Pflg)
Hdrinfo			*phdrinfo;
register CopyLetFlags	type;
register int		htype;
register int		*didafflines;
register int		*suppress;
register FILE		*f;
int			Pflg;
{
	static const char pn[] = "dumpaff";
	int		affspot;	/* Place to put H_AFWDFROM lines */
	Hdrs		*hptr;

	Dout(pn, 15, "type=%d, htype=%d/%s, *didafflines=%d, *suppress=%d\n",
		(int)type, htype, htype >= 0 ? header[htype].tag : "None", *didafflines, *suppress);

	affspot = pckaffspot(phdrinfo, 1);
	if (affspot == -1) {
		Dout(pn, 15, "\taffspot==-1\n");
		return;
	}

	switch (htype) {
	case H_AFWDCNT:
		*suppress = TRUE;
		Dout(pn, 15, "\tAuto-Forward-Count found\n");
		return;
	case H_AFWDFROM:
		*suppress = TRUE;
		break;
	}

	if (*didafflines == TRUE) {
		Dout(pn, 15, "\tdidafflines == TRUE\n");
		return;
	}

	if ((htype >= 0) && (affspot != htype)) {
		Dout(pn, 15, "\thtype < 0 || affspot != htype, *suppress=%d\n", *suppress);
		return;
	}

	*didafflines = TRUE;
	for (hptr = phdrinfo->hdrs[H_AFWDFROM];
	     hptr != (Hdrs *)NULL;
	     hptr = hptr->next) {
		printhdr(type, H_AFWDFROM, hptr, f, Pflg);
	}
	fprintf(f,"%s %d\n", header[H_AFWDCNT].tag, phdrinfo->affcnt);
	Dout(pn, 15, "\t*didafflines=%d, *suppress=%d\n", *didafflines, *suppress);
}
