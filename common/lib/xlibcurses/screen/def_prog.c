/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/def_prog.c	1.3.2.2"
#ident  "$Header: def_prog.c 1.2 91/06/26 $"
#include "curses_inc.h"

def_prog_mode()
{
    /* ioctl errors are ignored so pipes work */
#ifdef SYSV
    (void) ioctl(cur_term -> Filedes, TCGETA, &(PROGTTY));
#else
    (void) ioctl(cur_term -> Filedes, TIOCGETP, &(PROGTTY));
#endif
    return (OK);
}
