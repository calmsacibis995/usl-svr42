/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpMPrTxt.c	1.2"
/* $XConsortium: XimpMPrTxt.c,v 1.2 91/10/07 17:50:17 rws Exp $ */
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

extern int _Ximp_strcpy(), _Ximp_cttombs();

int
XmbTextPropertyToTextList(dpy, text_prop, list_ret, count_ret)
    Display *dpy;
    XTextProperty *text_prop;
    char ***list_ret;
    int *count_ret;
{
    unsigned char **list;
    unsigned char *buf, *buf_ptr, *str_ptr;
    int i, count, unconv_num, tmp_len, buf_len;
#define CNV_STR_FUNC	_Ximp_strcpy
#define CNV_CTEXT_FUNC	_Ximp_cttombs
#define CNV_TEXT_FUNC	_Ximp_strcpy

    /* XXX */
    buf_len = text_prop->nitems + 1;
    buf_len = (buf_len / BUFSIZE + 1) * BUFSIZE;
    if ((buf = (unsigned char *) Xmalloc(buf_len)) == NULL)
	return XNoMemory;
    /* XXX */

#include "XimpPrTxt.c"

    if ((list = (unsigned char **) Xmalloc(count * sizeof(char *))) == NULL) 
	goto no_mem;
    str_ptr = (unsigned char *) Xmalloc(buf_ptr - buf);
    if (str_ptr == NULL) {
no_mem:
	Xfree(buf);
	if (list)
	    Xfree(list);
	return XNoMemory;
    }
    
    buf_ptr = buf;
    for (i = 0; i < count; i++) {
	list[i] = str_ptr;
	strcpy(str_ptr, buf_ptr);
	tmp_len = strlen(str_ptr) + 1;
	str_ptr += tmp_len;
	buf_ptr += tmp_len;
    }
    Xfree(buf);

    *list_ret = (char **) list;
    *count_ret = count;

    return unconv_num;
}
