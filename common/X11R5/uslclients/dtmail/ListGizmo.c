/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtmail:ListGizmo.c	1.11"
#endif

/*
 * ListGizmo.c
 *
 */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Form.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/PopupGizmo.h>
#include "ListGizmo.h"
#include "mail.h"

static Gizmo     CopyListGizmo(Gizmo gizmo);
static void      FreeListGizmo (Gizmo gizmo);
static Widget    CreateListGizmo (Widget parent, ListGizmo * gizmo);
static void      ManipulateList(ListGizmo * gizmo, ManipulateOption option);
static XtPointer QueryList(ListGizmo * gizmo, int option, char * name);

GizmoClassRec ListGizmoClass[] = {
	"ListGizmo",
	CreateListGizmo, /* Create      */
	CopyListGizmo,   /* Copy        */
	FreeListGizmo,   /* Free        */
	NULL,            /* Map         */
	NULL,            /* GetGiz      */
	NULL,            /* GetMenu     */
	NULL,            /* Build       */
	ManipulateList,  /* Manipulate  */
	QueryList,       /* Query       */
};

static String flatListFields[] = {
	XtNset,
	XtNformatData,
	XtNclientData
};

/*
 * CopyListGizmo
 *
 */

static Gizmo
CopyListGizmo(Gizmo gizmo)
{
	ListGizmo * old = (ListGizmo *)gizmo;
	ListGizmo * new = (ListGizmo *)MALLOC(sizeof(ListGizmo));

	new->help                     = old->help;
	new->name                     = STRDUP (old->name);
	new->format                   = STRDUP (old->format);
	new->exclusive                = old->exclusive;
	new->height                   = old->height;
	new->list		      = old->list;
	new->executeCB                = old->executeCB;
	new->selectCB                 = old->selectCB;
	new->unselectCB               = old->unselectCB;
	new->limitsCB		      = old->limitsCB;
	new->args                     = old->args;
	new->num_args                 = old->num_args;
	new->flatList                 = NULL;
	if (old->font != NULL) {
		new->font             = STRDUP (old->font);
	}
	else {
		new->font             = NULL;
	}

	return (Gizmo)new;

} /* end of CopyListGizmo */

/*
 * FreeListGizmo
 *
 */

static void 
FreeListGizmo(Gizmo gizmo)
{
	ListGizmo * old = (ListGizmo *)gizmo;

	FREE(old->name);
	FREE(old->format);
	if (old->font != NULL) {
		FREE (old->font);
	}
	FREE(old);

} /* end of FreeListGizmo */

/*
 * CreateListGizmo
 *
 * Creates a flat scrolling list.
 */

