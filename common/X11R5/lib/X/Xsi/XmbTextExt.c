/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XmbTextExt.c	1.2"
/*
 * $XConsortium: XmbTextExt.c,v 1.15 91/10/08 12:55:12 rws Exp $
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
 *	Authors: Li Yuhong	OMRON Corporation
 *		 Tatsuya Kato	NTT Software Corporation
 *
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include "Xi18nint.h"

int 
_XsimbTextEscapement(font_set, text, text_len)
    XFontSet        font_set;
    char           *text;
    int             text_len;
{
#define DecomposeGlyphCharset(xlocale,mbstr,mb_bytes,cs_str,cs_bytes,scanned,ctid) \
        _XmbDecomposeGlyphCharset(xlocale, (unsigned char *)mbstr, mb_bytes, (unsigned char *)cs_str, cs_bytes, scanned, ctid)

#include "TextEscBd.c"

}

int
_XsimbTextExtents(font_set, text, text_len,
		  overall_ink_extents, overall_logical_extents)
    XFontSet        font_set;
    char           *text;
    int             text_len;
    XRectangle     *overall_ink_extents;
    XRectangle     *overall_logical_extents;
{

#include "TextExtBd.c"

#undef DecomposeGlyphCharset
}

int 
_Xsimb8TextEscapement(font_set, text, text_len)
    XFontSet        font_set;
    char           *text;
    int             text_len;
{
    return XTextWidth(font_set->core.font_struct_list[0], text, text_len);
}

int
_Xsimb8TextExtents(font_set, text, text_len,
		   overall_ink_extents, overall_logical_extents)
    XFontSet        font_set;
    char           *text;
    int             text_len;
    XRectangle     *overall_ink_extents;
    XRectangle     *overall_logical_extents;
{
    int dir, ascent, descent;
    XCharStruct overall;

    XTextExtents(font_set->core.font_struct_list[0], text, text_len,
		 &dir, &ascent, &descent, &overall);

    if (overall_ink_extents) {
	overall_ink_extents->x = overall.lbearing;
	overall_ink_extents->y = -overall.ascent;
	overall_ink_extents->width = overall.rbearing - overall.lbearing;
	overall_ink_extents->height = overall.ascent + overall.descent;
    }

    if (overall_logical_extents) {
	overall_logical_extents->x = 0;
	overall_logical_extents->y = -ascent;
	overall_logical_extents->width = overall.width;
	overall_logical_extents->height = ascent + descent;
    }

    return overall.width;
}
