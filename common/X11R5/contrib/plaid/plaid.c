/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5plaid:plaid.c	1.3"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

             /gh10/X.V11R2/demos/plaid           All Rights Reserved



******************************************************************/
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include "X11/Xatom.h"
#include <stdio.h>
#include <errno.h>

extern int errno;

char *ProgramName;

int rand();
#ifndef MEMUTIL
char *malloc();
#endif /* MEMUTIL */

Window myWin, newWin, threeWin;

Display *dpy;

#define NUMRECTS 10
XRectangle rects[NUMRECTS];
GC gc;

usage ()
{
    fprintf (stderr, "usage:  %s [-options ...]\n\n", ProgramName);
    fprintf (stderr, "where options include:\n");
    fprintf (stderr, "    -display host:dpy        X server to use\n");
    fprintf (stderr, "    -geometry geom           geometry of window\n");
    fprintf (stderr, "    -b                       use backing store\n");
    fprintf (stderr, "    -fg color                set foreground color\n");
    fprintf (stderr, "    -bg color                set background color\n");
    fprintf (stderr, "    -bd color                set border color\n");
    fprintf (stderr, "    -bw width                set border width\n");
    fprintf (stderr, "\n");
    exit (1);
}


main(argc, argv)
int	argc;
char	*argv[];
{
    int i, j ;
    char *geom = NULL;
    int winx, winy, winw, winh;
    register int xdir, ydir;
    register int xoff, yoff;
    register int centerX, centerY;
    XGCValues xgcv;
    XSetWindowAttributes xswa;
    Colormap map;
    unsigned int bw = 1;
    char *display = NULL;
    Status status;
    char *fg = NULL;
    char *bg = NULL;
    char *bd = NULL;
    int fg_pix, bg_pix, bd_pix;
    XColor fg_def, fg_exact, bg_def, bg_exact, bd_def, bd_exact;
    int bs = NotUseful;
    Visual visual;

	XSizeHints size_hints;
	XClassHint  class_hints;
	XWMHints	wm_hints;
	XTextProperty window_name , icon_name ;
	char *list1[1] ;

	Atom        protocol_atom, kill_atom;

    ProgramName = argv[0];

    for (i=1; i < argc; i++)
    {
	char *arg = argv[i];

	if (arg[0] == '-') {
	    switch (arg[1]) {
		case 'd':			/* -display host:dpy */
		    if (++i >= argc) usage ();
		    display = argv[i];
		    continue;
		case 'g':			/* -geometry host:dpy */
		    if (++i >= argc) usage ();
		    geom = argv[i];
		    continue;
		case 'b':			/* -b or -bg or -bd */
		    if (!strcmp(argv[i], "-bg")) {
			if (++i >= argc) usage ();
			bg = argv[i];
		    } else if (!strcmp(argv[i], "-bd")) {
			if (++i >= argc) usage ();
			bd = argv[i];
		    } else if (!strcmp(argv[i], "-bw")) {
			if (++i >= argc) usage ();
			bw = atoi(argv[i]);
		    } else
			bs = Always;
		    continue;
		case 'f':			/* assume -fg */
		    if (++i >= argc) usage ();
		    fg = argv[i];
		    continue;
		default:
		    usage ();
	    }
	} else if (argv [i] [0] == '=') 	/* obsolete */
	        geom = argv[i];
	else
	    usage ();
    }

    if (!(dpy = XOpenDisplay(display)))
    {
	perror("Cannot open display\n");
	exit(-1);
    }

    map = XDefaultColormap(dpy,DefaultScreen(dpy));
    if (fg) {
	status = XAllocNamedColor(dpy, map, fg, &fg_def, &fg_exact);
	fg_pix = status ? fg_def.pixel : WhitePixel(dpy, DefaultScreen(dpy));
    } else
	fg_pix = WhitePixel(dpy, DefaultScreen(dpy));

    if (bg) {
	status = XAllocNamedColor(dpy, map, bg, &bg_def, &bg_exact);
	bg_pix = status ? bg_def.pixel : BlackPixel(dpy, DefaultScreen(dpy));
    } else
	bg_pix = BlackPixel(dpy, DefaultScreen(dpy));

    if (bd) {
	status = XAllocNamedColor(dpy, map, bd, &bd_def, &bd_exact);
	bd_pix = status ? bd_def.pixel : WhitePixel(dpy, DefaultScreen(dpy));
    } else
	bd_pix = WhitePixel(dpy, DefaultScreen(dpy));

    winx = 0;
    winy = 0;
    winw = 101;
    winh = 201;

    if (geom) 
    {
        (void) XParseGeometry(geom, &winx, &winy,
			      (unsigned int *)&winw,
			      (unsigned int *)&winh);
    }

    xswa.backing_store = bs;
    xswa.event_mask = ExposureMask | StructureNotifyMask;
    xswa.background_pixel = bg_pix;
    xswa.border_pixel = bd_pix;
    visual.visualid = CopyFromParent;
    myWin = XCreateWindow(dpy,
		RootWindow(dpy, DefaultScreen(dpy)),
		winx, winy, winw, winh, bw, 
		DefaultDepth(dpy, DefaultScreen(dpy)), InputOutput,
		&visual, 
	        CWEventMask | CWBackingStore | CWBorderPixel | CWBackPixel, 
		&xswa);

/*
 *	WIPRO : vivek t.
 *	CHANGE # UNKNOWN
 *	FILE # plaid.c
 *  Setting the WMProtocols for WM_DELETE_WINDOW property.
 *	ENDCHANGE # UNKNOWN
 */
    protocol_atom = XInternAtom(dpy, "WM_PROTOCOLS", False);
	kill_atom = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, myWin, &kill_atom, 1);

