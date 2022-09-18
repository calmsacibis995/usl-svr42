/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_del.c	1.5"
#endif

#ident	"@(#)list_del.c	1.6"

/*
** list_del.c - This file defines ListDelete().
*/


#include <malloc.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
** This function deletes the node at position `pos' in `list' and returns
** the data that the node held.
*/
ListData
ListDelete(list, pos)
  List			list;
  register ListPosition	pos;
{
  ListData		deleted_data;
  register ListPosition	prev;


#ifdef LIST_DEBUG
  if (list == LIST_NULL)
    {
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_del,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_del_msg1,
                     (XrmDatabase)NULL),
                     NULL);
      exit(_LIST_USE_ERROR);
    }
  else
    if (pos == LIST_NULL_POS)
      {
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_del,
                     OleTmsg2,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_del_msg2,
                     (XrmDatabase)NULL),
                     NULL);
	exit(_LIST_USE_ERROR);
      }
#endif

/*
** Find the pointer to the previous node.  This is easy if we have a
** doubly-linked list; otherwise, we have to traverse.
*/
#ifdef LIST_DOUBLE
  prev = pos->prev;
#else

  if (pos != list->head)
    {
      /*
      ** Traverse until we find the previous node or we reach the end of the
      ** list.
      */
      prev = list->head;
      while (prev != LIST_NULL_POS && prev->next != pos)
	prev = prev->next;

#ifdef LIST_DEBUG  
      /*
      ** We reached the end of the list, so `pos' is not a pointer to any
      ** node in `list'.  We should probably exit here. @
      */
      if (prev == LIST_NULL_POS)
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_del,
                     OleTmsg3,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_del_msg3,
                     (XrmDatabase)NULL),
		     NULL);
#endif
    }
  else
    prev = LIST_NULL_POS;		/* The first node has no previous */
#endif	/* LIST_DOUBLE */

  /*
  ** Save the data for returning
  */
  deleted_data = pos->data;

  /*
  ** Rearrange the forward pointer
  */
  if (prev != LIST_NULL_POS)
    prev->next = pos->next;
  else					/* Deleting the first node */
    list->head = pos->next;

#ifdef LIST_DOUBLE
  /*
  ** Rearrange the backward pointer
  */
  if (pos->next != LIST_NULL_POS)
    pos->next->prev = prev;
#endif

#ifndef LIST_NO_TAIL
  /*
  ** Keep the tail consistent
  */
  if (pos == list->tail)
    list->tail = prev;
#endif

  /*
  ** The count should be decremented here @
  */

  free(pos);

  return deleted_data;

}	/* ListDelete() */
