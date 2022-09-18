/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/ungetch.c	1.5.2.2"
#ident  "$Header: ungetch.c 1.2 91/06/27 $"
#include "curses_inc.h"

/* Place a char onto the beginning of the input queue. */

ungetch(ch)
int	ch;
{
    register	int	i = cur_term->_chars_on_queue, j = i - 1;
    register	chtype	*inputQ = cur_term->_input_queue;

    /* Place the character at the beg of the Q */

    register chtype	r;

    if(ISCBIT(ch))
	{
	r = RBYTE(ch);
	ch = LBYTE(ch);
	/* do the right half first to maintain the byte order */
	if(r != MBIT && ungetch(r) == ERR)
		return ERR;
	}

    while (i > 0)
	inputQ[i--] = inputQ[j--];
    cur_term->_ungotten++;
    inputQ[0] = -ch;
    cur_term->_chars_on_queue++;
}
