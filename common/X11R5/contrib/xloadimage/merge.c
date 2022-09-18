/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:merge.c	1.2"

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

/* merge.c:
 *
 * this merges two images, folding and reducing colormaps as necessary.
 *
 * jim frost 09.27.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "image.h"

static void mergeColors(dest, src, verbose)
     Image        *dest, *src;
     unsigned int  verbose;
{ RGBMap           newcolors;
  unsigned int     a, b;

  if (dest->rgb.used + src->rgb.used > dest->rgb.size) {
    newRGBMapData(&newcolors, dest->rgb.used + src->rgb.used);
    newcolors.used= newcolors.size;
    for (a= 0; a < dest->rgb.used; a++) {
      *(newcolors.red + a)= *(dest->rgb.red + a);
      *(newcolors.green + a)= *(dest->rgb.green + a);
      *(newcolors.blue + a)= *(dest->rgb.blue + a);
    }
    for (b= 0; b < src->rgb.used; a++, b++) {
      *(newcolors.red + a)= *(src->rgb.red + b);
      *(newcolors.green + a)= *(src->rgb.green + b);
      *(newcolors.blue + a)= *(src->rgb.blue + b);
    }

    reduceRGBMap(&newcolors, dest->rgb.size, verbose);
    
    for (a= 0; a < dest->rgb.used; a++) {             /* put new colors into */
      *(dest->rgb.red + a)= *(newcolors.red + a);     /* old colormaps */
      *(dest->rgb.green + a)= *(newcolors.green + a);
      *(dest->rgb.blue + a)= *(newcolors.blue + a);
    }
    for (a= 0; a < src->rgb.used; a++) {
      *(src->rgb.red + a)= *(newcolors.red + a + dest->rgb.used);
      *(src->rgb.green + a)= *(newcolors.green + a + dest->rgb.used);
      *(src->rgb.blue + a)= *(newcolors.blue + a + dest->rgb.used);
    }
  }
  else
    for (a= 0; a < src->rgb.used; a++, (dest->rgb.used)++) {
      *(dest->rgb.red + dest->rgb.used)= *(src->rgb.red + a);
      *(dest->rgb.green + dest->rgb.used)= *(src->rgb.green + a);
      *(dest->rgb.blue + dest->rgb.used)= *(src->rgb.blue + a);
    }
}

static void bitmapToBitmap(src, dest, atx, aty, clipw, cliph, verbose)
     Image        *src, *dest;
     unsigned int  atx, aty, clipw, cliph;
     unsigned int  verbose;
{ unsigned int  dithered;
  unsigned int  destlinelen, srclinelen;
  unsigned int  deststart;
  unsigned int  flip;
  unsigned int  x, y;
  byte         *destline, *srcline;
  byte          deststartmask;
  byte          destmask, srcmask;
  byte         *destpixel, *srcpixel;

  if (verbose) {
    printf("  Merging bitmap image onto bitmap image...");
    fflush(stdout);
  }

  if (RGBP(src)) { /* dither the RGB image to mono */
    dithered= 1;
    src= dither(src, verbose);
  }
  else
    dithered= 0;

  destlinelen= (dest->width / 8) + (dest->width % 8 ? 1 : 0);
  srclinelen= (src->width / 8) + (src->width % 8 ? 1 : 0);
  destline= dest->data + (aty * destlinelen);
  srcline= src->data;
  deststart= atx / 8;
  deststartmask= 0x80 >> (atx % 8);
  flip= ((*dest->rgb.red == *(src->rgb.red + 1)) &&
	 (*dest->rgb.green == *(src->rgb.green + 1)) &&
	 (*dest->rgb.blue == *(src->rgb.blue + 1)));
  for (y= 0; y < cliph; y++) {
    destpixel= destline + deststart;
    srcpixel= srcline;
    destmask= deststartmask;
    srcmask= 0x80;
    for (x= 0; x < clipw; x++) {
      if (flip)
	if (*srcpixel & srcmask)
	  *destpixel &= ~destmask;
	else
	  *destpixel |= destmask;
      else
	if (*srcpixel & srcmask)
	  *destpixel |= destmask;
	else
	  *destpixel &= ~destmask;
      destmask >>= 1;
      srcmask >>= 1;
      if (destmask == 0) {
	destmask= 0x80;
	destpixel++;
      }
      if (srcmask == 0) {
	srcmask= 0x80;
	srcpixel++;
      }
    }
    destline += destlinelen;
    srcline += srclinelen;
  }
  if (dithered)
    freeImage(src);

  if (verbose)
    printf("done\n");
}

