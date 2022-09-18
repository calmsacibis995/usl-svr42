/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wput.c	1.2"
#include "cvttam.h"

int
TAMwputc (wn, c)
short wn;
char c;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    (void)waddch (Scroll(tw), (chtype)c);
    return (OK);
  }
  return (ERR);
}

int
TAMwputs (wn, s)
short wn;
char *s;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    (void)waddstr (Scroll(tw), s);
    return (OK);
  }
  return (ERR);
}
