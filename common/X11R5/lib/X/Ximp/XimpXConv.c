/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpXConv.c	1.1"
/* $XConsortium: XimpXConv.c,v 1.2 91/07/30 14:27:58 rws Exp $ */
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

static XFontStruct *
getFont(fontset, str, xstr)
    FontSetRec *fontset;
    unsigned char *str;
    unsigned char *xstr;
{
    register ExtFontRec *ext_font = fontset->ext_font_list;
    int ext_font_num = fontset->ext_font_num;
    register unsigned char ch;
    unsigned char ch_L, ch_R;

    ch_L = *str & 0x7f;
    ch_R = ch_L | 0x80;

    while (ext_font_num--) {
	ch = ext_font->msb_mask ? ch_R : ch_L;
	if (ch > ext_font->min_char && ch < ext_font->max_char) {
	    *xstr = ch;
	    return ext_font->font;
	}
	ext_font++;
    }

    *xstr = fontset->msb_mask ? ch_R : ch_L;

    return fontset->font;
}

static XFontStruct *
getFont16(fontset, str, xstr)
    FontSetRec *fontset;
    unsigned char *str;
    XChar2b *xstr;
{
    register ExtFontRec *ext_font = fontset->ext_font_list;
    int ext_font_num = fontset->ext_font_num;
    register unsigned xchar;
    unsigned xchar_L, xchar_R;

    xchar_L = ((unsigned) *str << 8 | *(str+1)) & 0x7f7f;
    xchar_R = xchar_L | 0x8080;

    while (ext_font_num--) {
	xchar = ext_font->msb_mask ? xchar_R : xchar_L;
	if (xchar > ext_font->min_char && xchar < ext_font->max_char)
	    goto done;
	ext_font++;
    }

    xchar = ext_font->msb_mask ? xchar_R : xchar_L;
done:
    xstr->byte1 = xchar >> 8;
    xstr->byte2 = (unsigned char) xchar;

    return fontset->font;
}

int
_Ximp_cstoxchar(fontset, string, length, ret_buf, ret_len, ret_font)
    FontSetRec *fontset;
    unsigned char *string;
    register int length;
    unsigned char *ret_buf;
    int *ret_len;
    XFontStruct **ret_font;
{
    register unsigned char mask, *strptr = string;
    register unsigned char *bufptr = ret_buf;
    XFontStruct *font;

    if (fontset->font == NULL)
	return -1;
    
    if (length > *ret_len)
	length = *ret_len;
    if (length < 1)
	return 0;

    if (fontset->ext_font_num == 0) {
	font = fontset->font;
	mask = fontset->msb_mask;
	while (length--)
	    *bufptr++ = (*strptr++ & 0x7f) | mask;
	goto done;
    }

    font = getFont(fontset, strptr, bufptr);
    strptr++;
    bufptr++;
    length--;

    for ( ; length > 0; length--, strptr++, bufptr++)
	if (font != getFont(fontset, strptr, bufptr))
	    break;

done:
    *ret_len = bufptr - ret_buf;
    *ret_font = font;

    return strptr - string;
}

int
_Ximp_cstoxchar2b(fontset, string, length, ret_buf, ret_len, ret_font)
    FontSetRec *fontset;
    unsigned char *string;
    register int length;
    XChar2b *ret_buf;
    int *ret_len;
    XFontStruct **ret_font;
{
    register unsigned char mask, *strptr = string;
    register XChar2b *bufptr = ret_buf;
    XFontStruct *font;

    if (fontset->font == NULL)
	return -1;
    
    length >>= 1;
    if (length > *ret_len)
	length = *ret_len;
    if (length < 1)
	return 0;

    if (fontset->ext_font_num == 0) {
	font = fontset->font;
	mask = fontset->msb_mask;
	while (length--) {
	    bufptr->byte1 = (*strptr++ & 0x7f) | mask;
	    bufptr->byte2 = (*strptr++ & 0x7f) | mask;
	    bufptr++;
	}
	goto done;
    }

    font = getFont16(fontset, strptr, bufptr);
    strptr += 2;
    bufptr++;
    length--;

    for ( ; length > 0; length--, strptr += 2, bufptr++)
	if (font != getFont16(fontset, strptr, bufptr))
	    break;

done:
    *ret_len = bufptr - ret_buf;
    *ret_font = font;

    return strptr - string;
}
