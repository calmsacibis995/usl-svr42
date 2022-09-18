/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/arefwding.c	1.6.2.2"
#ident "@(#)arefwding.c	2.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	areforwarding - check to see if mail is being forwarded

    SYNOPSIS
	int areforwarding(char *fwrdfile, char *fwrdbuf, unsigned n)

    DESCRIPTION
	areforwarding() looks in the forwarding file for the
	"Forward to " string and returns true if it is
	found. The string fwrdbuf is used as the
	work area. If mail is being forwarded, then
	fwrdbuf will return with the list of users.

    RETURNS
	TRUE	-> forwarding
	FALSE	-> local
*/

int areforwarding(user, fwrdbuf, n)
char *user;
char *fwrdbuf;
unsigned n;
{
    static const char pn[] = "areforwarding";
    static char frwrd[] = "Forward to ";	/* forwarding sentinel */
    FILE *fp;
    int flen = sizeof(frwrd) - 1;
    string *fn = s_copy(mailfwrd);

    Dout(pn, 0, "(%s)\n", user);
    fn = s_append(fn, user);

    if ((fp = fopen(s_to_c(fn), "r")) == NULL)
	{
	Dout(pn, 1, "No forwarding file\n");
	s_free(fn);
	return(FALSE);
	}

    s_free(fn);
    if ((fread(fwrdbuf, flen, 1, fp) == 1) &&
	(strncmp(fwrdbuf, frwrd, flen) == SAME))
	{
	fgets(fwrdbuf, (int)n, fp);
	fclose(fp);
	Dout(pn, 1, "Forwarding to %s\n", fwrdbuf);
	return(TRUE);
	}

    fwrdbuf[0] = '\0';    /* let's be nice */
    fclose(fp);
    Dout(pn, 1, "Empty forwarding file\n");
    return(FALSE);
}
