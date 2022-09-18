/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:clip.c	1.2"

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

/* clip.c:
 *
 * return a new image which is a clipped subsection of the old image
 *
 * jim frost 10.04.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "image.h"

Image *clip(simage, clipx, clipy, clipw, cliph, verbose)
     Image        *simage;
     unsigned int  clipx, clipy, clipw, cliph;
     unsigned int  verbose;
{ Image *image;
  unsigned int  x, y;
  unsigned int  slinelen, dlinelen;
  unsigned int  start;
  byte          startmask, smask, dmask;
  byte         *sp, *sline, *dp, *dline;

  goodImage(simage, "clip");

  if (verbose) {
    printf("  Clipping image...");
    fflush(stdout);
  }

  /* sane-ify clip area with respect to image
   */
/*
 *  Instead of  'sane-ify' clipw & cliph were becoming more insane .
 */

  if (clipx + clipw > simage->width)
/*
    clipw -= (simage->width - (clipx + clipw));
*/
    clipw = simage->width -clipx;
  if (clipy + cliph > simage->height)
/*
    cliph -= (simage->height - (clipy + cliph));
*/
    cliph = simage->height -clipy;
  if (verbose) {
    printf(" ..from %d,%d  size %dx%d ....",clipx,clipy,clipw,cliph);
    fflush(stdout);
  }

  switch (simage->type) {
  case IBITMAP:

    /* this could be sped up; i don't care
     */

    image= newBitImage(clipw, cliph);
    for (x= 0; x < simage->rgb.used; x++) {
      *(image->rgb.red + x)= *(simage->rgb.red + x);
      *(image->rgb.green + x)= *(simage->rgb.green + x);
      *(image->rgb.blue + x)= *(simage->rgb.blue + x);
    }
    slinelen= (simage->width / 8) + (simage->width % 8 ? 1 : 0);
    dlinelen= (clipw / 8) + (clipw % 8 ? 1 : 0);
    start= clipx / 8;
    startmask= 0x80 >> (clipx % 8);
    sline= simage->data + (slinelen * clipy);
    dline= image->data;
    for (y= 0; y < cliph; y++) {
      sp= sline + start;
      dp= dline;
      smask= startmask;
      dmask= 0x80;
      for (x= 0; x < clipw; x++) {
	if (*sp & smask)
	  *dp |= dmask;
	if (! (smask >>= 1)) {
	  smask= 0x80;
	  sp++;
	}
	if (! (dmask >>= 1)) {
	  dmask= 0x80;
	  dp++;
	}
      }
      sline += slinelen;
      dline += dlinelen;
    }
    break;

  case IRGB:
    image= newRGBImage(clipw, cliph, simage->depth);
    for (x= 0; x < simage->rgb.used; x++) {
      *(image->rgb.red + x)= *(simage->rgb.red + x);
      *(image->rgb.green + x)= *(simage->rgb.green + x);
      *(image->rgb.blue + x)= *(simage->rgb.blue + x);
    }
    image->rgb.used= simage->rgb.used;
    slinelen= simage->width * simage->pixlen;
    start= clipx * simage->pixlen;
    sline= simage->data + (clipy * slinelen);
    dp= image->data;
    for (y= 0; y < cliph; y++) {
      sp= sline + start;
      for (x= 0; x < clipw; x++) {
	valToMem(memToVal(sp, simage->pixlen), dp, simage->pixlen);
	sp += simage->pixlen;
	dp += simage->pixlen;
      }
      sline += slinelen;
    }
    break;
  default:
    printf("clip: Unsupported image type\n");
    exit(1);
  }
  image->title= dupString(simage->title);
  if (verbose)
    printf("done\n");
  return(image);
}
