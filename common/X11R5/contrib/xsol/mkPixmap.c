/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xsol:mkPixmap.c	1.1"
#include	<X11/Xlib.h>

Pixmap 
MakePixmap(dpy, scr, root, data, width, height, ximage)
Display	*dpy;
int	scr;
Drawable root;
short *data;
int width, height;
XImage	*ximage;
{
    GC pgc;
    XGCValues gcv;
    Pixmap pid;

    pid = XCreatePixmap(dpy, root, width, height, DefaultDepth(dpy, scr));

    gcv.foreground = BlackPixel(dpy, scr);
    gcv.background = WhitePixel(dpy, scr);
    pgc = XCreateGC(dpy, pid, GCForeground | GCBackground, &gcv);

    ximage->height = height;
    ximage->width = width;
    ximage->xoffset = 0;
    ximage->format = XYBitmap;
    ximage->data = (char *)data;
    ximage->byte_order = LSBFirst;
    ximage->bitmap_unit = 16;
    ximage->bitmap_bit_order = LSBFirst;
    ximage->bitmap_pad = 16;
    ximage->bytes_per_line = (width+7)/8;
    ximage->depth = 1;

    XPutImage(dpy, pid, pgc, ximage, 0, 0, 0, 0, width, height);
    XFreeGC(dpy, pgc);
    return(pid);
}

MakeImage(width, height, data, ximage)
int	width, height;
char	*data;
XImage	*ximage;
{
    ximage->height = height;
    ximage->width = width;
    ximage->xoffset = 0;
    ximage->format = XYBitmap;
    ximage->data = (char *)data;
    ximage->byte_order = LSBFirst;
    ximage->bitmap_unit = 16;
    ximage->bitmap_bit_order = LSBFirst;
    ximage->bitmap_pad = 16;
    ximage->bytes_per_line = (width+7)/8;
    ximage->depth = 1;
}
