/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:test/xtesttest.c	1.1"

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/extensions/XTest.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

char *ProgramName;

static void usage ()
{
    fprintf (stderr, "usage:  %s [-display dpy]\n", ProgramName);
    exit (1);
}

main (argc, argv)
    int argc;
    char **argv;
{
    char *displayname = NULL;
    Display *dpy;
    int i;    
    int event_base, error_base;
    int major_version, minor_version;
    unsigned long req;
    GC gc;
    XID gid;
    Window w;
    XSetWindowAttributes swa;
    int key, minkey, maxkey;
    XEvent ev, second_ev, third_ev;
    long    delta_time;

    ProgramName = argv[0];
    for (i = 1; i < argc; i++) {
	char *arg = argv[i];

	if (arg[0] == '-') {
	    switch (arg[1]) {
	      case 'd':			/* -display dpy */
		if (++i >= argc) usage ();
		displayname = argv[i];
		continue;
	    }
	}
	usage ();
    }

    dpy = XOpenDisplay (displayname);
    if (!dpy) {
	fprintf (stderr, "%s:  unable to open display \"%s\"\n",
		 ProgramName, XDisplayName(displayname));
	exit (1);
    }

    if (!XTestQueryExtension (dpy, &event_base, &error_base, &major_version, &minor_version)) {
	fprintf (stderr, 
	 "%s:  XTest extension not supported on server \"%s\"\n",
		 ProgramName, DisplayString(dpy));
	XCloseDisplay(dpy);
	exit (1);
    }
    printf ("XTest information for server \"%s\":\n",
	    DisplayString(dpy));
    printf ("  Major version:       %d\n", major_version);
    printf ("  Minor version:       %d\n", minor_version);
    printf ("  First event number:  %d\n", event_base);
    printf ("  First error number:  %d\n", error_base);

    
    swa.override_redirect = True;
    swa.cursor = XCreateFontCursor(dpy, XC_boat);
    swa.event_mask = KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
    w = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, 100, 100, 0, 0,
		      InputOnly, CopyFromParent,
		      CWEventMask|CWOverrideRedirect|CWCursor, &swa);
    XMapWindow(dpy, w);
    if (!XTestCompareCursorWithWindow(dpy, w, swa.cursor))
	printf("error: window cursor is not the expected one\n");
    XTestFakeMotionEvent(dpy, DefaultScreen(dpy), 10, 10, 0);
    if (!XTestCompareCurrentCursorWithWindow(dpy, w))
	printf("error: window cursor is not the displayed one\n");
    XUndefineCursor(dpy, w);
    if (!XTestCompareCursorWithWindow(dpy, w, None))
	printf("error: window cursor is not the expected None\n");
    XSync(dpy, True);
    XDisplayKeycodes(dpy, &minkey, &maxkey);
    key = XKeysymToKeycode(dpy, XK_a);
    if (!key)
	key = minkey;
    XTestFakeKeyEvent(dpy, key, True, 0);
    XNextEvent(dpy, &ev);
    if (ev.type != KeyPress ||
	ev.xkey.keycode != key ||
	ev.xkey.x_root != 10 ||
	ev.xkey.y_root != 10)
	printf("error: bad event received for key press\n");
    XTestFakeKeyEvent(dpy, key, False, 0);
    XNextEvent(dpy, &ev);
    if (ev.type != KeyRelease ||
	ev.xkey.keycode != key ||
	ev.xkey.x_root != 10 ||
	ev.xkey.y_root != 10)
	printf("error: bad event received for key release\n");
    XTestFakeButtonEvent(dpy, 1, True, 0);
    XTestFakeMotionEvent(dpy, DefaultScreen(dpy), 9, 8, 1000);
    XTestFakeButtonEvent(dpy, 1, False, 2000);
    XNextEvent(dpy, &ev);
    if (ev.type != ButtonPress ||
	ev.xbutton.button != 1 ||
	ev.xbutton.x_root != 10 ||
	ev.xbutton.y_root != 10)
	printf("error: bad event received for button press\n");
    XNextEvent(dpy, &second_ev);
    if (second_ev.type != MotionNotify ||
	second_ev.xmotion.x_root != 9 ||
	second_ev.xmotion.y_root != 8)
	printf("error: bad event received for motion\n");
    delta_time = second_ev.xmotion.time - ev.xbutton.time;
    if (delta_time > 1100 || delta_time < 900)
	printf ("Poor event spacing is %d should be %d\n", delta_time, 1000);
    XNextEvent(dpy, &third_ev);
    if (third_ev.type != ButtonRelease ||
	third_ev.xbutton.button != 1 ||
	third_ev.xbutton.x_root != 9 ||
	third_ev.xbutton.y_root != 8)
	printf("error: bad event received for button release\n");
    delta_time = third_ev.xbutton.time - second_ev.xmotion.time;
    if (delta_time > 2100 || delta_time < 1900)
	printf ("Poor event spacing is %d should be %d\n", delta_time, 2000);
    gc = DefaultGC(dpy, DefaultScreen(dpy));
    req = NextRequest(dpy);
    XDrawPoint(dpy, w, gc, 0, 0);
    if (!XTestDiscard(dpy) || req != NextRequest(dpy))
	printf("error: XTestDiscard failed to discard an XDrawPoint\n");
    gid = XGContextFromGC(gc);
    XTestSetGContextOfGC(gc, 3L);
    if (XGContextFromGC(gc) != 3L)
	printf("error: XTestSetGContextOfGC failed\n");
    XTestSetGContextOfGC(gc, gid);
    if (XGContextFromGC(gc) != gid)
	printf("error: XTestSetGContextOfGC failed\n");
    XCloseDisplay (dpy);
    exit (0);
}
