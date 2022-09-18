/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_next.c	1.5"
#endif

#ident	"@(#)list_next.c	1.4"

/*
** list_next.c - This file defines ListNext().
*/


#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
** This function returns the position after `pos' in `list'.
** `LIST_NULL_POS' is returned if `pos' points to the last node in the list.
*/
ListPosition
ListNext(list, pos)
  List			list;
  register ListPosition	pos;
{

#ifdef LIST_DEBUG
  if (pos == LIST_NULL_POS)
    {
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_next,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_nextw_msg1,
                     (XrmDatabase)NULL),
                     NULL);

      exit(_LIST_USE_ERROR);
    }
#endif

  return pos->next;

}	/* ListNext() */
