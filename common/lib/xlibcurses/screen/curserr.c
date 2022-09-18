/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/curserr.c	1.5.2.2"
#ident  "$Header: curserr.c 1.2 91/06/26 $"

#include 	"curses_inc.h"

char	*curs_err_strings[] =
{
    "I don't know how to deal with your \"%s\" terminal",
    "I need to know a more specific terminal type than \"%s\"",	/* unknown */
#ifdef	DEBUG
    "malloc returned NULL in function \"%s\"",
#else	/* DEBUG */
    "malloc returned NULL",
#endif	/* DEBUG */
};

void
curserr()
{
    fprintf(stderr, "Sorry, ");
    fprintf(stderr, curs_err_strings[curs_errno], curs_parm_err);
    fprintf(stderr, ".\r\n");
}
