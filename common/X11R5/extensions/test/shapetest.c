/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:test/shapetest.c	1.1"
/************************************************************
Copyright 1989 by the Massachusetts Institute of Technology


********************************************************/

/* $XConsortium: shapetest.c,v 1.4 91/01/24 15:14:58 gildea Exp $ */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

Display *dpy;

StartConnectionToServer(argc, argv)
int     argc;
char    *argv[];
{
    char *display;

    display = NULL;
    for(--argc, ++argv; argc; --argc, ++argv)
    {
	if ((*argv)[0] == '-') {
	    switch((*argv)[1]) {
	    case 'd':
		display = argv[1];
		++argv; --argc;
		break;
	    }
	}
    }
    if (!(dpy = XOpenDisplay(display)))
    {
       perror("Cannot open display\n");
       exit(0);
   }
}

XRectangle  rects[] = { 0,0, 100, 100, 10, 10, 100, 100 };

main(argc, argv)
    int argc;
    char **argv;

{
	Window  w;
	GC gc;
	char *windowName = "Test of Shape Extension";
	XSetWindowAttributes xswa;
	unsigned long	mask;
	XEvent pe;
	XColor screen_def_blue, exact_def_blue;
	XColor screen_def_red, exact_def_red;

	/*_Xdebug = 1;*/   /* turn on synchronization */

	StartConnectionToServer(argc, argv);

	xswa.event_mask = ExposureMask;
	xswa.background_pixel = BlackPixel (dpy, DefaultScreen (dpy));
	mask = CWEventMask | CWBackPixel;
	w = XCreateWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		100, 100, 340, 340, 0, 
		CopyFromParent, CopyFromParent,	CopyFromParent,
		mask, &xswa);

	XChangeProperty(dpy,
	    w, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
	    (unsigned char *)windowName, strlen(windowName));

	XShapeCombineRectangles (dpy, w, ShapeBounding, 0, 0, 
		          rects, sizeof (rects) / sizeof (rects[0]),
			  ShapeSet, Unsorted);

	XMapWindow(dpy, w);

	gc = XCreateGC(dpy, w, 0, 0);
	XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), "blue",
	       &screen_def_blue,  &exact_def_blue);
	XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), "red",
	       &screen_def_red,  &exact_def_red);
	XSetForeground(dpy, gc, screen_def_red.pixel);
	XSetBackground(dpy, gc, screen_def_blue.pixel);

	while (1) {
	    XNextEvent(dpy, &pe);
	}
}
