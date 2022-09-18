/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:drawlnicon.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "Dtm.h"
#include "extern.h"

/*
 *************************************************************************
 * DmDrawLinkIcon - draws an icon visual given an icon glyph and string.
 ****************************procedure*header*****************************
 */
void
DmDrawLinkIcon(w, client_data, call_data)
Widget	w;
XtPointer client_data;
XtPointer call_data;
{
	OlFIconDrawPtr draw_info = (OlFIconDrawPtr)call_data;
	Display *dpy = XtDisplay(w);
	GC gc = draw_info->label_gc;
	DmObjectPtr op = (DmObjectPtr)draw_info->op;
	DmGlyphPtr gp;
	DmGlyphPtr sgp; /* shortcut glyph */
	int x, y, width;

	/* draw the standard icon first */
	DmDrawIcon(w, (XtPointer)NULL, (XtPointer)draw_info);

	if (op->attrs & DM_B_SYMLINK) {
		gp = op->fcp->glyph;
		sgp = DESKTOP_SHORTCUT(Desktop);

		y = draw_info->y + gp->height + ICON_PADDING * 2 +
		    ICON_PADDING / 2;
		width = (gp->width / sgp->width) * sgp->width;
		x = draw_info->x + ((int)draw_info->width - width) / 2;

		XSetStipple(dpy, gc, sgp->pix);
		XSetFillStyle(dpy, gc, FillStippled);
		XSetTSOrigin(dpy, gc, x, y);
		XFillRectangle(dpy, XtWindow(w), gc, x, y, width, sgp->height);
		XSetFillStyle(dpy, gc, FillSolid);
		XSetTSOrigin(dpy, gc, 0, 0);
	}
} /* DmDrawLinkIcon() */

