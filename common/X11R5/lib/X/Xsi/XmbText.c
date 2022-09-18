/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XmbText.c	1.1"
/*
 * $XConsortium: XmbText.c,v 1.13 91/05/02 11:57:32 rws Exp $
 */

/*
 * Copyright 1990, 1991 by OMRON Corporation, NTT Software Corporation,
 *                      and Nippon Telegraph and Telephone Corporation
 * Copyright 1991 by the Massachusetts Institute of Technology
 *
 *
 * OMRON, NTT SOFTWARE, NTT, AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 *
 *	Authors: Li Yuhong		OMRON Corporation
 *		 Tatsuya Kato		NTT Software Corporation
 *		 Hiroshi Kuribayashi	OMRON Corporation
 *   
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include "Xi18nint.h"

/*
 * The bodies of function XmbDrawString and XmbDrawImageString are
 * almost same.
 */
int
_XsimbDrawString(dpy, d, font_set, gc, x, y, text, text_len)
    register Display   *dpy;
    Drawable            d;
    XFontSet            font_set;
    GC                  gc;
    int                 x, y;
    char               *text;
    int                 text_len;
{

#define DecomposeGlyphCharset(xlocale,mbstr,mb_bytes,cs_str,cs_bytes,scanned,ctid) \
        _XmbDecomposeGlyphCharset(xlocale, (unsigned char *)mbstr, mb_bytes, (unsigned char *)cs_str, cs_bytes, scanned, ctid)

#define DrawString(dpy,d,g,x,y,str,len)                           \
        XDrawString(dpy, d, g, x, y, str, len)

#define DrawString16(dpy,d,g,x,y,str,len)                         \
        XDrawString16(dpy, d, g, x, y, str, len)

#include "TextBody.c"

#undef DrawString
#undef DrawString16

    return 0;
}

void
_XsimbDrawImageString(dpy, d, font_set, gc, x, y, text, text_len)
    register Display   *dpy;
    Drawable            d;
    XFontSet            font_set;
    GC                  gc;
    int                 x, y;
    char               *text;
    int                 text_len;
{

#define DrawString(dpy,d,g,x,y,str,len)                           \
        XDrawImageString(dpy, d, g, x, y, str, len)

#define DrawString16(dpy,d,g,x,y,str,len)                         \
        XDrawImageString16(dpy, d, g, x, y, str, len)

#include "TextBody.c"

#undef DecomposeGlyphCharset
#undef DrawString
#undef DrawString16

}

int
_Xsimb8DrawString(dpy, d, font_set, gc, x, y, text, text_len)
    register Display   *dpy;
    Drawable            d;
    XFontSet            font_set;
    GC                  gc;
    int                 x, y;
    char               *text;
    int                 text_len;
{
    XSetFont(dpy, gc, font_set->core.font_struct_list[0]->fid);
    XDrawString(dpy, d, gc, x, y, text, text_len);
    return 0;
}

void
_Xsimb8DrawImageString(dpy, d, font_set, gc, x, y, text, text_len)
    register Display   *dpy;
    Drawable            d;
    XFontSet            font_set;
    GC                  gc;
    int                 x, y;
    char               *text;
    int                 text_len;
{
    XSetFont(dpy, gc, font_set->core.font_struct_list[0]->fid);
    XDrawImageString(dpy, d, gc, x, y, text, text_len);
}
