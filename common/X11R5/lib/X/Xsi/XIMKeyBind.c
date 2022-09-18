/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XIMKeyBind.c	1.2"

#define NEED_EVENTS
#include "Xlibint.h"
#include "Xi18nint.h"
#include "XIMlibint.h"
#include <X11/Xutil.h>

int
_XipwcLookupString(supic, ev, buffer, nchars, keysym, status)
    XIC supic;
    register XKeyEvent *ev;
    wchar_t *buffer;
    int nchars;
    KeySym *keysym;
    Status *status;
{
    XipIC		ic = (XipIC)supic;
    short		type;
    int			length;
    unsigned char	*ptr;
    int			ret_len, scanned_bytes;
    int			ret;
    unsigned char	buf[32];
#ifndef X_WCHAR
    char		*mbuf;
#endif

    if (ev->keycode > ev->display->max_keycode) {
	(void)_XipGetNextICQueue(ic, &type, &length, keysym, (char **)&ptr);
	if (type == XIM_STRING || (type == XIM_KEYSYM && length > 0)) {
#ifdef X_WCHAR
	    ret_len = nchars;
	    ret = _XConvertCTToWC(ic->wc, ptr, length, (wchar *)buffer,
				  &ret_len, &scanned_bytes, (_State *)NULL);
#else
	    ret_len = length * 2;
	    mbuf = _XAllocScratch(ev->display, ret_len);
	    ret = _XConvertCTToMB(ic->mb, ptr, length, mbuf,
				  &ret_len, &scanned_bytes, (_State *)NULL);
	    if (ret == Success) {
#ifdef macII
		ret_len = 0;
#else
		mbuf[ret_len] = '\0';
		ret_len = mbstowcs(buffer, mbuf, nchars);
#endif
		if (ret_len == nchars)
		    ret = XBufferOverflow;
	    }
#endif
	    if (ret != Success) {
		*status = XBufferOverflow;
		return(0);
	    }
	    if (type == XIM_STRING)
		*status = XLookupChars;
	    else
		*status = XLookupBoth;
	    return(ret_len);
	} else if (type == XIM_KEYSYM) {
	    *status = XLookupKeySym;
	    return(0);
	} else {
	    *status = 0;
	    return(0);
	}
    } else {
	ret_len = XLookupString(ev, (char *)buf, 32, keysym, NULL);
	if (ret_len > 0) {
	    if (*keysym != NoSymbol) {
		*status = XLookupBoth;
	    } else {
		*status = XLookupChars;
	    }
#ifdef X_WCHAR
	    /* XXX BUG Need to save current status */
	    _XConvertMBToWC(ic->wc, (unsigned char *)buf, ret_len,
			    (wchar *)buffer, &nchars, &scanned_bytes,
			    (_State *)NULL);
	    ret_len = nchars;
	    /* XXX BUG Need to restore saved status */
#else
#ifdef macII
	    ret_len = 0;
#else
	    buf[ret_len] = '\0';
	    ret_len = mbstowcs(buffer, (char *)buf, nchars);
#endif
#endif
	} else {
	    if (*keysym != NoSymbol) {
		*status = XLookupKeySym;
	    } else {
		*status = XLookupNone;
	    }
	}
	return(ret_len);
    }
}

int
_XipmbLookupString(supic, ev, buffer, nbytes, keysym, status)
    XIC supic;
    register XKeyEvent *ev;
    char *buffer;
    int nbytes;
    KeySym *keysym;
    Status *status;
{
    XipIC		ic = (XipIC)supic;
    short		type;
    int			length;
    unsigned char	*ptr;
    int			ret_len, scanned_bytes;
    int			ret;

    if (ev->keycode > ev->display->max_keycode) {
	_XipGetNextICQueue(ic, &type, &length, keysym, (char **)&ptr);
	if (type == XIM_STRING) {
	    ret_len = nbytes;
	    ret = _XConvertCTToMB(ic->mb, ptr, length, (unsigned char *)buffer,
				  &ret_len, &scanned_bytes, (_State *)NULL);
	    if (ret != Success) {
		*status = XBufferOverflow;
		return(0);
	    }
	    *status = XLookupChars;
	    return(ret_len);
	} else if (type == XIM_KEYSYM) {
	    if (length > 0) {
		ret_len = nbytes;
		ret = _XConvertCTToMB(ic->mb, ptr, length,
				      (unsigned char *)buffer,
				      &ret_len, &scanned_bytes,
				      (_State *)NULL);
		if (ret != Success) {
		    *status = XBufferOverflow;
		    return(0);
		}
		*status = XLookupBoth;
		return(length);
	    }
	    *status = XLookupKeySym;
	    return(0);
	} else {
	    *status = 0;
	    return(0);
	}
    } else {
	ret_len = XLookupString(ev, buffer, nbytes, keysym, NULL);
	if (ret_len > 0) {
	    if (*keysym != NoSymbol) {
		*status = XLookupBoth;
	    } else {
		*status = XLookupChars;
	    }
	} else {
	    if (*keysym != NoSymbol) {
		*status = XLookupKeySym;
	    } else {
		*status = XLookupNone;
	    }
	}
	return(ret_len);
    }
}

int
_XipctLookupString(ic, ev, buffer, nbytes, keysym, status)
    XIC ic;
    register XKeyEvent *ev;
    char *buffer;
    int nbytes;
    KeySym *keysym;
    Status *status;
{
    short		type;
    int			length;
    char		*ptr;
    int			ret_len;

    if (ev->keycode > ev->display->max_keycode) {
	_XipGetNextICQueue(ic, &type, &length, keysym, &ptr);
	if (type == XIM_STRING) {
	    if (length > nbytes) {
		*status = XBufferOverflow;
		return(0);
	    }
	    (void)strncpy(buffer, ptr, length);
	    buffer[length] = 0;
	    *status = XLookupChars;
	    return(nbytes);
	} else if (type == XIM_KEYSYM) {
	    if (length > 0) {
		if (length > nbytes) {
		    *status = XBufferOverflow;
		    return(0);
		}
		(void)strncpy(buffer, ptr, length);
		buffer[length] = 0;
		*status = XLookupBoth;
		return(nbytes);
	    }
	    *status = XLookupKeySym;
	    return(0);
	} else {
	    *status = 0;
	    return(0);
	}
    } else {
	ret_len = XLookupString(ev, buffer, nbytes, keysym, NULL);
	if (ret_len > 0) {
	    if (*keysym != NoSymbol) {
		*status = XLookupBoth;
	    } else {
		*status = XLookupChars;
	    }
	} else {
	    if (*keysym != NoSymbol) {
		*status = XLookupKeySym;
	    } else {
		*status = XLookupNone;
	    }
	}
	return(ret_len);
    }
}
