/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:view.c	1.12"
#endif

#include "pixmap.h"
#include "error.h"


extern void	ZoomIn();
extern void	ZoomOut();
extern Widget	AddMenu();


#define NUL (XtPointer) NULL
static MenuItem view_items[] = {
    {(XtArgVal)NULL, (XtArgVal)ShowPixmap,    (XtArgVal)NUL, 
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'S'},
    {(XtArgVal)NULL, (XtArgVal)0, (XtArgVal)NUL, (XtArgVal)True,
			(XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'Z'}
};
Menu ViewMenu = {
	"showpixmap",
	view_items,
	XtNumber(view_items),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

static MenuItem zoom_items[] = {
    {(XtArgVal)NULL, (XtArgVal)ZoomIn, (XtArgVal)NUL,
		 (XtArgVal)True,(XtArgVal)True,(XtArgVal)NUL, (XtArgVal)'I'},
    {(XtArgVal)NULL, (XtArgVal)ZoomOut, (XtArgVal)NUL,
		(XtArgVal)True,(XtArgVal)True,(XtArgVal)NUL,(XtArgVal) 'O'}
};

static Menu zoom_menu = {
	"zoom",
	zoom_items,
	XtNumber (zoom_items),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};


#define GETMESS(a,b)	 (XtArgVal)OlGetMessage(dsp, NULL, 0, \
		     OleNfixedString, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

#define GETMNEM(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNmnemonic, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)
Widget
SetView(button_stack)
Widget button_stack;
{
	Display *dsp = XtDisplayOfObject(button_stack);

	view_items[0].label =
			GETMESS(OleTshowPixmap,OleMfixedString_showPixmap);
	view_items[0].mnemonic =
			 (XtArgVal)*(GETMNEM(OleTshowPixmap,OleMmnemonic_showPixmap));
	view_items[1].mnemonic = 
				(XtArgVal)*(GETMNEM(OleTzoom, OleMmnemonic_zoom));
	view_items[1].label = 
		GETMESS(OleTzoom,OleMfixedString_zoom);

	zoom_items[0].label = 
		GETMESS(OleTzoomIn,OleMfixedString_zoomIn);
	zoom_items[0].mnemonic = 
				(XtArgVal)*(GETMNEM(OleTzoomIn, OleMmnemonic_zoomIn));
	zoom_items[1].label = 
		GETMESS(OleTzoomOut,OleMfixedString_zoomOut);
	zoom_items[1].mnemonic = 
				(XtArgVal)*(GETMNEM(OleTzoomOut, OleMmnemonic_zoomOut));

	view_items[1].popup = (XtArgVal)AddMenu (button_stack, &zoom_menu);
	return ViewMenu.widget = AddMenu (button_stack, &ViewMenu);
}
