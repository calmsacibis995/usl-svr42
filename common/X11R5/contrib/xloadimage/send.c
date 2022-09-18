/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:send.c	1.1"
/* send.c:
 *
 * send an Image to an X pixmap
 *
 * jim frost 10.02.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"

unsigned int sendImageToX(disp, scrn, visual, image, pixmap, cmap, verbose)
     Display      *disp;
     int           scrn;
     Visual       *visual;
     Image        *image;
     Pixmap       *pixmap;
     Colormap     *cmap;
     unsigned int  verbose;
{ Pixel        *index;
  unsigned int  a, b, newmap, x, y, linelen, ddepth;
  unsigned long plane;
  byte         *pixptr, *destline, *destptr, *bitplane, destmask;
  XColor        xcolor;
  XGCValues     gcv;
  GC            gc;
  XImage       *ximage;

  goodImage(image, "sendImageToX");

  switch(visual->class) {
  case PseudoColor:
  case GrayScale:
  case StaticColor:
  case StaticGray:
    break;
  default:
    printf("sendImageToX: unsupported display visual\n");
    exit(1);
  }

  index= (Pixel *)lmalloc(sizeof(Pixel) * image->rgb.used);
  xcolor.flags= DoRed | DoGreen | DoBlue;

  /* get the colormap to use
   */

  if (visual == DefaultVisual(disp, scrn)) {
    *cmap= DefaultColormap(disp, scrn);
    newmap= 0;

    /* allocate colors shareable (if we can)
     */

    for (a= 0; a < image->rgb.used; a++) {
      xcolor.red= *(image->rgb.red + a);
      xcolor.green= *(image->rgb.green + a);
      xcolor.blue= *(image->rgb.blue + a);
      if (! XAllocColor(disp, *cmap, &xcolor))
	if ((visual->class == StaticColor) || (visual->class == StaticGray)) {
	  printf("sendImageToX: XAllocColor failed on a static visual\n");
	  return(0);
	}
	else {

	  /* we can't allocate the colors shareable so free all the colors
	   * we had allocated and create a private colormap
	   */

	  for (b= 0; b < a; b++)
	    XFreeColors(disp, *cmap, index + b, 1, 0);
	  *cmap= XCreateColormap(disp, RootWindow(disp, scrn), visual,
				 AllocNone);
	  newmap= 1;
	  break;
	}
      *(index + a)= xcolor.pixel;
    }
  }
  else {
    if ((visual->class == PseudoColor) || (visual->class == GrayScale)) {
      *cmap= XCreateColormap(disp, RootWindow(disp, scrn), visual, AllocNone);
    }
    else
      *cmap= XCreateColormap(disp, RootWindow(disp, scrn), visual, AllocAll);
    newmap= 1;
  }

  if (newmap) {
    for (a= 0; a < image->rgb.used; a++) /* count entries we got */
      if (! XAllocColorCells(disp, *cmap, False, NULL, 0, index + a, 1))
	break;

    if (a < image->rgb.used)     /* can't get enough colors, so reduce */
      reduce(image, a, verbose); /* the colormap to fit what we have */

    for (b= 0; b < a; b++) {
      xcolor.pixel= *(index + b);
      xcolor.red= *(image->rgb.red + b);
      xcolor.green= *(image->rgb.green + b);
      xcolor.blue= *(image->rgb.blue + b);
      XStoreColor(disp, *cmap, &xcolor);
    }
  }

  ddepth= DefaultDepth(disp, scrn);
  *pixmap= XCreatePixmap(disp, RootWindow(disp, scrn), image->width,
			 image->height, ddepth);

  /* blast the image across
   */

  switch (image->type) {
  case IBITMAP:
    gcv.function= GXcopy;
    gcv.foreground= *(index + 1);
    gcv.background= *index;
    gc= XCreateGC(disp, *pixmap, GCFunction | GCForeground | GCBackground,
		  &gcv);
    ximage= XCreateImage(disp, visual, image->depth, XYBitmap, 0, (char*)image->data,
			 image->width, image->height, 8, 0);
    ximage->bitmap_bit_order= MSBFirst;
    ximage->byte_order= MSBFirst;
    XPutImage(disp, *pixmap, gc, ximage, 0, 0, 0, 0,
	      image->width, image->height);
    XFreeGC(disp, gc);
    break;

  case IRGB:

    /* modify image data to match colormap and pack pixels if necessary
     */

    if (verbose) {
      printf("  Building XImage...");
      fflush(stdout);
    }
    pixptr= image->data;
    for (y= 0; y < image->height; y++)
      for (x= 0; x < image->width; x++) {
	valToMem(*(index + memToVal(pixptr, image->pixlen)),
		 pixptr, image->pixlen);
	pixptr += image->pixlen;
      }

    if (verbose)
      printf("done\n");

    /* if the destination depth is not a multiple of 8, then we send each
     * plane as a bitmap because otherwise we would have to pack the pixel
     * data and the XImage format is pretty vague about how that should
     * be done.  this is not as fast as it would be if it were packed but
     * it should be a lot more portable and only slightly slower.
     */

    if (ddepth % 8) {
      gcv.function= GXcopy;
      gcv.background= 0;
      gc= XCreateGC(disp, *pixmap, GCFunction | GCBackground, &gcv);
      linelen= (image->width / 8) + (image->width % 8 ? 1 : 0);
      bitplane= lmalloc(image->height * linelen);
      ximage= XCreateImage(disp, visual, 1, XYBitmap, 0, (char*)bitplane,
			   image->width, image->height, 8, 0);
      ximage->bitmap_bit_order= MSBFirst;
      ximage->byte_order= MSBFirst;

      for (plane= 1 << (ddepth - 1); plane; plane >>= 1) {
	pixptr= image->data;
	destline= bitplane;
	for (y= 0; y < image->height; y++) {
	  destmask= 0x80;
	  destptr= destline;
	  for (x= 0; x < image->width; x++) {
	    if (*pixptr & plane)
	      *destptr |= destmask;
	    else
	      *destptr &= ~destmask;
	    if (!(destmask >>= 1)) {
	      destmask= 0x80;
	      destptr++;
	    }
	    pixptr += image->pixlen;
	  }
	  destline += linelen;
	}
	XSetForeground(disp, gc, plane);
	XSetPlaneMask(disp, gc, plane);
	XPutImage(disp, *pixmap, gc, ximage, 0, 0, 0, 0,
		  image->width, image->height);
      }

      ximage->data= NULL;
      XDestroyImage(ximage);
      lfree(bitplane);
      break;
    }

    /* send image across in one whack
     */

    gcv.function= GXcopy;
    gc= XCreateGC(disp, *pixmap, GCFunction, &gcv);
    ximage= XCreateImage(disp, visual, ddepth, ZPixmap, 0, (char*)image->data,
			 image->width, image->height, 8, 0);
    ximage->byte_order= MSBFirst; /* trust me, i know what i'm talking about */

    XPutImage(disp, *pixmap, gc, ximage, 0, 0,
	      0, 0, image->width, image->height);
    ximage->data= NULL;
    XDestroyImage(ximage); /* waste not want not */
    XFreeGC(disp, gc);
    break;

  default:
    printf("sendImageToX: bad image type\n");
    return(0);
  }
  lfree((byte *)index);
  return(1);
}
