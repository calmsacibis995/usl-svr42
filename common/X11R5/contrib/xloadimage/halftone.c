/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:halftone.c	1.1"
/* dither.c:
 *
 * routine for dithering a color image to monochrome based on color
 * intensity.  this is loosely based on an algorithm which barry shein
 * (bzs@std.com) used in his "xf" program.
 *
 * jim frost 07.10.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "image.h"

/* 4x4 arrays used for dithering, arranged by nybble
 */

#define GRAYS    17 /* ((4 * 4) + 1) patterns for a good dither */
#define GRAYSTEP ((unsigned long)(65536 * 3) / GRAYS)

static byte DitherBits[GRAYS][4] = {
  0xf, 0xf, 0xf, 0xf,
  0xe, 0xf, 0xf, 0xf,
  0xe, 0xf, 0xb, 0xf,
  0xa, 0xf, 0xb, 0xf,
  0xa, 0xf, 0xa, 0xf,
  0xa, 0xd, 0xa, 0xf,
  0xa, 0xd, 0xa, 0x7,
  0xa, 0x5, 0xa, 0x7,
  0xa, 0x5, 0xa, 0x5,
  0x8, 0x5, 0xa, 0x5,
  0x8, 0x5, 0x2, 0x5,
  0x0, 0x5, 0x2, 0x5,
  0x0, 0x5, 0x0, 0x5,
  0x0, 0x4, 0x0, 0x5,
  0x0, 0x4, 0x0, 0x1,
  0x0, 0x0, 0x0, 0x1,
  0x0, 0x0, 0x0, 0x0
};

/* simple dithering algorithm, really optimized for the 4x4 array
 */

Image *halftone(cimage, verbose)
     Image        *cimage;
     unsigned int  verbose;
{ Image         *image;
  unsigned char *sp, *dp, *dp2; /* data pointers */
  unsigned int   dindex;        /* index into dither array */
  unsigned int   spl;           /* source pixel length in bytes */
  unsigned int   dll;           /* destination line length in bytes */
  Pixel          color;         /* pixel color */
  unsigned int  *index;         /* index into dither array for a given pixel */
  unsigned int   a, x, y;       /* random counters */

  goodImage(cimage, "dither");
  if (! RGBP(cimage))
    return(NULL);

  /* set up
   */

  if (verbose) {
    printf("  Halftoning image...");
    fflush(stdout);
  }
  image= newBitImage(cimage->width * 4, cimage->height * 4);
  if (cimage->title) {
    image->title= (char *)malloc(strlen(cimage->title) + 12);
    sprintf(image->title, "%s (halftoned)", cimage->title);
  }
  spl= cimage->pixlen;
  dll= (image->width / 8) + (image->width % 8 ? 1 : 0);

  /* if the number of possible pixels isn't very large, build an array
   * which we index by the pixel value to find the dither array index
   * by color brightness.  we do this in advance so we don't have to do
   * it for each pixel.  things will break if a pixel value is greater
   * than (1 << depth), which is bogus anyway.  this calculation is done
   * on a per-pixel basis if the colormap is too big.
   */

  if (cimage->depth <= 16) {
    index= (unsigned int *)malloc(sizeof(unsigned int) * cimage->rgb.used);
    for (x= 0; x < cimage->rgb.used; x++) {
      *(index + x)=
	((unsigned long)(*(cimage->rgb.red + x)) +
	 *(cimage->rgb.green + x) +
	 *(cimage->rgb.blue + x)) / GRAYSTEP;
      if (*(index + x) >= GRAYS) /* rounding errors can do this */
	*(index + x)= GRAYS - 1;
    }
  }
  else
    index= NULL;

  /* dither each pixel
   */

  sp= cimage->data;
  dp= image->data;
  for (y= 0; y < cimage->height; y++) {
    for (x= 0; x < cimage->width; x++) {
      dp2= dp + (x >> 1);
      color= memToVal(sp, spl);
      if (index)
	dindex= *(index + color);
      else {
	dindex= ((unsigned long)(*(cimage->rgb.red + color)) +
		 *(cimage->rgb.green + color) +
		 *(cimage->rgb.blue + color)) / GRAYSTEP;
	if (dindex >= GRAYS) /* rounding errors can do this */
	  dindex= GRAYS - 1;
      }

      /* loop for the four Y bits in the dither pattern, putting all
       * four X bits in at once.  if you think this would be hard to
       * change to be an NxN dithering array, you're right, since we're
       * banking on the fact that we need only shift the mask based on
       * whether x is odd or not.  an 8x8 array wouldn't even need that,
       * but blowing an image up by 64x is probably not a feature.
       */

      if (x & 1)
	for (a= 0; a < 4; a++, dp2 += dll)
	  *dp2 |= DitherBits[dindex][a];
      else
	for (a= 0; a < 4; a++, dp2 += dll)
	  *dp2 |= (DitherBits[dindex][a] << 4);
      sp += spl;
    }
    dp += (dll << 2); /* (dll * 4) but I like shifts */
  }
  if (verbose)
    printf("done\n");
  return(image);
}
