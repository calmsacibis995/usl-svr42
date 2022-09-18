/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list.h	1.3"
#endif

#ident	"@(#)list.h	1.4"

/*
** list.h - This file contains structure definitions, macro definitions, and
** function declarations for an opaque linked list type.  The data is
** expected to be cast to ListData (which is currently char *).
** Doubly-linked lists are provided by "listd.h" and a library of these
** functions compiled with `LIST_DOUBLE' defined (see below).
** For a description of the operations, see the function declarations below.
**
** Unlike most formal definitions of linked lists, list positions in this
** implementation remain constant.  In other words, the position (say, P)
** returned by a ListInsert() is unaffected by future insertions and
** deletions, unless, of course, P is deleted.  A ListGet() of P always 
** returns the same data.  This does, however, cause deletions to be less
** efficient (mainly in a singly-linked list) since the previous node has
** to be found.
**
** FOR those concerned with the overhead of function calls, macros are
** defined for some functions.  Some of these macros are only defined
** depending on the value of one of the flags described below.
**
**	Function	Macro		Defined If
**	
**	ListCount	LIST_COUNT	LIST_NO_COUNT undefined (default)
**	ListGet		LIST_GET	always defined
**	ListHead	LIST_HEAD	always defined
**	ListNext	LIST_NEXT	always defined
**	ListPrev	LIST_PREV	LIST_DOUBLE defined (not default)
**	ListTail	LIST_TAIL	LIST_NO_TAIL undefined (default)
**
** Through compile-time flags, a programmer has some control over the size
** of `List's and list nodes.  Consequently, some operations are more or
** less efficient depending on their values.  For example, the `List'
** structure contains a member called `count'.  If an application had a need
** for lots of `List's and infrequently called ListCount(), it may be
** beneficial to define `LIST_NO_COUNT' while compiling the library.  Note
** that all operations are valid regardless of these compile time flags, but
** their performance may be affected (ie. ListCount() still works if
** `LIST_NO_COUNT' is defined; it just has to traverse the whole list to
** produce the count.).  All of the flags are undefined by default.  These
** are the flags (macros):
**
** LIST_DOUBLE,	Affected Functions: ListDelete(), ListInsert(), ListPrev()
**	This flag controls whether each node contains a pointer to the
** 	previous node (ie. a doubly-linked list).  It simultaneously makes
**	 ListInsert() slightly less efficient and ListDelete() and ListPrev()
** 	significantly more efficient.
**
** LIST_NO_COUNT, Affected Functions: ListCount(), ListDelete(),
**				      ListInsert(), ListNew()
**	This flag controls whether a count is kept (in each `List') of the
** 	current number of items in the list.
**
** LIST_NO_TAIL, Affected Functions: ListDelete(), ListInsert(), ListNew(),
** 				     ListTail()
**	This flag control whether a pointer to the list's last node is kept
** 	in each `List'.  Like `LIST_DOUBLE', ListDelete(), ListInsert(), and
** 	ListNew() are slightly more efficient while ListTail() is
** 	significantly less efficient.
**
** Another flag, `LIST_DEBUG' controls whether some checks are compiled in
** for checking illegal (usually NULL) arguments to many of the functions.
** This flag is meant to be used to debug applications that are using these
** functions.  It is not intended to debug the list functions themselves.
** This flag will also cause messages to be printed when the functions that
** return error statuses encounter an error (ie. ListInsert() and
** ListNew()).
*/


#ifndef _LIST_H_INCLUDED
#define _LIST_H_INCLUDED


typedef char *	ListData;		/* Data returned by ListDelete() and */
					/* ListGet() and data inserted with */
					/* ListInsert() has this type */

typedef struct _list_node_struct	/* A list node */
{
  ListData			data;
  struct _list_node_struct	*next;
#ifdef LIST_DOUBLE
  struct _list_node_struct	*prev;
#endif
}	*ListPosition;

typedef struct _list_struct		/* A list */
{
  ListPosition	head;
#ifndef LIST_NO_TAIL
  ListPosition	tail;
#endif
#ifndef LIST_NO_COUNT
  long		count;
#endif
}	*List;


#define LIST_NULL		((List)0)
#define LIST_NULL_POS		((ListPosition)0)


/*
** This function returns the number of nodes in `list'.
*/
extern long ListCount();
/*
  List	list;
*/
#ifndef LIST_NO_COUNT
#define LIST_COUNT(list)	((list)->count)
#endif


/*
** This function deletes the node at position `pos' in `list' and returns
** the data that the node held.
*/
extern ListData ListDelete();
/*
  List		list;
  ListPosition	pos;
*/


/*
** This function returns the data at `pos' in `list'.
*/
extern ListData ListGet();
/*
  List		list;
  ListPosition	pos;
*/
#define LIST_GET(list, pos)	((pos)->data)


/*
** This function inserts `data' after position `pos' in `list' and returns
** the position of the new node.  `pos' can be `LIST_NULL_POS' to insert at
** the beginning of the list.
** `LIST_NULL_POS' is returned if there was an error while trying to insert.
*/
extern ListPosition ListInsert();
/*
  List		list;
  ListPosition	pos;
  ListData	data;
*/


/*
** This function returns the first node in `list'.
** `LIST_NULL_POS' is returned if the list is empty.
*/
extern ListPosition ListHead();
/*
  List	list;
*/
#define LIST_HEAD(list)	((list)->head)


/*
** This function returns a new `List'.
** `LIST_NULL' is returned if there was an error while trying to create the
** new list.
*/
extern List ListNew();


/*
** This function returns the position after `pos' in `list'.
** `LIST_NULL_POS' is returned if `pos' points to the last node in the list.
*/
extern ListPosition ListNext();
/*
  List		list;
  ListPosition	pos;
*/
#define LIST_NEXT(list, pos)	((pos)->next)


/*
** This function returns the position before `pos' in `list'.
** `LIST_NULL_POS' is returned if `pos' points to the first node in the
** list.
*/
extern ListPosition ListPrev();
/*
  List		list;
  ListPosition	pos;
*/
#ifdef LIST_DOUBLE
#define LIST_PREV(list, pos)	((pos)->prev)
#endif


/*
** This function returns the last node in `list'.
** `LIST_NULL_POS' is returned if the list is empty.
*/
extern ListPosition ListTail();
/*
  List	list;
*/
#ifndef LIST_NO_TAIL
#define LIST_TAIL(list)	((list)->tail)
#endif


/*
** This function traverses `list' starting at `pos' and calls `func' with
** each node's data portion.  If `func' returns a true value, traversal is
** stopped and the position is returned.  If `list' is empty or the list
** is traversed to the end, `LIST_NULL_POS' is returned.
**
** `func' should be declared as
**
**	int func(data)
**	  ListData	data;
*/
extern ListPosition ListTraverse();
/*
  List		list;
  ListPosition	pos;
  int		(*func)();
*/


#endif	/* _LIST_H_INCLUDED */