static Widget
CreateListGizmo(Widget parent, ListGizmo * gizmo)
{
	Arg           arg[100];
	Cardinal      num_arg;
	Widget        scrolledWindow;
	ListHead *    hp;
	XrmValue      to;
	XrmValue      from;
	XFontStruct * large_font = NULL;

	if (gizmo->font != NULL)
	{
		from.addr = (caddr_t)gizmo->font;
		from.size = strlen((char *)from.addr);
		to.addr   = (caddr_t)&large_font;
		to.size   = sizeof(XFontStruct *);
		XtConvertAndStore(parent, XtRString, &from, XtRFontStruct, &to);
	}

	hp = gizmo->list;

	XtSetArg(arg[0], XtNyResizable,    True);
	XtSetArg(arg[1], XtNxResizable,    True);
	XtSetArg(arg[2], XtNxAttachRight,  True);
	XtSetArg(arg[3], XtNyAttachBottom, True);
	XtSetArg(arg[4], XtNyAddHeight,    True);
	scrolledWindow = 
		XtCreateManagedWidget("_X_", scrolledWindowWidgetClass, parent, arg, 5);

#ifdef OL_DESKTOP_SOURCE
	OlRegisterHelp(OL_WIDGET_HELP, scrolledWindow,
		gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);
#endif
	XtSetArg (arg[ 0], XtNitems,         hp->items);
	XtSetArg (arg[ 1], XtNnumItems,      hp->size);
	XtSetArg (arg[ 2], XtNformat,        gizmo->format);
	XtSetArg (arg[ 3], XtNexclusives,    gizmo->exclusive);
	XtSetArg (arg[ 4], XtNitemFields,    flatListFields);
	XtSetArg (arg[ 5], XtNnumItemFields, XtNumber(flatListFields));
	XtSetArg (arg[ 6], XtNselectProc,    gizmo->selectCB);
	XtSetArg (arg[ 7], XtNunselectProc,  gizmo->unselectCB);
	XtSetArg (arg[ 8], XtNdblSelectProc, gizmo->executeCB);
	XtSetArg (arg[ 9], XtNviewHeight,    gizmo->height);
	XtSetArg (arg[10], XtNmaintainView,  True);
	XtSetArg (arg[11], XtNitemsLimitExceeded,  gizmo->limitsCB);
	XtSetArg (arg[12], XtNfont,          large_font);
	num_arg = 
		AppendArgsToList(arg, large_font ? 13 : 12, gizmo->args, gizmo->num_args);
	gizmo->flatList = 
		XtCreateManagedWidget(gizmo->name, flatListWidgetClass, 
			scrolledWindow, arg, num_arg);

	return scrolledWindow;

} /* end of CreateListGizmo */
/*
 * GetListField
 *
 */

extern char *
GetListField(ListGizmo *gizmo, int item)
{
	ListHead *	hp = (ListHead *)gizmo->list;

	return (char *)hp->items[item].fields;

} /* end of GetListField */
/*
 * GetList
 *
 */

static Widget
GetList(ListGizmo *gizmo)
{
	return gizmo->flatList;

} /* end of GetList */
/*
 * UpdateList
 *
 */

void
UpdateList(ListGizmo * gizmo)
{
	ListHead *	hp = (ListHead *)gizmo->list;
	Arg		arg[10];
	extern int	LastSelectedMessage;
	Widget		list;

	list = GetList(gizmo);
	if (list != (Widget)0) {
		XtSetArg(arg[0], XtNnumItems,   	hp->size);
		XtSetArg(arg[1], XtNitems,      	hp->items);
		XtSetArg(arg[2], XtNviewItemIndex,      LastSelectedMessage);
		XtSetValues(list, arg, 2);
	}
	FPRINTF ((stderr, "LastSelectedMessage = %d\n", LastSelectedMessage));

} /* end of UpdateList */

/*
 * ManipulateList
 *
 */

static void
ManipulateList(ListGizmo * gizmo, ManipulateOption option)
{
	int		i;
	ListHead *	lp = gizmo->list;
	Widget		lw = gizmo->flatList;
	Boolean		listChanged = False;

	switch (option)
	{
		case GetGizmoValue: {
			break;
		}
		case ApplyGizmoValue: {
			for (i=0; i<lp->size; i++) {
				/* Current -> Previous */
				lp->items[i].clientData = lp->items[i].set;
			}
			listChanged = True;
			break;
		}
		case ResetGizmoValue: {
			for (i=0; i<lp->size; i++) {
				/* Previous -> Current */
				lp->items[i].set = lp->items[i].clientData;
			}
			listChanged = True;
			break;
		}
		default: {
			break;
		}
	}
	if (listChanged == True) {
		XtVaSetValues (lw, XtNitemsTouched, True, (String)0);
	}
}

/*
 * QueryList
 *
 */

static XtPointer
QueryList(ListGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch(option) {
			case GetGizmoSetting: {
				return (XtPointer)(gizmo->settings);
				break;
			}
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->flatList);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)(gizmo);
				break;
			}
			default: {
				return (XtPointer)(NULL);
				break;
			}
		}
	}
	else {
		return (XtPointer)(NULL);
	}

} /* end of QueryList */
