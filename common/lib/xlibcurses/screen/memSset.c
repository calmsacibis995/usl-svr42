/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/memSset.c	1.9.2.3"
#ident  "$Header: memSset.c 1.2 91/06/26 $"
#include "curses.h"
/*
 * additional memory routine to deal with memory areas in units of chtypes.
 */

void memSset (s, c, n)

register chtype *s;
register chtype c;
register int n;
{
    while (n-- > 0)
	*s++ = c;
}
