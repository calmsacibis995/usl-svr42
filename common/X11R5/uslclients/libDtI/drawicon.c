/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)libDtI:drawicon.c	1.8" */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "DtI.h"

/* same as OL_DEFAULT_POINT_SIZE, Unfortunately, it is defined in 
   OpenLookP.h and this is not enough reason to include that file.
   Ideally, XGetFontProperty() on XA_POINT_SIZE should be done.
   FIconBox is too flexible in providing font resource for each sub-item.
   But.., efficiency wins. 
*/
#define	POINT_SIZE	12	

/*
 * This dummy function is used OlgDrawRectButton() so that frame can be drawn.
 */
static void
StringDraw(Screen scr, Drawable win, OlgAttrs *pInfo, int x, int y,
	   unsigned width, unsigned height, XtPointer labeldata)
{
}

/****************************procedure*header*****************************
    DmDrawIconGlyph-
*/
void
DmDrawIconGlyph(Widget w,
		GC gc,
		DmGlyphPtr gp,
		OlgAttrs *attrs,
		int x,
		int y,
		unsigned attr_flags)
{
	Display		*dpy = XtDisplay(w);
	Window		win = XtWindow(w);

	/* Draw frame, if item is selected */
	if (attr_flags & RB_SELECTED) {
		int		frame_x;
		int		frame_y;
		unsigned int	width;
		unsigned int	height;

		frame_x = x - ICON_PADDING;
		frame_y = y - ICON_PADDING;
		width   = gp->width + ICON_PADDING * 2;
		height  = gp->height + ICON_PADDING * 2;
		OlgDrawRectButton(XtScreen(w), win, attrs, frame_x, frame_y,
			  width, height, NULL,
			  (OlgLabelProc)StringDraw, attr_flags);
	}

	XSetClipMask(dpy, gc, gp->mask);
	XSetClipOrigin(dpy,gc, x, y);

	if (gp->depth == 1) {
		XCopyPlane(dpy, gp->pix, win, gc,
			0, 0, (unsigned int)(gp->width),
			(unsigned int)(gp->height),
			x, y, (unsigned long)1);
	}
	else {
		XCopyArea(dpy, gp->pix, win, gc,
			0, 0, (unsigned int)(gp->width),
			(unsigned int)(gp->height),
			x, y);
	}

	if (attr_flags & RB_DIM) {
		XSetStipple(dpy, gc, attrs->pDev->busyStipple);
		XSetFillStyle(dpy, gc, FillStippled);
		XFillRectangle(dpy, win, gc, x, y, gp->width, gp->height);
		XSetFillStyle(dpy, gc, FillSolid);
	}
	XSetClipMask(dpy, gc, None);
}

DmDrawIconLabel(Widget w,
		GC gc,
		char *label,
		OlFontList *font_list,
		XFontStruct *font,
		OlgAttrs *attrs,
		int x,
		int y,
		unsigned int attr_flags)
{
	Display		*dpy = XtDisplay(w);
	Window		win = XtWindow(w);
	int		length = strlen(label);
	Dimension	text_height = OlFontHeight(font, font_list);
	int		frame_x;
	int		frame_y;
	unsigned int	width;
	unsigned int	height;

	frame_x = x - ICON_PADDING;
	frame_y = y - ICON_PADDING;
	width = (font_list?OlTextWidth(font_list,(unsigned char *)label,length):
			    XTextWidth(font, label, length)) +
		ICON_PADDING * 2; /* for 3D Box */
	height  = text_height + ICON_PADDING * 2; /* for 3D box */

	/* now draw the string label and selection Box if necessary */
	OlgDrawRectButton(XtScreen(w), win, attrs, frame_x, frame_y,
			  width, height, label,
			  (OlgLabelProc)StringDraw, attr_flags);

	if (font_list)
		OlDrawString(dpy, win, font_list, gc,
		     x, y + OlFontAscent(font, font_list),
		     (unsigned char *)label, length);
	else
		XDrawString(dpy, win, gc,
			     x, y + OlFontAscent(font, font_list),
			     label, length);
}

/****************************procedure*header*****************************
    DmDrawIcon - draws an icon visual if given an icon glyph and string.
*/
void
DmDrawIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFIconDrawPtr draw_info = (OlFIconDrawPtr)call_data;
	int x_offset;
	DmGlyphPtr gp = ((DmObjectPtr)(draw_info->op))->fcp->glyph;
	OlgAttrs *bg_attrs;
	OlgAttrs *attrs;
	Pixel pixel;
	GC gc = draw_info->label_gc;
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);
	unsigned flags;

	flags = (draw_info->select) ? RB_SELECTED : RB_NOFRAME;
	if (draw_info->busy == True)
		flags |= RB_DIM;

	bg_attrs = OlgCreateAttrs(XtScreen(w), draw_info->fg_color,
				  (OlgBG *)&(draw_info->bg_color), False,
				  POINT_SIZE);
	if (draw_info->focus)
		attrs = OlgCreateAttrs(XtScreen(w), draw_info->fg_color,
				       (OlgBG *)&(draw_info->focus_color),
				       False, POINT_SIZE);
	else
		attrs = bg_attrs;

	/* Use offset to center glyph horizontally */
	x_offset = ((int)draw_info->width - (int)(gp->width)) / 2;

	DmDrawIconGlyph(w, gc, gp, bg_attrs, draw_info->x + x_offset,
			draw_info->y + ICON_PADDING, flags);

	if (draw_info->label != (String)NULL) {
		Dimension	text_width;
		int		y_offset;

		text_width = draw_info->font_list ? 
				OlTextWidth(draw_info->font_list,
					(unsigned char *)(draw_info->label),
					strlen(draw_info->label)) :
				XTextWidth(draw_info->font,
					draw_info->label,
					strlen(draw_info->label));

		/* Use offset to center text horizontally */
		x_offset = ((int)draw_info->width - (int)text_width) / 2;

		/* Text offset is below glyph */
		y_offset = gp->height + ICON_PADDING * 5 + ICON_PADDING / 2;

		if (draw_info->focus) {
			if (draw_info->focus_color == draw_info->fg_color) {
				XSetBackground(dpy, gc, draw_info->fg_color);
				XSetForeground(dpy, gc, draw_info->bg_color);
			}
			else
				XSetBackground(dpy, gc, draw_info->focus_color);
		}
		DmDrawIconLabel(w, gc, draw_info->label,
				draw_info->font_list,
				draw_info->font,
				attrs,
				draw_info->x + x_offset,
				draw_info->y + y_offset,
				flags);
		if (draw_info->focus_color == draw_info->fg_color)
			XSetForeground(dpy, gc, draw_info->fg_color);
		if (draw_info->focus)
			XSetBackground(dpy, gc, draw_info->bg_color);
	}
} /* DmDrawIcon() */

