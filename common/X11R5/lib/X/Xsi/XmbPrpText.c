/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XmbPrpText.c	1.3"

#include <X11/Xlibint.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "Xlocaleint.h"

#define XA_COMPOUND_TEXT(d) XInternAtom(d, "COMPOUND_TEXT", False)

#if NeedFunctionPrototypes
int
XmbTextPropertyToTextList(
    Display *dpy,
    XTextProperty *tp,
    char ***list_return,
    int *count_return
)
#else
int
XmbTextPropertyToTextList(dpy, tp, list_return, count_return)
    Display *dpy;
    XTextProperty *tp;
    char ***list_return;
    int *count_return;
#endif
{
    unsigned char **list;
    unsigned int nelements;
    unsigned char *cp;
    unsigned char *start;
    int i, j;
    int len, scand;
    unsigned int datalen = (int) tp->nitems;
    XsiLCd lcd = (XsiLCd)_XlcCurrentLC();
    int ret, error = 0;

    if (tp->format != 8 ||
	(tp->encoding != XA_STRING &&
	 tp->encoding != XA_COMPOUND_TEXT(dpy) &&
	 !(lcd &&
	   tp->encoding == XInternAtom(dpy, lcd->xlc->xlc_db->lc_encoding,
				       False))))
	return XConverterNotFound;

    if (datalen == 0) {
	*list_return = (char **)Xmalloc(sizeof(char *));
	**list_return = 0;
	*count_return = 0;
	return Success;
    }

    nelements = 1;
    for (cp = tp->value, i = datalen; i > 0; cp++, i--) {
	if (*cp == '\0') nelements++;
    }

    list = (unsigned char **) Xmalloc (nelements * sizeof (unsigned char *));
    if (!list) return XNoMemory;

    start = (unsigned char *)Xmalloc ((datalen + 1) * sizeof (unsigned char));
    if (!start) {
	Xfree ((char *) list);
	return XNoMemory;
    }

    if (tp->encoding == XA_STRING ||
	tp->encoding == XA_COMPOUND_TEXT(dpy)) {
	cp = (unsigned char *)tp->value;
	for (i = j = 0; i < nelements; i++, j++) {
	    list[j] = start;
	    len = datalen + 1;
	    if (i == nelements - 1)
		scand = datalen;
	    else
		scand = strlen((char *)cp);
	    if ((ret = _XConvertCTToMB(0, cp, scand, start,
				&len, &scand, 0)) < 0) {
		XFreeStringList((char **)list);
		return (XConverterNotFound);
	    }
	    error += ret;
	    start += len + 1;
	    datalen -= len + 1;
	    cp += scand + 1;
	}
    } else {
	(void)bcopy ((char *) tp->value, (char *)start, (unsigned)tp->nitems);
	start[datalen] = '\0';
	 
	for (cp = start, i = datalen + 1, j = 0; i > 0; cp++, i--) {
	    if (*cp == '\0') {
	        list[j] = start;
	        start = (cp + 1);
	        j++;
	    }
	}
    }

    *list_return = (char **)list;
    *count_return = nelements;
    return error;
}
