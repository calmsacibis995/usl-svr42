/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpWTxtPr.c	1.2"
/* $XConsortium: XimpWTxtPr.c,v 1.2 91/10/07 17:50:57 rws Exp $ */
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
	by Katsuhisa Yano,TOSHIBA Corp..

	Modification to the high level pluggable interface is done
	by Takashi Fujiwara,FUJITSU LIMITED.
*/

#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximplc.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>

extern int _Ximp_wcstostring(), _Ximp_wcstoct(), _Ximp_wcstombs();
extern int _Xwcslen();

int
XwcTextListToTextProperty(dpy, list, count, style, text_prop)
    Display *dpy;
    wchar_t **list;
    int count;
    XICCEncodingStyle style;
    XTextProperty *text_prop;
{
    wchar_t **list_ptr = list;
    int i, buf_len = 0;

#define CNV_STR_FUNC	_Ximp_wcstostring
#define CNV_CTEXT_FUNC	_Ximp_wcstoct
#define CNV_TEXT_FUNC	_Ximp_wcstombs
#define STRLEN_FUNC	_Xwcslen

    /* XXX */
    for (i = 0; i < count; i++)
	if (list[i])
	    buf_len += _Xwcslen(list[i]);
    
    buf_len *= 5;
    buf_len = (buf_len / BUFSIZE + 1) * BUFSIZE;
    /* XXX */

#include "XimpTxtPr.c"
}
