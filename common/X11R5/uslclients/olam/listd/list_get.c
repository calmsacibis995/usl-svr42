/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_get.c	1.5"
#endif

#ident	"@(#)list_get.c	1.4"

/*
** list_get.c - This file defines ListGet().
*/


#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "../errors.h"

#include "list_priv.h"


/*
** This function returns the data at `pos' in `list'.
*/
ListData
ListGet(list, pos)
  List		list;
  ListPosition	pos;
{

#ifdef LIST_DEBUG
  if (pos == LIST_NULL_POS)
    {
      _LIST_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilelist_get,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilelist_del_msg1,
                     (XrmDatabase)NULL),
                     NULL);
      exit(_LIST_USE_ERROR);
    }
#endif

  return pos->data;

}	/* ListGet() */
