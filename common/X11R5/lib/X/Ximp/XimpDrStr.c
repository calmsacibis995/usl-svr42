/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpDrStr.c	1.1"
/* $XConsortium: XimpDrStr.c,v 1.1 91/07/09 17:36:53 rws Exp $ */
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

/*
 *	_Ximp_mb_draw_string()
 *	_Ximp_wc_draw_string()
 */

{
    Ximp_XLCd lcd = (Ximp_XLCd) xfont_set->core.lcd;
    unsigned char *strptr, strbuf[BUFSIZE];
    unsigned char xchar_buf[BUFSIZE];
    XChar2b xchar2b_buf[BUFSIZE];
    FontSetRec *fontset;
    XFontStruct *font;
    int	(*cnv_func)();
    int cset_num, char_length;
    int count, length, tmp_len;

    cnv_func = lcd->ximp_lcpart->methods->CNV_FUNC;

    (*lcd->ximp_lcpart->methods->cnv_start)(lcd);

    while (text_length > 0) {
        length = BUFSIZE;
	count = (*cnv_func)(lcd, text, text_length, strbuf, &length, 
			    &cset_num, &char_length);
	if (count <= 0)
	    break;

	text += count;
	text_length -= count;

	strptr = strbuf;
	fontset = ((Ximp_XFontSet) xfont_set)->ximp_fspart->fontset + cset_num;
	if (fontset == NULL)
	    continue;
	while (length > 0) {
	    tmp_len = BUFSIZE;
	    if (char_length < 2)
		count = _Ximp_cstoxchar(fontset, strptr, length,
					xchar_buf, &tmp_len, &font);
	    else
		count = _Ximp_cstoxchar2b(fontset, strptr, length,
					  xchar2b_buf, &tmp_len, &font);
	    if (count <= 0)
		break;

            XSetFont(dpy, gc, font->fid);
	    if (char_length < 2) {
	        XDrawString(dpy, d, gc, x, y, xchar_buf, tmp_len);
		x += XTextWidth(font, xchar_buf, tmp_len);
            } else {
	        XDrawString16(dpy, d, gc, x, y, xchar2b_buf, tmp_len);
		x += XTextWidth16(font, xchar2b_buf, tmp_len);
	    }
	    strptr += count;
	    length -= count;
	}
    }

    (*lcd->ximp_lcpart->methods->cnv_end)(lcd);
}
