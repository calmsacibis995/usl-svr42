/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:compress.c	1.1"
/* compress.c:
 *
 * compress a colormap by removing unused RGB colors
 *
 * jim frost 10.05.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "image.h"

void compress(image, verbose)
     Image        *image;
     unsigned int  verbose;
{ Pixel        *index;
  unsigned int *used;  
  RGBMap        rgb;
  byte         *pixptr;
  unsigned int  a, x, y;
  Pixel         color;

  goodImage(image, "compress");
  if (! RGBP(image)) /* we're AT&T */
    return;

  if (verbose) {
    printf("  Compressing colormap...");
    fflush(stdout);
  }

  newRGBMapData(&rgb, image->rgb.size);
  index= (Pixel *)lmalloc(sizeof(Pixel) * image->rgb.used);
  used= (unsigned int *)lmalloc(sizeof(unsigned int) * image->rgb.used);
  for (x= 0; x < image->rgb.used; x++)
    *(used + x)= 0;

  pixptr= image->data;
  for (y= 0; y < image->height; y++)
    for (x= 0; x < image->width; x++) {
      color= memToVal(pixptr, image->pixlen);
      if (*(used + color) == 0) {
	for (a= 0; a < rgb.used; a++)
	  if ((*(rgb.red + a) == *(image->rgb.red + color)) &&
	      (*(rgb.green + a) == *(image->rgb.green + color)) &&
	      (*(rgb.blue + a) == *(image->rgb.blue + color)))
	    break;
	*(index + color)= a;
	*(used + color)= 1;
	if (a == rgb.used) {
	  *(rgb.red + a)= *(image->rgb.red + color);
	  *(rgb.green + a)= *(image->rgb.green + color);
	  *(rgb.blue + a)= *(image->rgb.blue + color);
	  rgb.used++;
	}
      }
      valToMem(*(index + color), pixptr, image->pixlen);
      pixptr += image->pixlen;
    }

  if (verbose)
    if (rgb.used < image->rgb.used)
      printf("%d unique colors of %d\n", rgb.used, image->rgb.used);
    else
      printf("no improvement\n");

  freeRGBMapData(&(image->rgb));
  image->rgb= rgb;
}
