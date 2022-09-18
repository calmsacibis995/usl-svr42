/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libDtI:icon_proc.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include "DtI.h"
#include "FIconBox.h"

void
DmIconSelect1Proc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{  	
	OlFIconBoxButtonCD *data = (OlFIconBoxButtonCD *)call_data;
	DmItemPtr ip = (DmItemPtr)(data->item_data.items);
	Boolean old_selected;
	Boolean selected;
	Boolean managed;
	int i;

	for (i=0; i < data->item_data.num_items; i++, ip++) {
		XtSetArg(Dm__arg[0], XtNset, &old_selected);
		XtSetArg(Dm__arg[1], XtNmanaged, &managed);
		OlFlatGetValues(w, i, Dm__arg, 2);
		if (managed != False) {
			if (i == data->item_data.item_index)
				selected = True;
			else
				selected = False;

			if (selected != old_selected) {
				XtSetArg(Dm__arg[0], XtNset, selected);
				OlFlatSetValues(w, i, Dm__arg, 1);
			}
		}
	}
}

void
DmIconAdjustProc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	OlFIconBoxButtonCD *data = (OlFIconBoxButtonCD *)call_data;
	int select_count;
	int last_select;
	Boolean selected;

	XtSetArg(Dm__arg[0], XtNselectCount, &select_count);
	XtGetValues(w, Dm__arg, 1);
	XtSetArg(Dm__arg[0], XtNset, &selected);
	OlFlatGetValues(w, data->item_data.item_index, Dm__arg, 1);
	if (selected == True) {
		select_count--;
		selected = False;
		last_select = OL_NO_ITEM;
	}
	else {
		select_count++;
		selected = True;
		last_select = data->item_data.item_index;
	}
	XtSetArg(Dm__arg[0], XtNselectCount, select_count);
	XtSetArg(Dm__arg[1], XtNlastSelectItem, last_select);
	XtSetValues(w, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNset, selected);
	OlFlatSetValues(w, data->item_data.item_index, Dm__arg, 1);
}
