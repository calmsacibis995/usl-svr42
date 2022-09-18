/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:dither.c	1.1"
/* dither.c:
 *
 * this is a modified version of the dithering algorithm in halftone.c
 * which doesn't enlarge the image.  modifications made by
 * Steve Losen (scl@virginia.edu).
 *
 * jim frost 07.10.89
 * Steve Losen 11.17.89
 *
 * Copyright 1989, 1990 Jim Frost and Steve Losen.  See included file
 * "copyright.h" for complete copyright information.
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

Image *dither(cimage, verbose)
     Image        *cimage;
     unsigned int  verbose;
{ Image         *image;
  unsigned char *sp, *dp; /* data pointers */
  unsigned int   dindex;  /* index into dither array */
  unsigned int   spl;     /* source pixel length in bytes */
  unsigned int   dll;     /* destination line length in bytes */
  Pixel          color;   /* pixel color */
  unsigned int  *index;   /* index into dither array for a given pixel */
  unsigned int   x, y;    /* random counters */

  goodImage(cimage, "dither");
  if (! RGBP(cimage))
    return(NULL);

  /* set up
   */

  if (verbose) {
    printf("  Dithering image...");
    fflush(stdout);
  }
  image= newBitImage(cimage->width, cimage->height);
  if (cimage->title) {
    image->title= (char *)malloc(strlen(cimage->title) + 12);
    sprintf(image->title, "%s (dithered)", cimage->title);
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
      if (*(index + x) >= GRAYS)
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
      color= memToVal(sp, spl);
      if (index)
	dindex= *(index + color);
      else {
	dindex= ((unsigned long)(*(cimage->rgb.red + color)) +
		 *(cimage->rgb.green + color) +
		 *(cimage->rgb.blue + color)) / GRAYSTEP;
	if (dindex >= GRAYS)
	  dindex= GRAYS - 1;
      }
      if (DitherBits[dindex][y & 3] & (1 << (x & 3)))
	 dp[x / 8] |= 1 << (7 - (x & 7));
      sp += spl;
    }
    dp += dll;
  }
  if (verbose)
    printf("done\n");
  return(image);
}
