/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/menuserptr.c	1.1"
#include "private.h"

int
set_menu_userptr(m, c)
register MENU *m;
char *c;
{
  if (m) {
    Muserptr(m) = c;
  }
  else {
    Muserptr(Dfl_Menu) = c;
  }
  return E_OK;
}

char *
menu_userptr(m)
register MENU *m;
{
  return Muserptr(m ? m : Dfl_Menu);
}
