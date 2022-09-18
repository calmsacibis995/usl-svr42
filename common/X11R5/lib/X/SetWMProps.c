/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:SetWMProps.c	1.1"
/* $XConsortium: SetWMProps.c,v 1.7 91/04/01 20:18:31 gildea Exp $ */

/***********************************************************
Copyright 1988 by Wyse Technology, Inc., San Jose, Ca.,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include <X11/Xlibint.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

/* 
 * XSetWMProperties sets the following properties:
 *	WM_NAME		  type: TEXT		format: varies?
 *	WM_ICON_NAME	  type: TEXT		format: varies?
 *	WM_HINTS	  type: WM_HINTS	format: 32
 *	WM_COMMAND	  type: TEXT		format: varies?
 *	WM_CLIENT_MACHINE type: TEXT		format: varies?
 *	WM_NORMAL_HINTS	  type: WM_SIZE_HINTS 	format: 32
 *	WM_CLASS	  type: STRING/STRING	format: 8
 */
	
void XSetWMProperties (dpy, w, windowName, iconName, argv, argc, sizeHints,
		       wmHints, classHints)
     Display *dpy;
     Window w;			/* window to decorate */
     XTextProperty *windowName;	/* name of application */
     XTextProperty *iconName;	/* name string for icon */
     char **argv;		/* command line */
     int argc;			/* size of command line */
     XSizeHints *sizeHints;	/* size hints for window in its normal state */
     XWMHints *wmHints;		/* miscelaneous window manager hints */
     XClassHint *classHints;	/* resource name and class */
{
    XTextProperty textprop;
    char hostName[256];
    int len = _XGetHostname (hostName, sizeof hostName);

    /* set names of window and icon */
    if (windowName) XSetWMName (dpy, w, windowName);
    if (iconName) XSetWMIconName (dpy, w, iconName);

    /* set the command if given */
    if (argv) {
	/*
	 * for UNIX and other operating systems which use nul-terminated
	 * arrays of STRINGs.
	 */
	XSetCommand (dpy, w, argv, argc);
    }

    /* set the name of the machine on which this application is running */
    textprop.value = (unsigned char *) hostName;
    textprop.encoding = XA_STRING;
    textprop.format = 8;
    textprop.nitems = len;
    XSetWMClientMachine (dpy, w, &textprop);
	
    /* set hints about how geometry and window manager interaction */
    if (sizeHints) XSetWMNormalHints (dpy, w, sizeHints);
    if (wmHints) XSetWMHints (dpy, w, wmHints);
    if (classHints) {
	XClassHint tmp;

	if (!classHints->res_name) {
	    tmp.res_name = getenv ("RESOURCE_NAME");
	    if (!tmp.res_name && argv && argv[0]) {
		/*
		 * UNIX uses /dir/subdir/.../basename; other operating
		 * systems will have to change this.
		 */
		char *cp = rindex (argv[0], '/');
		tmp.res_name = (cp ? cp + 1 : argv[0]);
	    }
	    tmp.res_class = classHints->res_class;
	    classHints = &tmp;
	}
	XSetClassHint (dpy, w, classHints);
    }
}

