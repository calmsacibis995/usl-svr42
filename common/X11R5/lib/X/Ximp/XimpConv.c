/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpConv.c	1.2"
/* $XConsortium: XimpConv.c,v 1.5 91/10/07 17:47:44 rws Exp $ */
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

#define CHAR_LENGTH(xxxtocs) \
    unsigned char buf[BUFSIZE]; \
    int char_length; \
    int buf_len, scan_len; \
    int ret = 0; \
\
    (*lcd->ximp_lcpart->methods->cnv_start)(lcd); \
\
    while (from_len > 0) { \
        buf_len = BUFSIZE; \
        scan_len = (*xxxtocs)(lcd, from_ptr, from_len, buf, &buf_len, \
                              NULL, &char_length); \
        if (scan_len == -1) { \
	    ret = -1; \
	    goto error; \
	} \
        if (scan_len == 0)  \
	    break; \
\
	ret += buf_len / char_length; \
        from_ptr += scan_len; \
        from_len -= scan_len; \
    } \
\
error: \
    (*lcd->ximp_lcpart->methods->cnv_end)(lcd); \
\
    return ret;

int
_Ximp_str_charlen(lcd, strtocs, from, from_len)
    Ximp_XLCd lcd;
    int (*strtocs)();
    unsigned char *from;
    int from_len;
{
    unsigned char *from_ptr = from;
    CHAR_LENGTH(strtocs)
}

int
_Ximp_mbs_charlen(lcd, mbstr, mbstr_len)
    Ximp_XLCd lcd;
    unsigned char *mbstr;
    int mbstr_len;
{
    return _Ximp_str_charlen(lcd, lcd->ximp_lcpart->methods->mbstocs,
			     mbstr, mbstr_len);
}


#define STRING_CONV(xxxtocs, cstoxxx) \
    unsigned char buf[BUFSIZE]; \
    int cs_num; \
    int to_length, buf_len, scan_len, tmp_len; \
    int ret = -1; \
\
    if (to_len) { \
        to_length = *to_len; \
	*to_len = 0; \
    } else \
        to_length = MAXINT; \
    if (unconv_num) \
        *unconv_num = 0; \
\
    (*lcd->ximp_lcpart->methods->cnv_start)(lcd); \
\
    while (from_len > 0 && to_length > 0) { \
        buf_len = BUFSIZE; \
        scan_len = (*xxxtocs)(lcd, from_ptr, from_len, buf, &buf_len, \
                              &cs_num, NULL); \
        if (scan_len == -1) \
            goto error; \
        if (scan_len == 0)  \
	    break; \
\
        from_ptr += scan_len; \
        from_len -= scan_len; \
\
        tmp_len = to_length; \
        if ((*cstoxxx)(lcd, buf, buf_len, to_ptr, &tmp_len, cs_num) == -1) { \
            if (unconv_num) { \
                *unconv_num += scan_len; \
		continue; \
	    } \
	    goto error; \
        } \
\
	if (to_ptr) \
            to_ptr += tmp_len; \
	if (to_len) \
	    *to_len += tmp_len; \
        to_length -= tmp_len; \
    } \
\
    ret =  from_ptr - from; \
\
error: \
    (*lcd->ximp_lcpart->methods->cnv_end)(lcd); \
\
    return ret;

int
_Ximp_strtostr(lcd, strtocs, from, from_len, cstostr, to, to_len, unconv_num)
    Ximp_XLCd lcd;
    int (*strtocs)();
    unsigned char *from;
    int from_len;
    int (*cstostr)();
    unsigned char *to;
    int *to_len;
    int *unconv_num;
{
    unsigned char *from_ptr = from;
    unsigned char *to_ptr = to;
    STRING_CONV(strtocs, cstostr)
}

int
_Ximp_strtowstr(lcd, strtocs, from, from_len, cstowstr, to, to_len, unconv_num)
    Ximp_XLCd lcd;
    int (*strtocs)();
    unsigned char *from;
    int from_len;
    int (*cstowstr)();
    wchar_t *to;
    int *to_len;
    int *unconv_num;
{
    unsigned char *from_ptr = from;
    wchar_t *to_ptr = to;
    STRING_CONV(strtocs, cstowstr)
}

int
_Ximp_wstrtostr(lcd, wstrtocs, from, from_len, cstostr, to, to_len, unconv_num)
    Ximp_XLCd lcd;
    int (*wstrtocs)();
    wchar_t *from;
    int from_len;
    int (*cstostr)();
    unsigned char *to;
    int *to_len;
    int *unconv_num;
{
    wchar_t *from_ptr = from;
    unsigned char *to_ptr = to;
    STRING_CONV(wstrtocs, cstostr)
}

int
_Ximp_wstrtowstr(lcd, wstrtocs,from,from_len, cstowstr,to,to_len, unconv_num)
    Ximp_XLCd lcd;
    int (*wstrtocs)();
    wchar_t *from;
    int from_len;
    int (*cstowstr)();
    wchar_t *to;
    int *to_len;
    int *unconv_num;
{
    wchar_t *from_ptr = from;
    wchar_t *to_ptr = to;
    STRING_CONV(wstrtocs, cstowstr)
}


