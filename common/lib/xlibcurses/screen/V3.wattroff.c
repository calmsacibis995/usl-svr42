/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/V3.wattroff.c	1.1.2.2"
#ident  "$Header: V3.wattroff.c 1.2 91/06/26 $"

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
#undef	wattroff
wattroff(win, attrs)
WINDOW		*win;
_ochtype	attrs;
{
    win->_attrs &= (~_FROM_OCHTYPE(attrs) | win->_bkgd) & A_ATTRIBUTES;
    return (OK);
}
#endif	/* _VR3_COMPAT_CODE */
