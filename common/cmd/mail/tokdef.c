/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/tokdef.c	1.4.2.2"
#ident "@(#)tokdef.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	tokdef - check token to see if it has already been defined

    SYNOPSIS
	string *tokdef(string *fld, string *tok, char *name)

    DESCRIPTION
	tokdef() looks at a field to see if it has already been
	given a definition. If so, it reports the error. It also
	does a quick semantic check for an '='.

    RETURNS
	The token is returned.
*/

string *tokdef(fld, tok, name)
string *fld, *tok;
char *name;
{
    char *pn = "tokdef";

    Dout(pn, 0, "Looking at field '%s'\n", s_to_c(tok));

    if (fld)
	{
	Tout(pn, "Field %s= redefined, was '%s', now '%s'\n", name, s_to_c(fld), s_to_c(tok));
	s_free(fld);
	}

    if (s_ptr_to_c(tok)[1] != '=')
	Tout(pn, "Field %s does not have '=' sign. The first character will be lost!\n", name);

    return tok;
}