int
_Ximp_mbstowcs(lcd, mbstr, mbstr_len, wcstr, wcstr_len, unconv_num)
    Ximp_XLCd lcd;
    unsigned char *mbstr;
    int mbstr_len;
    wchar_t *wcstr;
    int *wcstr_len;
    int *unconv_num;
{
    if (lcd == NULL && (lcd = (Ximp_XLCd) _XlcCurrentLC()) == NULL)
	return -1;

    return _Ximp_strtowstr(lcd, lcd->ximp_lcpart->methods->mbstocs, mbstr,
			   mbstr_len, lcd->ximp_lcpart->methods->cstowcs,
			   wcstr, wcstr_len, unconv_num);
}


int
_Ximp_wcstombs(lcd, wcstr, wcstr_len, mbstr, mbstr_len, unconv_num)
    Ximp_XLCd lcd;
    wchar_t *wcstr;
    int wcstr_len;
    unsigned char *mbstr;
    int *mbstr_len;
    int *unconv_num;
{
    if (lcd == NULL && (lcd = (Ximp_XLCd) _XlcCurrentLC()) == NULL)
	return -1;

    return _Ximp_wstrtostr(lcd, lcd->ximp_lcpart->methods->wcstocs, wcstr,
			   wcstr_len, lcd->ximp_lcpart->methods->cstombs,
			   mbstr, mbstr_len, unconv_num);
}

int
_Ximp_strcpy(lcd, str1, str1_len, str2, str2_len, unconv_num)
    Ximp_XLCd lcd;
    register unsigned char *str1;
    register int str1_len;
    register unsigned char *str2;
    int *str2_len;
    int *unconv_num;
{
    unsigned char *str1_tmp = str1;

    if (str2_len && str1_len > *str2_len)
	str1_len = *str2_len;

    while (str1_len--)
	*str2++ = *str1++;

    if (unconv_num)
	*unconv_num = 0;
    if (str2_len)
	*str2_len = str1 - str1_tmp;

    return str1 - str1_tmp;
}


int
_Xmblen(str, len)
    char *str;
    int len;
{
    return _Xmbtowc(NULL, str, len);
}

int
_Xmbtowc(wstr, str, len)
    wchar_t *wstr;
    char *str;
    int len;
{
    Ximp_XLCd lcd = (Ximp_XLCd) _XlcCurrentLC();
    wchar_t tmp_wc;

    if (lcd == NULL)
	return -1;
    if (str == NULL)
	return lcd->ximp_lcpart->state_dependent;
    if (len == 0)
	return 0;
    if (*str == '\0') {
	*wstr = 0;
	return 0;
    }
    if (wstr == NULL)
	wstr = &tmp_wc;

    return _Ximp_strtowstr(lcd, lcd->ximp_lcpart->methods->mbstocs, str, len,
			   lcd->ximp_lcpart->methods->cstowcs, wstr, 1, NULL);
}

int
_Xwctomb(str, wc)
    char *str;
    wchar_t wc;
{
    int len;

    Ximp_XLCd lcd = (Ximp_XLCd) _XlcCurrentLC();

    if (lcd == NULL)
	return -1;
    if (str == NULL)
	return lcd->ximp_lcpart->state_dependent;
    len = XIMP_MB_CUR_MAX(lcd);

    if (_Ximp_wstrtostr(lcd, lcd->ximp_lcpart->methods->wcstocs, &wc, 1,
		lcd->ximp_lcpart->methods->cstombs, str, &len, NULL) < 0)
	return -1;
    
    return len;
}

int
_Xmbstowcs(wstr, str, len)
    wchar_t *wstr;
    char *str;
    int len;
{
    Ximp_XLCd lcd = (Ximp_XLCd) _XlcCurrentLC();

    if (lcd == NULL)
	return -1;
    
    if (_Ximp_strtowstr(lcd, lcd->ximp_lcpart->methods->mbstocs, str, 
			strlen(str), lcd->ximp_lcpart->methods->cstowcs,
			wstr, &len, NULL) < 0)
	return -1;

    return len;
}

int
_Xwcstombs(str, wstr, len)
    char *str;
    wchar_t *wstr;
    int len;
{
    Ximp_XLCd lcd = (Ximp_XLCd) _XlcCurrentLC();

    if (lcd == NULL)
	return -1;

    if (_Ximp_wstrtostr(lcd, lcd->ximp_lcpart->methods->wcstocs, wstr,
			_Xwcslen(wstr), lcd->ximp_lcpart->methods->cstombs,
			str, &len, NULL) < 0)
	return -1;

    return len;
}

wchar_t *
_Xwcscpy(wstr1, wstr2)
    register wchar_t *wstr1, *wstr2;
{
    wchar_t *wstr_tmp = wstr1;

    while (*wstr1++ = *wstr2++)
	;

    return wstr_tmp;
}

wchar_t *
_Xwcsncpy(wstr1, wstr2, len)
    register wchar_t *wstr1, *wstr2;
    register len;
{
    wchar_t *wstr_tmp = wstr1;

    while (len-- > 0)
	if (!(*wstr1++ = *wstr2++))
	    break;

    while (len-- > 0)
	*wstr1++ = (wchar_t) 0;

    return wstr_tmp;
}

int
_Xwcslen(wstr)
    register wchar_t *wstr;
{
    register wchar_t *wstr_ptr = wstr;

    while (*wstr_ptr)
	wstr_ptr++;
    
    return wstr_ptr - wstr;
}


char *
XDefaultString()
{
    return "";
}
