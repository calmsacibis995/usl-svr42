/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_head.c	1.5"
#endif

#ident	"@(#)list_head.c	1.1"

/*
** list_head.c - This file defines ListHead().
*/


#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
**  This function returns the first node in `list'.
** `LIST_NULL_POS' is returned if the list is empty.
*/
ListPosition
ListHead(list)
  List	list;
{

#ifdef LIST_DEBUG
  if (list == LIST_NULL)
    {
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_head,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_head_msg1,
                     (XrmDatabase)NULL),
                     NULL);

      exit(_LIST_USE_ERROR);
    }
#endif

  return list->head;

}	/* ListFirst() */
