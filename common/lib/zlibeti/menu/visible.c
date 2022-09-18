/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/visible.c	1.5"
#include "private.h"

/* Check to see if an item is being displayed on the current page */

int
item_visible (i)
register ITEM *i;
{
  register int bottom;
  register MENU *m;

  if (!i || !Imenu(i)) {
    return FALSE;
  }
  m = Imenu(i);
  if (Posted(m)) {
    bottom = Top(m) + Height(m) - 1;
    if (Y(i) >= Top(m) && Y(i) <= bottom) {
      return (TRUE);
    }
  }
  return (FALSE);
}
