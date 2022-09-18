/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpWPrTxt.c	1.2"
/* $XConsortium: XimpWPrTxt.c,v 1.2 91/10/07 17:50:54 rws Exp $ */
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

extern int _Ximp_mbstowcs(), _Ximp_cttowcs();
extern wchar_t *_Xwcscpy();

int
XwcTextPropertyToTextList(dpy, text_prop, list_ret, count_ret)
    Display *dpy;
    XTextProperty *text_prop;
    wchar_t ***list_ret;
    int *count_ret;
{
    wchar_t **list, *wstr_ptr;
    wchar_t *buf, *buf_ptr;
    unsigned char *str_ptr;
    int i, count, unconv_num, tmp_len, buf_len;
#define CNV_STR_FUNC	_Ximp_mbstowcs
#define CNV_CTEXT_FUNC	_Ximp_cttowcs
#define CNV_TEXT_FUNC	_Ximp_mbstowcs

    /* XXX */
    buf_len = text_prop->nitems + 1;
    buf_len = (buf_len / BUFSIZE + 1) * BUFSIZE;
    if ((buf = (wchar_t *) Xmalloc(buf_len * sizeof(wchar_t))) == NULL)
	return XNoMemory;
    /* XXX */

#include "XimpPrTxt.c"

    if ((list = (wchar_t **) Xmalloc(count * sizeof(wchar_t *))) == NULL)
	goto no_mem;
    wstr_ptr = (wchar_t *) Xmalloc((buf_ptr - buf) * sizeof(wchar_t));
    if (wstr_ptr == NULL) {
no_mem:
	Xfree(buf);
	if (list)
	    Xfree(list);
	return XNoMemory;
    }
    
    buf_ptr = buf;
    for (i = 0; i < count; i++) {
	list[i] = wstr_ptr;
	_Xwcscpy(wstr_ptr, buf_ptr);
	tmp_len = _Xwcslen(wstr_ptr) + 1;
	wstr_ptr += tmp_len;
	buf_ptr += tmp_len;
    }
    Xfree(buf);

    *list_ret = list;
    *count_ret = count;

    return unconv_num;
}

void XwcFreeStringList(list)
    wchar_t **list;
{
    if (list) {
        if (*list)
	     Xfree(*list);
        Xfree(list);
    }
}

