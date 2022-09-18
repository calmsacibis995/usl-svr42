/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:pfsl.h	1.3"
#endif

/*
** pfsl.h - This file contains the typedef, function declarations, and macro
** definitions for use with "pfsl.c"
**
** A pfsl is a combination of a scrolling list widget and a general purpose
** linked list.  The linked list must provide a means traversing in both
** directions, so a doubly linked list is used.  This code also counts on
** the fact that a linked list position references the same node throughout
** the lifetime of the list.
** It is possible to convert a scrolling list token into a linked list
** position and vice versa.  This is because each scrolling list item's `tag'
** field is used to hold the corresponding list position, and each linked
** list node contains the corresponding scrolling list token.
*/


#ifndef _OLAM_PFSL_H
#define _OLAM_PFSL_H


#include <Xol/ScrollingL.h>

#include "listd.h"


typedef struct _pfsl_struct
{
  OlListToken	current_token;		/* Scrolling list token for the */
					/* current item */
  List		linked_list;		/* Linked list part */
  Widget	scrolling_list;		/* Scrolling list part */
  /*
  ** Cached scrolling list routines
  */
  OlListToken	(*add)();
  void		(*delete)();
  void		(*touch)();
  void		(*update_view)();
  void		(*view)();
}	*PFScrollList, _PFScrollList;


/*
** Fills `pfsl' with the result of calling `get_string' with `stream' as an
** argument.
** `get_stream' is expected to return NULL on EOF.
** The first item is made the current item and the scrolling list's
** userMakeCurrent callbacks are called with it as an argument.
*/
void PFSLfill();			/* @ */
/*
  PFScrollList pfsl;
  String       (*get_string)();
  FILE         *stream;
*/


/*
** Change the current item's (if any) label to `string'.
*/
void		PFSLchange();
/*
  PFScrollList pfsl;
  String       string;
*/


/*
** Delete the item in `pfsl' corresponding to `token'.
*/
void		PFSLdelete();
/*
  PFScrollList  pfsl;
  OlListToken   token;
*/

/*
** Delete all the items in `pfsl'.
*/
void		PFSLdeleteAll();
/*
  PFScrollList  pfsl;
*/


/*
** Delete the current item in `pfsl' and choose a new one.
** If there is a following item, it is made current, otherwise the preceding
** item is chosed (if it exists).  If neither (the list is now empty) there
** is no longer a current item.  The scrolling list's userMakeCurrent
** callbacks  are then called with the new current item (NULL if none).
*/
void		PFSLdeleteCurrent();
/*
  register PFScrollList pfsl;
*/


/*
** Returns a new `pfsl' with `parent' as the parent for the widgets that are
** created.  The scrolling list is labeled by a caption with `label' as its
** string.  `select_callback' and `view_height' correspond to the scrolling
** list's XtNuserMakeCurrent and XtNviewHeight resources respectively.  The
** scrolling list's view is forced to `view_width' characters in width.
** A fixed font is used so that scrolling list items will line up.  This
** font is cached in a static variable.
** If a new pfsl cannot be created, PFSL_ERROR is returned.
*/
PFScrollList	PFSLnew();
/*
  Widget		parent;
  String		label;
  XtCallbackProc	select_callback;
  Cardinal		view_width;
  Cardinal		view_height;
*/


/*
** Open a new item before or after the current item.
** `where' can be PFSL_BEFORE or PFSL_AFTER.
** The new item is made current.
*/
void		PFSLopen();
/*
  PFScrollList	pfsl;
  int		where;
*/


#define PFSL_NULL_TOKEN	((OlListToken)0)

/*
** Used as `where' argument to PFSLopen()
*/
#define PFSL_AFTER	0
#define PFSL_BEFORE	1

#define PFSL_ERROR	((PFScrollList)0)


#endif	/* _OLAM_PFSL_H */
