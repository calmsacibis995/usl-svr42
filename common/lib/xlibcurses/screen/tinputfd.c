/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/tinputfd.c	1.1.2.2"
#ident  "$Header: tinputfd.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* Set the input channel for the current terminal. */

void
tinputfd(fd)
int	fd;
{
    cur_term->_inputfd = fd;
    cur_term->_delay = -1;

    /* so that tgetch will reset it to be _inputd */
    /* cur_term->_check_fd = -2; */
}
