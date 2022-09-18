/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:linkedList.c	1.1"
#endif

/*************************************************************************
 *
 * Description:	implements a generic linked list
 *
 *******************************file*header******************************/

						/* #includes go here	*/
#include <malloc.h>
#include <stdio.h>
#include <Xol/linkedList.h>

/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables*****************************/

/*************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public  Procedures
 *
 **************************forward*declarations**************************/

					/* private procedures		*/

					/* public procedures		*/

/*************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures***************************/

/*************************************************************************
 *
 * Public Procedures
 *
 ***************************private*procedures***************************/

/*************************************************************************
 * _OlListDelete - Deletes node at `pos' from `list'
 ***************************function*header*******************************/

void
_OlListDelete(
#ifdef OlNeedFunctionPrototypes
    _OlList list, _OlListPos pos)
#else
	list, pos)
   _OlList	list;
   _OlListPos	pos;
#endif
{
   _OlListPos deleted_node = pos->next;

   pos->next = deleted_node->next;

   if (deleted_node->next != _OL_NULL_LIST_POS) /* There is a following node */
      deleted_node->next->prev = pos;

   /* If we're deleting the last node, the new last node is the previous one. */
   if (deleted_node == list->end)
      list->end = pos;

   free(deleted_node);

} /* _OlListDelete() */


/*************************************************************************
 * _OlListDestroy - Frees all memory associated with `list'
 ***************************function*header*******************************/

void
_OlListDestroy(
#ifdef OlNeedFunctionPrototypes
    _OlList list)
#else
	list)
  _OlList	list;
#endif
{
   register _OlListPos	pos1;
   register _OlListPos	pos2;


   pos1 = list->first;
   while (pos1 != _OL_NULL_LIST_POS)
      {
	 pos2 = pos1->next;
	 free(pos1);
	 pos1 = pos2;
      }

   free(list);

} /* _OlListDestroy() */


/*************************************************************************
 * _OlListFind - Returns position of `w' in `trav_list'. 
 ***************************function*header*******************************/

_OlListPos
_OlListFind(
#ifdef OlNeedFunctionPrototypes
    _OlList list, _OlPointer data)
#else
	list, data)
   _OlList		list;
   _OlPointer		data;
#endif
{
   register _OlListPos	pos;

   if (list == _OL_NULL_LIST || _OL_LIST_IS_EMPTY(list))
      return _OL_NULL_LIST_POS;

   pos = _OL_LIST_FIRST(list);
   while (pos != _OL_LIST_END(list))
      {
	 if (data == _OL_LIST_GET(list, pos))
	     return pos;		/* Found it */
	 pos = _OL_LIST_NEXT(list, pos);
      }

   return _OL_NULL_LIST_POS;		/* Didn't find it */

} /* _OlListFind() */


/*************************************************************************
 * _OlListInsert - Insert `w' into `list' before the node at position `pos'
 *		The position of the new node is returned if there was not an
 *		allocation problem, _OL_NULL_LIST_POS is returned otherwise
 ***************************function*header*******************************/

_OlListPos
_OlListInsert(
#ifdef OlNeedFunctionPrototypes
    _OlList list, _OlListPos pos, _OlPointer data)
#else
	list, pos, data)
   _OlList	list;
   _OlListPos	pos;
   _OlPointer	data;
#endif
{
   _OlListPos	new_node;

  if ((new_node = (_OlListPos)malloc(sizeof(struct _ol_list_node))) !=
      _OL_NULL_LIST_POS)
     {
	new_node->data = data;		/* Store the data */

	/* Arrange the forward pointers */
	new_node->next = pos->next;
	pos->next = new_node;

	/* Arrange the backward pointers */
	new_node->prev = pos;
	if (new_node->next != _OL_NULL_LIST_POS)
	   new_node->next->prev = new_node;

	if (pos == list->end)		/* Move the end pointer if necessary */
	   list->end = new_node;
     }

   return new_node;

} /* _OlListInsert() */


/*************************************************************************
 * _OlListNew - This function returns a new `_OlList'.
 *
 * `_OL_NULL_LIST' is returned if there was an error while trying to
 * create the new list.
 ***************************function*header*******************************/

_OlList
_OlListNew()
{
   _OlList	list;

   if ((list = (_OlList)malloc(sizeof(struct _ol_list))) != _OL_NULL_LIST)
      if ((list->first = (_OlListPos)malloc(sizeof(struct _ol_list_node))) ==
	  _OL_NULL_LIST_POS)
      {
	    free(list);
	    list = _OL_NULL_LIST;
      } else {
	    list->first->next = _OL_NULL_LIST_POS;
	    list->first->prev = _OL_NULL_LIST_POS;
	    list->end = list->first;
      }

   return list;
}	/* _OlListNew() */
