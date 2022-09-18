/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:Distinct.c	1.2"
/*
 * $XConsortium: Distinct.c,v 1.3 90/12/28 19:12:21 gildea Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

# include   <X11/Xlib.h>

/*
 * Distinguishable colors routine.  Determines if two colors are
 * distinguishable or not.  Somewhat arbitrary meaning.
 */

#define MIN_DISTINGUISH	10000.0

Bool
XmuDistinguishableColors (colors, count)
XColor	*colors;
int	count;
{
    double	    deltaRed, deltaGreen, deltaBlue;
    double	    dist;
    int		    i, j;

    for (i = 0; i < count - 1; i++)
	for (j = i + 1; j < count; j++)
	{
     	    deltaRed = (double)colors[i].red - (double)colors[j].red;
    	    deltaGreen = (double)colors[i].green - (double)colors[j].green;
    	    deltaBlue = (double)colors[i].blue - (double)colors[j].blue;
    	    dist = deltaRed * deltaRed +
	       	   deltaGreen * deltaGreen +
 	       	   deltaBlue * deltaBlue;
	    if (dist <= MIN_DISTINGUISH * MIN_DISTINGUISH)
		return False;
	}
    return True;
}

Bool
XmuDistinguishablePixels (dpy, cmap, pixels, count)
    Display	    *dpy;
    Colormap	    cmap;
    unsigned long   *pixels;
    int		    count;
{
    XColor  *defs;
    int	    i, j;
    Bool    ret;

    for (i = 0; i < count - 1; i++)
	for (j = i + 1; j < count; j++)
	    if (pixels[i] == pixels[j])
		return False;
    defs = (XColor *) malloc (count * sizeof (XColor));
    if (!defs)
	return False;
    for (i = 0; i < count; i++)
	defs[i].pixel = pixels[i];
    XQueryColors (dpy, cmap, defs, count);
    ret = XmuDistinguishableColors (defs, count);
    free ((char *) defs);
    return ret;
}
