/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/slk_atroff.c	1.1.2.2"
#ident  "$Header: slk_atroff.c 1.2 91/06/26 $"
#include "curses_inc.h"

slk_attroff (a)
chtype a;
{
    WINDOW *win;

    /* currently we change slk attribute only when using software */
    /* slk's.  However, we may introduce a new terminfo variable  */
    /* which would allow manipulating the hardware slk's as well  */

    if ((SP->slk == NULL) || ((win = SP->slk->_win) == NULL))
	return (ERR);

    return (wattroff (win, a));
}
    
