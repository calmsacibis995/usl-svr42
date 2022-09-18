/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:werase.c	1.1"
#include "cvttam.h"

int
TAMwerase (wn)
short wn;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    (void)werase (Scroll(tw));
    return (OK);
  }
  return (ERR);
}
