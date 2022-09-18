/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wgetpos.c	1.2"
#include "cvttam.h"

int
TAMwgetpos (wn, r, c)
short wn;
int *r, *c;
{
  TAMWIN *tw;
  int lines, cols;

  if (tw = _validwindow (wn)) {
    getyx (Scroll(tw), lines, cols);
    *r = lines;
    *c = cols;
    return (OK);
  }
  *r = -1;
  *c = -1;
  return (ERR);
}
