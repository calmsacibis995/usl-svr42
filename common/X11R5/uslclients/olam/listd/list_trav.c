/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_trav.c	1.3"
#endif

#ident	"@(#)list_trav.c	1.4"

/*
** list_trav.c - This file defines ListTraverse().
*/


#include "list_priv.h"


/*
** This function traverses `list' starting at `pos' and calls `func' with
** each node's data portion.  If `func' returns a true value, traversal is
** stopped and the position is returned.  If `list' is empty or the list
** is traversed to the end, `LIST_NULL_POS' is returned.
**
** `func' should be declared as
**
**	int func(data)
**	  ListData	data;
*/
ListPosition
ListTraverse(list, pos, func)
  List			list;
  register ListPosition	pos;
  register int		(*func)();
{


  while (pos != LIST_NULL_POS )
    {
      if ((*func)(pos->data) == 0)
	break;
      pos = pos->next;
    }

  return pos;

}	/* ListTraverse() */
