/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XwcDeGlyph.c	1.3"

/*
 *      Function Name: _XwcDecomposeGlyphCharset
*/
#include <stdio.h>
#include <X11/Xlibint.h>
#include "Xlocaleint.h"
#include <X11/Xos.h>
#include <X11/Xutil.h>

#define MAGIC_NUMBER	(9999)

#define Return(result) {                                                \
        *cs_bytes = cscnt;                                              \
        *scanned_len = wccnt;                                           \
        if (cscnt < limit) *cs_str = '\0'; /*additional service*/       \
        if (error > 0) return(error);                                   \
        return(result);                                                 \
    }

#define StoreByte(code) {                                               \
        int byte = (code);                                              \
        if (byte < (int)stateinfo.code_min || 				\
	    byte > (int)stateinfo.code_max) {				\
            error++;                                                    \
            *cs_str++ = stateinfo.code_min;                             \
        } else                                                          \
            *cs_str++ = byte;                                           \
    }

int
_XwcDecomposeGlyphCharset(xlocale, wc_str, wc_len, cs_str, cs_bytes,
			  scanned_len, ctid)
    XLocale        xlocale;
    wchar	   *wc_str;
    int	            wc_len;
    unsigned char  *cs_str;
    int	           *cs_bytes;
    int	           *scanned_len;
    int		   *ctid;
{
    int    	    cscnt, wccnt;
    wchar           woffset, newwoffset, code, wc;
    int             limit, error;
    ISOStateInfo    stateinfo;
    char	    *esc;
    int i;
    char *defstr = XDefaultString();

    limit = *cs_bytes;
    wccnt = cscnt = error = 0;
    woffset = MAGIC_NUMBER;
    while (wc_len > 0 && (wc = *wc_str) != WNULL) {
	/*
	 * filter control characters.
	 */
        if (_Xiswcntrl(wc)) {
	    if (cscnt >= limit) 
		Return(BadBuffer);
	    *cs_str++ = _Xwctoa(wc);
	    cscnt++;
	    wc_str++, wc_len--, wccnt++;
	    continue;
	}
	if (woffset == MAGIC_NUMBER) {
    	    _XcwGetAll(xlocale, wc, &esc, &woffset, &stateinfo);
	    if (woffset == 0) {
		/* XXX BUG: need to check designate sequence of default string.
		   But current default string is NULL, so OK. :-) */
		for (i = 0; *(defstr + i) != 0; i++) {
		    *cs_str++ = *(defstr + i);
		    cscnt++;
		}
		error++;
		wc_str++, wc_len--, wccnt++;
		continue;
	    }
	    if (ctid != NULL)
		*ctid = ctGetid(xlocale);
	    newwoffset = woffset;
	} else {
            if(_XcwGetWoffset(wc, &newwoffset) == ND) { /* MUST not change Status */
		/* XXX BUG: need to check designate sequence of default string.
		   But current default string is NULL, so OK. :-) */
		for (i = 0; *(defstr + i) != 0; i++) {
		    *cs_str++ = *(defstr + i);
		    cscnt++;
		}
		error++;
		wc_str++, wc_len--, wccnt++;
		continue;
	    }
	}
	if (woffset != newwoffset)
	    break;
        code = wc - woffset;
        if ((cscnt + stateinfo.code_bytes) > limit)
            Return(BadBuffer);
	
	/* The space charcter(0x20) is include into the latin-1 charset */
	if (wc == WCHARSPACE && woffset == LATINSCRIPT)
	    *cs_str++ = code;
	else {
	    /*
	     * only 2 or 1 byte(s) supported by X.
	    */
	    if (stateinfo.code_bytes == 2)
		StoreByte((code >> 8) & 0x007F | ctGetGLorGR(xlocale));
	    StoreByte(code & 0x007F | ctGetGLorGR(xlocale));
	}
        wc_str++, wc_len--, wccnt++;
        cscnt += stateinfo.code_bytes;
    }
    Return(Success);    
}
