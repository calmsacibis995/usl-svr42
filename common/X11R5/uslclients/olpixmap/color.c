/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:color.c	1.9"
#endif

#include "pixmap.h"
#include <MenuShell.h>
#include <FButtons.h>
#include <Olg.h>


Widget		ColorMenuShell;

typedef struct {
	XtArgVal	pixel;
} ColorFlats;


static String	color_fields[] = { XtNbackground };

static void	ForegroundSelectCallback();



Widget
SetPalette(button_stack)
Widget button_stack;
{
      	Widget        		color_menu;
	static ColorFlats *	color_entries = NULL;
	static int		ncolors;
	Widget			menu_pane;

	if (!color_entries) {
		int i;

		ncolors = CellsOfScreen(SCREEN);
		if (ncolors > MAX_COLORS)
			ncolors = MAX_COLORS;
		color_entries = (ColorFlats *)
					XtMalloc(ncolors * sizeof(ColorFlats));
		for (i = 0 ; i < ncolors ; i++)
			color_entries[i].pixel = (XtArgVal) i;
	}

	INIT_ARGS();
	SET_ARGS(XtNpushpin, OL_OUT);
	menu_pane = CREATE_POPUP (
		"Palette", popupMenuShellWidgetClass, button_stack
	);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlayoutType, OL_FIXEDCOLS);
	SET_ARGS(XtNbuttonType, OL_RECT_BTN);
	SET_ARGS(XtNexclusives, True);
	SET_ARGS(XtNmeasure, power(2, (PlanesOfScreen(SCREEN) + 1) / 2));
	SET_ARGS(XtNitemMinWidth, OlPointToPixel(OL_HORIZONTAL, 22));
	SET_ARGS(XtNitemMinHeight, OlPointToPixel(OL_VERTICAL, 22));
	SET_ARGS(XtNselectProc, ForegroundSelectCallback);
	SET_ARGS(XtNitems, color_entries);
	SET_ARGS(XtNnumItems, ncolors);
	SET_ARGS(XtNitemFields, color_fields);
	SET_ARGS(XtNnumItemFields, XtNumber(color_fields));
	color_menu = CREATE_MANAGED(
                "_menu_",
		flatButtonsWidgetClass,
		menu_pane
	);
	END_ARGS();

        ColorMenuShell = menu_pane;
	return menu_pane;
}


static void
ForegroundSelectCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
 	OlFlatCallData *	fd = (OlFlatCallData *)call_data;

	CurrentForeground = fd->item_index;
	XSetForeground(DISPLAY, DrawGC, CurrentForeground);
	ResetCursorColors();
}
