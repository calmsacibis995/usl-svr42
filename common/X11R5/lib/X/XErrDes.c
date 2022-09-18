/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XErrDes.c	1.2"
/*
 * $XConsortium: XErrDes.c,v 11.45 91/05/04 14:02:29 rws Exp $
 */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "Xlibint.h"
#include <X11/Xos.h>
#include "Xresource.h"
#include <stdio.h>

#ifndef ERRORDB
#define ERRORDB "/usr/X/lib/XErrorDB"
#endif

#if defined(__STDC__)
#define Const const
#else
#define Const /**/
#endif

/*
 * descriptions of errors in Section 4 of Protocol doc (pp. 350-351); more
 * verbose descriptions are given in the error database
 */
static Const char * Const _XErrorList[] = {
    /* No error	*/		"no error",
    /* BadRequest */		"BadRequest",
    /* BadValue	*/		"BadValue",
    /* BadWindow */		"BadWindow",
    /* BadPixmap */		"BadPixmap",
    /* BadAtom */		"BadAtom",
    /* BadCursor */		"BadCursor",
    /* BadFont */		"BadFont",
    /* BadMatch	*/		"BadMatch",
    /* BadDrawable */		"BadDrawable",
    /* BadAccess */		"BadAccess",
    /* BadAlloc	*/		"BadAlloc",
    /* BadColor */  		"BadColor",
    /* BadGC */  		"BadGC",
    /* BadIDChoice */		"BadIDChoice",
    /* BadName */		"BadName",
    /* BadLength */		"BadLength",
    /* BadImplementation */	"BadImplementation",
};


XGetErrorText(dpy, code, buffer, nbytes)
    register int code;
    register Display *dpy;
    char *buffer;
    int nbytes;
{
    char buf[32];
    register _XExtension *ext;

    if (nbytes == 0) return;
    if (code <= BadImplementation && code > 0) {
	sprintf(buf, "%d", code);
	XGetErrorDatabaseText(dpy, "XProtoError", buf, _XErrorList[code],
			      buffer, nbytes);
    } else
	sprintf(buffer, "%d", code);
    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
 	if (ext->error_string != NULL) 
 	    (*ext->error_string)(dpy, code, &ext->codes, buffer, nbytes);
 	ext = ext->next;
    }    
    return;
}

#if NeedFunctionPrototypes
/*ARGSUSED*/
XGetErrorDatabaseText(
    Display *dpy,
    register _Xconst char *name,
    register _Xconst char *type,
    _Xconst char *defaultp,
    char *buffer,
    int nbytes)
#else
/*ARGSUSED*/
XGetErrorDatabaseText(dpy, name, type, defaultp, buffer, nbytes)
    Display *dpy;
    register char *name, *type;
    char *defaultp;
    char *buffer;
    int nbytes;
#endif
{

    static XrmDatabase db;
    static int initialized = False;
    XrmString type_str;
    XrmValue result;
    char temp[BUFSIZ];

    if (nbytes == 0) return;
    if (!initialized) {
	XrmInitialize();
	db = XrmGetFileDatabase(ERRORDB);
	initialized = True;
    }
    if (db)
    {
	sprintf(temp, "%s.%s", name, type);
	XrmGetResource(db, temp, "ErrorType.ErrorNumber", &type_str, &result);
    }
    else
	result.addr = (XPointer)NULL;
    if (!result.addr) {
	result.addr = (XPointer) defaultp;
	result.size = strlen(defaultp) + 1;
    }
    (void) strncpy (buffer, (char *) result.addr, nbytes);
    if (result.size > nbytes) buffer[nbytes-1] = '\0';
}
