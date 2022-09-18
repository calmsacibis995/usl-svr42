/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/longname.c	1.6.2.2"
#ident  "$Header: longname.c 1.2 91/06/26 $"

/* This routine returns the long name of the terminal. */

char *
longname()
{
    extern	char	ttytype[], *strrchr();
    register	char	*cp = strrchr(ttytype, '|');

    if (cp)
	return (++cp);
    else
	return (ttytype);
}
