/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/nonexclu.h	1.3"
#endif

#ifndef _NONEXCLU_H
#define _NONEXCLU_H

typedef struct NonexclusiveItem {
	XtArgVal		name;
	XtArgVal		addr;
	XtArgVal		is_default;
	XtArgVal		is_set;
}			NonexclusiveItem;

typedef struct {
	Boolean			caption;
	String			name;
	String			string;
	Modifiers		modifiers;
	NonexclusiveItem *	default_item;
	List *			items;
	void			(*f)();
	ADDR			addr;
	Widget			w;
	String			current_label;
	Boolean			track_changes;
}			Nonexclusive;

extern void		CreateNonexclusive OL_ARGS((
	Widget			parent,
	Nonexclusive *		nonexclusive,
	Boolean			track_changes
));
extern void		UnsetAllNonexclusiveItems OL_ARGS((
	Nonexclusive *		nonexclusive
));
extern void		SetNonexclusiveItem OL_ARGS((
	Nonexclusive *		nonexclusive,
	NonexclusiveItem *		item
));
extern void		SetSavedItems OL_ARGS((
	Nonexclusive *		nonexclusive
));
extern void		ReadSavedItems OL_ARGS((
	Nonexclusive *		nonexclusive
));

#endif
