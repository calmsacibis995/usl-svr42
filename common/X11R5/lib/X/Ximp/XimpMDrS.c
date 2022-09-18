/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpMDrS.c	1.1"
/* $XConsortium: XimpMDrS.c,v 1.2 91/07/09 17:22:03 rws Exp $ */
/*
 * Copyright 1990, 1991 by TOSHIBA Corp.
 * Copyright 1990, 1991 by SORD Computer Corp.
 *
 *
 * TOSHIBA CORP. AND SORD COMPUTER CORP. DISCLAIM ALL WARRANTIES WITH REGARD
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *				mopi@ome.toshiba.co.jp
 *	   Osamu Touma		SORD Computer Corp.
 */

/******************************************************************

              Copyright 1991, by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
that both that copyright notice and this permission notice appear

IN NO EVENT SHALL FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE

  Author: Takashi Fujiwara     FUJITSU LIMITED 

******************************************************************/
/*
	HISTORY:

	An sample implementation for Xi18n function of X11R5
	based on the public review draft 
	"Xlib Changes for internationalization,Nov.1990"
	by Katsuhisa Yano,TOSHIBA Corp. and Osamu Touma,SORD Computer Corp..

	Modification to the high level pluggable interface is done
	by Takashi Fujiwara,FUJITSU LIMITED.
*/

#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximplc.h"

int
_Ximp_mb_draw_string(dpy, d, xfont_set, gc, x, y, text, text_length)
    Display	*dpy;
    Drawable	d;
    XFontSet	xfont_set;
    GC		gc;
    int		x, y;
    char	*text;
    int		text_length;
{
#define CNV_FUNC	mbstocs

#include "XimpDrStr.c"

    return x;
}

void
_Ximp_mb_draw_image_string(dpy, d, xfont_set, gc, x, y, text, text_length)
    Display	*dpy;
    Drawable	d;
    XFontSet	xfont_set;
    GC		gc;
    int		x, y;
    char	*text;
    int		text_length;
{
    XRectangle		log;
    XGCValues		val;
    XGCValues		old;

    old.function = gc->values.function;
    old.fill_style = gc->values.fill_style;
    old.foreground = gc->values.foreground;

    val.function = GXcopy;
    val.fill_style = FillSolid;
    val.foreground = gc->values.background;

    XChangeGC(dpy, gc, GCFunction | GCFillStyle | GCForeground, &val);

    _Ximp_mb_extents(xfont_set, text, text_length, 0, &log);
    XFillRectangle(dpy, d, gc, x + log.x, y + log.y, log.width, log.height);

    XChangeGC(dpy, gc, GCFunction | GCFillStyle | GCForeground, &old);

    _Ximp_mb_draw_string(dpy, d, xfont_set, gc, x, y, text, text_length);
}
