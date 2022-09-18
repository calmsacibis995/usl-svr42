/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_new.c	1.5"
#endif

#ident	"@(#)list_new.c	1.4"

/*
** list_new.c - This file defines ListNew().
*/


#include <malloc.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
** This function returns a new `List'.
** `LIST_NULL' is returned if there was an error while trying to create the
** new list.
*/
List
ListNew()
{
  register List	list;


  if ((list = (List)malloc(sizeof(struct _list_struct))) == LIST_NULL)
    _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_new,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_new_msg1,
                     (XrmDatabase)NULL),
                     NULL);

  else
    {
      list->head = LIST_NULL_POS;

#ifndef LIST_NO_TAIL
      list->tail = LIST_NULL_POS;
#endif

#ifndef LIST_NO_COUNT
      list->count = (long)0;
#endif
    }

  return list;

}	/* ListNew() */
