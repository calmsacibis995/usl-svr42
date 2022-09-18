/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pickFrom.c	1.7.2.2"
#ident "@(#)pickFrom.c	2.7 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	pickFrom - pick out user and system from UNIX From line

    SYNOPSIS
	void pickFrom(char *lineptr, string **fromU, string **fromS)

    DESCRIPTION
	pickFrom scans a line, ASSUMED TO BE of the form
		[>]From <fromU> <date> [remote from <fromS>]
	and fills in the message's fromU and fromS strings appropriately.
*/

void pickFrom (lineptr, fromU, fromS)
register char *lineptr;
string **fromU;
string **fromS;
{
	static char rf[] = "remote from ";
	int rfl = sizeof(rf) - 1;
	int i;

	/* skip past the "From " or ">From " */
	if (*lineptr == '>')
		lineptr++;
	lineptr += 5;

	/* copy the user name */
	if (fromU) {
		s_restart(*fromU);
		for ( ; *lineptr; lineptr++) {
			if (isspace (*lineptr))
				break;
			s_putc(*fromU, *lineptr);
		}
		s_terminate(*fromU);
	}

	if (fromS) {
		/* find "remote from" */
		i = substr(lineptr, rf);

		/* copy the system name */
		s_restart(*fromS);
		if (i > 0) {
			lineptr += i + rfl;
			for ( ; *lineptr; lineptr++) {
				if (isspace (*lineptr))
					break;
				s_putc(*fromS, *lineptr);
			}
		}
		s_terminate(*fromS);
	}
}
