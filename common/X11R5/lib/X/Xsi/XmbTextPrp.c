/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XmbTextPrp.c	1.3"

#include <X11/Xlibint.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "Xlocaleint.h"

#define XA_COMPOUND_TEXT(d) XInternAtom(d, "COMPOUND_TEXT", False)

#pragma weak XmbTextListToTextProperty = _XmbTextListToTextProperty

#if NeedFunctionPrototypes
int
_XmbTextListToTextProperty(
    Display	      *dpy,
    char             **list,
    int		       count,
    XICCEncodingStyle  style,
    XTextProperty     *text_prop
)
#else
int
_XmbTextListToTextProperty(dpy, list, count, style, text_prop)
    Display	      *dpy;
    char             **list;
    int		       count;
    XICCEncodingStyle  style;
    XTextProperty     *text_prop;
#endif
{
    int len, datalen;
    unsigned char *buf, *buf_sv;
    int i, scand;
    register unsigned int nbytes;
    XTextProperty proto;
    int ret, error = 0;

    for (i = 0, nbytes = 0; i < count; i++) {
	nbytes += (unsigned) ((list[i] ? strlen (list[i]) : 0) + 1);
    }
    proto.format = 8;
    proto.nitems = 0;

    if (nbytes) {
	datalen = len = nbytes * 6 + 6; /* Is it correct/enough ? */
	buf_sv = buf = (unsigned char *)Xmalloc((unsigned)len);
	if (!buf) return (XNoMemory);
	proto.value = (unsigned char *) buf;

	if (style == XStringStyle) {
	    proto.encoding = XA_STRING;
	    for (i = 0; i < count; i++, list++) {
		if (*list) {
		    len = datalen;
		    if ((ret = _XConvertMBToString((unsigned char *)(*list),
					    strlen(*list), buf,
					    &len, &scand)) < 0) {
			Xfree((char *)buf_sv);
			return (XConverterNotFound);
		    }
		    error += ret;
		    buf += len + 1;
		    datalen -= len + 1;
		    proto.nitems += len + 1;
		} else {
		    *buf++ = '\0';
		    datalen--;
		    proto.nitems++;
		}
	    }
	    proto.nitems--;
	} else if (style == XCompoundTextStyle) {
	    proto.encoding = XA_COMPOUND_TEXT(dpy);
	    for (i = 0; i < count; i++, list++) {
		if (*list) {
		    len = datalen;
		    if ((ret = _XConvertMBToCT(0, (unsigned char *)(*list),
					strlen(*list), buf,
					&len, &scand, 0)) < 0) {
			Xfree((char *)buf_sv);
			return (XConverterNotFound);
		    }
		    error += ret;
		    buf += len + 1;
		    datalen -= len + 1;
		    proto.nitems += len + 1;
		} else {
		    *buf++ = '\0';
		    datalen--;
		    proto.nitems++;
		}
	    }
	    proto.nitems--;
	} else if (style == XTextStyle) { /* MB: need not to convert */
	    XsiLCd lcd = (XsiLCd)_XlcCurrentLC();
	    proto.nitems = nbytes - 1;
	    if (lcd)
		proto.encoding = XInternAtom(dpy,
					     lcd->xlc->xlc_db->lc_encoding,
					     False);
	    else
		proto.encoding = XA_STRING;
	    for (i = 0; i < count; i++, list++) {
		if (*list) {
		    (void) strcpy((char *)buf, *list);
		    buf += (strlen(*list) + 1);
		} else {
		    *buf++ = '\0';
		}
	    }
	} else if (style == XStdICCTextStyle) {
	    int is_xstring = 1;	/* Yes */
	    for (i = 0; i < count; i++, list++) {
		if (*list) {
		    len = datalen;
		    if ((ret = _XConvertMBToCT(0, (unsigned char *)*list,
					strlen(*list),
					buf, &len, &scand, 0)) < 0) {
			Xfree((char *)buf_sv);
			return (XConverterNotFound);
		    }
		    error += ret;
		    if (is_xstring)
			for (i = 0; *(buf+i); i++) {
			    if (!_isXString(*(buf+i))) {
				is_xstring = 0; /* Not XString */
				break;
			    }
			}
		    buf += len + 1;
		    datalen -= len + 1;
		    proto.nitems += len + 1;
		} else {
		    *buf++ = '\0';
		    datalen--;
		    proto.nitems++;
		}
	    }
	    proto.nitems--;
	    if (is_xstring)
		proto.encoding = XA_STRING;
	    else
		proto.encoding = XA_COMPOUND_TEXT(dpy);
	} else {
	    /* I don't know such a encoding */
	    return (XConverterNotFound);
	}
    } else {
	proto.nitems = 0;
	proto.value = 0;
    }
    *text_prop = proto;
    return (error);
}
