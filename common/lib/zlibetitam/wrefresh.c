/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wrefresh.c	1.2"
#include "cvttam.h"

int
TAMwrefresh (w)
short w;
{
  TAMWIN *tw;

  if (w == -1) {
    for (tw=LastWin; tw!=CurrentWin; tw=Next(tw)) {
      _noncurrent (tw);
    }
    if (tw) {
      _current (tw);
      _post (tw);
    }
    else {
      (void)wclear (stdscr);
      (void)wnoutrefresh (stdscr);
    }
  }
  else {
    if (tw = _validwindow (w)) {
      if (Border(tw)) {
	(void)touchwin (Border(tw));
	(void)wnoutrefresh (Border(tw));
      }
      (void)touchwin (Scroll(tw));
      (void)wnoutrefresh (Scroll(tw));
    }
    else {
      return (ERR);
    }
  }
  (void)doupdate ();
  return (OK);
}
