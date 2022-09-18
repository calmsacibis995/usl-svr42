/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_ins.c	1.5"
#endif

#ident	"@(#)list_ins.c	1.4"

/*
** list_ins.c - This file defines ListInsert().
*/


#include <malloc.h>

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
** This function inserts `data' after position `pos' in `list' and returns
** the position of the new node.  `pos' can be `LIST_NULL_POS' to insert at
** the beginning of the list.
** `LIST_NULL_POS' is returned if there was an error while trying to insert.
*/
ListPosition
ListInsert(list, pos, data)
  register List		list;
  register ListPosition	pos;
  ListData		data;
{
  register ListPosition	new_node;


  if ((new_node = (ListPosition)malloc(sizeof(struct _list_node_struct)))
      == LIST_NULL_POS)
    _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_ins,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_ins_msg1,
                     (XrmDatabase)NULL),
                     NULL);

  else
    {
      /*
      ** Save data
      */
      new_node->data = data;

      /*
      ** Rearrange the forward pointer
      */
      if (pos == LIST_NULL_POS)		/* Insert at beginning of list */
	{
	  new_node->next = list->head;
	  list->head = new_node;
	}
      else
	{
	  new_node->next = pos->next;
	  pos->next = new_node;
	}

#ifdef LIST_DOUBLE
      /*
      ** Rearrange the backward pointer
      */
      new_node->prev = pos;
      if (new_node->next != LIST_NULL_POS)
	new_node->next->prev = new_node;
#endif

#ifndef LIST_NO_TAIL
      /*
      ** Keep the tail consistent
      */
      if (pos == list->tail)
	list->tail = new_node;
#endif

#ifndef LIST_NO_COUNT
      ++(list->count);
#endif

    }

  return new_node;

}	/* ListInsert() */
