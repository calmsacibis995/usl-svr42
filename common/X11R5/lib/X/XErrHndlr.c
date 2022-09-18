/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XErrHndlr.c	1.2"

#include "Xlibint.h"

extern int _XDefaultError();
extern int _XDefaultIOError();
/* 
 * XErrorHandler - This procedure sets the X non-fatal error handler
 * (_XErrorFunction) to be the specified routine.  If NULL is passed in
 * the original error handler is restored.
 */
 
#if NeedFunctionPrototypes
XErrorHandler XSetErrorHandler(XErrorHandler handler)
#else
XErrorHandler XSetErrorHandler(handler)
    register XErrorHandler handler;
#endif
{
    int (*oldhandler)() = _XErrorFunction;

    if (!oldhandler)
	oldhandler = _XDefaultError;

    if (handler != NULL) {
	_XErrorFunction = handler;
    }
    else {
	_XErrorFunction = _XDefaultError;
    }

    return oldhandler;
}

/* 
 * XIOErrorHandler - This procedure sets the X fatal I/O error handler
 * (_XIOErrorFunction) to be the specified routine.  If NULL is passed in 
 * the original error handler is restored.
 */
 
extern int _XIOError();
#if NeedFunctionPrototypes
XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler handler)
#else
XIOErrorHandler XSetIOErrorHandler(handler)
    register XIOErrorHandler handler;
#endif
{
    int (*oldhandler)() = _XIOErrorFunction;

    if (!oldhandler)
	oldhandler = _XDefaultIOError;

    if (handler != NULL) {
	_XIOErrorFunction = handler;
    }
    else {
	_XIOErrorFunction = _XDefaultIOError;
    }

    return oldhandler;
}
