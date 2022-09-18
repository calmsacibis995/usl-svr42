/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:bright.c	1.1"
/* bright.c
 *
 * alter an image's brightness by a given percentage
 *
 * jim frost 10.10.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "image.h"

void brighten(image, percent, verbose)
     Image        *image;
     unsigned int  percent;
     unsigned int  verbose;
{ int          a;
  unsigned int newrgb;
  float        fperc;

  if (! RGBP(image)) /* we're AT&T */
    return;

  if (verbose) {
    printf("  Brightening colormap by %d%%...", percent);
    fflush(stdout);
  }

  fperc= (float)percent / 100.0;
  for (a= 0; a < image->rgb.used; a++) {
    newrgb= *(image->rgb.red + a) * fperc;
    if (newrgb > 65535)
      newrgb= 65535;
    *(image->rgb.red + a)= newrgb;
    newrgb= *(image->rgb.green + a) * fperc;
    if (newrgb > 65535)
      newrgb= 65535;
    *(image->rgb.green + a)= newrgb;
    newrgb= *(image->rgb.blue + a) * fperc;
    if (newrgb > 65535)
      newrgb= 65535;
    *(image->rgb.blue + a)= newrgb;
  }

  if (verbose)
    printf("done\n");
}
