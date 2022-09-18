/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpDefCnv.c	1.2"
/* $XConsortium: XimpDefCnv.c,v 1.2 91/10/07 17:47:57 rws Exp $ */
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

static Bool def_initialize();
static void null_proc();
static char def_mbchar();
static int def_mbstocs(), def_wcstocs(), def_cstombs(), def_cstowcs();

XLCdXimpMethods def_lc_methods =
{
    def_initialize,
    null_proc,
    null_proc,
    def_mbchar,
    def_mbstocs,
    def_wcstocs,
    def_cstombs,
    def_cstowcs,
};

static void
null_proc(lcd)
    Ximp_XLCd lcd;
{
}

static Bool
def_initialize(lcd)
    Ximp_XLCd lcd;
{
    lcd->ximp_lcpart->mb_cur_max = 1;
    lcd->ximp_lcpart->state_dependent = False;

    return True;
}

static char
def_mbchar(lcd, str, lenp)
    Ximp_XLCd lcd;
    register char *str;
    register int *lenp;
{
    *lenp = 1;

    return *str;
}

static int
def_mbstocs(lcd, mbstr, mbstr_len, csbuf, csbuf_len, cs_number, char_length)
    Ximp_XLCd lcd;
    unsigned char *mbstr;
    register int mbstr_len;
    register unsigned char *csbuf;
    int *csbuf_len;
    int *cs_number;
    int *char_length;
{
    register unsigned char *mbptr = mbstr;

    if (csbuf_len && mbstr_len > *csbuf_len)
	mbstr_len = *csbuf_len;

    while (mbstr_len--)
	*csbuf++ = *mbptr++;

    if (cs_number)
	*cs_number = 0;

    if (char_length)
	*char_length = 1;

    if (csbuf_len)
	*csbuf_len = mbptr - mbstr;

    return mbptr - mbstr;
}

static int
def_wcstocs(lcd, wcstr, wcstr_len, csbuf, csbuf_len, cs_number, char_length)
    Ximp_XLCd lcd;
    wchar_t *wcstr;
    register int wcstr_len;
    register unsigned char *csbuf;
    int *csbuf_len;
    int *cs_number;
    int *char_length;
{
    register wchar_t *wcptr = wcstr;

    if (csbuf_len && wcstr_len > *csbuf_len)
        wcstr_len = *csbuf_len;

    while (wcstr_len--)
        *csbuf++ = (unsigned char) *wcptr++;

    if (cs_number)
        *cs_number = 0;

    if (char_length)
        *char_length = 1;

    if (csbuf_len)
        *csbuf_len = wcptr - wcstr;

    return wcptr - wcstr;
}

static int
def_cstombs(lcd, csstr, csstr_len, mbbuf, mbbuf_len, cs_number)
    Ximp_XLCd lcd;
    unsigned char *csstr;
    register int csstr_len;
    register unsigned char *mbbuf;
    int *mbbuf_len;
    int cs_number;
{
    register unsigned char *csptr = csstr;

    if (mbbuf_len && csstr_len > *mbbuf_len)
	csstr_len = *mbbuf_len;

    while (csstr_len--)
	*mbbuf++ = *csptr++;

    if (mbbuf_len)
	*mbbuf_len = csptr - csstr;

    return csptr - csstr;
}

static int
def_cstowcs(lcd, csstr, csstr_len, wcbuf, wcbuf_len, cs_number)
    Ximp_XLCd lcd;
    unsigned char *csstr;
    int csstr_len;
    wchar_t *wcbuf;
    int *wcbuf_len;
    int cs_number;
{
    register unsigned char *csptr = csstr;

    if (wcbuf_len && csstr_len > *wcbuf_len)
	csstr_len = *wcbuf_len;

    while (csstr_len--)
	*wcbuf++ = (wchar_t) *csptr++;

    if (wcbuf_len)
	*wcbuf_len = csptr - csstr;

    return csptr - csstr;
}
