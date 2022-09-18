/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/V2.makenew.c	1.5.2.2"
#ident  "$Header: V2.makenew.c 1.2 91/06/26 $"

#include "curses_inc.h"

#ifdef _VR2_COMPAT_CODE
extern WINDOW *_makenew();

WINDOW *
makenew(num_lines, num_cols, begy, begx)
int	num_lines, num_cols, begy, begx;
{
	return _makenew(num_lines, num_cols, begy, begx);
}
#endif
