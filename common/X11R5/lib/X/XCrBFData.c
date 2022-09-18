/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XCrBFData.c	1.2"

#include "Xlib.h"

/*
 * XCreateBitmapFromData: Routine to make a pixmap of depth 1 from user 
 *	                  supplied data.
 *	D is any drawable on the same screen that the pixmap will be used in.
 *	Data is a pointer to the bit data, and 
 *	width & height give the size in bits of the pixmap.
 *
 * The following format is assumed for data:
 *
 *    format=XYPixmap
 *    bit_order=LSBFirst
 *    byte_order=LSBFirst
 *    padding=8
 *    bitmap_unit=8
 *    xoffset=0
 *    no extra bytes per line
 */  
#if NeedFunctionPrototypes
Pixmap XCreateBitmapFromData(
     Display *display,
     Drawable d,
     _Xconst char *data,
     unsigned int width,
     unsigned int height)
#else
Pixmap XCreateBitmapFromData(display, d, data, width, height)
     Display *display;
     Drawable d;
     char *data;
     unsigned int width, height;
#endif
{
    XImage ximage;
    GC gc;
    Pixmap pix;

    pix = XCreatePixmap(display, d, width, height, 1);
    if (! (gc = XCreateGC(display, pix, (unsigned long) 0, (XGCValues *) 0)))
	return (Pixmap) None;
    ximage.height = height;
    ximage.width = width;
    ximage.depth = 1;
    ximage.bits_per_pixel = 1;
    ximage.xoffset = 0;
    ximage.format = XYPixmap;
    ximage.data = (char *)data;
    ximage.byte_order = LSBFirst;
    ximage.bitmap_unit = 8;
    ximage.bitmap_bit_order = LSBFirst;
    ximage.bitmap_pad = 8;
    ximage.bytes_per_line = (width+7)/8;

    XPutImage(display, pix, gc, &ximage, 0, 0, 0, 0, width, height);
    XFreeGC(display, gc);
    return(pix);
}
