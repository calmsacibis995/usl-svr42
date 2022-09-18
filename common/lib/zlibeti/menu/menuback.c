/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/menuback.c	1.4"
#include "private.h"

int
set_menu_back(m, attr)
register MENU *m;
chtype attr;
{
  if (InvalidAttr(attr)) {
    return E_BAD_ARGUMENT;
  }
  if (m) {
    Back(m) = attr;
    if (Posted(m)) {
      _draw (m);		/* Redraw menu */
      _show (m);		/* Redisplay menu */
    }
  }
  else {
    Back(Dfl_Menu) = attr;
  }
  return E_OK;
}

chtype
menu_back (m)
register MENU *m;
{
  return Back(m ? m : Dfl_Menu);
}
