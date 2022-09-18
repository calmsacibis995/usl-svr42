/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:drawutil.c	1.13"

/******************************file*header********************************

    Description:
	This file contains convenience routines for drawing icons.
*/
						/* #includes go here	*/
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Olg.h>

#include "Dtm.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void	DrawIcon(Widget w, OlFIconDrawPtr draw_info,
			 DmViewFormatType type);

					/* public procedures		*/
void		DmDrawLongIcon(Widget, XtPointer, XtPointer);
void		DmDrawNameIcon(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* same as OL_DEFAULT_POINT_SIZE, Unfortunately, it is defined in 
   OpenLookP.h and this is not enough reason to include that file.
   Ideally, XGetFontProperty() on XA_POINT_SIZE should be done.
   FIconBox is too flexible in providing font resource for each sub-item.
   But.., efficiency wins. 
*/
#define	POINT_SIZE	12	

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    DrawIcon-
*/
static void
DrawIcon(Widget	w, OlFIconDrawPtr draw_info, DmViewFormatType type)
{
    DmObjectPtr		op = (DmObjectPtr)draw_info->op;
    int			y_offset;
    DmGlyphPtr		gp;
    OlgAttrs *		attrs;
    Pixel		pixel;
    GC			gc = draw_info->label_gc;
    Display *		dpy = XtDisplay(w);
    Window		win = XtWindow(w);
    unsigned		flags;

    if (draw_info->busy == True)
	flags = RB_DIM;
    else
	flags = 0;

    pixel = (draw_info->focus) ?
	draw_info->focus_color : draw_info->bg_color;
    attrs = OlgCreateAttrs(XtScreen(w), draw_info->fg_color,
			   (OlgBG *)&(pixel), False, POINT_SIZE);
    gp = DmFtypeToFmodeKey(op->ftype)->small_icon;

    /* Use offset to center glyph vertically */
    y_offset = ((int)draw_info->height - (int)(gp->height)) / 2;

    DmDrawIconGlyph(w, gc, gp, attrs, draw_info->x,
		    draw_info->y + y_offset, flags);

    flags |= (draw_info->select) ? RB_SELECTED : RB_NOFRAME;

    /* Draw the label under the icon */
    if (draw_info->label != (String)NULL)
    {
	int		length = strlen(draw_info->label);
	int		i;	/* ignore this value	*/
	XFontStruct *	font;
	Dimension	text_height;
	Dimension	x_offset;

	font = (type == DM_LONG) ?
	       DESKTOP_FIXED_FONT(Desktop) : draw_info->font;
	text_height = OlFontHeight(font, draw_info->font_list);

	/* Use offset to center label vertically */
	y_offset = ((int)draw_info->height - (int)text_height) / 2;

	/* Text offset is to the right of glyph */
	x_offset = gp->width + ICON_LABEL_GAP;

	if (!(draw_info->font_list))
		XSetFont(XtDisplay(w), gc, font->fid);

	if (draw_info->focus) {
		if (draw_info->focus_color == draw_info->fg_color) {
			XSetBackground(dpy, gc, draw_info->fg_color);
			XSetForeground(dpy, gc, draw_info->bg_color);
		}
		else
			XSetBackground(dpy, gc, draw_info->focus_color);
	}
	DmDrawIconLabel(w, gc, draw_info->label,
/*			(type == DM_LONG) ? NULL : draw_info->font_list, */
			draw_info->font_list,
			font, attrs,
			draw_info->x + x_offset,
			draw_info->y + y_offset,
			flags);
	if (draw_info->focus_color == draw_info->fg_color)
		XSetForeground(dpy, gc, draw_info->fg_color);
	if (draw_info->focus)
		XSetBackground(dpy, gc, draw_info->bg_color);

	/* restore original font */
	if (!(draw_info->font_list))
	    XSetFont(XtDisplay(w), gc, draw_info->font->fid);
    }

    if (op->attrs & DM_B_SYMLINK) {
	DmGlyphPtr sgp;		/* shortcut glyph */
	int        x, y, width;

	sgp = DESKTOP_SHORTCUT(Desktop);
	
	y = draw_info->y + gp->height + ICON_PADDING;
	width = (gp->width / sgp->width) * sgp->width;
	x = draw_info->x /* + ((int)draw_info->width - width) / 2 */ ;
	
	XSetStipple(dpy, gc, sgp->pix);
	XSetFillStyle(dpy, gc, FillStippled);
	XSetTSOrigin(dpy, gc, x, y);
	XFillRectangle(dpy, XtWindow(w), gc, x, y, width, sgp->height);
	XSetFillStyle(dpy, gc, FillSolid);
	XSetTSOrigin(dpy, gc, 0, 0);
    }
}				/* end of DrawIcon */

/***************************private*procedures****************************

    Private Procedures
*/
/****************************procedure*header*****************************
    DmDrawLongIcon-
*/
void 
DmDrawLongIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
    DrawIcon(w, (OlFIconDrawPtr)call_data, DM_LONG);
}

/****************************procedure*header*****************************
    DmDrawNameIcon-
*/
void
DmDrawNameIcon(Widget w, XtPointer client_data, XtPointer call_data)
{
    DrawIcon(w, (OlFIconDrawPtr)call_data, DM_NAME);
}


