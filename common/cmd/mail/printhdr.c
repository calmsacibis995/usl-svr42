/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/printhdr.c	1.5.2.2"
#ident "@(#)printhdr.c	2.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	printhdr - Print a header from the structure

    SYNOPSIS
	int	printhdr (int type, int hdrtype, Hdrs *hptr, FILE *fp, int Pflg);

    DESCRIPTION
	printhdr() prints the headers of the given type from the
	hdrlines structure. It returns 0 on success, -1 on failure.
	On failure, sav_errno will hold the value of errno.
*/

int printhdr (type, hdrtype, hptr, fp, Pflg)
int	hdrtype;
Hdrs	*hptr;
FILE	*fp;
int	Pflg;
{
	Hdrs	 	*contptr;

	if (sel_disp(type, hdrtype, header[hdrtype].tag, Pflg) < 0) {
		return (0);
	}

	if (fprintf(fp, "%s %s\n", header[hdrtype].tag, hptr->value) == EOF) {
		sav_errno = errno;
		return(-1);
	}

	/* Print continuation lines, if any... */
	for (contptr = hptr; contptr->cont != (Hdrs *)NULL; ) {
		contptr = contptr->cont;
		if (fprintf(fp, "%s\n", contptr->value) == EOF) {
			sav_errno = errno;
			return(-1);
		}
	}

	if (fflush(fp) == EOF) {
		sav_errno = errno;
		return(-1);
	}

	return (0);
}
