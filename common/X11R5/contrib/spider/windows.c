/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:windows.c	1.1"
/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)windows.c	2.1	90/04/25
 *
 */

/*
 * spider window manipulation
 */

#include	"defs.h"
#include	"globals.h"

#ifdef KITLESS
#include	"spider.bm"

window_init(ac, av, geom)
int	ac;
char	**av;
char	*geom;
{
XSetWindowAttributes	winattr;
long			winmask;
XSizeHints		xsh;
XWMHints		xwmh;
int			mwin_h;
int			x, y;
Pixmap			icon_map;

	x = TABLE_X;
	y = TABLE_Y;
	table_width = TABLE_WIDTH;
	table_height = TABLE_HEIGHT;

	xsh.min_width = TABLE_WIDTH;
	xsh.min_height = TABLE_HEIGHT;

	xsh.flags = PPosition | PSize | PMinSize;

	if (geom)	{
		int	flags;

		flags = XParseGeometry(geom, &x, &y, &table_width,
			&table_height);
		
		/* don't let it start too short */
		if (flags & HeightValue && table_height < TABLE_HEIGHT)
			table_height = TABLE_HEIGHT;

		/* don't let it start too narrow */
		if (flags & WidthValue && table_width < TABLE_WIDTH)
			table_width = TABLE_WIDTH;

		if (flags & (WidthValue | HeightValue))
			xsh.flags |= USSize;

		if (flags & (XValue | YValue))
			xsh.flags |= USPosition;

		if (flags & XValue && flags & XNegative)	{
			x = DisplayWidth(dpy, screen) - (table_width + x);
		}

		if (flags & YValue && flags & YNegative)	{
			y = DisplayHeight(dpy, screen) - (table_height + y);
		}

	}

	winattr.backing_store = WhenMapped;
	winattr.border_pixel = blackpixel;
	winattr.event_mask = KeyPressMask | ExposureMask | ButtonPressMask | 
		ButtonReleaseMask | StructureNotifyMask;
	winmask = CWBorderPixel | CWEventMask | CWBackingStore;

	if (is_color)	{
		winattr.background_pixel = greenpixel;
		winmask |= CWBackPixel;
	} else	{
		winattr.background_pixmap = greenmap;
		winmask |= CWBackPixmap;
	}
 
	table = XCreateWindow(dpy, RootWindow(dpy, screen), 
		x, y, table_width, table_height,
		TABLE_BW, CopyFromParent, CopyFromParent, 
		CopyFromParent, winmask, &winattr);

	xsh.x = x;
	xsh.y = y;

	xsh.width = table_width;
	xsh.height = table_height;

	icon_map = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
		(char *)spider_bits, spider_width, spider_height);

	XSetStandardProperties(dpy, table, "Spider", "Spider", icon_map,
		av, ac, &xsh);
	xwmh.flags = InputHint | IconPixmapHint;
	xwmh.input = True;
	xwmh.icon_pixmap = icon_map;
	XSetWMHints(dpy, table, &xwmh);

	mwin_h = message_font->ascent + message_font->descent;
	message_win = XCreateSimpleWindow(dpy, table, 
		0, table_height - 2 * TABLE_BW - mwin_h,
		table_width - 2 * TABLE_BW, mwin_h,
		TABLE_BW, borderpixel, whitepixel);
	XMapWindow(dpy, message_win);
	XMapWindow(dpy, table);
}
#endif /*KITLESS*/

#ifndef	KITLESS
table_init(win)
Window	win;
{
XSetWindowAttributes    winattr;
long winmask;

	/* save this for later */
	table = win;

        winattr.backing_store = WhenMapped;
	winattr.bit_gravity = ForgetGravity;
	winmask = CWBackingStore | CWBitGravity;
        if (is_color)   {
                winattr.background_pixel = greenpixel;
                winmask |= CWBackPixel;
        } else  {
                winattr.background_pixmap = greenmap;
                winmask |= CWBackPixmap;
        }
        XChangeWindowAttributes(dpy, table, winmask, &winattr);
}
#endif	/*KITLESS*/