static void bitmapToRGB(src, dest, atx, aty, clipw, cliph, verbose)
     Image        *src, *dest;
     unsigned int  atx, aty, clipw, cliph;
     unsigned int  verbose;
{ unsigned int  bg, fg;
  unsigned int  destlinelen, srclinelen;
  unsigned int  deststart;
  unsigned int  x, y;
  byte         *destline, *srcline;
  byte         *destpixel, *srcpixel;
  byte          srcmask;

  if (verbose) {
    printf("  Merging bitmap image onto RGB image...");
    fflush(stdout);
  }

  /* get fg and bg colors from dest image
   */
  
  fg= bg= 0;
  for (x= 0; x < dest->rgb.used; x++)
    if ((*(dest->rgb.red + x) == *src->rgb.red) &&
	(*(dest->rgb.green + x) == *src->rgb.green) &&
	(*(dest->rgb.blue + x) == *src->rgb.blue)) {
      bg= x;
      break;
    }
  if (x == dest->rgb.used)
    printf("merge: warning: can't find background color in dest image\n");
  for (x= 0; x < dest->rgb.used; x++)
    if ((*(dest->rgb.red + x) == *(src->rgb.red + 1)) &&
	(*(dest->rgb.green + x) == *(src->rgb.green + 1)) &&
	(*(dest->rgb.blue + x) == *(src->rgb.blue + 1))) {
      fg= x;
      break;
    }
  if (x == dest->rgb.used)
    printf("merge: warning: can't find foreground color in dest image\n");
  
  /* merge 'em
   */

  destlinelen= dest->width * dest->pixlen;
  srclinelen= (src->width / 8) + (src->width % 8 ? 1 : 0);
  destline= dest->data + (aty * destlinelen);
  srcline= src->data;
  deststart= atx * dest->pixlen;

  for (y= 0; y < cliph; y++) {
    destpixel= destline + deststart;
    srcpixel= srcline;
    srcmask= 0x80;
    for (x= 0; x < clipw; x++) {
      valToMem((*srcpixel & srcmask ? fg : bg), destpixel, dest->pixlen);
      destpixel += dest->pixlen;
      srcmask >>= 1;
      if (srcmask == 0) {
	srcpixel++;
	srcmask= 0x80;
      }
    }
    destline += destlinelen;
    srcline += srclinelen;
  }

  if (verbose)
    printf("done\n");
}

static void RGBToRGB(src, dest, atx, aty, clipw, cliph, verbose)
     Image        *src, *dest;
     unsigned int  atx, aty, clipw, cliph;
     unsigned int  verbose;
{ unsigned int  destlinelen, srclinelen;
  unsigned int  deststart;
  unsigned int  x, y;
  Pixel        *index;
  byte         *destline, *srcline;
  byte         *destpixel, *srcpixel;

  if (verbose) {
    printf("  Merging RGB images...");
    fflush(stdout);
  }

  /* build src->dest pixel mapping
   */
  
  index= (Pixel *)lmalloc(sizeof(Pixel) * src->rgb.used);
  for (x= 0; x < src->rgb.used; x++) {
    for (y= 0; y < dest->rgb.used; y++)
      if ((*(dest->rgb.red + y) == *(src->rgb.red + x)) &&
	  (*(dest->rgb.green + y) == *(src->rgb.green + x)) &&
	  (*(dest->rgb.blue + y) == *(src->rgb.blue + x))) {
	*(index + x)= y;
	break;
      }
    if (y == dest->rgb.used)
      if (y < dest->rgb.size) {
	*(dest->rgb.red + y)= *(src->rgb.red + x);
	*(dest->rgb.green + y)= *(src->rgb.green + x);
	*(dest->rgb.blue + y)= *(src->rgb.blue + x);
	*(index + x)= y;
	dest->rgb.used++;
      }
      else {
	printf("merge: warning: To few colors in destination colormap?!?\n");
	*(index + x)= 0;
      }
  }

  destlinelen= dest->width * dest->pixlen;
  srclinelen= src->width * src->pixlen;
  deststart= atx * dest->pixlen;
  destline= dest->data + (aty * destlinelen);
  srcline= src->data;

  for (y= 0; y < cliph; y++) {
    destpixel= destline + deststart;
    srcpixel= srcline;
    for (x= 0; x < clipw; x++) {
      valToMem(*(index + memToVal(srcpixel, src->pixlen)),
	       destpixel, dest->pixlen);
      destpixel += dest->pixlen;
      srcpixel += src->pixlen;
    }
    destline += destlinelen;
    srcline += srclinelen;
  }
  lfree(index);

  if (verbose)
    printf("done\n");
}

/* put src image on dest image (and clip while we're at it).  the bitmap
 * to bitmap merge could be sped up by a factor of four for the general
 * case and eight to thirty-two for specific cases.  i'm for simplicity,
 * though.
 */

void merge(dest, src, atx, aty, verbose)
     Image        *dest;
     Image        *src;
     int           atx, aty;
     unsigned int  verbose;
{ unsigned int clipw, cliph;

  goodImage(dest, "merge");
  goodImage(src, "merge");

  /* adjust clipping of src to fit within dest
   */

  clipw= src->width;
  cliph= src->height;
  if ((atx >= dest->width) || (aty >= dest->height)) /* not on dest, ignore */
    return;
  if (atx < 0) {
    clipw += atx;
    atx= 0;
  }
  if (aty < 0) {
    cliph += aty;
    aty= 0;
  }

/*
 *		Making the value of cliph and clipw sane .
 * 		Value of clipw was becoming even higher if dest->width > clipw 
 *			 ( Same for cliph )
 */

  if (atx + clipw > dest->width)
  /*
    clipw -= (dest->width - (atx + clipw));
	*/
	clipw =  dest->width -atx ;
  if (aty + cliph > dest->height)
  /*
    cliph -= (dest->width - (aty + cliph));
	*/
	cliph = dest->height - aty ;

  if (BITMAPP(dest) && (BITMAPP(src) || RGBP(src)))
    bitmapToBitmap(src, dest, (unsigned int)atx, (unsigned int)aty,
		   clipw, cliph, verbose);
  else {
    mergeColors(dest, src, verbose);
    if (RGBP(dest) && BITMAPP(src))
      bitmapToRGB(src, dest, (unsigned int)atx, (unsigned int)aty,
		  clipw, cliph, verbose);
    else if (RGBP(dest) && RGBP(src))
      RGBToRGB(src, dest, (unsigned int)atx, (unsigned int)aty,
	       clipw, cliph, verbose);
    else {
      printf("merge: Can't merge these two types of images (sorry)\n");
      exit(1);
    }
  }
  compress(dest, verbose);
}
