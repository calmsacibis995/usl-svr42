/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XwcPrpText.c	1.3"

#include <X11/Xlibint.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "Xlocaleint.h"

#define XA_COMPOUND_TEXT(d) XInternAtom(d, "COMPOUND_TEXT", False)

#if NeedFunctionPrototypes
int
XwcTextPropertyToTextList(
    Display *dpy,
    XTextProperty *tp,
    wchar_t ***list_return,
    int *count_return
)
#else
int
XwcTextPropertyToTextList(dpy, tp, list_return, count_return)
    Display *dpy;
    XTextProperty *tp;
    wchar_t ***list_return;
    int *count_return;
#endif
{
    wchar_t **list;
    unsigned int nelements;
    unsigned char *cp;
    wchar_t *start;
    int i, j;
    int len, scand;
    unsigned int datalen = (int) tp->nitems;
    int ret, error = 0;
#ifndef X_WCHAR
    char *mbuf;
#endif
    XsiLCd lcd = (XsiLCd)_XlcCurrentLC();

    if (tp->format != 8 ||
	(tp->encoding != XA_STRING &&
	 tp->encoding != XA_COMPOUND_TEXT(dpy) &&
	 !(lcd &&
	   tp->encoding == XInternAtom(dpy, lcd->xlc->xlc_db->lc_encoding,
				       False))))
	return XConverterNotFound;

    if (datalen == 0) {
	*list_return = (wchar_t **) Xmalloc(sizeof (wchar_t *));
	**list_return = 0;
	*count_return = 0;
	return Success;
    }

    nelements = 1;
    for (cp = tp->value, i = datalen; i > 0; cp++, i--) {
	if (*cp == '\0') nelements++;
    }

    list = (wchar_t **) Xmalloc (nelements * sizeof (wchar_t *));
    if (!list) return XNoMemory;

    start = (wchar_t *) Xmalloc ((datalen + 1) * sizeof (wchar_t));
    if (!start) {
	Xfree ((char *) list);
	return XNoMemory;
    }

    if (tp->encoding == XA_STRING ||
	tp->encoding == XA_COMPOUND_TEXT(dpy)) {
	cp = tp->value;
	for (i = j = 0; i < nelements; i++, j++) {
	    list[j] = start;
	    if (i == nelements - 1)
		scand = datalen;
	    else
		scand = strlen((char *)cp);
#ifdef X_WCHAR
	    len = datalen + 1;
	    if ((ret = _XConvertCTToWC(0, cp,  scand, start,
				&len, &scand, 0)) < 0) {
		XwcFreeStringList(list);
		return (XConverterNotFound);
	    }
#else
#ifdef macII
	    len = 0; ret = 0;
#else
	    len = scand * 2;
	    mbuf = _XAllocScratch(dpy, len);
	    if ((ret = _XConvertCTToMB(0, cp, scand, mbuf,
				&len, &scand, 0)) < 0) {
		XwcFreeStringList(list);
		return (XConverterNotFound);
	    }
	    len = mbstowcs(start, mbuf, datalen + 1);
	    if (len == datalen) {
		XwcFreeStringList(list);
		return (XConverterNotFound);
	    }
#endif
#endif
	    error += ret;
	    start += len + 1;
	    datalen -= len + 1;
	    cp += scand + 1;
	}
    } else {
	cp = tp->value;
	for (i = j = 0; i < nelements; i++, j++) {
	    list[j] = start;
#ifdef X_WCHAR
	    len = datalen + 1;
	    if (i == nelements - 1)
		scand = datalen;
	    else
		scand = strlen((char *)cp);
	    if ((ret = _XConvertMBToWC(0, cp, scand, start,
				&len, &scand, 0)) < 0) {
		XwcFreeStringList(list);
		return (XConverterNotFound);
	    }
	    error += ret;
#else
#ifdef macII
	    len = 0;
#else
	    len = mbstowcs(start, (char *)cp, datalen);
	    if (len == datalen) {
		XwcFreeStringList(list);
		return (XConverterNotFound);
	    }
#endif
#endif
	    start += len + 1;
	    datalen -= len + 1;
	    cp += scand + 1;
	}
    }

    *list_return = list;
    *count_return = nelements;
    return error;
}

void XwcFreeStringList (list)
    wchar_t **list;
{
    if (list) {
	if (list[0]) Xfree ((char *)(list[0]));
	Xfree ((char *) list);
    }
}
