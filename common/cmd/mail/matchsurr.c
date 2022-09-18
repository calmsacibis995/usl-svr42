/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/matchsurr.c	1.3.2.2"
#ident "@(#)matchsurr.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	matchsurr - check for match of name against surrogate pattern

    SYNOPSIS
	int matchsurr(string *name, char *pattern,
	    char **lbraslist, char **lbraelist, int nbrak)

    DESCRIPTION
	matchsurr() will check a name against the pattern and fill in
	the bracket lists appropriately (if they are non-null).

    RETURNS
	0 - match failed
	1 - match succeeded
*/

int matchsurr(s_name, pattern, lbraslist, lbraelist, nbrak)
string	*s_name;
re_re	*pattern;
char	**lbraslist;
char	**lbraelist;
int	nbrak;
{
    register int i;
    char *name = s_to_c(s_name);
    char *match[10][2];

    if (!re_reexec(pattern, name, name + strlen(name), match))
	return 0;

    if (lbraslist && lbraelist)
	for (i = 0; i < nbrak; i++)
	    {
	    lbraslist[i] = name + (match[i+RE_OFFSET][0] - name);
	    lbraelist[i] = name + (match[i+RE_OFFSET][1] - name);
	    }

    return 1;
}
