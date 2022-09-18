/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XwcTextExt.c	1.1"
/*
 * $XConsortium: XwcTextExt.c,v 1.17 91/06/05 13:30:25 rws Exp $
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
_XsiwcTextEscapement(font_set, text, text_len)
    XFontSet        font_set;
    wchar_t        *text;
    int             text_len;
{
#ifdef X_WCHAR

#define DecomposeGlyphCharset(xlocale,wcstr,wc_len,cs_str,cs_bytes,scanned,ctid) \
        _XwcDecomposeGlyphCharset(xlocale, wcstr, wc_len, cs_str, cs_bytes, scanned, ctid)

#include "TextEscBd.c"

#else

    char *mbstr;
    int mb_len;
    mb_len = _Xsiwcstombs(((XsiFontSet)font_set)->display, font_set->core.lcd,
			  text, text_len, False, &mbstr);
    return (*font_set->methods->mb_escapement) (font_set, mbstr, mb_len);

#endif
}

int
_XsiwcTextExtents(font_set, text, text_len,
		  overall_ink_extents, overall_logical_extents)
    XFontSet        font_set;
    wchar_t        *text;
    int             text_len;
    XRectangle      *overall_ink_extents;
    XRectangle      *overall_logical_extents;
{
#ifdef X_WCHAR

#define DecomposeGlyphCharset(xlocale,wcstr,wc_len,cs_str,cs_bytes,scanned,ctid) \
        _XwcDecomposeGlyphCharset(xlocale, wcstr, wc_len, cs_str, cs_bytes, scanned, ctid)

#include "TextExtBd.c"

#else

    char *mbstr;
    int mb_len;
    mb_len = _Xsiwcstombs(((XsiFontSet)font_set)->display, font_set->core.lcd,
			  text, text_len, False, &mbstr);
    return (*font_set->methods->mb_extents) (font_set, mbstr, mb_len,
					     overall_ink_extents,
					     overall_logical_extents);

#endif
}
