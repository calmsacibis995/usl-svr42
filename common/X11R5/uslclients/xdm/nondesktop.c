/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:nondesktop.c	1.7"
/*
 *	nondesktop - handle people without graphical preferences from xdm
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <limits.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Xlib.h>
#include <X11/Vendor.h>
#include <X11/RectObj.h>
#include <X11/keysym.h>

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/MenuShell.h>
#include <Xol/PopupWindo.h>
#include <Xol/BaseWindow.h>
#include <Xol/ControlAre.h>
#include <Xol/Flat.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Notice.h>
#include <Xol/OlCursors.h>

#include <libDtI/DtI.h>
#include <Gizmo/Gizmos.h>

#include "dm.h"
#include "nondesktop.h"

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	sensitive;
		  XtArgVal	selCB;
} Items;

#define	GGT	GetGizmoText

#define	N_FIELDS	4

static	char    *Fields[]   = { XtNlabel, XtNmnemonic, XtNsensitive,
			XtNselectProc};

#define	SET_BTN(id,n,name) \
				id[n].label = GGT(label##_##name);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].sensitive = TRUE;\
				id[n].selCB = (XtArgVal)name##CB;

static	Widget	button;
static	Boolean	showDesktop = FALSE;

static void
okCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	if (!(OlFlatCallAcceptFocus (button, 0, CurrentTime)))
		fprintf (stderr, "Could not accept Input Focus\n");
	return;
	}

static void
dexitCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	exit(EXIT_XDM);
	}

static void
dloginCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	exit(OBEYSESS_DISPLAY);
	}

static void
dtuserCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	exit(DTUSER);
	}

/*
static void
xtermCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	exit (USE_XTERM);
	}
*/

static void
dhelpCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	Widget	popup, control, text, but;
	static	Items	Ok[1];

	popup = XtVaCreatePopupShell ("NDHelp", noticeShellWidgetClass,
				w,
				XtNx,		(XtArgVal)20,
				XtNy,		(XtArgVal)20,
				XtNnoticeType,	(XtArgVal)OL_INFORMATION,
				NULL);

	XtVaGetValues (popup, XtNcontrolArea, &control,
			XtNtextArea, &text, NULL);

	if (showDesktop)
		XtVaSetValues (text, XtNalignment, (XtArgVal)OL_LEFT,
			XtNstring,	(XtArgVal)GGT(string_dhelp2),
			NULL);
	else
		XtVaSetValues (text, XtNalignment, (XtArgVal)OL_LEFT,
			XtNstring,	(XtArgVal)GGT(string_dhelp1),
			NULL);

	SET_BTN (Ok, 0, ok);
	but = XtVaCreateManagedWidget("Ok", flatButtonsWidgetClass,
			control,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)Ok,
			XtNnumItems,		(XtArgVal)1,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			NULL);

	XtPopup (popup, XtGrabNone);
	XDefineCursor (XtDisplay(popup), XtWindow(popup),
		GetOlStandardCursor (XtScreen(popup)));

	if (!(OlFlatCallAcceptFocus (but, 0, CurrentTime)))
		{
		fprintf (stderr, "Could not set Input Focus\n");
		}
	return;
	}

void
PopupHelp (w)
Widget	w;
	{
	dhelpCB (w, NULL, NULL);
	return;
	}

static void
WarnUser (Widget toplevel)
	{
	int	numButs = 0;
	Widget	text, control;
	static	Items	buttons[5];

	control = XtVaCreateManagedWidget ("NonDesktop", controlAreaWidgetClass,
				toplevel,
				XtNlayoutType, (XtArgVal)OL_FIXEDCOLS,
				XtNalignment, (XtArgVal)OL_CENTER,
				NULL);

	if (showDesktop)
		{
		text = XtVaCreateManagedWidget ("DesktopMsg", 
			staticTextWidgetClass, control,
			XtNstring, (XtArgVal)GGT(string_nondesktop1),
			XtNalignment, (XtArgVal)OL_CENTER,
			NULL);
		}
	else
		{
		text = XtVaCreateManagedWidget ("DesktopMsg", 
			staticTextWidgetClass, control,
			XtNstring, (XtArgVal)GGT(string_nondesktop),
			XtNalignment, (XtArgVal)OL_CENTER,
			NULL);
		}

/*
	SET_BTN(buttons, numButs, xterm); numButs++;
*/
	if (showDesktop)
		{
		SET_BTN(buttons, numButs, dtuser); numButs++;
		}
	SET_BTN(buttons, numButs, dexit); numButs++;
	SET_BTN(buttons, numButs, dlogin); numButs++;
	SET_BTN(buttons, numButs, dhelp); numButs++;
	button = XtVaCreateManagedWidget("Choices", flatButtonsWidgetClass,
			control,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)buttons,
			XtNnumItems,		(XtArgVal)numButs,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			NULL);

	return;
	}

/*
 *	Create the nondesktop handler
 */
main (int argc, char **argv)
{
	int		xinch, yinch;
	Dimension	center_x, center_y;
	Dimension	theWidth, theHeight;
	Dimension	width, height;
	Display		*theDisplay;
	Screen		*theScreen;
	Pixel		back;
	XtAppContext	app;
	Widget		toplevel, popup;
	XEvent		event;
	int		cnt;
	char		buf[64];
	KeySym		keysym;
	XComposeStatus	compose;
	Dimension	w;


	if (argc >= 2)
		showDesktop = FALSE;
	else
		showDesktop = TRUE;

	OlToolkitInitialize(&argc, argv, NULL);
	toplevel = XtAppInitialize(&app, "Nondesktop", NULL, 0, &argc,
		argv, NULL, NULL, 0);

	theDisplay = XtDisplay(toplevel);
	theScreen = XtScreen(toplevel);

	xinch = OlPointToPixel(OL_HORIZONTAL,72);
	yinch = OlPointToPixel(OL_VERTICAL,72);

	/*
	 *  Place the warning window in the center of the screen
	 *  based on the width and height
	 */
	theWidth = WidthOfScreen(theScreen);
	theHeight = HeightOfScreen(theScreen);

	WarnUser (toplevel);

	XtSetMappedWhenManaged (toplevel, FALSE);
	XtRealizeWidget(toplevel);

	XtVaGetValues (toplevel, XtNwidth, &width, XtNheight, &height, NULL);
	center_x = (Dimension)((Dimension)(theWidth - width) / (Dimension)2);
	center_y = (Dimension)((Dimension)(theHeight - height) / (Dimension)2);

	XtVaSetValues(toplevel, XtNx, (XtArgVal)center_x,
			XtNy, (XtArgVal)center_y, NULL);

	XtSetMappedWhenManaged (toplevel, TRUE);
	XtMapWidget (toplevel);

	/* install the standard cursor */
	XDefineCursor (theDisplay, XtWindow(toplevel),
		GetOlStandardCursor (theScreen));

	if (!(OlFlatCallAcceptFocus (button, 0, CurrentTime)))
		{
		fprintf (stderr, "Could not set Input Focus\n");
		}

	while (TRUE)
		{
		XtAppNextEvent (app, &event);
                if (event.type == KeyPress)
                    {
                    cnt = XLookupString (&event, buf, 64, &keysym, &compose);
                    if (keysym == XK_F1)
                        PopupHelp (toplevel);
                    else
                        XtDispatchEvent (&event);
                    }
                else
                    XtDispatchEvent (&event);
		}
}
