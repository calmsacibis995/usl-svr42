/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:DelCmap.c	1.2"
/* $XConsortium: DelCmap.c,v 1.1 89/05/19 14:37:16 converse Exp $
 * 
 * Copyright 1989 by the Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Donna Converse, MIT X Consortium
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* To remove any standard colormap property, use XmuDeleteStandardColormap().
 * XmuDeleteStandardColormap() will remove the specified property from the
 * specified screen, releasing any resources used by the colormap(s) of the
 * property if possible.
 */

void XmuDeleteStandardColormap(dpy, screen, property)
    Display	*dpy;		/* specifies the X server to connect to */
    int		screen;		/* specifies the screen of the display */
    Atom	property;	/* specifies the standard colormap property */
{
    XStandardColormap	*stdcmaps, *s;
    int			count = 0;

    if (XGetRGBColormaps(dpy, RootWindow(dpy, screen), &stdcmaps, &count,
			 property))
    {
	for (s=stdcmaps; count > 0; count--, s++) {
	    if ((s->killid == ReleaseByFreeingColormap) &&
		(s->colormap != None) &&
		(s->colormap != DefaultColormap(dpy, screen)))
		XFreeColormap(dpy, s->colormap);
	    else if (s->killid != None)
		XKillClient(dpy, s->killid);
	}
	XDeleteProperty(dpy, RootWindow(dpy, screen), property);
	XFree((char *) stdcmaps);
	XSync(dpy, False);
    }
}

