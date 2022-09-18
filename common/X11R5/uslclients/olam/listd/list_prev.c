/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_prev.c	1.6"
#endif

#ident	"@(#)list_prev.c	1.1"

/*
** list_prev.c - This function defines ListPrev().
*/


#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
** This function returns the position before `pos' in `list'.
** `LIST_NULL_POS' is returned if `pos' points to the first node in the
** list.
** If the list isn't doubly-linked, we have to traverse.
*/
ListPosition
ListPrev(list, pos)
  List		list;
  ListPosition	pos;
{

#ifdef LIST_DEBUG
  if (pos == LIST_NULL_POS)
    {
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_prev,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_prev_msg1,
                     (XrmDatabase)NULL),
                     NULL);
      exit(_LIST_USE_ERROR);
    }
#endif

#ifdef LIST_DOUBLE
  return pos->prev;
#else
  {
    register ListPosition	prev;


#ifdef LIST_DEBUG
    if (list == LIST_NULL)
      {
        _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_prev,
                     OleTmsg2,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_prev_msg2,
                     (XrmDatabase)NULL),
                     NULL);
	exit(_LIST_USE_ERROR);
      }
#endif

    /*
    ** This code should see if `pos' == `list->head' (see "list_del.c") @
    */

    /*
    ** Traverse until we find the previous node or we reach the end of the
    ** list.
    */
    prev = list->head;
    while (prev != LIST_NULL_POS && prev->next != pos)
      prev = prev->next;

#ifdef LIST_DEBUG  
    if (prev == LIST_NULL_POS)
      {
        _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_prev,
                     OleTmsg3,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_prev_msg3,
                     (XrmDatabase)NULL),
                     NULL);

	exit(_LIST_USE_ERROR);
      }
#endif

    return prev;

  }
#endif	/* LIST_DOUBLE */

}	/* ListPrev() */
