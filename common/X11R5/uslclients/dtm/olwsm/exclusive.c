/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/exclusive.c	1.16"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/ChangeBar.h>

#include <misc.h>
#include <list.h>
#include <exclusive.h>

/*
 * Local data:
 */

static String			exc_fields[] = {
	XtNlabel,
	XtNuserData,
	XtNdefault,
	XtNset
};

/*
 * Local functions:
 */

static void		SelectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		UnselectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));

/**
 ** CreateExclusive()
 **/

void
#if	OlNeedFunctionPrototypes
CreateExclusive (
	Widget			parent,
	Exclusive *		exclusive,
	Boolean			track_changes
)
#else
CreateExclusive (parent, exclusive, track_changes)
	Widget			parent;
	Exclusive *		exclusive;
	Boolean			track_changes;
#endif
{
	list_ITERATOR		I;

	ExclusiveItem *		p;


	I = list_iterator(exclusive->items);
	while ((p = (ExclusiveItem *)list_next(&I))) {
		p->is_default = (exclusive->default_item == p);
		p->is_set     = (exclusive->current_item == p);
	}

	if (exclusive->caption)
		parent = CreateCaption(exclusive->name, exclusive->string,
				       parent);

	exclusive->w = XtVaCreateManagedWidget(
		exclusive->name,
		flatButtonsWidgetClass,
		parent,
      		XtNbuttonType,    (XtArgVal)OL_RECT_BTN,
      		XtNexclusives,    (XtArgVal)True,
		XtNselectProc,    (XtArgVal)SelectCB,
		XtNunselectProc,  (XtArgVal)UnselectCB,
		XtNclientData,    (XtArgVal)exclusive,
		XtNitems,         (XtArgVal)exclusive->items->entry,
		XtNnumItems,      (XtArgVal)exclusive->items->count,
		XtNitemFields,    (XtArgVal)exc_fields,
		XtNnumItemFields, (XtArgVal)XtNumber(exc_fields),
		(String)0
	);

	exclusive->track_changes = track_changes;

	return;
} /* CreateExclusive */

/**
 ** SetExclusive()
 **/

void
#if	OlNeedFunctionPrototypes
SetExclusive (
	Exclusive *		exclusive,
	ExclusiveItem *		item,
	OlDefine		change_state
)
#else
SetExclusive (exclusive, item, change_state)
	Exclusive *		exclusive;
	ExclusiveItem *		item;
	OlDefine		change_state;
#endif
{
	if (!item) {
		debug((stderr, "SetExclusive: name = NULL\n"));
		OlVaFlatSetValues (
			exclusive->w,
			list_index(exclusive->items, exclusive->current_item),
			XtNset,	(XtArgVal)FALSE,
			(String)0
		);

	} else if (exclusive->current_item != item) {
		OlVaFlatSetValues (
			exclusive->w,
			list_index(exclusive->items, item),
			XtNset,	(XtArgVal)TRUE,
			(String)0
		);
		if (exclusive->track_changes)
			_OlSetChangeBarState (exclusive->w, change_state, OL_PROPAGATE);

	} else
		if (exclusive->track_changes && change_state == OL_NONE)
			_OlSetChangeBarState (exclusive->w, OL_NONE, OL_PROPAGATE);

	exclusive->current_item = item;
	return;
} /* SetExclusive */

/**
 ** SelectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
SelectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
SelectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlFlatCallData *	fd        = (OlFlatCallData *)call_data;

	Exclusive *		exclusive = (Exclusive *)client_data;


	exclusive->current_item = (ExclusiveItem *)fd->items + fd->item_index;
	if (exclusive->track_changes)
		_OlSetChangeBarState (exclusive->w, OL_NORMAL, OL_PROPAGATE);
	if (exclusive->f)
		(*exclusive->f) (exclusive);

	return;
} /* SelectCB */

/**
 ** UnselectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
UnselectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
UnselectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Exclusive *		exclusive = (Exclusive *)client_data;


	exclusive->current_item = 0;
	if (exclusive->track_changes)
		_OlSetChangeBarState (exclusive->w, OL_NORMAL, OL_PROPAGATE);
	if (exclusive->f)
		(*exclusive->f) (exclusive);

	return;
} /* UnselectCB */
