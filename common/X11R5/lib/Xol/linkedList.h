/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:linkedList.h	1.1"
#endif

#ifndef _linkedList_h_
#define _linkedList_h_

/*************************************************************************
 *
 * Description:	interface to a generic linked list
 *
 *******************************file*header******************************/

#ifdef __STDC__
typedef void *	_OlPointer;
#else
typedef char *	_OlPointer;
#endif

typedef struct _ol_list_node		/* A list node */
{
    _OlPointer			data;
    struct _ol_list_node *	next;
    struct _ol_list_node *	prev;
} *_OlListPos;

typedef struct _ol_list			/* A list */
{
    struct _ol_list_node	*first;
    struct _ol_list_node	*end;
} *_OlList;


#define _OL_NULL_LIST		((_OlList)0)
#define _OL_NULL_LIST_POS	((_OlListPos)0)

#define _OL_LIST_END(l)		((l)->end)
#define _OL_LIST_FIRST(l)	((l)->first)
#define _OL_LIST_GET(l, p)	((p)->next->data)
#define _OL_LIST_IS_EMPTY(l)	((l)->first->next == _OL_NULL_LIST_POS)
#define _OL_LIST_NEXT(l, p)	((p)->next)
#define _OL_LIST_PREV(l, p)	((p)->prev)

void		_OlListDelete();
void		_OlListDestroy();
_OlListPos	_OlListFind();
_OlListPos	_OlListInsert();
_OlList		_OlListNew();

#endif /* _linkedList_h_ */
