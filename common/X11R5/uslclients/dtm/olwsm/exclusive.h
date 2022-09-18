/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/exclusive.h	1.9"
#endif

#ifndef _EXCLUSIVE_H
#define _EXCLUSIVE_H

#include <list.h>

typedef struct ExclusiveItem {
	XtArgVal		name;
	XtArgVal		addr;
	XtArgVal		is_default;
	XtArgVal		is_set;
}			ExclusiveItem;

typedef struct Exclusive {
	Boolean			caption;
	String			name;
	String			string;
	ExclusiveItem *		current_item;
	ExclusiveItem *		default_item;
	List *			items;
	void			(*f) OL_ARGS(( struct Exclusive * ));
	ADDR			addr;
	Widget			w;
	Boolean			track_changes;
}			Exclusive;

extern void		CreateExclusive OL_ARGS((
	Widget			parent,
	Exclusive *		exclusive,
	Boolean			track_changes
));
extern void		SetExclusive OL_ARGS((
	Exclusive *		exclusive,
	ExclusiveItem *		item,
	OlDefine		change_state
));

#define EXCLUSIVE(LAB,STR,ITEMS) \
    {									\
	True,								\
	(LAB),								\
	(STR),								\
	(ExclusiveItem *)0,						\
	(ExclusiveItem *)0,						\
	(ITEMS),							\
	(void (*) OL_ARGS(( struct Exclusive * )))0,			\
	(ADDR)0,							\
	(Widget)0,							\
	True								\
    }

#endif
