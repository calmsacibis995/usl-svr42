/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpPrTxt.c	1.2"
/* $XConsortium: XimpPrTxt.c,v 1.2 91/10/07 17:50:23 rws Exp $ */
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

/*
 *	XmbTextPropertyToTextList()
 *	XwcTextPropertyToTextList()
 */

{
    Ximp_XLCd lcd;
    Atom encoding;
    unsigned char *last_ptr;
    int nitems, tmp_num;
    int (*cnv_func)();

    if ((nitems = text_prop->nitems) <= 0) {
	*list_ret = NULL;
	*count_ret = 0;
	Xfree(buf);
	return Success;
    }
    if (text_prop->format != 8) {
	Xfree(buf);
	return XConverterNotFound;
    }
    if ((lcd = (Ximp_XLCd) _XlcCurrentLC()) == NULL) {
	Xfree(buf);
	return XLocaleNotSupported;
    }

    encoding = text_prop->encoding;

    if (encoding == XA_STRING)
	cnv_func = CNV_STR_FUNC;
    else if (encoding == XInternAtom(dpy, "COMPOUND_TEXT", False))
	cnv_func = CNV_CTEXT_FUNC;
    else if (encoding == XInternAtom(dpy, "TEXT", False))
	cnv_func = CNV_TEXT_FUNC;
    else {
	Xfree(buf);
	return XConverterNotFound;
    }

    last_ptr = str_ptr = text_prop->value;
    buf_ptr = buf;
    unconv_num = count = 0;
    while (1) {
	if (nitems == 0 || *str_ptr == 0) {
	    tmp_len = buf_len;
	    if ((*cnv_func)(lcd, last_ptr, str_ptr - last_ptr, 
			    buf_ptr, &tmp_len, &tmp_num) == -1) {
		Xfree(buf);
		return XConverterNotFound;
	    }

	    buf_ptr += tmp_len++;
	    *buf_ptr++ = 0;
	    buf_len -= tmp_len;
	    unconv_num += tmp_num;
	    count++;

	    if (nitems == 0)
		break;
	    last_ptr = ++str_ptr;
	} else
	    str_ptr++;

	nitems--;
    }
}
