/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XSetLocale.c	1.2"

/*
 * $XConsortium: XSetLocale.c,v 1.32 91/05/13 08:45:00 rws Exp $
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
 *		 Tetsuya Kato		NTT Software Corporation
 *		 Hiroshi Kuribayashi	OMRON Corporation
 *   
 */
#pragma weak _XrmInitParseInfo

#include "Xlibint.h"
#include "Xlcint.h"
#include <X11/Xlocale.h>
#include <X11/Xos.h>

#ifdef X_LOCALE

/* alternative setlocale() for when the OS does not provide one */

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

#define MAXLOCALE	64	/* buffer size of locale name */

#if NeedFunctionPrototypes
char *
_Xsetlocale(
    int		  category,
    _Xconst char *name
)
#else
char *
_Xsetlocale(category, name)
    int		category;
    char       *name;
#endif
{
    static char *xsl_name;
    char *old_name;
    XrmMethods methods;
    XPointer state;

    if (category != LC_CTYPE && category != LC_ALL)
	return NULL;
    if (!name) {
	if (xsl_name)
	    return xsl_name;
	return "C";
    }
    if (!*name)
	name = getenv("LC_CTYPE");
    if (!name || !*name)
	name = getenv("LANG");
    if (!name || !*name)
	name = "C";
    old_name = xsl_name;
    xsl_name = (char *)name;
    methods = _XrmInitParseInfo(&state);
    xsl_name = old_name;
    if (!methods)
	return NULL;
    name = (*methods->lcname)(state);
    xsl_name = Xmalloc(strlen(name) + 1);
    if (!xsl_name) {
	xsl_name = old_name;
	(*methods->destroy)(state);
	return NULL;
    }
    strcpy(xsl_name, name);
    if (old_name)
	Xfree(old_name);
    (*methods->destroy)(state);
    return xsl_name;
}

#else /* X_LOCALE */

/*
 * _XlcMapOSLocaleName is an implementation dependent routine that derives
 * the LC_CTYPE locale name as used in the sample implementation from that
 * returned by setlocale.
 */

#ifdef hpux

char *
_XlcMapOSLocaleName(osname, siname)
    char *osname;
    char *siname;
{
    char *start;
    char *end;
    int   len;

    start = index(osname, ':');
    if (!start)
	return osname;
    start++;
    end = index(start, ';');
    if (!end)
	return osname;
    len = end - start;
    strncpy(siname, start, len);
    siname[len] = '\0';
    return siname;
}

#else /* hpux */

/*ARGSUSED*/
char *
_XlcMapOSLocaleName(osname, siname)
    char *osname;
    char *siname;
{
    return osname;
}

#endif /* hpux */

#endif  /* X_LOCALE */
