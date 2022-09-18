/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XStName.c	1.1"
/* $XConsortium: XStName.c,v 11.15 91/01/08 14:42:00 gildea Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include <X11/Xlibint.h>
#include <X11/Xatom.h>

#if NeedFunctionPrototypes
XStoreName (
    register Display *dpy,
    Window w,
    _Xconst char *name)
#else
XStoreName (dpy, w, name)
    register Display *dpy;
    Window w;
    char *name;
#endif
{
    XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, 
		8, PropModeReplace, (unsigned char *)name,
                name ? strlen(name) : 0);
}

#if NeedFunctionPrototypes
XSetIconName (
    register Display *dpy,
    Window w,
    _Xconst char *icon_name)
#else
XSetIconName (dpy, w, icon_name)
    register Display *dpy;
    Window w;
    char *icon_name;
#endif
{
    XChangeProperty(dpy, w, XA_WM_ICON_NAME, XA_STRING, 
		8, PropModeReplace, (unsigned char *)icon_name,
		icon_name ? strlen(icon_name) : 0);
}
