/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XlcDefLd.c	1.3"

/*
 * $XConsortium: XlcDefLd.c,v 1.1 91/05/02 09:14:44 rws Exp $
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
 *	Authors: Li Yuhong		OMRON Corporation
 *		 Tatsuya Kato		NTT Software Corporation
 *		 Hiroshi Kuribayashi	OMRON Corporation
 *   
 */

#include "Xlibint.h"
#include "Xi18nint.h"

#pragma weak _XlcDefaultLoader = __XlcDefaultLoader

#if defined(__STDC__) && !defined(VMS)
#define RConst const
#else
#define RConst /**/
#endif

static RConst XLCdMethodsRec lcd_methods = {
    _XlcDefaultMapModifiers,
    _XsiCreateFontSet,
    _XipOpenIM
};

XLCd __XlcDefaultLoader(osname)
    char *osname;
{
    char *name;
    XLocale xlc;
    XsiLCd lcd;
#if !defined(X_NOT_STDC_ENV) && !defined(X_LOCALE)
    char siname[256];
    char *_XlcMapOSLocaleName();

    name = _XlcMapOSLocaleName(osname, siname);
#else
    name = osname;
#endif
    xlc = _XlcMakeLocale(name);
    if (!xlc)
	return NULL;
    lcd = (XsiLCd)Xmalloc(sizeof(XsiLCdRec));
    if (!lcd)
	return NULL;
    lcd->methods = (XLCdMethods)&lcd_methods;
    lcd->core.name = (char *)Xmalloc(strlen(osname) + 1);
    if (!lcd->core.name) {
	Xfree((char *)lcd);
	return NULL;
    }
    strcpy(lcd->core.name, osname);
    lcd->core.modifiers = NULL;
    lcd->xlc = xlc;

    return (XLCd)lcd;
}
