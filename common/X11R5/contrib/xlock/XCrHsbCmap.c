/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xlock:XCrHsbCmap.c	1.1"
/*-
 * XCrHsbCmap.c - X11 library routine to create an HSB ramp colormaps.
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

extern void HSBmap();

Status
XCreateHSBColormap(dsp, screen, cmap, count, h1, s1, b1, h2, s2, b2, bw)
    Display    *dsp;
    int         screen;
    Colormap   *cmap;		/* colormap return value */
    int         count;		/* number of entrys to use */
    double      h1,		/* starting hue */
                s1,		/* starting saturation */
                b1,		/* starting brightness */
                h2,		/* ending hue */
                s2,		/* ending saturation */
                b2;		/* ending brightness */
    int         bw;		/* Boolean: True = save black and white */
{
    u_char      red[256];
    u_char      green[256];
    u_char      blue[256];
    unsigned long pixel;
    Status      status;
    Visual     *visual;
    XColor      xcolors[256];

    if (count > 256)
	return BadValue;

    HSBramp(h1, s1, b1, h2, s2, b2, 0, count - 1, red, green, blue);

    if (bw) {
	pixel = WhitePixel(dsp, screen);
	red[pixel] = green[pixel] = blue[pixel] = 0xff;

	pixel = BlackPixel(dsp, screen);
	red[pixel] = green[pixel] = blue[pixel] = 0;
    }
    status = XCreateDynamicColormap(dsp, screen, cmap, &visual, xcolors,
				    count, red, green, blue);

    return status;
}
