/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_tail.c	1.6"
#endif

#ident	"@(#)list_tail.c	1.1"

/*
** list_tail.c - This file defines ListTail().
*/


#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
** This function returns the last node in `list'.
** `LIST_NULL_POS' is returned if the list is empty.
** If the list doesn't have a tail pointer, we have to traverse.
*/
ListPosition
ListTail(list)
  List	list;
{

#ifdef LIST_DEBUG
  if (list == LIST_NULL)
    {
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_tail,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_tail_msg1,
                     (XrmDatabase)NULL),
                     NULL);

      exit(_LIST_USE_ERROR);
    }
#endif

#ifndef LIST_NO_TAIL
  return list->tail;
#else
  {
    register ListPostition	pos;
    register ListPostition	tail;


    /*
    ** Traverse list with `tail' one position behind `pos'
    ** Why not?: @

    tail = list->head;
    if (tail != LIST_NULL_POS)
      while (tail->next != LIST_NULL_POS)
        tail = tail->next;

    */
    pos = list->head;
    tail = pos;
    if (pos != LIST_NULL_POS)
      pos = pos->next;
    while (pos != LIST_NULL_POS)
      {
	tail = pos;
	pos = pos->next;
      }

    return tail;
  }
#endif

}	/* ListTail() */
