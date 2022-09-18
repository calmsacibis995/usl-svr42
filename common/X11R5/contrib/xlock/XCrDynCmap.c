/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xlock:XCrDynCmap.c	1.1"
/*-
 * XCrDynCmap.c - X11 library routine to create dynamic colormaps.
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@sun.com
 *
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 */

#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

Status
XCreateDynamicColormap(dsp, screen, cmap, visual, colors,
		       count, red, green, blue)
    Display    *dsp;
    int		screen;
    Colormap   *cmap;
    Visual    **visual;
    XColor     *colors;
    int		count;
    u_char     *red,
	       *green,
	       *blue;
{
    XVisualInfo vinfo;
    int		pixels[256];
    int		i,
		ncolors,
		planes;
    unsigned long pmasks;
    Status	allocReturn;

    planes = DisplayPlanes(dsp, screen);
    if (XMatchVisualInfo(dsp, screen, planes, PseudoColor, &vinfo)) {

	*visual = vinfo.visual;
	*cmap = XCreateColormap(dsp, RootWindow(dsp, screen),
				*visual, AllocNone);
	ncolors = vinfo.colormap_size;

	if (count > ncolors)
	    return BadValue;

	allocReturn = XAllocColorCells(dsp, *cmap,
				       False, &pmasks, 0,
				       (unsigned long*)pixels, count);

/*	This should return Success, but it doesn't... Xlib bug?
 *	(I'll ignore the return value for now...)
 */
#ifdef NOTDEF
	if (allocReturn != Success)
	    return allocReturn;
#endif				/* NOTDEF */

	for (i = 0; i < count; i++) {
	    colors[i].pixel = pixels[i];
	    colors[i].red = *red++ << 8;
	    colors[i].green = *green++ << 8;
	    colors[i].blue = *blue++ << 8;
	    colors[i].flags = DoRed | DoGreen | DoBlue;
	}
	XStoreColors(dsp, *cmap, colors, count);
	return Success;
    } else
	return BadMatch;
}
