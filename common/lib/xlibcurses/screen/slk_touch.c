/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/slk_touch.c	1.3.2.2"
#ident  "$Header: slk_touch.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* Make the labels appeared changed. */

slk_touch()
{
    register	SLK_MAP	*slk;
    register	int	i;

    if (((slk = SP->slk) == NULL) || (slk->_changed == 2))
	return (ERR);

    for (i = 0; i < slk->_num; ++i)
	slk->_lch[i] = TRUE;
    slk->_changed = TRUE;

    return (OK);
}
