/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/devmenu.c	1.3"
#endif

#include "media.h"

char	*DevFields[]  = {XtNlabel, XtNdefault};

Widget	DevMenu(item_list, start, max_items, parent, caption, selCB,
		type, fbwid)
	DevItem		*item_list;
	int		start;
	int		max_items;
	Widget		parent;
	char		*caption;
	XtPointer	selCB;
	char		*type;
        Widget          *fbwid;
{
	Widget	w_pop, w_cap, w_desc;
	char	*devline, *attr, *name;
	Boolean	setting;
	int	n = start;

	for (devline=DtamGetDev(type,FIRST);
	     devline;
	     devline=DtamGetDev(type,NEXT)) {
		if (strstr(devline,"display=\"false\"")) {
			FREE(devline);
			continue;
		}
		name = DtamDevAttr(devline, CDEVICE);
		if (access(name,R_OK) != 0) {
			FREE(devline);
			FREE(name);
			continue;
		}
		FREE(name);
		name = DtamDevAlias(devline);
		item_list[n].label = name;
		FREE(devline);
		if (++n == max_items)
			break;
	}
	if (n < max_items)
		item_list[n].label = NULL;
/*
 *	Construct the following composite:  caption1 [v] caption2
 *	with the abbreviated button doing a popup of the items defined
 *	above (i.e., those passed in plus matching device aliases)
 *	The second caption contains a description field that requires
 *	knowledge of the initial items to set; it is returned for that
 *	purpose.  The SelCB will generally do a SetValues on the label
 *	of that widget; to assist that, the widget is passed as UserData
 */
	w_pop = XtCreatePopupShell("popup", popupMenuShellWidgetClass,
			XtParent(parent), NULL, 0);
/**/
	XtSetArg(arg[0], XtNposition,		OL_LEFT);
	XtSetArg(arg[1], XtNlabel,		caption);
	XtSetArg(arg[2], XtNspace,		8);
	w_cap = XtCreateManagedWidget("caption",
			 captionWidgetClass, parent, arg, 3);

	XtSetArg(arg[0], XtNposition,		OL_RIGHT);
	XtSetArg(arg[1], XtNspace,		8);
	w_desc = XtCreateManagedWidget("caption",
			captionWidgetClass, w_cap, arg, 2);
/**/
	XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
	XtSetArg(arg[1], XtNitemFields,         DevFields);
	XtSetArg(arg[2], XtNnumItemFields,      NUM_DevFields);
	XtSetArg(arg[3], XtNitems,              item_list);
	XtSetArg(arg[4], XtNnumItems,           n);
	XtSetArg(arg[5], XtNselectProc,         selCB);
	XtSetArg(arg[6], XtNuserData,		w_desc);
	*fbwid = XtCreateManagedWidget("device",
			flatButtonsWidgetClass, w_pop, arg, 7);

	XtSetArg(arg[0], XtNpopupWidget, w_pop);
	XtCreateManagedWidget("device",
			abbreviatedButtonWidgetClass, w_desc, arg, 1);
	return w_desc;
}
