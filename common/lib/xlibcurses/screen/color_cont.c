/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/color_cont.c	1.4.2.2"
#ident  "$Header: color_cont.c 1.2 91/06/26 $"

#include "curses_inc.h"

color_content(color, r, g, b)
short  color;
short  *r, *g, *b;
{
    register _Color *ctp;

    if (color < 0 || color > COLORS || !can_change ||
	(ctp = cur_term->_color_tbl) == (_Color *) NULL)
        return (ERR);

    ctp += color;
    *r = ctp->r;
    *g = ctp->g;
    *b = ctp->b;
    return (OK);
}
