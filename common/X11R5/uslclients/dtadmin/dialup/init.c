/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/init.c	1.6"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <X11/Shell.h>
#include <RubberTile.h>
#include <Form.h>
#include "uucp.h"
#include "error.h"

extern Widget	InitButtons();
extern Widget	InitLists();
extern void	InitFooter();
extern void	InitDevice();
extern void	InitializeIcon();


void
initialize()
{
	Widget form;
        Widget topButtons;

	/* Create icon window	*/
	InitializeIcon();

	/*SetValue(sf->toplevel, XtNallowShellResize, False);*/
	form = XtVaCreateManagedWidget(
		"form",
		rubberTileWidgetClass,
		sf->toplevel,
		XtNorientation, OL_VERTICAL,
		XtNweight, 	0,
		(String)0
	);
        topButtons = InitButtons (form);
	SetValue(topButtons, XtNgravity, NorthWestGravity);
	SetValue(topButtons, XtNweight,  0);
        InitLists (form);
        InitFooter (form);
	InitDevice ();
} /* initialize */

void
InitializeIcon()
{
	Pixmap		icon,
			iconmask;
	DmGlyphPtr	glyph;

	glyph = DmGetPixmap (SCREEN, "modem48.icon");
	if (glyph) {
		icon = glyph->pix;
		iconmask = glyph->mask;
	} else
		icon = iconmask = (Pixmap) 0;

	XtVaSetValues(sf->toplevel,
		XtNiconPixmap, (XtArgVal) icon,
		XtNiconMask, (XtArgVal) iconmask,
		XtNiconName, (XtArgVal) GGT(string_appName),
		0
	);
}
