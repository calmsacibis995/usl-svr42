/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/nonexclu.c	1.7"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/FButtons.h>
#include <Xol/ChangeBar.h>

#include <misc.h>
#include <list.h>
#include <nonexclu.h>
#include <xtarg.h>

/*
 * Convenient macros:
 */

#define TouchItems(N) \
	XtVaSetValues((N)->w, XtNitemsTouched, (XtArgVal)True, (String)0)

/*
 * Local data:
 */

static char		_shift [] = "Shift";
static char		_ctrl  [] = "Ctrl";
static char		_mod1  [] = "Alt";

static String		fields[] = {
	XtNlabel,
	XtNuserData,
	XtNdefault,
	XtNset
};

/*
 * Local routines:
 */

static void		NonexclusiveSelectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		NonexclusiveUnselectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));

/**
 ** CreateNonexclusive()
 **/

void
#if	OlNeedFunctionPrototypes
CreateNonexclusive (
	Widget			parent,
	Nonexclusive *		nonexclusive,
	Boolean			track_changes
)
#else
CreateNonexclusive (parent, nonexclusive, track_changes)
	Widget			parent;
	Nonexclusive *		nonexclusive;
	Boolean			track_changes;
#endif
{
	list_ITERATOR		I;

	NonexclusiveItem *	p;


	I = list_iterator(nonexclusive->items);
	while ((p = (NonexclusiveItem *)list_next(&I)))
		p->is_default = (nonexclusive->default_item == p);

	if (nonexclusive->caption)
		parent = CreateCaption(
				nonexclusive->name,
				nonexclusive->string,
				parent
		);

	nonexclusive->w = XtVaCreateManagedWidget(
		nonexclusive->name,
		flatButtonsWidgetClass,
		parent,
		XtNbuttonType,	  (XtArgVal)OL_RECT_BTN,
		XtNexclusives,	  (XtArgVal)False,
		XtNlayoutType,	  (XtArgVal)OL_FIXEDCOLS,
		XtNselectProc,    (XtArgVal)NonexclusiveSelectCB,
		XtNunselectProc,  (XtArgVal)NonexclusiveUnselectCB,
		XtNclientData,    (XtArgVal)nonexclusive,
		XtNitems,         (XtArgVal)nonexclusive->items->entry,
		XtNnumItems,      (XtArgVal)nonexclusive->items->count,
		XtNitemFields,    (XtArgVal)fields,
		XtNnumItemFields, (XtArgVal)XtNumber(fields),
		(String)0
	);

	nonexclusive->track_changes = track_changes;

	return;
} /* CreateNonexclusive */

/**
 ** UnsetAllNonexclusiveItems()
 **/

void
#if	OlNeedFunctionPrototypes
UnsetAllNonexclusiveItems (
	Nonexclusive *		nonexclusive
)
#else
UnsetAllNonexclusiveItems (nonexclusive)
	Nonexclusive *		nonexclusive;
#endif
{
	list_ITERATOR		I;

	NonexclusiveItem *	ni;


	I = list_iterator(nonexclusive->items);
	while ((ni = (NonexclusiveItem *)list_next(&I)))
		ni->is_set = False;
	TouchItems (nonexclusive);

	return;
} /* UnsetAllNonexclusiveItems */

/**
 ** SetNonexclusiveItem()
 **/

void
#if	OlNeedFunctionPrototypes
SetNonexclusiveItem (
	Nonexclusive *		nonexclusive,
	NonexclusiveItem *		item
)
#else
SetNonexclusiveItem (nonexclusive, item)
	Nonexclusive *		nonexclusive;
	NonexclusiveItem *		item;
#endif
{
	if (item) {
		item->is_set = True;
		TouchItems (nonexclusive);
	}

	return;
} /* SetNonexclusiveItem */

/**
 ** SetSavedItems()
 **/

void
#if	OlNeedFunctionPrototypes
SetSavedItems (
	Nonexclusive *		nonexclusive
)
#else
SetSavedItems (nonexclusive)
	Nonexclusive *		nonexclusive;
#endif
{
	list_ITERATOR		I;

	NonexclusiveItem *	ni;


	nonexclusive->modifiers = 0;
	I = list_iterator(nonexclusive->items);
	while ((ni = (NonexclusiveItem *)list_next(&I)))
		if (ni->is_set) {
			if (MATCH((String)ni->addr, _shift))
				nonexclusive->modifiers |= ShiftMask;
			else if (MATCH((String)ni->addr, _ctrl))
				nonexclusive->modifiers |= ControlMask;
			else if (MATCH((String)ni->addr, _mod1))
				nonexclusive->modifiers |= Mod1Mask;
		}

	return;
} /* SetSavedItems */

/**
 ** ReadSavedItems
 **/

void
#if	OlNeedFunctionPrototypes
ReadSavedItems (
	Nonexclusive *		nonexclusive
)
#else
ReadSavedItems (nonexclusive)
	Nonexclusive *		nonexclusive;
#endif
{
	list_ITERATOR		I;

	NonexclusiveItem *	ni;


	/*
	 * For each modifier that is set,
	 * traverse items list and set that item
	 */
	if (nonexclusive->modifiers & ShiftMask) {
		I = list_iterator(nonexclusive->items);
		while ((ni = (NonexclusiveItem *)list_next(&I)))
			if (MATCH((String)ni->addr, _shift)) {
				ni->is_set = True;
				break;
			}
	}
	if (nonexclusive->modifiers & ControlMask) {
		I = list_iterator(nonexclusive->items);
		while ((ni = (NonexclusiveItem *)list_next(&I)))
			if (MATCH((String)ni->addr, _ctrl)) {
				ni->is_set = True;
				break;
			}
	}
	if (nonexclusive->modifiers & Mod1Mask) {
		I = list_iterator(nonexclusive->items);
		while ((ni = (NonexclusiveItem *)list_next(&I)))
			if (MATCH((String)ni->addr, _mod1)) {
				ni->is_set = True;
				break;
			}
	}
	TouchItems (nonexclusive);

	return;
} /* ReadSavedItems */

/**
 ** NonexclusiveSelectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
NonexclusiveSelectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
NonexclusiveSelectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Nonexclusive *		nonexclusive = (Nonexclusive *)client_data;


	if (nonexclusive->track_changes)
		_OlSetChangeBarState (nonexclusive->w, OL_NORMAL, OL_PROPAGATE);
	if (nonexclusive->f)
		(*nonexclusive->f) (nonexclusive);

	return;
} /* NonexclusiveSelectCB */

/**
 ** NonexclusiveUnselectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
NonexclusiveUnselectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
NonexclusiveUnselectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Nonexclusive *		nonexclusive = (Nonexclusive *)client_data;


	if (nonexclusive->track_changes)
		_OlSetChangeBarState (nonexclusive->w, OL_NORMAL, OL_PROPAGATE);
	if (nonexclusive->f)
		(*nonexclusive->f) (nonexclusive);

	return;
} /* NonexclusiveUnselectCB */
