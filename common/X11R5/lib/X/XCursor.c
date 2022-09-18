/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XCursor.c	1.1"
/* $XConsortium: XCursor.c,v 11.14 91/01/06 11:45:02 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1987	*/

/*
*/

#include "Xlibint.h"
static XColor foreground = { 0,    0,     0,     0  };  /* black */
static XColor background = { 0, 65535, 65535, 65535 };  /* white */

Cursor XCreateFontCursor(dpy, which)
	Display *dpy;
	unsigned int which;
{
	/* 
	 * the cursor font contains the shape glyph followed by the mask
	 * glyph; so character position 0 contains a shape, 1 the mask for 0,
	 * 2 a shape, etc.  <X11/cursorfont.h> contains hash define names
	 * for all of these.
	 */

	if (dpy->cursor_font == None) {
	    dpy->cursor_font = XLoadFont (dpy, CURSORFONT);
	    if (dpy->cursor_font == None) return None;
	}

	return XCreateGlyphCursor (dpy, dpy->cursor_font, dpy->cursor_font, 
				   which, which + 1, &foreground, &background);
}

