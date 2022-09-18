/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpTxtPr.c	1.3"
/* $XConsortium: XimpTxtPr.c,v 1.2 91/10/07 17:50:45 rws Exp $ */
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
 *	XmbTextListToTextProperty()
 *	XwcTextListToTextProperty()
 */

{
/* PERF: (Performance improvement work, 5/92)                             */
/*       These macros (ALLOCATE_MAYBE and FREE_MAYBE) are substituted for */
/*       inoperative ALLOCATE_LOCAL and DEALLOCATE_LOCAL when a probably  */
/*       constant amount of storage is required a number of times.  That  */
/*       storage is set aside (local...Buf) and used if it is sufficiently*/
/*       large for ALLOCATE_LOCAL demands.  If it isn't big enough, or if */
/*       it's already in use, a real ALLOC takes place.  FREE_MAYBE       */
/*       unwinds this situation.   [AS]                                   */

#define ALLOCATE_MAYBE(num,auto,inuse) \
                ((!inuse&&(num)<=sizeof(auto))?inuse++,(auto):(malloc(num)))
#define FREE_MAYBE(actual,auto,inuse) \
                {if ((actual)!=(auto)) free(actual); else inuse--;}
unsigned char localBuf[2050]; static char localBufInUse = 0; /* PERF */

    Ximp_XLCd lcd;
    unsigned char *value, *buf, *buf_ptr;
    Atom encoding;
    int nitems, unconv_num;
    int tmp_len, tmp_num;
    int (*cnv_func)();

    if ((lcd = (Ximp_XLCd) _XlcCurrentLC()) == NULL)
	return XLocaleNotSupported;

    switch (style) {
	case XStringStyle:
	case XStdICCTextStyle:
	    encoding = XA_STRING;
	    cnv_func = CNV_STR_FUNC;
	    break;
	case XCompoundTextStyle:
	    encoding = XInternAtom(dpy, "COMPOUND_TEXT", False);
	    cnv_func = CNV_CTEXT_FUNC;
	    break;
	case XTextStyle:
	    encoding = XInternAtom(dpy, "TEXT", False);
	    cnv_func = CNV_TEXT_FUNC;
	    break;
	default:
	    return XConverterNotFound;
    }

    /* if ((buf = (unsigned char *) Xmalloc(buf_len)) == NULL) 	/** PERF */
    if ((buf = (unsigned char *) 
	  ALLOCATE_MAYBE(buf_len,localBuf,localBufInUse)) == NULL) /** PERF */
	return XNoMemory;
retry:
    buf_ptr = buf;
    unconv_num = 0;
    for (i = 0; i < count && buf_len > 0; i++, list_ptr++) {
	tmp_len = buf_len;
	if ((*cnv_func)(lcd, *list_ptr, STRLEN_FUNC(*list_ptr), 
			buf_ptr, &tmp_len, &tmp_num) == -1)
	    continue;

	if (tmp_num > 0 && style == XStdICCTextStyle && encoding == XA_STRING) {
	    encoding = XInternAtom(dpy, "COMPOUND_TEXT", False);
	    cnv_func = CNV_CTEXT_FUNC;
	    list_ptr = list;
	    goto retry;
	}
	buf_ptr += tmp_len++;
	*buf_ptr++ = 0;
	buf_len -= tmp_len;
	unconv_num += tmp_num;
    }

    if ((nitems = buf_ptr - buf) <= 0)
	nitems = 1;
    if ((value = (unsigned char *) Xmalloc(nitems)) == NULL) {
	/* Xfree(buf); 	/** PERF: see above */
	FREE_MAYBE(buf,localBuf,localBufInUse);	/* PERF */
	return XNoMemory;
    }
    if (nitems == 1)
	*value = 0;
    else
    	bcopy(buf, value, nitems);
    nitems--;
    /* Xfree(buf); 	/** PERF: see above */
    FREE_MAYBE(buf,localBuf,localBufInUse);	/* PERF */

    text_prop->value = value;
    text_prop->encoding = encoding;
    text_prop->format = 8;
    text_prop->nitems = nitems;

    return unconv_num;
}
