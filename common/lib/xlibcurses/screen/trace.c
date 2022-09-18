/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/trace.c	1.8.2.2"
#ident  "$Header: trace.c 1.2 91/06/27 $"
#include "curses_inc.h"

traceon()
{
#ifdef DEBUG
    if (outf == NULL)
    {
	outf = fopen("trace", "a");
	if (outf == NULL)
	{
	    perror("trace");
	    exit(-1);
	}
	fprintf(outf, "trace turned on\n");
    }
#endif /* DEBUG */
    return (OK);
}

traceoff()
{
#ifdef DEBUG
    if (outf != NULL)
    {
	fprintf(outf, "trace turned off\n");
	fclose(outf);
	outf = NULL;
    }
#endif /* DEBUG */
    return (OK);
}

#ifdef DEBUG
#include <ctype.h>

char *
_asciify(str)
register char *str;
{
    static	char	string[1024];
    register	char	*p1 = string;
    register	char	*p2;
    register	char	c;

    while (c = *str++)
    {
	p2 = unctrl(c);
	while (*p1 = *p2++)
	    p1++;
    }
    return string;
}
#endif /* DEBUG */
