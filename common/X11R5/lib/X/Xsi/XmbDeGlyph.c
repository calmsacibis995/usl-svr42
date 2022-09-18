/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XmbDeGlyph.c	1.1"
/*
 * $XConsortium: XmbDeGlyph.c,v 1.5 91/04/02 15:10:41 rws Exp $
 */

/*
 * Copyright 1990, 1991 by OMRON Corporation, NTT Software Corporation,
 *                      and Nippon Telegraph and Telephone Corporation
 * Copyright 1991 by the Massachusetts Institute of Technology
 *
 *
 * OMRON, NTT SOFTWARE, NTT, AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 *
 *	Author: Li Yuhong	OMRON Corporation
 *   
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include "Xlocaleint.h"

#define MAGIC_NUMBER		(222)

#define Return(result) {                                                \
        *cs_bytes = cscnt;                                              \
        *scanned_bytes = mbcnt;                                         \
        if (cscnt < limit) 						\
            *cs_str = 0;	/* additional service */		\
        if (error != 0) return(error);                                  \
        return(result);                                                 \
    }
         
#define SaveStore(byte) {                                               \
        if (cscnt >= limit) Return(BadBuffer);                          \
        *cs_str++ = byte;                                               \
        cscnt++;                                                        \
    }
    
            
/*
 * _XmbDecomposeGlyphCharset():
 *   If the converter meets a wrong codepoint, the function will try to 
 *   recover this error in a way in which this codepoint will be substituted  
 *   with the first character of the current charset.
 *   For example, if the wrong codepoint occurs in Charset 0, ASCII, function
 *   will substitute it with the first character blank "0x20".
 *  
*/
int
_XmbDecomposeGlyphCharset(xlocale, mb_str, mb_bytes, cs_str, cs_bytes,
			  scanned_bytes, ctid)
    XLocale		 xlocale;
    unsigned char	*mb_str;
    int			mb_bytes;
    unsigned char	*cs_str;
    int			*cs_bytes;
    int			*scanned_bytes;
    int			*ctid;	/* charset id for find font */
{
    int		mbcnt, cscnt, limit, codelen, deslen, numbytes;
    _CSID     	crcsid, newcsid;
    int     	code;
    int     	error;
    unsigned char c;

    limit = *cs_bytes;
    mbcnt = cscnt = error = 0;
    crcsid = MAGIC_NUMBER;
    while (mb_bytes > 0 && (c = *mb_str) != 0) {
        /* 
	 * filter control characters.
         *
         * For all codeset, we assume that the Control Set 0 and
         * Control Set 1 are encoded as same: one byte.
         * Maybe it's not correct in whole world.
         */
        if ((newcsid = _Xmbcsid(xlocale, mb_str)) == ND)
	    Return(BadEncoding);
        if (newcsid == C0 || newcsid == C1) {
	    SaveStore(c);
	    mb_str++, mb_bytes--, mbcnt++;
	    continue;
	}
        /*
         * skip the escape sequence if codeset is state-dependent.
        */
        if ((_Xmbtype(xlocale) == CDS_STATEFUL) &&
            ((deslen = _Xmbdlen(xlocale, mb_str)) > 0)) {
            mb_str += deslen, mb_bytes -= deslen, mbcnt += deslen;
            continue;
        }
        /*
         * set the current csid.
         */
	if (crcsid == MAGIC_NUMBER) {
	    if (ctid != NULL)
		*ctid = _Xmbctid(xlocale, newcsid);
	    crcsid = newcsid;
	    /* 
             * number of bytes of font encoding.
    	     */
	    numbytes = _Xmbfslen(xlocale);
	    /*
     	     * number of bytes of mb encoding.
	     */
    	    codelen = _Xmblen(xlocale);
	}
 	if (crcsid != newcsid) 
	    break;
        /*
         * pick up each codepoint from the current charset.
         */
        if (codelen > mb_bytes) Return(BadTerminate);
        if (_Xmbctocsc(xlocale, mb_str, &code) < 0)
            error++;            /* wrong codepoint, recovered by function */
        /*
         * Suppose that numbytes is only 1 or 2 because X supports fonts
         * encoded in only one or two bytes.
        */
        if (numbytes == 2)
                SaveStore((code >> 8) & 0x00FF);
        SaveStore(code & 0x00FF);
        mb_str += codelen, mb_bytes -= codelen, mbcnt += codelen;
    }
    Return(Success);
}
