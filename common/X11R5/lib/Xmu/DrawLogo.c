/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:DrawLogo.c	1.2"
/* static char Xrcsid[] = "$XConsortium: DrawLogo.c,v 1.3 90/12/01 12:58:02 rws Exp $"; */

#include <X11/Xlib.h>  

/*
Copyright 1988 by the Massachusetts Institute of Technology

*/

/*
 *  Draw the "official" X Window System Logo, designed by Danny Chong
 *
 *  Written by Ollie Jones, Apollo Computer
 *
 *  Does some fancy stuff to make the logo look acceptable even
 *  if it is tiny.  Also makes the various linear elements of
 *  the logo line up as well as possible considering rasterization.
 */

XmuDrawLogo(dpy, drawable, gcFore, gcBack, x, y, width, height)
    Display *dpy;
    Drawable drawable;
    GC gcFore, gcBack;
    int x, y;
    unsigned int width, height;
{

    unsigned int size;
    int d11, d21, d31;
    XPoint poly[4];

    XFillRectangle(dpy, drawable, gcBack, x, y, width, height);

    /* for now, do a centered even-sized square, at least for now */
    size = width;
    if (height < width)
	 size = height;
    size &= ~1;
    x += (width - size) >> 1;
    y += (height - size) >> 1;

/*    
 *           ----- 
 *          /    /
 *         /    /
 *        /    /
 *       /    /
 *      /____/
 */

    d11 = (size / 11);
    if (d11 < 1) d11 = 1;
    d21 = (d11+3) / 4;
    d31 = d11 + d11 + d21;
    poly[0].x = x + size;              poly[0].y = y;
    poly[1].x = x + size-d31;          poly[1].y = y;
    poly[2].x = x + 0;                 poly[2].y = y + size;
    poly[3].x = x + d31;               poly[3].y = y + size;
    XFillPolygon(dpy, drawable, gcFore, poly, 4, Convex, CoordModeOrigin);

/*    
 *           ------ 
 *          /     /
 *         /  __ /
 *        /  /  /
 *       /  /  /
 *      /__/__/
 */

    poly[0].x = x + d31/2;                       poly[0].y = y + size;
    poly[1].x = x + size / 2;                    poly[1].y = y + size/2;
    poly[2].x = x + (size/2)+(d31-(d31/2));      poly[2].y = y + size/2;
    poly[3].x = x + d31;                         poly[3].y = y + size;
    XFillPolygon(dpy, drawable, gcBack, poly, 4, Convex, CoordModeOrigin);

/*    
 *           ------ 
 *          /  /  /
 *         /--/  /
 *        /     /
 *       /     /
 *      /_____/
 */

    poly[0].x = x + size - d31/2;                poly[0].y = y;
    poly[1].x = x + size / 2;                    poly[1].y = y + size/2;
    poly[2].x = x + (size/2)-(d31-(d31/2));      poly[2].y = y + size/2;
    poly[3].x = x + size - d31;                  poly[3].y = y;
    XFillPolygon(dpy, drawable, gcBack, poly, 4, Convex, CoordModeOrigin);

/*
 * -----
 * \    \
 *  \    \
 *   \    \
 *    \    \
 *     \____\
 */

    poly[0].x = x;                     poly[0].y = y;
    poly[1].x = x + size/4;            poly[1].y = y;
    poly[2].x = x + size;              poly[2].y = y + size;
    poly[3].x = x + size - size/4;     poly[3].y = y + size;
    XFillPolygon(dpy, drawable, gcFore, poly, 4, Convex, CoordModeOrigin);

/*    
 *          /
 *         /
 *        /
 *       /
 *      /
 */

    poly[0].x = x + size- d11;        poly[0].y = y;
    poly[1].x = x + size-( d11+d21);  poly[1].y = y;
    poly[2].x = x + d11;              poly[2].y = y + size;
    poly[3].x = x + d11 + d21;        poly[3].y = y + size;
    XFillPolygon(dpy, drawable, gcBack, poly, 4, Convex, CoordModeOrigin);
}