/*
 *	WIPRO : vivek t.
 *	CHANGE # UNKNOWN
 *	FILE # plaid.c
 *	The client does nto consider the x & y offsets given in the geometry command 
 *  line option. So setting the size_hints in the WM properties, only if 
 *  these values are given some values in geometry.
 *	ENDCHANGE # UNKNOWN
 */
	if (winx || winy)
		size_hints.flags = USPosition | USSize;
	else
		size_hints.flags = NULL;
	class_hints.res_name = ProgramName;
	class_hints.res_class = "Plaid";

    list1[0] = "Plaid" ;
	XStringListToTextProperty(list1,1,&window_name);
    list1[0] = "Plaid" ;
	XStringListToTextProperty(list1,1,&icon_name);


	XSetWMProperties (dpy, myWin, &window_name, &icon_name, argv, argc, 
			&size_hints, NULL, &class_hints);

	/****
    XChangeProperty(dpy, myWin, XA_WM_NAME, XA_STRING, 8,
		PropModeReplace, (unsigned char *)"Plaid", 5);
	****/

    XMapWindow(dpy, myWin);

    xgcv.foreground = fg_pix;
    xgcv.function = GXinvert;
    xgcv.plane_mask = fg_pix ^ bg_pix;
    xgcv.fill_style = FillSolid;
    xgcv.graphics_exposures = False;
    gc = XCreateGC(dpy, myWin,
		GCForeground | GCFunction | GCPlaneMask | GCFillStyle |
		GCGraphicsExposures, &xgcv);
    j=0;
    while(1)
    {
	XEvent pe;
	XExposeEvent *ee;
	XConfigureEvent *ce;

        XNextEvent(dpy, &pe);	/* this should get first exposure event */    
	switch (pe.type) {

		case ClientMessage:
					{
						XClientMessageEvent *ev = (XClientMessageEvent *)&pe;

						if (ev->message_type == protocol_atom &&
									ev->data.l[0] == kill_atom)
						{
								XDestroyWindow (dpy, myWin);
								XCloseDisplay (dpy);
								exit(0);
						}
					}
																							break;

	  case Expose:
	    ee = (XExposeEvent *) &pe;
	    while (ee->count)
	    {
                XNextEvent(dpy, &pe);	    
	        ee = (XExposeEvent *) &pe;
	    }
	    break;
	  case ConfigureNotify:
	    ce = (XConfigureEvent *)&pe;
	    winx = ce->x;
	    winy = ce->y;
	    winw = ce->width;
	    winh = ce->height;
	    break;
	  case CirculateNotify:
	  case DestroyNotify:
	  case GravityNotify:
	  case MapNotify:
	  case ReparentNotify:
	  case UnmapNotify:
	    break;
	  default:
	    printf("Unknown event type: %d\n", pe.type);
	}

	printf("PLAID: Dealing with exposures\n");	
	XClearArea(dpy, myWin, 0, 0, winw, winh, 0);
        printf("PLAID: drawing rects\n");

	centerX = winw / 2;
	centerY = winh / 2;
	xdir = ydir = -2;
	xoff = yoff = 2;

	i = 0;
        while (! XPending(dpy))
	{
	    
	    rects[i].x = centerX - xoff;
	    rects[i].y = centerY - yoff;
	    rects[i].width = 2 * xoff;
	    rects[i].height = 2 * yoff;
	    xoff += xdir;
	    yoff += ydir;
	    if ((xoff <= 0) || (xoff >= centerX))
	    {
		xoff -= 2*xdir;
	        xdir = -xdir;
	    }
	    if ((yoff <= 0) || (yoff >= centerY))
	    {
	        yoff -= 2*ydir;
		ydir = -ydir;
	    }
	    if (i == (NUMRECTS - 1))
	    {
                XFillRectangles(dpy, myWin, gc, rects, NUMRECTS);
		i = 0;
	    }
	    else
		i++;
	}
    }
}
